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

class PatternPulse : public LEDStripEffect
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

    PatternPulse() : LEDStripEffect(EFFECT_MATRIX_PULSE, "Pulse")
    {
    }

    PatternPulse(const JsonObjectConst& jsonObject) : LEDStripEffect(jsonObject)
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
                graphics->DrawSafeCircle(centerX, centerY, step, graphics->to16bit(ColorFromPalette(RainbowColors_p, hue, pow(fadeRate, step - 2) * 255)));

                // secondary pulse
                if (step > 5)
                {
                    graphics->DrawSafeCircle(centerX, centerY, step - 3, graphics->to16bit(ColorFromPalette(RainbowColors_p, hue, pow(fadeRate, step - 2) * 255)));
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
class PatternPulsar : public BeatEffectBase, public LEDStripEffect
{
  private:

    struct PulsePop
    {
      public:

        int hue = HUE_RED;
        int centerX = 0;
        int centerY = 0;
        int maxSteps = random_range(0, 8)+6;
        int step = -1;
    };

    std::vector<PulsePop> _pops;

    float fadeRate = 0.9;
    int diff;

  public:

    PatternPulsar() :
        BeatEffectBase(1.5, 0.25 ),
        LEDStripEffect(EFFECT_MATRIX_PULSAR, "Pulsars")
    {
    }

    PatternPulsar(const JsonObjectConst& jsonObject) :
        BeatEffectBase(1.5, 0.25 ),
        LEDStripEffect(jsonObject)
    {
    }

    virtual size_t DesiredFramesPerSecond() const override
    {
        return 30;
    }

    virtual void HandleBeat(bool bMajor, float elapsed, float span) override
    {
        if (span > 1.5)
        {
            for (int i = 0; i < random(2)+2; i ++)
                _pops.push_back( PulsePop() );
        }
        else
        {
            PulsePop small;
            small.maxSteps = random(8)+4;
            _pops.push_back( small );
        }

    }

    void Draw() override
    {
        ProcessAudio();
        fadeAllChannelsToBlackBy(10);

        // Add some sparkle

        const int maxNewStarsPerFrame = 8;
        for (int i = 0; i < maxNewStarsPerFrame; i++)
            if (random(4) < g_Analyzer._VURatio)
                g()->drawPixel(random(MATRIX_WIDTH), random(MATRIX_HEIGHT), RandomSaturatedColor());


        for (auto pop = _pops.begin(); pop != _pops.end();)
        {
            if (pop->step == -1)
            {
                pop->centerX = random(MATRIX_WIDTH);
                pop->centerY = random(MATRIX_HEIGHT);
                pop->hue = random(256); // 170;
                pop->step = 0;
            }

            if (pop->step == 0)
            {
                g()->DrawSafeCircle(pop->centerX, pop->centerY, pop->step, g()->to16bit(g()->ColorFromCurrentPalette(pop->hue)));
                pop->step++;
                pop++;
            }
            else
            {
                if (pop->step < pop->maxSteps)
                {
                    // initial pulse
                    g()->DrawSafeCircle(pop->centerX, pop->centerY, pop->step, g()->to16bit(g()->ColorFromCurrentPalette(pop->hue, pow(fadeRate, pop->step - 1) * 255)));

                    // secondary pulse
                    if (pop->step > 3)
                        g()->DrawSafeCircle(pop->centerX, pop->centerY, pop->step - 3, g()->to16bit(g()->ColorFromCurrentPalette(pop->hue, pow(fadeRate, pop->step - 2) * 255)));

                    // This looks like PDP-11 code to me.  double post-inc for the win!
                    pop++->step++;
                }
                else
                {
                    // We remove the pulsar and do not increment the pop in the loop
                    // because we just deleted the current position
                    _pops.erase(pop);
                }
            }
        }

        // effects.standardNoiseSmearing();
    }
};
#endif
