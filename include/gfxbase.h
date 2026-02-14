#pragma once

//+--------------------------------------------------------------------------
//
// File:        gfxbase.h
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
//   Provides an Adafruit_GFX implementation for our RGB LED panel so that
//   we can use primitives such as lines and fills on it.  Incorporates
//   the Effects class from Aurora (see below) so it's available as well.
//
// History:     Oct-9-2018         Davepl      Created from other projects
//              May-26-2022        Davepl      Refactor and add Effects features
//
//---------------------------------------------------------------------------

/*
 * Aurora: https://github.com/pixelmatix/aurora
 * Copyright (c) 2014 Jason Coon
 *
 * Portions of this code are adapted from "Funky Clouds" by Stefan Petrick:
 *   https://gist.github.com/anonymous/876f908333cd95315c35
 * Portions of this code are adapted from "NoiseSmearing" by Stefan Petrick:
 *   https://gist.github.com/StefanPetrick/9ee2f677dbff64e3ba7a
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

#include "globals.h"

#include <algorithm>
#include <memory>
#include <mutex>

#include "Adafruit_GFX.h"
#include "pixeltypes.h"

// Calculates a weight for anti-aliasing in Wu's algorithm.
constexpr static inline uint8_t WU_WEIGHT(uint8_t a, uint8_t b)
{
    return (uint8_t)(((a) * (b) + (a) + (b)) >> 8);
}

#if USE_MATRIX
    #define USE_NOISE 1
#endif

#if USE_NOISE
    typedef struct
    {
        uint32_t noise_x;
        uint32_t noise_y;
        uint32_t noise_z;
        uint32_t noise_scale_x;
        uint32_t noise_scale_y;
        uint8_t  noise[MATRIX_WIDTH][MATRIX_HEIGHT];
        uint8_t  noisesmoothing;
    } Noise;

    // A "Noise Pool" in the context of computer graphics is a multi-dimensional array of
    // pseudo-random values that are spatially coherent. Unlike the "static" on a TV
    // or white noise in electronics (which is totally random from one pixel to the next),
    // graphics noise (like Perlin or Simplex) changes smoothly across space.
    // This allows for organic-looking animations like clouds, smoke, or fire.
    //
    // The Noise Pool here is populated by the FillGetNoise() method.
    //
    // Both noise approaches introduced below are identical as to how the noise pool is filled,
    // but they differ in how they use the noise pool after it's been filled to achieve different
    // visual effects.

    enum class NoiseApproach
    {
        General,    // General approach used by most noise effects
        MRI         // MRI-style complex symmetries
    };
#endif

class Boid;

class GFXBase : public Adafruit_GFX
{
#if USE_NOISE
private:
    // The standard noise approach used for noise function templates, if none is specified
    // at the point of invocation.
    static constexpr NoiseApproach _defaultNoiseApproach = NoiseApproach::General;
#endif

protected:
    size_t _width;
    size_t _height;
    size_t _ledcount;

    // 32 Entries in the 5-bit gamma table
    static const uint8_t gamma5[32];

    // 64 Entries in the 6-bit gamma table
    static const uint8_t gamma6[64];

    static constexpr int _paletteCount = 10;
    int _paletteIndex = -1;
    uint _lastSecond = 99;
    bool _palettePaused = false;

    TBlendType _currentBlendType = LINEARBLEND;
    CRGBPalette16 _currentPalette;
    CRGBPalette16 _targetPalette;
    String _currentPaletteName;

    #if USE_NOISE
        std::unique_ptr<Noise> _ptrNoise;
    #endif

    static constexpr int _heatColorsPaletteIndex = 6;
    static constexpr int _randomPaletteIndex = 9;

public:
    static const uint16_t kMatrixWidth = MATRIX_WIDTH;                                  // known working for actual matrix effects: 32, 64, 96, 128
    static const uint16_t kMatrixHeight = MATRIX_HEIGHT;                                // known working for actual matrix effects: 16, 32, 48, 64

    // A 3-byte struct will have one byte of padding so each element
    // begins on a NA boundary. Making this
    // struct __attribute__((packed)) PolarMap
    // might conserve 25% of this buffer, but it might also force
    // single elements to be split across a (locked) cache line.
    // We'll thus leave this naturally aligned unless we have a
    // really great reason not to.
    struct PolarMap {
        uint8_t angle;
        uint8_t scaled_radius;
        uint8_t unscaled_radius;
    };

    // Many of the Aurora effects need direct access to these from external classes

    CRGB *leds = nullptr;
    #if MATRIX_HEIGHT > 1
        std::unique_ptr<Boid[]> _boids;
    #endif

    using PolarMapArray = PolarMap[kMatrixWidth][kMatrixHeight];

    // Definition moved to GFXBase.cpp because it uses the FillGetNoise() function template
    GFXBase(int w, int h);

    virtual ~GFXBase() override;

    #if USE_NOISE
    Noise &GetNoise() const
    {
        return *_ptrNoise;
    }
    #endif

    const CRGBPalette16 &GetCurrentPalette() const
    {
        return _currentPalette;
    }

    virtual size_t GetLEDCount() const
    {
        return _ledcount;
    }

    static uint8_t beatcos8(accum88 beats_per_minute, uint8_t lowest = 0, uint8_t highest = 255, uint32_t timebase = 0, uint8_t phase_offset = 0);
    static uint8_t mapsin8(uint8_t theta, uint8_t lowest = 0, uint8_t highest = 255);
    static uint8_t mapcos8(uint8_t theta, uint8_t lowest = 0, uint8_t highest = 255);

    static CRGB from16Bit(uint16_t color); // Convert 16bit 5:6:5 to 24bit color using lookup table for gamma
    static uint16_t to16bit(uint8_t r, uint8_t g, uint8_t b); // Convert RGB -> 16bit 5:6:5
    static uint16_t to16bit(const CRGB rgb); // Convert CRGB -> 16 bit 5:6:5
    static uint16_t to16bit(CRGB::HTMLColorCode code); // Convert HtmlColorCode -> 16 bit 5:6:5

    virtual void Clear(CRGB color = CRGB::Black);

    __attribute__((always_inline))
    virtual bool isValidPixel(uint x, uint y) const noexcept
    {
        // Check that the pixel location is within the matrix's bounds
        return x < _width && y < _height;
    }

    __attribute__((always_inline))
    virtual bool isValidPixel(uint n) const noexcept
    {
        // Check that the pixel location is within the matrix's bounds
        return n < _ledcount;
    }

    // Matrices that are built from individually addressable strips like WS2812b generally
    // follow a boustrophedon layout as follows:
    //
    //     0 >  1 >  2 >  3 >  4
    //                         |
    //     9 <  8 <  7 <  6 <  5
    //     |
    //    10 > 11 > 12 > 13 > 14
    //                         |
    //    19 < 18 < 17 < 16 < 15
    //     |
    //    (etc.)
    //
    // If your matrix uses a different approach, you can override this function and implement it
    // in the XY() function of your class

    __attribute__((always_inline))
    inline virtual uint16_t xy(uint16_t x, uint16_t y) const noexcept
    {
        if (x & 0x01)
        {
            // Odd rows run backwards
            uint8_t reverseY = (_height - 1) - y;
            return (x * _height) + reverseY;
        }
        else
        {
            // Even rows run forwards
            return (x * _height) + y;
        }
    }

    // This is an optimization that allows us to use direct math for the XY lookup when using the matrix, where
    // it's a very simple layout.  Others may need to override this function.  Using a #define here allows
    // us to avoid an extra virtual function call in the inner loop of the effects.

    #if USE_HUB75
        #define XY(x, y) ((y) * MATRIX_WIDTH + (x))
    #elif HELMET
        #define XY(x, y) (x, MATRIX_HEIGHT - 1 - y)           // Invert the Y axis for the helmet display
    #else
        #define XY(x, y) (((x) & 0x01) ? (((x) * MATRIX_HEIGHT) + ((MATRIX_HEIGHT - 1) - (y))) : (((x) * MATRIX_HEIGHT) + (y)))
    #endif

    // Retrieves the color of a pixel at the specified X and Y coordinates.
    virtual CRGB getPixel(int16_t x, int16_t y) const;

    // Retrieves the color of a pixel at the specified linear index.
    virtual CRGB getPixel(int16_t i) const;

    __attribute__((always_inline)) virtual void addColor(int16_t i, CRGB c)
    {
        if (isValidPixel(i))
            leds[i] += c;
    }

    __attribute__((always_inline)) virtual void drawPixel(int16_t x, int16_t y, CRGB color)
    {
        if (isValidPixel(x, y))
            leds[XY(x, y)] = color;
    }

    void drawPixel(int16_t x, int16_t y, uint16_t color) override;

    // Blends a color with the existing pixel color.
    void drawPixelXY_Blend(uint8_t x, uint8_t y, CRGB color, uint8_t blend_amount);

    // Draws an anti-aliased pixel using Wu's algorithm, blending with the background.
    void drawPixelXYF_Wu(float x, float y, CRGB color);

    // Draws a gradient line using floating point coordinates (DDA algorithm).
    void drawLineF(float x1, float y1, float x2, float y2, const CRGB &col1, const CRGB &col2 = CRGB::Black);

    // Draws a solid circle using floating point coordinates.
    void drawSafeFilledCircleF(float cx, float cy, float radius, CRGB col);

    virtual void fillLeds(std::unique_ptr<CRGB[]> &pLEDs);

    virtual void setPixel(int16_t x, int16_t y, uint16_t color);
    virtual void setPixel(int16_t x, int16_t y, CRGB color);
    virtual void setPixel(int16_t x, int r, int g, int b);

    // Fast per-pixel fade toward black by 'fadeValue' (0..255).
    // Applies scale = 255 - fadeValue to the pixel's RGB in-place.
    void fadePixelToBlackBy(int16_t x, int16_t y, uint8_t fadeValue) noexcept;

    // Linear-index overload
    void fadePixelToBlackBy(int16_t i, uint8_t fadeValue) noexcept;

    __attribute__((always_inline)) virtual void setPixel(int x, CRGB color) noexcept
    {
        if (isValidPixel(x))
            leds[x] = color;
        else
            debugE("Invalid setPixel request: x=%d, NUM_LEDS=%d", x, NUM_LEDS);
    }

    // DrawSafeCircle
    virtual void DrawSafeCircle(int centerX, int centerY, int radius, CRGB color) noexcept;

    // setPixelsF - Floating point variant
    //
    // This variant of setPixels includes a few important features:  it can merge its color
    // into the existing pixels or replace it entirely.  It can also draw fractionally, so
    // you can draw from 1.5 to 4.25, including when merging.
    //
    //   Example:
    //
    //   Starting at 3.25, draw for 1.5:
    //   We start at pixel 3.
    //   We fill pixel with .75 worth of color
    //   We advance to next pixel
    //
    //   We fill one pixel and advance to next pixel
    //   We are now at pixel 5, frac2 = .75
    //   We fill pixel with .75 worth of color
    void setPixelsF(float fPos, float count, CRGB c, bool bMerge = false) const;

    void blurRows(CRGB *leds, uint16_t width, uint16_t height, uint16_t first, fract8 blur_amount);

    // blurColumns: perform a blur1d on each column of a rectangular matrix
    void blurColumns(CRGB *leds, uint16_t width, uint16_t height, uint16_t first, fract8 blur_amount);

    void blur2d(CRGB *leds, uint16_t width, uint16_t firstColumn, uint16_t height, uint16_t firstRow, fract8 blur_amount);

    void BlurFrame(int amount);

    void CyclePalette(int offset = 1);

    void ChangePalettePeriodically();

    // Cross-fade current palette slowly toward the target palette
    //
    // Each time that nblendPaletteTowardPalette is called, small changes
    // are made to currentPalette to bring it closer to matching targetPalette.
    // You can control how many changes are made in each call:
    //   - the default of 24 is a good balance
    //   - meaningful values are 1-48.  1=very very slow, 48=quickest
    //   - "0" means do not change the currentPalette at all; freeze
    void PausePalette(bool bPaused);

    bool IsPalettePaused() const
    {
        return _palettePaused;
    }

    void UpdatePaletteCycle();

    void RandomPalette();

    virtual void fillRectangle(int x0, int y0, int x1, int y1, CRGB color);

    void setPalette(const CRGBPalette16& palette);

    // Note that this function may recurse without
    // bound if your random() is very very dumb.
    void loadPalette(int index);

    void setPalette(const String& paletteName);

    static void listPalettes();

    void setupGrayscalePalette();

    void setupIcePalette();

    // Oscillators and Emitters

    // the oscillators: linear ramps 0-255
    uint8_t osci[6];

    // sin8(osci) swinging between 0 to _width - 1
    uint8_t p[6];

    // set the speeds (and by that ratios) of the oscillators here

    void MoveOscillators();

    void ResetOscillators();

    // All the Caleidoscope functions work directly within the screenbuffer (leds array).
    // Draw whatever you like in the area x(0-15) and y (0-15) and then copy it around.

    // rotates the first 16x16 quadrant 3 times onto a 32x32 (+90 degrees rotation for each one)
    void Caleidoscope1() const;

    // mirror the first 16x16 quadrant 3 times onto a 32x32
    void Caleidoscope2() const;

    // copy one diagonal triangle into the other one within a 16x16
    void Caleidoscope3() const;

    // copy one diagonal triangle into the other one within a 16x16 (90 degrees rotated compared to Caleidoscope3)
    void Caleidoscope4() const;

    // copy one diagonal triangle into the other one within a 8x8
    void Caleidoscope5() const;

    // rotates the first 8x8 quadrant 3 times onto a 16x16
    void Caleidoscope6() const;

    // SpiralStream
    //
    // create a square twister to the left or counter-clockwise
    // x and y for center, r for radius
    void SpiralStream(int x, int y, int r, uint8_t dimm) const;

    // expand everything within a circle
    void Expand(int centerX, int centerY, int radius, uint8_t dimm);

    // give it a linear tail to the right
    void StreamRight(uint8_t scale, int fromX = 0, int toX = MATRIX_WIDTH, int fromY = 0, int toY = MATRIX_HEIGHT);

    // give it a linear tail to the left
    void StreamLeft(uint8_t scale, int fromX = MATRIX_WIDTH, int toX = 0, int fromY = 0, int toY = MATRIX_HEIGHT);

    // give it a linear tail downwards
    void StreamDown(uint8_t scale);

    // give it a linear tail upwards
    void StreamUp(uint8_t scale);

    // give it a linear tail up and to the left
    void StreamUpAndLeft(uint8_t scale);

    // give it a linear tail up and to the right
    void StreamUpAndRight(uint8_t scale);

    // just move everything one line down
    void MoveDown();

    // just move everything one line down
    void VerticalMoveFrom(int start, int end);

    // copy the rectangle defined with 2 points x0, y0, x1, y1
    // to the rectangle beginning at x2, x3
    void Copy(uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1, uint8_t x2, uint8_t y2);

    void BresenhamLine(int x0, int y0, int x1, int y1, CRGB color, bool bMerge = false);

    void BresenhamLine(int x0, int y0, int x1, int y1, uint8_t colorIndex, bool bMerge = false);

    virtual void drawLine(int x0, int y0, int x1, int y1, CRGB color);

    void DimAll(uint8_t value);

    CRGB ColorFromCurrentPalette(uint8_t index = 0, uint8_t brightness = 255, TBlendType blendType = LINEARBLEND) const;

    static CRGB HsvToRgb(uint8_t h, uint8_t s, uint8_t v);

    #if USE_NOISE
        // the oscillators: linear ramps 0-255
        // osci[0-3] are used for noise animation
        // osci[4-5] are used for palette rotation
        void NoiseVariablesSetup();

        void SetNoise(uint32_t nx, uint32_t ny, uint32_t nz, uint32_t sx, uint32_t sy);

        void FillGetNoise();

        // The next couple of two-liners define function templates for the different noise approaches
        // that are implemented in the project. The desired noise approach for a particular use case
        // can be chosen by passing one of the NoiseApproach enum's values as a template parameter.
        // For instance, using MoveFractionalNoiseX() with the "MRI" noise approach can be achieved by
        // calling gfxbase.MoveFractionalNoiseX<NoiseApproach::MRI>()
        //
        // The actual implementations for the noise functions (in the shape of specializations of the
        // function templates) are included in gfxbase.cpp, because of the way C++ demands things to be
        // structured.
        //
        // The default approach for the templated functions is determined by the value of _defaultNoiseApproach,
        // which is defined earlier in this class.
        template<NoiseApproach = _defaultNoiseApproach>
        void MoveFractionalNoiseX(uint8_t amt, uint8_t shift = 0);

        template<NoiseApproach = _defaultNoiseApproach>
        void MoveFractionalNoiseY(uint8_t amt, uint8_t shift = 0);

    #endif

    virtual void MoveInwardX(int startY = 0, int endY = MATRIX_HEIGHT - 1);

    virtual void MoveOutwardsX(int startY = 0, int endY = MATRIX_HEIGHT - 1);

    // MoveX - Shift the content on the matrix left or right
    void MoveX(uint8_t delta) const;

    // MoveY - Shifts the content on the matrix up or down
    void MoveY(uint8_t delta) const;

    virtual void PrepareFrame();

    virtual void PostProcessFrame(uint16_t, uint16_t);

    static const PolarMapArray& getPolarMap();
};
