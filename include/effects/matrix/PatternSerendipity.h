//+--------------------------------------------------------------------------
//
// File:        PatternSerendipity.h
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
 * Portions of this code are adapted from "Funky Clouds" by Stefan Petrick:
 * https://gist.github.com/anonymous/876f908333cd95315c35
 *
 * Copyright (c) 2014 Stefan Petrick
 * http://www.stefan-petrick.de/wordpress_beta
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

#ifndef PatternSpiral_H
#define PatternSpiral_H

class PatternSerendipity : public LEDStripEffect
{
private:
    // Timer stuff (Oszillators)
    struct timer
    {
        unsigned long takt;
        unsigned long lastMillis;
        unsigned long count;
        int delta;
        uint8_t up;
        uint8_t down;
    };
    timer multiTimer[5];

    int timers = sizeof(multiTimer) / sizeof(multiTimer[0]);

    // counts all variables with different speeds linear up and down
    void UpdateTimers()
    {
        unsigned long now = millis();
        for (int i = 0; i < timers; i++)
        {
            while (now - multiTimer[i].lastMillis >= multiTimer[i].takt)
            {
                multiTimer[i].lastMillis += multiTimer[i].takt;
                multiTimer[i].count = multiTimer[i].count + multiTimer[i].delta;
                if ((multiTimer[i].count == multiTimer[i].up) || (multiTimer[i].count == multiTimer[i].down))
                {
                    multiTimer[i].delta = -multiTimer[i].delta;
                }
            }
        }
    }

public:
    PatternSerendipity() : LEDStripEffect(EFFECT_MATRIX_SERENDIPITY, "Serendipiti")
    {
    }

    PatternSerendipity(const JsonObjectConst& jsonObject) : LEDStripEffect(jsonObject)
    {
    }

    bool Init(std::vector<std::shared_ptr<GFXBase>>& gfx) override
    {
        if (!LEDStripEffect::Init(gfx))
            return false;

        // set all counting directions positive for the beginning
        for (int i = 0; i < timers; i++)
            multiTimer[i].delta = 1;

        // set range (up/down), speed (takt=ms between steps) and starting point of all oszillators

        unsigned long now = millis();

        multiTimer[0].lastMillis = now;
        multiTimer[0].takt = 42; // x1
        multiTimer[0].up = MATRIX_WIDTH - 1;
        multiTimer[0].down = 0;
        multiTimer[0].count = 0;

        multiTimer[1].lastMillis = now;
        multiTimer[1].takt = 55; // y1
        multiTimer[1].up = MATRIX_HEIGHT - 1;
        multiTimer[1].down = 0;
        multiTimer[1].count = 0;

        multiTimer[2].lastMillis = now;
        multiTimer[2].takt = 3; // color
        multiTimer[2].up = 255;
        multiTimer[2].down = 0;
        multiTimer[2].count = 0;

        multiTimer[3].lastMillis = now;
        multiTimer[3].takt = 71; // x2
        multiTimer[3].up = MATRIX_WIDTH - 1;
        multiTimer[3].down = 0;
        multiTimer[3].count = 0;

        multiTimer[4].lastMillis = now;
        multiTimer[4].takt = 89; // y2
        multiTimer[4].up = MATRIX_HEIGHT - 1;
        multiTimer[4].down = 0;
        multiTimer[4].count = 0;

        return true;
    }

    void Draw() override
    {
        auto graphics = g();

        // manage the Oszillators
        UpdateTimers();

        // draw just a line defined by 5 oszillators

        graphics->drawLine(
            multiTimer[3].count, // x1
            multiTimer[4].count, // y1
            multiTimer[0].count, // x2
            multiTimer[1].count, // y2
            CRGB(CHSV(multiTimer[2].count, 255, 255)));

        graphics->BlurFrame(50);

        // increase the contrast
//        graphics->DimAll(252);
        return;
    }
};

#endif
