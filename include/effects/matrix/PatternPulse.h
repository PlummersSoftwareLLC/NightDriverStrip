//+--------------------------------------------------------------------------
//
// File:        PatternPulse.h
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
//
// Description:
//
//   Effect code ported from Aurora to Mesmerizer's draw routines
//   and
//
// History:     May-26-2022         Davepl      Converted from Aurora
//
//---------------------------------------------------------------------------

/*
 * Aurora: https://github.com/pixelmatix/aurora
 * Copyright (c) 2014 Jason Coon
 *
 * Based at least in part on someone else's work that I can no longer find.
 * Please let me know if you recognize any of this code!
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this software and associated documentation files (the "Software"), to deal in
 * the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
 * the Software, and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#ifndef PatternPulse_H
#define PatternPulse_H

class PatternPulse : public EffectWithId<PatternPulse>
{
  private:
    int hue;
    int centerX = 0;
    int centerY = 0;
    int step = -1;
    int maxSteps = min(MATRIX_HEIGHT, MATRIX_WIDTH);
    float fadeRate = 0.90;
    int diff;

  public:
    PatternPulse() : EffectWithId<PatternPulse>("Pulse")
    {
    }
    PatternPulse(const JsonObjectConst &jsonObject) : EffectWithId<PatternPulse>(jsonObject)
    {
    }

    void Draw() override
    {
        auto graphics = g();

        graphics->DimAll(245);

        if (step == -1)
        {
            centerX = random(MATRIX_WIDTH);
            centerY = random(MATRIX_HEIGHT);
            hue = random(256); // 170;
            step = 0;
        }

        if (step == 0)
        {
            graphics->DrawSafeCircle(centerX, centerY, step, graphics->to16bit(graphics->ColorFromCurrentPalette(hue)));
            step++;
        }
        else
        {
            if (step < maxSteps)
            {
                // initial pulse
                graphics->DrawSafeCircle(
                    centerX, centerY, step,
                    graphics->to16bit(ColorFromPalette(RainbowColors_p, hue, pow(fadeRate, step - 2) * 255)));

                // secondary pulse
                if (step > 5)
                {
                    graphics->DrawSafeCircle(
                        centerX, centerY, step - 3,
                        graphics->to16bit(ColorFromPalette(RainbowColors_p, hue, pow(fadeRate, step - 2) * 255)));
                }
                step++;
            }
            else
            {
                step = -1;
            }
        }
        // effects.standardNoiseSmearing();
    }
};
class PatternPulsar : public BeatEffectBase, public EffectWithId<PatternPulsar>
{
  private:
    struct PulsePop
    {
        int hue = HUE_RED;
        int centerX = 0;
        int centerY = 0;
        int maxSteps = random_range(0, 10) + 6; // wider pulse variety
        int step = -1;
    };

    std::vector<PulsePop> _pops;

    float fadeRate = 0.9;
    int diff;

  public:
    PatternPulsar() : BeatEffectBase(1.5, 0.25), EffectWithId<PatternPulsar>("Pulsars")
    {
        SetBeatSensitivity(1.5f, 0.25f, 2);
    }

    PatternPulsar(const JsonObjectConst &jsonObject)
        : BeatEffectBase(1.5, 0.25), EffectWithId<PatternPulsar>(jsonObject)
    {
        SetBeatSensitivity(1.5f, 0.25f, 2);
    }

    virtual size_t DesiredFramesPerSecond() const override
    {
        return 30;
    }

    virtual void HandleBeat(bool bMajor, float elapsed, float span) override
    {
        // Consider a major beat if it's been a while or the energy span is large
        const bool major = bMajor || elapsed > 0.6f || span > 2.0f;
        if (major)
        {
            int count = 2 + random(3); // 2..4 pops
            while (count--)
                _pops.push_back(PulsePop());
        }
        else
        {
            PulsePop small;
            small.maxSteps = random(8) + 4;
            _pops.push_back(small);
        }
        // Prevent unbounded growth in extreme audio, trim oldest
        constexpr size_t kMaxPops = 64;
        if (_pops.size() > kMaxPops)
            _pops.erase(_pops.begin(), _pops.begin() + (_pops.size() - kMaxPops));
    }

    void Draw() override
    {
        ProcessAudio();
        fadeAllChannelsToBlackBy(10);
        // Add some sparkle
        const int maxNewStarsPerFrame = 8;
        for (int i = 0; i < maxNewStarsPerFrame; i++)
            if (random(4) < g_Analyzer.VURatio())
                g()->drawPixel(random(MATRIX_WIDTH), random(MATRIX_HEIGHT), RandomSaturatedColor());
        for (auto pop = _pops.begin(); pop != _pops.end();)
        {
            if (pop->step == -1)
            {
                // Initialize at full size immediately for tight audio sync
                pop->centerX = random(MATRIX_WIDTH);
                pop->centerY = random(MATRIX_HEIGHT);
                pop->hue = random(256);
                pop->step = pop->maxSteps; // start at full radius, then shrink
            }
            if (pop->step > 0)
            {
                // Brightest at start, then decay as radius shrinks
                const float a1 = powf(fadeRate, (float)(pop->maxSteps - pop->step));
                const uint8_t v1 = (uint8_t)std::min(255.0f, a1 * 255.0f);
                g()->DrawSafeCircle(pop->centerX, pop->centerY, pop->step,
                                    g()->to16bit(g()->ColorFromCurrentPalette(pop->hue, v1)));
                // Secondary outer ring, if still within bounds
                int r2 = pop->step + 3;
                if (r2 <= pop->maxSteps)
                {
                    const float a2 = powf(fadeRate, (float)(pop->maxSteps - r2));
                    const uint8_t v2 = (uint8_t)std::min(255.0f, a2 * 255.0f);
                    g()->DrawSafeCircle(pop->centerX, pop->centerY, r2,
                                        g()->to16bit(g()->ColorFromCurrentPalette(pop->hue, v2)));
                }
                // Shrink for next frame
                pop->step--;
                ++pop;
            }
            else
            {
                // Finished shrinking; remove this pop
                _pops.erase(pop);
            }
        }
        // effects.standardNoiseSmearing();
    }
};
#endif
