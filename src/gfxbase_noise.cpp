//+--------------------------------------------------------------------------
//
// File:        gfxbase_noise.cpp
//
// This file is part of gfxbase.cpp; see that file header for additional context.
//
// Split scope: GFXBase noise generation, frame prep, and post-processing helpers.
//---------------------------------------------------------------------------


#include "globals.h"

#include <algorithm>
#include <cmath>
#include <gfxfont.h>
#include <memory>
#include <stdexcept>
#include <unordered_map>

#include "effectmanager.h"
#include "effects/matrix/Boid.h"
#include "gfxbase.h"
#include "systemcontainer.h"

#if USE_NOISE
    // The following functions are specializations of noise-related member function
    // templates declared in gfxbase.h.

    void GFXBase::SetNoise(uint32_t nx, uint32_t ny, uint32_t nz, uint32_t sx, uint32_t sy)
    {
        EnsureNoise();
        _ptrNoise->noise_x += nx;
        _ptrNoise->noise_y += ny;
        _ptrNoise->noise_z += nz;
        _ptrNoise->noise_scale_x = sx;
        _ptrNoise->noise_scale_y = sy;
    }

    // Internal implementation: assumes _ptrNoise is already initialized.
    // Must NOT call EnsureNoise() — this is invoked from within EnsureNoise()'s
    // call_once lambda, and a recursive call_once on the same flag is UB.
    void GFXBase::FillGetNoiseImpl() const
    {
        // Subtracting the center offset before scaling ensures the noise pattern radiates
        // outwards from the center of the display (exactly as #803 intended).
        //
        // We use uint32_t for the indices as it's the native register size for the ESP32,
        // avoiding the unnecessary overhead of masking/extending smaller types.
        for (uint32_t i = 0; i < _width; i++)
        {
            int32_t ioffset = _ptrNoise->noise_scale_x * (int32_t)(i - (_width / 2));

            for (uint32_t j = 0; j < _height; j++)
            {
                int32_t joffset = _ptrNoise->noise_scale_y * (int32_t)(j - (_height / 2));
                uint8_t data    = inoise16(_ptrNoise->noise_x + ioffset, _ptrNoise->noise_y + joffset, _ptrNoise->noise_z) >> 8;
                uint8_t olddata = _ptrNoise->noise[i][j];
                uint8_t newdata = scale8(olddata, _ptrNoise->noisesmoothing) + scale8(data, 256 - _ptrNoise->noisesmoothing);

                _ptrNoise->noise[i][j] = newdata;
            }
        }
    }

    void GFXBase::FillGetNoise() const
    {
        if (!EnsureNoise())
            FillGetNoiseImpl();
    }

    template<>
    void GFXBase::MoveFractionalNoiseX<NoiseApproach::MRI>(uint8_t amt, uint8_t shift)
    {
        EnsureNoise();
        std::unique_ptr<CRGB[]> ledsTemp = std::make_unique<CRGB[]>(_ledcount);

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
        EnsureNoise();
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
        EnsureNoise();
        std::unique_ptr<CRGB[]> ledsTemp = std::make_unique<CRGB[]>(_ledcount);

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
        EnsureNoise();
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

void GFXBase::PrepareFrame()
{
}

void GFXBase::PostProcessFrame(uint16_t, uint16_t)
{
}
