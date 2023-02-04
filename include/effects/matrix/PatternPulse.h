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

    PatternPulse() : LEDStripEffect("Pulse")
    {
    }

    virtual void Draw()
    {
        auto graphics = (GFXBase *) _GFX[0].get();

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
            graphics->drawCircle(centerX, centerY, step, graphics->to16bit(graphics->ColorFromCurrentPalette(hue)));
            step++;
        }
        else
        {
            if (step < maxSteps)
            {
                // initial pulse
                graphics->drawCircle(centerX, centerY, step, graphics->to16bit(ColorFromPalette(RainbowColors_p, hue, pow(fadeRate, step - 2) * 255)));

                // secondary pulse
                if (step > 5)
                {
                    graphics->drawCircle(centerX, centerY, step - 3, graphics->to16bit(ColorFromPalette(RainbowColors_p, hue, pow(fadeRate, step - 2) * 255)));
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

        int hue;
        int centerX;
        int centerY;
        int maxSteps = random(32)+16;
        int step = -1;
    };

    std::vector<PulsePop> _pops;

    float fadeRate = 0.9;
    int diff;

  public:

    PatternPulsar(double lowLatch = 1, double highLatch = 1, double minElapsed = 0.00) :
        BeatEffectBase(1.95, 0.25 ),
        LEDStripEffect("Pulsars")
    {
    }
    
    virtual size_t DesiredFramesPerSecond() const
    {
        return 50;
    }

    virtual void HandleBeat(bool bMajor, float elapsed, double span)
    {
        if (span > 1.95)
        {
            for (int i = 0; i < random(2)+2; i ++)
                _pops.push_back( PulsePop() );            
        }
        else
        {
            PulsePop small;
            small.maxSteps = random(12)+4;
            _pops.push_back( small );
        }
    }

    virtual void Draw()
    {
        ProcessAudio();
        
        //VUMeterEffect::DrawVUMeter(graphics, 0);
        //blur2d(graphics->leds, MATRIX_WIDTH, MATRIX_HEIGHT, 25);
        fadeAllChannelsToBlackBy(20);

        // Add some sparkle

        const int maxNewStarsPerFrame = 4;
        for (int i = 0; i < maxNewStarsPerFrame; i++)
            if (random(4) < g_Analyzer._VURatio)
                graphics()->drawPixel(random(MATRIX_WIDTH), random(MATRIX_HEIGHT), RandomSaturatedColor());

        for (auto pop = _pops.begin(); pop != _pops.end(); pop++)
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
                graphics()->drawCircle(pop->centerX, pop->centerY, pop->step, graphics()->to16bit(graphics()->ColorFromCurrentPalette(pop->hue)));
                pop->step++;
            }
            else
            {
                if (pop->step < pop->maxSteps)
                {
                    // initial pulse
                    graphics()->drawCircle(pop->centerX, pop->centerY, pop->step, graphics()->to16bit(graphics()->ColorFromCurrentPalette(pop->hue, pow(fadeRate, pop->step - 2) * 255)));

                    // secondary pulse
                    if (pop->step > 3)
                    {
                        graphics()->drawCircle(pop->centerX, pop->centerY, pop->step - 3, graphics()->to16bit(graphics()->ColorFromCurrentPalette(pop->hue, pow(fadeRate, pop->step - 2) * 255)));
                    }
                    pop->step++;
                }
                else
                {
                    _pops.erase(pop--);
                }
            }
        }

        // effects.standardNoiseSmearing();
    }
};
#endif
