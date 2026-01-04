//---------------------------------------------------------------------------
//
// File:        gfxbase.cpp
//
// NightDriverStrip - (c) 2023 Plummer's Software LLC.  All Rights Reserved.
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
//   Provides definitions for GFXBase member functions that can't be included
//   in gfxbase.h because C++ won't allow us.
//
// History:     Sep-15-2023        Rbergen     Created
//
//---------------------------------------------------------------------------

#include "globals.h"
#include "gfxbase.h"
#include <gfxfont.h>                 // Include Adafruit GFX font types BEFORE any M5/LovyanGFX includes
#include "systemcontainer.h"

#if USE_NOISE
    // The following functions are specializations of noise-related member function
    // templates declared in gfxbase.h.

    template<>
    void GFXBase::MoveFractionalNoiseX<NoiseApproach::MRI>(uint8_t amt, uint8_t shift)
    {
        std::unique_ptr<CRGB[]> ledsTemp = make_unique_psram<CRGB[]>(_ledcount);

        // move delta pixelwise
        for (uint32_t y = 0; y < _height; y++)
        {
            uint16_t amount = _ptrNoise->noise[0][y] * amt;
            uint8_t delta = _width - 1 - (amount / 256);

            // Process up to the end less the delta
            for (uint32_t x = 0; x < _width - delta; x++)
                ledsTemp[XY(x, y)] = leds[XY(x + delta, y)];

            // Do the tail portion while wrapping around
            for (uint32_t x = _width - delta; x < _width; x++)
                ledsTemp[XY(x, y)] = leds[XY(x + delta - _width, y)];
        }

        // move fractions
        CRGB PixelA;
        CRGB PixelB;

        for (uint32_t y = 0; y < _height; y++)
        {
            uint16_t amount = _ptrNoise->noise[0][y] * amt;
            uint8_t delta = _width - 1 - (amount / 256);
            uint8_t fractions = amount - (delta * 256);

            for (uint32_t x = 1; x < _width; x++)
            {
                PixelA = ledsTemp[XY(x, y)];
                PixelB = ledsTemp[XY(x - 1, y)];

                PixelA %= 255 - fractions;
                PixelB %= fractions;

                leds[XY(x, y)] = PixelA + PixelB;
            }

            PixelA = ledsTemp[XY(0, y)];
            PixelB = ledsTemp[XY(_width - 1, y)];

            PixelA %= 255 - fractions;
            PixelB %= fractions;

            leds[XY(0, y)] = PixelA + PixelB;
        }
    }

    template<>
    void GFXBase::MoveFractionalNoiseX<NoiseApproach::General>(uint8_t amt, uint8_t shift)
    {
        // Aligning with Approach::One while keeping the "Approach::Two" optimized behavior.
        // We use int32_t for the 'amount' and 'delta' as they can be large or negative.
        for (uint32_t y = 0; y < _height; y++)
        {
            int32_t amount = ((int32_t)_ptrNoise->noise[0][y] - 128) * 2 * amt + shift * 256;
            int32_t delta = abs(amount) >> 8;
            int32_t fraction = abs(amount) & 255;

            for (uint32_t x = 0; x < _width; x++)
            {
                int32_t zD;
                int32_t zF;

                if (amount < 0)
                {
                    zD = (int32_t)x - delta;
                    zF = zD - 1;
                }
                else
                {
                    zD = (int32_t)x + delta;
                    zF = zD + 1;
                }

                CRGB PixelA = CRGB::Black;
                if ((zD >= 0) && (zD < (int32_t)_width))
                    PixelA = leds[XY(zD, y)];

                CRGB PixelB = CRGB::Black;
                if ((zF >= 0) && (zF < (int32_t)_width))
                    PixelB = leds[XY(zF, y)];

                leds[XY(x, y)] = (PixelA.nscale8(ease8InOutApprox(255 - fraction))) + (PixelB.nscale8(ease8InOutApprox(fraction)));
            }
        }
    }

    template<>
    void GFXBase::MoveFractionalNoiseY<NoiseApproach::MRI>(uint8_t amt, uint8_t shift)
    {
        std::unique_ptr<CRGB[]> ledsTemp = make_unique_psram<CRGB[]>(_ledcount);

        // move delta pixelwise
        for (uint32_t x = 0; x < _width; x++)
        {
            uint16_t amount = _ptrNoise->noise[x][0] * amt;
            uint8_t delta = _height - 1 - (amount / 256);

            for (uint32_t y = 0; y < _height - delta; y++)
            {
                ledsTemp[XY(x, y)] = leds[XY(x, y + delta)];
            }
            for (uint32_t y = _height - delta; y < _height; y++)
            {
                ledsTemp[XY(x, y)] = leds[XY(x, y + delta - _height)];
            }
        }

        // move fractions
        CRGB PixelA;
        CRGB PixelB;

        for (uint32_t x = 0; x < _width; x++)
        {
            uint16_t amount = _ptrNoise->noise[x][0] * amt;
            uint8_t delta = _height - 1 - (amount / 256);
            uint8_t fractions = amount - (delta * 256);

            for (uint32_t y = 1; y < _height; y++)
            {
                PixelA = ledsTemp[XY(x, y)];
                PixelB = ledsTemp[XY(x, y - 1)];

                PixelA %= 255 - fractions;
                PixelB %= fractions;

                leds[XY(x, y)] = PixelA + PixelB;
            }

            PixelA = ledsTemp[XY(x, 0)];
            PixelB = ledsTemp[XY(x, _height - 1)];

            PixelA %= 255 - fractions;
            PixelB %= fractions;

            leds[XY(x, 0)] = PixelA + PixelB;
        }
    }

    template<>
    void GFXBase::MoveFractionalNoiseY<NoiseApproach::General>(uint8_t amt, uint8_t shift)
    {
        for (uint32_t x = 0; x < _width; x++)
        {
            int32_t amount = ((int32_t)_ptrNoise->noise[x][0] - 128) * 2 * amt + shift * 256;
            int32_t delta = abs(amount) >> 8;
            int32_t fraction = abs(amount) & 255;

            for (uint32_t y = 0; y < _height; y++)
            {
                int32_t zD;
                int32_t zF;

                if (amount < 0)
                {
                    zD = (int32_t)y - delta;
                    zF = zD - 1;
                }
                else
                {
                    zD = (int32_t)y + delta;
                    zF = zD + 1;
                }

                CRGB PixelA = CRGB::Black;
                if ((zD >= 0) && (zD < (int32_t)_height))
                    PixelA = leds[XY(x, zD)];

                CRGB PixelB = CRGB::Black;
                if ((zF >= 0) && (zF < (int32_t)_height))
                    PixelB = leds[XY(x, zF)];

                leds[XY(x, y)] = (PixelA.nscale8(ease8InOutApprox(255 - fraction))) + (PixelB.nscale8(ease8InOutApprox(fraction)));
            }
        }
    }
