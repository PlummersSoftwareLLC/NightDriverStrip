//+--------------------------------------------------------------------------
//
// File:        PatternMandala.h
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
// History:     Jun-25-2022         Davepl      Based on Aurora
//
//---------------------------------------------------------------------------

/*
 * Aurora: https://github.com/pixelmatix/aurora
 * Copyright (c) 2014 Jason Coon
 *
 * Portions of this code are adapted from "Funky Noise" by Stefan Petrick: https://github.com/StefanPetrick/FunkyNoise
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

#ifndef PatternMandala_H
#define PatternMandala_H

#include "globals.h"
#include "ledstripeffect.h"
#include "gfxbase.h"

class PatternMandala : public LEDStripEffect
{
private:
    // The coordinates for 16-bit noise spaces.
#define NUM_LAYERS 1

    // used for the random based animations
    int16_t dx;
    int16_t dy;
    int16_t dz;
    int16_t dsx;
    int16_t dsy;

public:
    PatternMandala() : LEDStripEffect("MRI")
    {
    }

    /// Generate an 8-bit random number

    virtual size_t DesiredFramesPerSecond()
    {
        return 45;
    }

    virtual void Start()
    {
        // set to reasonable values to avoid a black out
        graphics()->noisesmoothing = 100;

        // just any free input pin
        // random16_add_entropy(analogRead(18));

        // fill coordinates with random values
        // set zoom levels
        graphics()->noise_x = random16();
        graphics()->noise_y = random16();
        graphics()->noise_z = random16();
        graphics()->noise_scale_x = 6000;
        graphics()->noise_scale_y = 6000;

        // for the random movement
        dx = random8();
        dy = random8();
        dz = random8();
        dsx = random8();
        dsy = random8();
    }

    virtual void Draw()
    {
        // a new parameter set every 15 seconds
        EVERY_N_SECONDS(15)
        {
            // SetupRandomPalette3();
            dy = random16(500) - 250; // random16(2000) - 1000 is pretty fast but works fine, too
            dx = random16(500) - 250;
            dz = random16(500) - 250;
            graphics()->noise_scale_x = random16(10000) + 2000;
            graphics()->noise_scale_y = random16(10000) + 2000;
        }

        graphics()->noise_y += dy * 4;
        graphics()->noise_x += dx * 4;
        graphics()->noise_z += dz * 4;

        graphics()->FillNoise();
        ShowNoiseLayer(0, 1, 0);

        graphics()->Caleidoscope3();
        graphics()->Caleidoscope1();
    }

    // show just one layer
    void ShowNoiseLayer(byte layer, byte colorrepeat, byte colorshift)
    {
        for (uint8_t i = 0; i < MATRIX_WIDTH; i++)
        {
            for (uint8_t j = 0; j < MATRIX_HEIGHT; j++)
            {

                uint8_t color = graphics()->noise[i][j];

                uint8_t bri = color;

                // assign a color depending on the actual palette
                CRGB pixel = ColorFromPalette(graphics()->currentPalette, colorrepeat * (color + colorshift), bri);

                graphics()->leds[graphics()->xy(i, j)] = pixel;
            }
        }
    }
};

#endif
