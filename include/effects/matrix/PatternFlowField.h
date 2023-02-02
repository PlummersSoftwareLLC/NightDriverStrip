//+--------------------------------------------------------------------------
//
// File:        PatternFlowField.h
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
//
// History:     Jul-27-2022         Davepl      Based on Aurora
//
//---------------------------------------------------------------------------

/*
 * Aurora: https://github.com/pixelmatix/aurora
 * Copyright (c) 2014 Jason Coon
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

#pragma once

#if USE_MATRIX

class PatternFlowField : public LEDStripEffect
{
public:
    PatternFlowField() : LEDStripEffect("FlowField")
    {
    }
    uint16_t x;
    uint16_t y;
    uint16_t z;

    uint16_t speed = 1;
    uint16_t scale = 26;

    static const int count = 40;

    uint8_t hue = 0;

    virtual void Start()
    {
        x = random16();
        y = random16();
        z = random16();

        for (int i = 0; i < count; i++)
        {
            mgraphics()->boids[i] = Boid(random(MATRIX_WIDTH), 0);
        }
    }

    virtual size_t DesiredFramesPerSecond() const
    {
        return 16;
    }
    
    virtual void Draw()
    {
        graphics()->DimAll(240);

        // CRGB color = effects.ColorFromCurrentPalette(hue);

        for (int i = 0; i < count; i++)
        {
            Boid *boid = &(mgraphics()->boids[i]);

            int ioffset = scale * boid->location.x;
            int joffset = scale * boid->location.y;

            uint8_t angle = inoise8(x + ioffset, y + joffset, z);

            boid->velocity.x = (float)sin8(angle) * 0.0078125 - 1.0;
            boid->velocity.y = -((float)cos8(angle) * 0.0078125 - 1.0);
            boid->update();

            graphics()->drawPixel(boid->location.x, boid->location.y, graphics()->ColorFromCurrentPalette(angle + hue)); // color

            if (boid->location.x < 0 || boid->location.x >= MATRIX_WIDTH ||
                boid->location.y < 0 || boid->location.y >= MATRIX_HEIGHT)
            {
                boid->location.x = random(MATRIX_WIDTH);
                boid->location.y = 0;
            }
        }

        EVERY_N_MILLIS(200)
        {
            hue++;
        }

        x += speed;
        y += speed;
        z += speed;
    }
};


#endif
