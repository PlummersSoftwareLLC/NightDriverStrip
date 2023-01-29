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
//    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
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

#include <sys/types.h>
#include <errno.h>
#include <iostream>
#include <vector>
#include <deque>
#include <math.h>

#include "soundanalyzer.h"
#include "effectmanager.h"
#include "colorutils.h"
#include "globals.h"
#include "ledstripeffect.h" 
#include "faneffects.h"
#include "gfxbase.h"

extern DRAM_ATTR AppTime g_AppTime;

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

    bool   _latched = false;                          // Have we seen a low val yet?
    double _lastBeat = 0;                             // When was the last beat?

    double _lowLatch;                                 // Low Point at we we set the latch ON
    double _highLatch;                                // High Point we must crest for a beat
    double _minElapsed;                               // Min time between beats or we ignore
    double _lowestSeen;

  public:
   
    BeatEffectBase(double lowLatch = 1.0, double highLatch = 1.75, double minElapsed = 0.25)      // Eighth note at 120BPM is .125
     : _lowLatch(lowLatch),
       _highLatch(highLatch),
       _minElapsed(minElapsed),
       _lowestSeen(2.0)
    {
    }

    // When a beat is detected, this is called.  The 'bMajor' indicates whether this is a more important beat, which
    // for now simply means it's been a minimum delay since the last beat.

    virtual void HandleBeat(bool bMajor, float elapsed, double span)
    {
        debugV("BEAT: [%s], g_Analyzer.gVURatio=%f, since last=%lf, span = %lf\n", bMajor ? "Major" : "Minor", g_Analyzer._VURatio, g_AppTime.CurrentTime() - _lastBeat, g_Analyzer._VURatio - _lowestSeen);
    }

    double SecondsSinceLastBeat()
    {
      return g_AppTime.CurrentTime() - _lastBeat;
    }


    // BeatEffectBase::ProcessAudio
    //
    // Doesn't actually "draw" anything, but rather it scans the audio VU to detect beats, and when it finds one,
    // it calls the virtual "HandleBeat" function.

    virtual void ProcessAudio()
    {
        debugV("BeatEffectBase::Draw");
        double elapsed = SecondsSinceLastBeat();
    
        if (elapsed < _minElapsed)                      // Too soon since last beat to even consider another.  
          return;

        if (g_Analyzer._VURatio < _lowestSeen)
            _lowestSeen = g_Analyzer._VURatio;

        if (g_Analyzer._VURatio < _lowLatch)                       // If we dip below the low latch, we're latched (armed)
            _latched = true;                            //   to trigger a beat if we then go above the high latch

        if (_latched && g_Analyzer._VURatio >= _highLatch)
        {
            // When we crest above the high latch point while latched (ie: we see a high value aftering confirming
            // that we've seen a low one) we know it's a round trip on the VU meter and so we call that a beat.  We
            // call the client handler and then reset the latch, then record now as the last beat time.

            HandleBeat(elapsed > 1.0, elapsed, g_Analyzer._VURatio - _lowestSeen);
            _lastBeat = g_AppTime.CurrentTime();
            _latched = false;
            _lowestSeen = 2.0;
        }
    }
};

class BeatEffectBase2
{
  protected:
    const int _maxSamples = 60;
    std::deque<double> _samples;
    double _lastBeat = 0;
    double _minRange = 0;
    double _minElapsed = 0;

  public:
   
    BeatEffectBase2(double minRange = 0, double minElapsed = 0)      
     :
       _minRange(minRange),
       _minElapsed(minElapsed)
    {
    }

    // When a beat is detected, this is called.  The 'bMajor' indicates whether this is a more important beat, which
    // for now simply means it's been a minimum delay since the last beat.

    virtual void HandleBeat(bool bMajor, float elapsed, double span)
    {
    }

    double SecondsSinceLastBeat()
    {
      return g_AppTime.CurrentTime() - _lastBeat;
    }


    // BeatEffectBase::Draw
    //
    // Doesn't actually "draw" anything, but rather it scans the audio VU to detect beats, and when it finds one,
    // it calls the virtual "HandleBeat" function.

    virtual void ProcessAudio()
    {
        debugV("BeatEffectBase2::Draw");
        double elapsed = SecondsSinceLastBeat();
    
        _samples.push_back(g_Analyzer._VURatio);
        double minimum = *min_element(_samples.begin(), _samples.end());
        double maximum = *max_element(_samples.begin(), _samples.end());

        // debugI("Samples: %d, max: %0.2lf, min: %0.2lf, span: %0.2lf\n", _samples.size(), maximum, minimum, maximum-minimum);

        if (_samples.size() >= _maxSamples)
          _samples.pop_front();

        if (maximum - minimum > _minRange)
        {
            if (elapsed < _minElapsed)
            {
                // False beat too early, clear data but don't reset lastBeat
                 _samples.clear();
            }
            else
            {
              debugV("Beat: elapsed: %0.2lf, range: %0.2lf\n", elapsed, maximum - minimum);

              HandleBeat(false, elapsed, maximum - minimum);
              _lastBeat = g_AppTime.CurrentTime();
              _samples.clear();
            }
        }
    }
};

//
// MusicalInsulatorEffect
//

class BeatEffect
{
  public:

    virtual void OnBeat()
    {

    }

    virtual void ProcessAudio()
    {
        static bool  latch = false;
        static float minVUSeen = 0.0;

        if (latch)
        {
          if (g_Analyzer._VURatio < minVUSeen)
            minVUSeen = g_Analyzer._VURatio;
        }

        if (g_Analyzer._VURatio < 0.5)             // Crossing center going down
        {
          latch = true;
          minVUSeen = g_Analyzer._VURatio;
        }

        if (latch)
        {
            if (g_Analyzer._VURatio > 1.75)      // Going back up
            {
                latch = false;
                OnBeat();
                debugV("Beat: %f, %f\n", g_Analyzer._VURatio, g_Analyzer._VU);
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
           
    virtual void Draw()
    {
        ProcessAudio();

        CRGB c = CRGB::Blue * g_Analyzer._VURatio * g_AppTime.DeltaTime() * 0.75;
        setPixelsOnAllChannels(0, NUM_LEDS, c, true);

        fadeAllChannelsToBlackBy(min(255.0,1000 * g_AppTime.DeltaTime()));
        delay(1);
    }

    virtual void HandleBeat(bool bMajor, float elapsed, double span)
    {
        BeatEffectBase::HandleBeat(bMajor, elapsed, span);

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

          DrawFanPixels(0, FAN_SIZE, c, Sequential, i);     // Draw twice to double-saturate our color
          DrawFanPixels(0, FAN_SIZE, c, Sequential, i);
        }
    } 

  public:
  
    SimpleColorBeat(const String pszName)
      : BeatEffectBase(1.0, 1.0, 0.25), LEDStripEffect(pszName)
    {
    }

};