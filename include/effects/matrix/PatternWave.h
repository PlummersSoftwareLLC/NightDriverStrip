//+--------------------------------------------------------------------------
//
// File:        PatternWave.h
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
// History:     May-26-2022        Davepl      Converted from Aurora
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

#ifndef PatternWave_H
#define PatternWave_H

class PatternWave : public LEDStripEffect
{
private:
    uint8_t thetaUpdate = 4;
    uint8_t thetaUpdateFrequency = 0;
    uint8_t theta = 0;

    uint8_t hueUpdate = 0;
    uint8_t hueUpdateFrequency = 0;
    uint8_t hue = 0;

    uint8_t rotation = 0;

    uint8_t scale = 256 / MATRIX_WIDTH;

    uint8_t maxX = MATRIX_WIDTH - 1;
    uint8_t maxY = MATRIX_HEIGHT - 1;

    uint8_t waveCount = 1;

    void construct()
    {
        rotation = random(0, 4);
        waveCount = random(1, 3);
    }


public:
    PatternWave() : LEDStripEffect(EFFECT_MATRIX_WAVE, "Wave")
    {
        construct();
    }

    PatternWave(const JsonObjectConst& jsonObject) : LEDStripEffect(jsonObject)
    {
        construct();
    }

    void Draw() override
    {
        auto graphics = g();

        int n = 0;

        switch (rotation) {
            case 0:
                for (int x = 0; x < MATRIX_WIDTH; x++) {
                    n = quadwave8(x * 2 + theta) / scale;
                    if (n < MATRIX_HEIGHT)
                    {
                        graphics->setPixel(x, n, graphics->ColorFromCurrentPalette(x + hue));
                        if (waveCount == 2)
                            graphics->setPixel(x, maxY - n, graphics->ColorFromCurrentPalette(x + hue));
                    }
                }
                break;

            case 1:
                for (int y = 0; y < MATRIX_HEIGHT; y++) {
                    n = quadwave8(y * 2 + theta) / scale;
                    if (n < MATRIX_WIDTH)
                    {
                        graphics->setPixel(n, y, graphics->ColorFromCurrentPalette(y + hue));
                        if (waveCount == 2)
                            graphics->setPixel(maxX - n, y, graphics->ColorFromCurrentPalette(y + hue));
                    }
                }
                break;

            case 2:
                for (int x = 0; x < MATRIX_WIDTH; x++) {
                    n = quadwave8(x * 2 - theta) / scale;
                    if (n < MATRIX_HEIGHT)
                    {
                        graphics->setPixel(x, n, graphics->ColorFromCurrentPalette(x + hue));
                        if (waveCount == 2)
                            graphics->setPixel(x, maxY - n, graphics->ColorFromCurrentPalette(x + hue));
                    }
                }
                break;

            case 3:
                for (int y = 0; y < MATRIX_HEIGHT; y++) {
                    n = quadwave8(y * 2 - theta) / scale;
                    if (n < MATRIX_WIDTH)
                    {
                        graphics->setPixel(n, y, graphics->ColorFromCurrentPalette(y + hue));
                        if (waveCount == 2)
                            graphics->setPixel(maxX - n, y, graphics->ColorFromCurrentPalette(y + hue));
                    }
                }
                break;
        }

        graphics->DimAll(254);

        if (thetaUpdate >= thetaUpdateFrequency) {
            thetaUpdate = 0;
            theta++;
        }
        else {
            thetaUpdate++;
        }

        if (hueUpdate >= hueUpdateFrequency) {
            hueUpdate = 0;
            hue++;
        }
        else {
            hueUpdate++;
        }
    }
};

#endif
