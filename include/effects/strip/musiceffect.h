//+--------------------------------------------------------------------------
//
// File:        MusicEffect.h
//
// NightDriverStrip - (c) 2018 Plummer's Software LLC.  All Rights Reserved.
//
// This file is part of the NightDriver software project.
//
//    NightDriver is free software: you can redistribute it and/or modify
//    it under the terms of the GNU General Public License as published by
//    the Free Software Foundation, either version 3 of the License, or
//    (at your option) any later version.
//
//    NightDriver is distributed in the hope that it will be useful,
//    but WITHOUT ANY WARRANTY; without even the implied warranty of
//    MERHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//    GNU General Public License for more details.
//
//    You should have received a copy of the GNU General Public License
//    along with Nightdriver.  It is normally found in copying.txt
//    If not, see <https://www.gnu.org/licenses/>.
//
// Description:
//
//    Floating point framerate independent version of the classic Flame effect
//
// History:     Apr-13-2019         Davepl      Adapted from LEDWifiSocket
//
//---------------------------------------------------------------------------

#pragma once

#include "effects.h"
#include "faneffects.h"

#if ENABLE_AUDIO

// BeatEffectBase
//
// A specialization of LEDStripEffect, adds a HandleBeat function that allows apps to
// draw based on the music beat.  The Draw() function does the audio processing and calls
// HandleBeat() whenever.  Apps are free to draw in both Draw() and HandleBeat().
//
// The constructor allows you to specify the sensitivity by where the latch points are/
// For a highly sensitive (defaults), keep them both close to 1.0.  For a wider beat
// detection you could use 0.25 and 1.75 for example.


class BeatEffectBase
{
  protected:
    const int _maxSamples = 60;
    std::deque<float> _samples;
    double _lastBeat = 0;
    float _minRange = 0;
    float _minElapsed = 0;

  public:

    BeatEffectBase(float minRange = 0.75, float minElapsed = 0.20 )
     :
       _minRange(minRange),
       _minElapsed(minElapsed)
    {
    }

    // When a beat is detected, this is called.  The 'bMajor' indicates whether this is a more important beat, which
    // for now simply means it's been a minimum delay since the last beat.

    virtual void HandleBeat(bool bMajor, float elapsed, float span) = 0;

    double SecondsSinceLastBeat()
    {
      return g_Values.AppTime.CurrentTime() - _lastBeat;
    }


    // BeatEffectBase::Draw
    //
    // Doesn't actually "draw" anything, but rather it scans the audio VU to detect beats, and when it finds one,
    // it calls the virtual "HandleBeat" function.

    virtual void ProcessAudio()
    {
        debugV("BeatEffectBase2::Draw");
        double elapsed = SecondsSinceLastBeat();

        auto basslevel = g_Analyzer.GetPeakData()._Level[0] * 2;  // Since VURatio was historically a 0-2 range, we do the same

        debugV("basslevel: %0.2f", basslevel);
        _samples.push_back(basslevel);
        float minimum = *min_element(_samples.begin(), _samples.end());
        float maximum = *max_element(_samples.begin(), _samples.end());

        //Serial.printf("Samples: %d, max: %0.2f, min: %0.2f, span: %0.2f\n", _samples.size(), maximum, minimum, maximum-minimum);

        if (_samples.size() >= _maxSamples)
          _samples.pop_front();

        if (maximum - minimum > _minRange)
        {
            if (elapsed < _minElapsed)
            {
                //Serial.printf("False Beat: elapsed: %0.2f, range: %0.2f, time: %0.2lf\n", elapsed, maximum - minimum, g_AppTime.CurrentTime());
                // False beat too early, clear data but don't reset lastBeat
                 _samples.clear();
            }
            else
            {
              debugV("Beat: elapsed: %0.2lf, range: %0.2lf\n", elapsed, maximum - minimum);

              HandleBeat(false, elapsed, maximum - minimum);
              _lastBeat = g_Values.AppTime.CurrentTime();
              _samples.clear();
            }
        }
    }
};

// InsulatorColorBeatEffect
//
// Uses a very sensitive beat detection.  Fills all pixels blue based on VU, and on beats, fills a random insulator
// with a random color.  Longer beats get more insulators.  Very long beats get everything filled with purple.

class SimpleColorBeat : public BeatEffectBase, public LEDStripEffect
{
  protected:

    int _iLastInsulator = -1;

    void Draw() override
    {
        ProcessAudio();

        CRGB c = CRGB::Blue * g_Analyzer._VURatio * g_Values.AppTime.LastFrameTime() * 0.75;
        setPixelsOnAllChannels(0, NUM_LEDS, c, true);

        fadeAllChannelsToBlackBy(min(255.0,1000.0 * g_Values.AppTime.LastFrameTime()));
        delay(1);
    }

    void HandleBeat(bool bMajor, float elapsed, float span) override
    {
        CRGB c;
        int  cInsulators = 1;

        if (elapsed < 0.10)
        {
          c = CHSV(beatsin8(2), 255, 255);
        }
        else if (elapsed < 0.25)
        {
          c = CHSV(beatsin8(3), 255, 255);
        }
        else
        {
          c = CRGB::Cyan;

          if (elapsed > 0.5)                                // Medium beats fill blue and proceed with insulator2
          {
            c = CHSV(beatsin8(4), 255, 255);
            cInsulators = random(1, NUM_FANS);
          }
          else if (elapsed > 1.0)                           // Long beats fill purple and return
          {
            c = CRGB::Purple;
            setAllOnAllChannels(c.r, c.g, c.b);
            return;
          }
        }

        int i;
        for (int iPass = 0; iPass < cInsulators; iPass++)
        {
          do {                                              // Pick a different insulator than was used last time by:
            i = random(0, NUM_FANS);                        //  - Starting with a random number
          } while (i == _iLastInsulator);                   //  - Repeating until it doesn't match the last pass
          _iLastInsulator = i;                              // Our current choice forms the new "last" choice for next pass

          DrawFanPixels(0, FAN_SIZE, c, Sequential, i);     // Draw twice to float-saturate our color
          DrawFanPixels(0, FAN_SIZE, c, Sequential, i);
        }
    }

  public:

    SimpleColorBeat(const String & strName)
      : BeatEffectBase(0.5, 0.25), LEDStripEffect(EFFECT_STRIP_SIMPLE_COLOR_BEAT, strName)
    {
    }

    SimpleColorBeat(const JsonObjectConst& jsonObject)
      : BeatEffectBase(0.5, 0.25), LEDStripEffect(jsonObject)
    {
    }
};

#endif // ENABLE_AUDIO