#endif

// This can't be in gfxbase.h because it uses the FillGetNoise() function template.

GFXBase::GFXBase(int w, int h) : Adafruit_GFX(w, h),
                        _width(w),
                        _height(h),
                        _ledcount(w*h)
{
    // Allocate boids for matrix effects (like PatternBounce) when we have matrix dimensions
    #if MATRIX_HEIGHT > 1
        debugV("Allocating boids for matrix effects");
        _boids.reset(psram_allocator<Boid>().allocate(MATRIX_WIDTH));
        assert(_boids);
    #endif

    #if USE_NOISE
        debugV("Allocating noise");
        _ptrNoise = std::make_unique<Noise>();          // Avoid specific PSRAM allocation since highly random access
        assert(_ptrNoise);
        debugV("Setting up noise");
        NoiseVariablesSetup();
        debugV("Filling noise");
        FillGetNoise();
    #endif

    debugV("Setting up palette");
    loadPalette(0);
    ResetOscillators();
}

// Remove the XY macro definition that was set in gfxbase.h. In this file we won't use it beyond this point anyway.
#undef XY

// Dirty hack to support FastLED, which calls out of band to get the pixel index for "the" array, without
// any indication of which array or who's asking, so we assume the first matrix. If you have trouble with
// more than one matrix and some FastLED functions like blur2d, this would be why.

uint16_t XY(uint8_t x, uint8_t y)
{
    static auto& g = *(g_ptrSystem->EffectManager().g());

    return g.xy(x, y);
}
