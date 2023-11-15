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
#if     USE_HUB75

// Introduction:
// -------------
// This file contains the implementation of the `PatternMandala` class, a sophisticated
// effect for LED strip displays. It utilizes a noise-based algorithm to create
// intricate, continuously evolving mandala patterns. This effect is part of a larger 
// system that drives LED strip animations.
//
// Class Overview:
// ---------------
// `PatternMandala` is derived from `LEDStripEffect`, indicating its purpose as a specific 
// visual effect for LED strips. It is designed to generate mandala-like patterns using 
// noise and random number generation to achieve a dynamic, ever-changing display.
//
// Key Variables:
// --------------
// - `dx`, `dy`, `dz`, `dsx`, `dsy`: These integers store the delta values for noise
//   coordinates and scaling, controlling the movement and zoom level of the noise pattern.
// - `NUM_LAYERS`: A macro defining the number of noise layers used in the pattern.

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
    PatternMandala() : LEDStripEffect(EFFECT_MATRIX_MANDALA, "MRI")
    {
    }

    PatternMandala(const JsonObjectConst& jsonObject) : LEDStripEffect(jsonObject)
    {
    }

    /// Generate an 8-bit random number

    virtual size_t DesiredFramesPerSecond() const override
    {
        return 30;
    }

    bool RequiresDoubleBuffering() const override
    {
        return true;
    }

    void Start() override
    {
        // set to reasonable values to avoid a black out
        g()->GetNoise().noisesmoothing = 100;

        // just any free input pin
        // random16_add_entropy(analogRead(18));

        // fill coordinates with random values
        // set zoom levels
        g()->GetNoise().noise_x = random16();
        g()->GetNoise().noise_y = random16();
        g()->GetNoise().noise_z = random16();
        g()->GetNoise().noise_scale_x = 6000;
        g()->GetNoise().noise_scale_y = 6000;

        // for the random movement
        dx = random8();
        dy = random8();
        dz = random8();
        dsx = random8();
        dsy = random8();
    }

    void Draw() override
    {
        // a new parameter set every 30 seconds
        EVERY_N_SECONDS(30)
        {
            // SetupRandomPalette3();
            dy = random16(500) - 250; // random16(2000) - 1000 is pretty fast but works fine, too
            dx = random16(500) - 250;
            dz = random16(500) - 250;
            g()->GetNoise().noise_scale_x = random16(10000) + 2000;
            g()->GetNoise().noise_scale_y = random16(10000) + 2000;
        }

        g()->GetNoise().noise_y += dy * 4;
        g()->GetNoise().noise_x += dx * 4;
        g()->GetNoise().noise_z += dz * 4;

        g()->FillGetNoise<NoiseApproach::One>();

        ShowNoiseLayer(0, 1, 0);

        g()->Caleidoscope3();
        g()->Caleidoscope1();
    }

    // show just one layer
    void ShowNoiseLayer(uint8_t layer, uint8_t colorrepeat, uint8_t colorshift)
    {
        for (uint16_t i = 0; i < MATRIX_WIDTH; i++)
        {
            for (uint16_t j = 0; j < MATRIX_HEIGHT; j++)
            {

                uint8_t color = g()->GetNoise().noise[i][j];

                uint8_t bri = color;

                // assign a color depending on the actual palette
                CRGB pixel = ColorFromPalette(g()->GetCurrentPalette(), colorrepeat * (color + colorshift), bri);

                g()->leds[XY(i, j)] = pixel;
            }
        }
    }
};

#endif
#endif
