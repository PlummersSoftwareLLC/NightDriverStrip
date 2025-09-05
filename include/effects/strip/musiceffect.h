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

#include <deque>

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
    // Adaptive detection parameters
    int _bandsToUse = std::min<int>(NUM_BANDS, 3); // average first N bands
    float _sigmaThresh = 1.8f;                     // z-score threshold
    float _ratioThresh = 0.35f;                    // relative rise over rolling mean (35%)
  public:
    BeatEffectBase(float minRange = 0.75, float minElapsed = 0.20) : _minRange(minRange), _minElapsed(minElapsed)
    {
    }

    // Allow effects to tune beat sensitivity at runtime
    inline void SetBeatSensitivity(float sigmaThresh, float ratioThresh, int bandsToUse = std::min<int>(NUM_BANDS, 3))
    {
        _sigmaThresh = sigmaThresh;
        _ratioThresh = ratioThresh;
        _bandsToUse = std::clamp(bandsToUse, 1, (int)NUM_BANDS);
    }

    virtual void HandleBeat(bool bMajor, float elapsed, float span) = 0;

    double SecondsSinceLastBeat()
    {
        return g_Values.AppTime.CurrentTime() - _lastBeat;
    }

    virtual void ProcessAudio()
    {
        debugV("BeatEffectBase2::Draw");
        double elapsed = SecondsSinceLastBeat();
        const PeakData &peaks = g_Analyzer.Peaks();
        // Average the first N bands (bass/low-mid) for robust beat energy
        float level = 0.0f;
        const int bands = std::clamp(_bandsToUse, 1, (int)NUM_BANDS);
        for (int i = 0; i < bands; ++i)
            level += peaks[i];
        level /= (float)bands;
        // Scale roughly to old 0..2 range for backward compatibility of _minRange
        const float basslevel = level * 2.0f;
        debugV("basslevel(avg%db): %0.2f", bands, basslevel);
        _samples.push_back(basslevel);
        if ((int)_samples.size() > _maxSamples)
            _samples.pop_front();
        if (_samples.size() < 8)
            return; // need a minimum buffer to stabilize statistics
        const float minimum = *std::min_element(_samples.begin(), _samples.end());
        const float maximum = *std::max_element(_samples.begin(), _samples.end());
        // Compute rolling mean and standard deviation
        const float mean = std::accumulate(_samples.begin(), _samples.end(), 0.0f) / (float)_samples.size();
        float var = 0.0f;
        for (float v : _samples)
        {
            const float d = v - mean;
            var += d * d;
        }
        var /= std::max<size_t>(1, _samples.size() - 1);
        const float stddev = sqrtf(std::max(0.0f, var));
        // Multiple detectors: legacy span, z-score, and relative ratio
        const bool spanBeat = (maximum - minimum) > _minRange;
        const bool zBeat = (stddev > 1e-6f) && ((basslevel - mean) / stddev >= _sigmaThresh);
        const bool ratioBeat = (mean > 1e-6f) && (basslevel >= mean * (1.0f + _ratioThresh));
        if (elapsed >= _minElapsed && (spanBeat || zBeat || ratioBeat))
        {
            debugV("Beat: elapsed=%0.2lf, span=%0.2f, z=%0.2f, ratio=%0.2f\n", elapsed, maximum - minimum,
                   (stddev > 1e-6f) ? ((basslevel - mean) / stddev) : 0.0f,
                   (mean > 1e-6f) ? (basslevel / mean - 1.0f) : 0.0f);
            HandleBeat(false, elapsed, maximum - minimum);
            _lastBeat = g_Values.AppTime.CurrentTime();
            _samples.clear(); // reset to avoid rapid re-triggering
        }
    }
};

// InsulatorColorBeatEffect
//
// Uses a very sensitive beat detection.  Fills all pixels blue based on VU, and on beats, fills a random insulator
// with a random color.  Longer beats get more insulators.  Very long beats get everything filled with purple.

class SimpleColorBeat : public BeatEffectBase, public EffectWithId<SimpleColorBeat>
{
  protected:
    int _iLastInsulator = -1;

    void Draw() override
    {
        ProcessAudio();

        CRGB c = CRGB::Blue * g_Analyzer.VURatio() * g_Values.AppTime.LastFrameTime() * 0.75;
        setPixelsOnAllChannels(0, NUM_LEDS, c, true);

        fadeAllChannelsToBlackBy(min(255.0, 1000.0 * g_Values.AppTime.LastFrameTime()));
        delay(1);
    }

    void HandleBeat(bool bMajor, float elapsed, float span) override
    {
        CRGB c;
        int cInsulators = 1;

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

            if (elapsed > 0.5) // Medium beats fill blue and proceed with insulator2
            {
                c = CHSV(beatsin8(4), 255, 255);
                cInsulators = random(1, NUM_FANS);
            }
            else if (elapsed > 1.0) // Long beats fill purple and return
            {
                c = CRGB::Purple;
                setAllOnAllChannels(c.r, c.g, c.b);
                return;
            }
        }

        int i;
        for (int iPass = 0; iPass < cInsulators; iPass++)
        {
            do
            {                            // Pick a different insulator than was used last time by:
                i = random(0, NUM_FANS); //  - Starting with a random number
            } while (i == _iLastInsulator); //  - Repeating until it doesn't match the last pass
            _iLastInsulator = i; // Our current choice forms the new "last" choice for next pass

            DrawFanPixels(0, FAN_SIZE, c, Sequential, i); // Draw twice to float-saturate our color
            DrawFanPixels(0, FAN_SIZE, c, Sequential, i);
        }
    }

  public:
    SimpleColorBeat(const String &strName) : BeatEffectBase(0.5, 0.25), EffectWithId<SimpleColorBeat>(strName)
    {
    }

    SimpleColorBeat(const JsonObjectConst &jsonObject)
        : BeatEffectBase(0.5, 0.25), EffectWithId<SimpleColorBeat>(jsonObject)
    {
    }
};

#endif // ENABLE_AUDIO
