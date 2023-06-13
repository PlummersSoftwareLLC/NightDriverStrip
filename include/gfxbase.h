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
//   Provides a Adafruit_GFX implementation for our RGB LED panel so that
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

#pragma once

#include <stdexcept>
#include "Adafruit_GFX.h"
#include "pixeltypes.h"
#include "effects/matrix/Boid.h"
#include "effects/matrix/Vector.h"
#include <memory>

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
#endif

class GFXBase : public Adafruit_GFX
{
protected:
    size_t _width;
    size_t _height;

    static const uint8_t gamma5[];
    static const uint8_t gamma6[];

    static const int _paletteCount = 10;
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

    static const int _heatColorsPaletteIndex = 6;
    static const int _randomPaletteIndex = 9;

public:
    // Many of the Aurora effects need direct access to these from external classes

    CRGB *leds = nullptr;
    std::unique_ptr<Boid[]> _boids;

    GFXBase(int w, int h) : Adafruit_GFX(w, h),
                            _width(w),
                            _height(h)
    {
        #if USE_NOISE
            debugV("Allocating boids and noise");
            _boids.reset(psram_allocator<Boid>().allocate(_width));
            _ptrNoise.reset(psram_allocator<Noise>().allocate(1));
            assert(_ptrNoise && _boids);
            debugV("Setting up noise");
            NoiseVariablesSetup();
            debugV("Filling noise");
            FillGetNoise();
        #endif

        debugV("Setting up palette");
        loadPalette(0);
        ResetOscillators();
    }

    virtual ~GFXBase()
    {
    }

    #if USE_NOISE
    Noise &GetNoise()
    {
        return *_ptrNoise;
    }
    #endif

    CRGBPalette16 &GetCurrentPalette()
    {
        return _currentPalette;
    }

    virtual size_t GetLEDCount() const
    {
        return _width * _height;
    }

    virtual CRGB getPixel(int16_t x, int16_t y) const
    {
        if (x >= 0 && x < _width && y >= 0 && y < _height)
            return leds[xy(x, y)];
        else
            throw std::runtime_error(str_sprintf("Invalid index in getPixel: x=%d, y=%d, NUM_LEDS=%d", x, y, NUM_LEDS).c_str());
    }

    virtual CRGB getPixel(int16_t i) const
    {
        if (i >= 0 && i < GetLEDCount())
            return leds[i];
        else
            throw std::runtime_error("Pixel out of range in getPixel(x)");
    }

    static uint8_t beatcos8(accum88 beats_per_minute, uint8_t lowest = 0, uint8_t highest = 255, uint32_t timebase = 0, uint8_t phase_offset = 0)
    {
        uint8_t beat = beat8(beats_per_minute, timebase);
        uint8_t beatcos = cos8(beat + phase_offset);
        uint8_t rangewidth = highest - lowest;
        uint8_t scaledbeat = scale8(beatcos, rangewidth);
        uint8_t result = lowest + scaledbeat;
        return result;
    }

    static uint8_t mapsin8(uint8_t theta, uint8_t lowest = 0, uint8_t highest = 255)
    {
        uint8_t beatsin = sin8(theta);
        uint8_t rangewidth = highest - lowest;
        uint8_t scaledbeat = scale8(beatsin, rangewidth);
        uint8_t result = lowest + scaledbeat;
        return result;
    }

    static uint8_t mapcos8(uint8_t theta, uint8_t lowest = 0, uint8_t highest = 255)
    {
        uint8_t beatcos = cos8(theta);
        uint8_t rangewidth = highest - lowest;
        uint8_t scaledbeat = scale8(beatcos, rangewidth);
        uint8_t result = lowest + scaledbeat;
        return result;
    }

    static CRGB from16Bit(uint16_t color) // Convert 16bit 5:6:5 to 24bit color using lookup table for gamma
    {
        uint8_t r = gamma5[color >> 11];
        uint8_t g = gamma6[(color >> 5) & 0x3F];
        uint8_t b = gamma5[color & 0x1F];

        return CRGB(r, g, b);
    }

    static uint16_t to16bit(uint8_t r, uint8_t g, uint8_t b) // Convert RGB -> 16bit 5:6:5
    {
        return ((r / 8) << 11) | ((g / 4) << 5) | (b / 8);
    }

    static uint16_t to16bit(const CRGB rgb) // Convert CRGB -> 16 bit 5:6:5
    {
        return ((rgb.r / 8) << 11) | ((rgb.g / 4) << 5) | (rgb.b / 8);
    }

    static uint16_t to16bit(CRGB::HTMLColorCode code) // Convert HtmlColorCode -> 16 bit 5:6:5
    {
        return to16bit(CRGB(code));
    }

    virtual void Clear()
    {
        memset(leds, 0, sizeof(CRGB) * _width * _height);
    }

    // Matrices that are built from individually addressable strips like WS2812b generally
    // follow a boustrophodon layout as follows:
    //
    //     0 >  1 >  2 >  3 >  4
    //                         |
    //     9 <  8 <  7 <  6 <  5
    //     |
    //    10 > 11 > 12 > 13 > 14
    //                         |
    //    19 < 18 < 17 < 16 < 15
    //     |
    //    (etc)
    //
    // If your matrix uses a different approach, you can override this function and implement it
    // in the xy() function of your class

    virtual uint16_t xy(uint16_t x, uint16_t y) const
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

    virtual void addColor(int16_t i, CRGB c)
    {
        if (i >= 0 && i < _width * _height)
            leds[i] += c;
    }

    virtual void drawPixel(int16_t x, int16_t y, CRGB color)
    {
        leds[xy(x, y)] = color;
    }

    virtual void drawPixel(int16_t x, int16_t y, uint16_t color)
    {
        leds[xy(x, y)] = from16Bit(color);
    }

    virtual void fillLeds(std::unique_ptr<CRGB[]> &pLEDs)
    {
        // A mesmerizer panel has the same layout as in memory, so we can memcpy.  Others may require transposition,
        // so we do it the "slow" way for other matrices

        for (int x = 0; x < _width; x++)
            for (int y = 0; y < _height; y++)
                setPixel(x, y, pLEDs[y * _width + x]);
    }

    virtual void setPixel(int16_t x, int16_t y, uint16_t color)
    {
        if (x >= 0 && x < _width && y >= 0 && y < _height)
            leds[xy(x, y)] = from16Bit(color);
    }

    void setPixel(int16_t x, int16_t y, CRGB color)
    {
        if (x >= 0 && x < _width && y >= 0 && y < _height)
            leds[xy(x, y)] = color;
    }

    virtual void setPixel(int16_t x, int r, int g, int b)
    {
        setPixel(x, CRGB(r, g, b));
    }

    virtual void setPixel(int x, CRGB color)
    {
        if (x >= 0 && x < _width * _height)
            leds[x] = color;
    }

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

    void setPixelsF(float fPos, float count, CRGB c, bool bMerge = false)
    {
        float frac1 = fPos - floor(fPos);                 // eg:   3.25 becomes 0.25
        float frac2 = fPos + count - floor(fPos + count); // eg:   3.25 + 1.5 yields 4.75 which becomes 0.75

        /* Example:

          Starting at 3.25, draw for 1.5:
          We start at pixel 3.
          We fill pixel with .75 worth of color
          We advance to next pixel

          We fill one pixel and advance to next pixel

          We are now at pixel 5, frac2 = .75
          We fill pixel with .75 worth of color
        */

        uint8_t fade1 = (std::max(frac1, 1.0f - count)) * 255; // Fraction is how far past pixel boundary we are (up to our total size) so larger fraction is more dimming
        uint8_t fade2 = (1.0 - frac2) * 255;                   // Fraction is how far we are poking into this pixel, so larger fraction is less dimming
        CRGB c1 = c;
        CRGB c2 = c;
        c1 = c1.fadeToBlackBy(fade1);
        c2 = c2.fadeToBlackBy(fade2);

        // These assignments use the + operator of CRGB to merge the colors when requested, and its pretty
        // naive, just saturating each color element at 255, so the operator could be improved or replaced
        // if needed...

        float p = fPos;
        if (p >= 0 && p < GetLEDCount())
            for (int i = 0; i < NUM_CHANNELS; i++)
                leds[(int)p] = bMerge ? leds[(int)p] + c1 : c1;

        p = fPos + (1.0 - frac1);
        count -= (1.0 - frac1);

        // Middle (body) pixels

        while (count >= 1)
        {
            if (p >= 0 && p < GetLEDCount())
                for (int i = 0; i < NUM_CHANNELS; i++)
                    leds[(int)p] = bMerge ? leds[(int)p] + c : c;
            count--;
            p++;
        };

        // Final pixel, if in bounds
        if (count > 0)
            if (p >= 0 && p < GetLEDCount())
                for (int i = 0; i < NUM_CHANNELS; i++)
                    leds[(int)p] = bMerge ? leds[(int)p] + c2 : c2;
    }

    void blurRows(CRGB *leds, uint16_t width, uint16_t height, uint16_t first, fract8 blur_amount)
    {
        // blur rows same as columns, for irregular matrix
        uint8_t keep = 255 - blur_amount;
        uint8_t seep = blur_amount >> 1;
        for (uint16_t row = 0; row < height; row++)
        {
            CRGB carryover = CRGB::Black;
            for (uint16_t i = first; i < width; i++)
            {
                CRGB cur = leds[xy(i, row)];
                CRGB part = cur;
                part.nscale8(seep);
                cur.nscale8(keep);
                cur += carryover;
                if (i)
                    leds[xy(i - 1, row)] += part;
                leds[xy(i, row)] = cur;
                carryover = part;
            }
        }
    }

    // blurColumns: perform a blur1d on each column of a rectangular matrix
    void blurColumns(CRGB *leds, uint16_t width, uint16_t height, uint16_t first, fract8 blur_amount)
    {
        // blur columns
        uint8_t keep = 255 - blur_amount;
        uint8_t seep = blur_amount >> 1;
        for (uint16_t col = 0; col < width; ++col)
        {
            CRGB carryover = CRGB::Black;
            for (uint16_t i = first; i < height; ++i)
            {
                CRGB cur = leds[xy(col, i)];
                CRGB part = cur;
                part.nscale8(seep);
                cur.nscale8(keep);
                cur += carryover;
                if (i)
                    leds[xy(col, i - 1)] += part;
                leds[xy(col, i)] = cur;
                carryover = part;
            }
        }
    }

    void blur2d(CRGB *leds, uint16_t width, uint16_t firstColumn, uint16_t height, uint16_t firstRow, fract8 blur_amount)
    {
        blurRows(leds, width, height, firstColumn, blur_amount);
        blurColumns(leds, width, height, firstRow, blur_amount);
    }

    void BlurFrame(int amount)
    {
        // BUGBUG (davepl) Needs to call isVuVisible on the effects manager to find out if it starts at row 1 or 0
        blur2d(leds, _width, 0, _height, 1, amount);
    }

    void CyclePalette(int offset = 1)
    {
        loadPalette(_paletteIndex + offset);
    }

    void ChangePalettePeriodically()
    {
        if (_palettePaused)
            return;

        const int minutesPerPaletteCycle = 2;
        uint8_t secondHand = ((millis() / minutesPerPaletteCycle) / 1000) % 60;

        if (_lastSecond != secondHand)
        {
            _lastSecond = secondHand;
            if (secondHand == 0)
            {
                _targetPalette = RainbowColors_p;
            }
            if (secondHand == 10)
            {
                _targetPalette = HeatColors_p;
            } // CRGBPalette16( g,g,b,b, p,p,b,b, g,g,b,b, p,p,b,b); }
            if (secondHand == 20)
            {
                _targetPalette = ForestColors_p;
            } // CRGBPalette16( b,b,b,w, b,b,b,w, b,b,b,w, b,b,b,w); }
            if (secondHand == 30)
            {
                _targetPalette = LavaColors_p;
            } // Black gaps
            if (secondHand == 40)
            {
                _targetPalette = CloudColors_p;
            }
            if (secondHand == 50)
            {
                _targetPalette = PartyColors_p;
            }
        }
    }

    // Crossfade current palette slowly toward the target palette
    //
    // Each time that nblendPaletteTowardPalette is called, small changes
    // are made to currentPalette to bring it closer to matching targetPalette.
    // You can control how many changes are made in each call:
    //   - the default of 24 is a good balance
    //   - meaningful values are 1-48.  1=veeeeeeeery slow, 48=quickest
    //   - "0" means do not change the currentPalette at all; freeze

    void PausePalette(bool bPaused)
    {
        _palettePaused = bPaused;
    }

    bool IsPalettePaused()
    {
        return _palettePaused;
    }

    void UpdatePaletteCycle()
    {

        ChangePalettePeriodically();
        uint8_t maxChanges = 24;
        nblendPaletteTowardPalette(_currentPalette, _targetPalette, maxChanges);
    }

    void RandomPalette()
    {
        loadPalette(_randomPaletteIndex);
    }

    void fillRectangle(int x0, int y0, int x1, int y1, CRGB color)
    {
        for (int x = x0; x < x1; x++)
            for (int y = y0; y < y1; y++)
                drawPixel(x, y, color);
    }

    void setPalette(CRGBPalette16 palette)
    {
        _currentPalette = palette;
        _targetPalette = palette;
        _currentPaletteName = "Custom";
    }

    void loadPalette(int index)
    {
        _paletteIndex = index;

        if (_paletteIndex >= _paletteCount)
            _paletteIndex = 0;
        else if (_paletteIndex < 0)
            _paletteIndex = _paletteCount - 1;

        switch (_paletteIndex)
        {
        case 0:
            _targetPalette = RainbowColors_p;
            _currentPaletteName = "Rainbow";
            break;
            // case 1:
            //   targetPalette = RainbowStripeColors_p;
            //   currentPaletteName = "RainbowStripe";
            //   break;
        case 1:
            _targetPalette = OceanColors_p;
            _currentPaletteName = "Ocean";
            break;
        case 2:
            _targetPalette = CloudColors_p;
            _currentPaletteName = "Cloud";
            break;
        case 3:
            _targetPalette = ForestColors_p;
            _currentPaletteName = "Forest";
            break;
        case 4:
            _targetPalette = PartyColors_p;
            _currentPaletteName = "Party";
            break;
        case 5:
            setupGrayscalePalette();
            _currentPaletteName = "Grey";
            break;
        case _heatColorsPaletteIndex:
            _targetPalette = HeatColors_p;
            _currentPaletteName = "Heat";
            break;
        case 7:
            _targetPalette = LavaColors_p;
            _currentPaletteName = "Lava";
            break;
        case 8:
            setupIcePalette();
            _currentPaletteName = "Ice";
            break;
        case _randomPaletteIndex:
            loadPalette(random(0, _paletteCount - 1));
            _paletteIndex = _randomPaletteIndex;
            _currentPaletteName = "Random";
            break;
        }
        _currentPalette = _targetPalette;
    }

    void setPalette(String paletteName)
    {
        if (paletteName == "Rainbow")
            loadPalette(0);
        // else if (paletteName == "RainbowStripe")
        //   loadPalette(1);
        else if (paletteName == "Ocean")
            loadPalette(1);
        else if (paletteName == "Cloud")
            loadPalette(2);
        else if (paletteName == "Forest")
            loadPalette(3);
        else if (paletteName == "Party")
            loadPalette(4);
        else if (paletteName == "Grayscale")
            loadPalette(5);
        else if (paletteName == "Heat")
            loadPalette(6);
        else if (paletteName == "Lava")
            loadPalette(7);
        else if (paletteName == "Ice")
            loadPalette(8);
        else if (paletteName == "Random")
            RandomPalette();
    }

    void listPalettes() const
    {
        Serial.println(F("{"));
        Serial.print(F("  \"count\": "));
        Serial.print(_paletteCount);
        Serial.println(",");
        Serial.println(F("  \"results\": ["));

        String paletteNames[] = {
            "Rainbow",
            // "RainbowStripe",
            "Ocean",
            "Cloud",
            "Forest",
            "Party",
            "Grayscale",
            "Heat",
            "Lava",
            "Ice",
            "Random"};

        for (int i = 0; i < _paletteCount; i++)
        {
            Serial.print(F("    \""));
            Serial.print(paletteNames[i]);
            if (i == _paletteCount - 1)
                Serial.println(F("\""));
            else
                Serial.println(F("\","));
        }

        Serial.println("  ]");
        Serial.println("}");
    }

    void setupGrayscalePalette()
    {
        _targetPalette = CRGBPalette16(CRGB::Black, CRGB::White);
    }

    void setupIcePalette()
    {
        _targetPalette = CRGBPalette16(CRGB::Black, CRGB::Blue, CRGB::Aqua, CRGB::White);
    }

    // write one pixel with the specified color from the current palette to coordinates

    void Pixel(int x, int y, uint8_t colorIndex)
    {
        leds[xy(x, y)] = ColorFromCurrentPalette(colorIndex);
    }

    // Oscillators and Emitters

    // the oscillators: linear ramps 0-255
    uint8_t osci[6];

    // sin8(osci) swinging between 0 to _width - 1
    uint8_t p[6];

    // set the speeds (and by that ratios) of the oscillators here

    void MoveOscillators()
    {
        osci[0] = osci[0] + 5;
        osci[1] = osci[1] + 2;
        osci[2] = osci[2] + 3;
        osci[3] = osci[3] + 4;
        osci[4] = osci[4] + 1;
        if (osci[4] % 2 == 0)
            osci[5] = osci[5] + 1; // .5
        for (int i = 0; i < 4; i++)
        {
            p[i] = map8(sin8(osci[i]), 0, std::min(255U, _width - 1)); // why? to keep the result in the range of 0-_width (matrix size)
        }
    }

    void ResetOscillators()
    {
        memset(osci, 0, sizeof(osci));
        memset(p, 0, sizeof(p));
    }

    // All the caleidoscope functions work directly within the screenbuffer (leds array).
    // Draw whatever you like in the area x(0-15) and y (0-15) and then copy it arround.

    // rotates the first 16x16 quadrant 3 times onto a 32x32 (+90 degrees rotation for each one)

    void Caleidoscope1()
    {
        for (int x = 0; x < ((_width + 1) / 2); x++)
        {
            for (int y = 0; y < ((_height + 1) / 2); y++)
            {
                leds[xy(_width - 1 - x, y)] = leds[xy(x, y)];
                leds[xy(_width - 1 - x, _height - 1 - y)] = leds[xy(x, y)];
                leds[xy(x, _height - 1 - y)] = leds[xy(x, y)];
            }
        }
    }

    // mirror the first 16x16 quadrant 3 times onto a 32x32
    void Caleidoscope2()
    {
        for (int x = 0; x < ((_width + 1) / 2); x++)
        {
            for (int y = 0; y < ((_height + 1) / 2); y++)
            {
                leds[xy(_width - 1 - x, y)] = leds[xy(y, x)];
                leds[xy(x, _height - 1 - y)] = leds[xy(y, x)];
                leds[xy(_width - 1 - x, _height - 1 - y)] = leds[xy(x, y)];
            }
        }
    }

    // copy one diagonal triangle into the other one within a 16x16
    void Caleidoscope3()
    {
        for (int x = 0; x <= ((_width + 1) / 2); x++)
        {
            for (int y = 0; y <= x; y++)
            {
                leds[xy(x, y)] = leds[xy(y, x)];
            }
        }
    }

    // copy one diagonal triangle into the other one within a 16x16 (90 degrees rotated compared to Caleidoscope3)
    void Caleidoscope4()
    {
        for (int x = 0; x <= ((_width + 1) / 2); x++)
        {
            for (int y = 0; y <= ((_height + 1) / 2) - x; y++)
            {
                leds[xy(((_height + 1) / 2) - y, ((_width + 1) / 2) - x)] = leds[xy(x, y)];
            }
        }
    }

    // copy one diagonal triangle into the other one within a 8x8
    void Caleidoscope5()
    {
        for (int x = 0; x < _width / 4; x++)
        {
            for (int y = 0; y <= x; y++)
            {
                leds[xy(x, y)] = leds[xy(y, x)];
            }
        }

        for (int x = _width / 4; x < _width / 2; x++)
        {
            for (int y = _height / 4; y >= 0; y--)
            {
                leds[xy(x, y)] = leds[xy(y, x)];
            }
        }
    }

    void Caleidoscope6()
    {
        for (int x = 1; x < ((_width + 1) / 2); x++)
        {
            leds[xy(7 - x, 7)] = leds[xy(x, 0)];
        } // a
        for (int x = 2; x < ((_width + 1) / 2); x++)
        {
            leds[xy(7 - x, 6)] = leds[xy(x, 1)];
        } // b
        for (int x = 3; x < ((_width + 1) / 2); x++)
        {
            leds[xy(7 - x, 5)] = leds[xy(x, 2)];
        } // c
        for (int x = 4; x < ((_width + 1) / 2); x++)
        {
            leds[xy(7 - x, 4)] = leds[xy(x, 3)];
        } // d
        for (int x = 5; x < ((_width + 1) / 2); x++)
        {
            leds[xy(7 - x, 3)] = leds[xy(x, 4)];
        } // e
        for (int x = 6; x < ((_width + 1) / 2); x++)
        {
            leds[xy(7 - x, 2)] = leds[xy(x, 5)];
        } // f
        for (int x = 7; x < ((_width + 1) / 2); x++)
        {
            leds[xy(7 - x, 1)] = leds[xy(x, 6)];
        } // g
    }

    // SpiralStream
    //
    // create a square twister to the left or counter-clockwise
    // x and y for center, r for radius

    void SpiralStream(int x, int y, int r, uint8_t dimm)
    {
        for (int d = r; d >= 0; d--)
        { // from the outside to the inside
            for (int i = x - d; i <= x + d; i++)
            {
                leds[xy(i, y - d)] += leds[xy(i + 1, y - d)]; // lowest row to the right
                leds[xy(i, y - d)].nscale8(dimm);
            }
            for (int i = y - d; i <= y + d; i++)
            {
                leds[xy(x + d, i)] += leds[xy(x + d, i + 1)]; // right colum up
                leds[xy(x + d, i)].nscale8(dimm);
            }
            for (int i = x + d; i >= x - d; i--)
            {
                leds[xy(i, y + d)] += leds[xy(i - 1, y + d)]; // upper row to the left
                leds[xy(i, y + d)].nscale8(dimm);
            }
            for (int i = y + d; i >= y - d; i--)
            {
                leds[xy(x - d, i)] += leds[xy(x - d, i - 1)]; // left colum down
                leds[xy(x - d, i)].nscale8(dimm);
            }
        }
    }

    // expand everything within a circle
    void Expand(int centerX, int centerY, int radius, uint8_t dimm)
    {
        if (radius == 0)
            return;

        int currentRadius = radius;

        while (currentRadius > 0)
        {
            int a = radius, b = 0;
            int radiusError = 1 - a;

            int nextRadius = currentRadius - 1;
            int nextA = nextRadius - 1, nextB = 0;
            int nextRadiusError = 1 - nextA;

            while (a >= b)
            {
                // move them out one pixel on the radius
                leds[xy(a + centerX, b + centerY)]   = leds[xy(nextA + centerX, nextB + centerY)];
                leds[xy(b + centerX, a + centerY)]   = leds[xy(nextB + centerX, nextA + centerY)];
                leds[xy(-a + centerX, b + centerY)]  = leds[xy(-nextA + centerX, nextB + centerY)];
                leds[xy(-b + centerX, a + centerY)]  = leds[xy(-nextB + centerX, nextA + centerY)];
                leds[xy(-a + centerX, -b + centerY)] = leds[xy(-nextA + centerX, -nextB + centerY)];
                leds[xy(-b + centerX, -a + centerY)] = leds[xy(-nextB + centerX, -nextA + centerY)];
                leds[xy(a + centerX, -b + centerY)]  = leds[xy(nextA + centerX, -nextB + centerY)];
                leds[xy(b + centerX, -a + centerY)]  = leds[xy(nextB + centerX, -nextA + centerY)];

                // dim them
                leds[xy(a + centerX, b + centerY)].nscale8(dimm);
                leds[xy(b + centerX, a + centerY)].nscale8(dimm);
                leds[xy(-a + centerX, b + centerY)].nscale8(dimm);
                leds[xy(-b + centerX, a + centerY)].nscale8(dimm);
                leds[xy(-a + centerX, -b + centerY)].nscale8(dimm);
                leds[xy(-b + centerX, -a + centerY)].nscale8(dimm);
                leds[xy(a + centerX, -b + centerY)].nscale8(dimm);
                leds[xy(b + centerX, -a + centerY)].nscale8(dimm);

                b++;
                if (radiusError < 0)
                    radiusError += 2 * b + 1;
                else
                {
                    a--;
                    radiusError += 2 * (b - a + 1);
                }

                nextB++;
                if (nextRadiusError < 0)
                    nextRadiusError += 2 * nextB + 1;
                else
                {
                    nextA--;
                    nextRadiusError += 2 * (nextB - nextA + 1);
                }
            }

            currentRadius--;
        }
    }

    // give it a linear tail to the right
    void StreamRight(uint8_t scale, int fromX = 0, int toX = MATRIX_WIDTH, int fromY = 0, int toY = MATRIX_HEIGHT)
    {
        for (int x = fromX + 1; x < toX; x++)
        {
            for (int y = fromY; y < toY; y++)
            {
                leds[xy(x, y)] += leds[xy(x - 1, y)];
                leds[xy(x, y)].nscale8(scale);
            }
        }
        for (int y = fromY; y < toY; y++)
            leds[xy(0, y)].nscale8(scale);
    }

    // give it a linear tail to the left
    void StreamLeft(uint8_t scale, int fromX = MATRIX_WIDTH, int toX = 0, int fromY = 0, int toY = MATRIX_HEIGHT)
    {
        for (int x = toX; x < fromX; x++)
        {
            for (int y = fromY; y < toY; y++)
            {
                leds[xy(x, y)] += leds[xy(x + 1, y)];
                leds[xy(x, y)].nscale8(scale);
            }
        }
        for (int y = fromY; y < toY; y++)
            leds[xy(0, y)].nscale8(scale);
    }

    // give it a linear tail downwards
    void StreamDown(uint8_t scale)
    {
        for (int x = 0; x < _width; x++)
        {
            for (int y = 1; y < _height; y++)
            {
                leds[xy(x, y)] += leds[xy(x, y - 1)];
                leds[xy(x, y)].nscale8(scale);
            }
        }
        for (int x = 0; x < _width; x++)
            leds[xy(x, 0)].nscale8(scale);
    }

    // give it a linear tail upwards
    void StreamUp(uint8_t scale)
    {
        for (int x = 0; x < _width; x++)
        {
            for (int y = _height - 2; y >= 0; y--)
            {
                leds[xy(x, y)] += leds[xy(x, y + 1)];
                leds[xy(x, y)].nscale8(scale);
            }
        }
        for (int x = 0; x < _width; x++)
            leds[xy(x, _height - 1)].nscale8(scale);
    }

    // give it a linear tail up and to the left
    void StreamUpAndLeft(uint8_t scale)
    {
        for (int x = 0; x < _width - 1; x++)
        {
            for (int y = _height - 2; y >= 0; y--)
            {
                leds[xy(x, y)] += leds[xy(x + 1, y + 1)];
                leds[xy(x, y)].nscale8(scale);
            }
        }
        for (int x = 0; x < _width; x++)
            leds[xy(x, _height - 1)].nscale8(scale);
        for (int y = 0; y < _height; y++)
            leds[xy(_width - 1, y)].nscale8(scale);
    }

    // give it a linear tail up and to the right

    void StreamUpAndRight(uint8_t scale)
    {
        for (int x = 0; x < _width - 1; x++)
        {
            for (int y = _height - 2; y >= 0; y--)
            {
                leds[xy(x + 1, y)] += leds[xy(x, y + 1)];
                leds[xy(x, y)].nscale8(scale);
            }
        }
        // fade the bottom row
        for (int x = 0; x < _width; x++)
            leds[xy(x, _height - 1)].nscale8(scale);

        // fade the right column
        for (int y = 0; y < _height; y++)
            leds[xy(_width - 1, y)].nscale8(scale);
    }

    // just move everything one line down - BUGBUG (DAVEPL) Redundant with MoveX?

    void MoveDown()
    {
        for (int y = _height - 1; y > 0; y--)
        {
            for (int x = 0; x < _width; x++)
            {
                leds[xy(x, y)] = leds[xy(x, y - 1)];
            }
        }
    }

    // just move everything one line down - BUGBUG (davepl) Redundant with MoveY?

    void VerticalMoveFrom(int start, int end)
    {
        for (int y = end; y > start; y--)
        {
            for (int x = 0; x < _width; x++)
            {
                leds[xy(x, y)] = leds[xy(x, y - 1)];
            }
        }
    }

    // copy the rectangle defined with 2 points x0, y0, x1, y1
    // to the rectangle beginning at x2, x3

    void Copy(uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1, uint8_t x2, uint8_t y2)
    {
        for (int y = y0; y < y1 + 1; y++)
        {
            for (int x = x0; x < x1 + 1; x++)
            {
                leds[xy(x + x2 - x0, y + y2 - y0)] = leds[xy(x, y)];
            }
        }
    }

    // rotate + copy triangle (((_width + 1) / 2)*((_width + 1) / 2))
    void RotateTriangle()
    {
        for (int x = 1; x < ((_width + 1) / 2); x++)
        {
            for (int y = 0; y < x; y++)
            {
                leds[xy(x, 7 - y)] = leds[xy(7 - x, y)];
            }
        }
    }

    // mirror + copy triangle (((_width + 1) / 2)*((_width + 1) / 2))
    void MirrorTriangle()
    {
        for (int x = 1; x < ((_width + 1) / 2); x++)
        {
            for (int y = 0; y < x; y++)
            {
                leds[xy(7 - y, x)] = leds[xy(7 - x, y)];
            }
        }
    }

    // draw static rainbow triangle pattern (((_width + 1) / 2)xWIDTH / 2)
    // (just for debugging)

    void RainbowTriangle()
    {
        for (int i = 0; i < ((_width + 1) / 2); i++)
        {
            for (int j = 0; j <= i; j++)
            {
                Pixel(7 - i, j, i * j * 4);
            }
        }
    }

    void BresenhamLine(int x0, int y0, int x1, int y1, CRGB color, bool bMerge = false)
    {
        int dx = abs(x1 - x0), sx = x0 < x1 ? 1 : -1;
        int dy = -abs(y1 - y0), sy = y0 < y1 ? 1 : -1;
        int err = dx + dy, e2;
        for (;;)
        {
            leds[xy(x0, y0)] = bMerge ? leds[xy(x0, y0)] + color : color;
            if (x0 == x1 && y0 == y1)
                break;
            e2 = 2 * err;
            if (e2 > dy)
            {
                err += dy;
                x0 += sx;
            }
            if (e2 < dx)
            {
                err += dx;
                y0 += sy;
            }
        }
    }

    void BresenhamLine(int x0, int y0, int x1, int y1, uint8_t colorIndex, bool bMerge = false)
    {
        BresenhamLine(x0, y0, x1, y1, ColorFromCurrentPalette(colorIndex), bMerge);
    }

    void drawLine(int x0, int y0, int x1, int y1, CRGB color)
    {
        BresenhamLine(x0, y0, x1, y1, color);
    }

    void DimAll(uint8_t value)
    {
        for (int i = 0; i < NUM_LEDS; i++)
        {
            // if ((leds[i].r != 255) || (leds[i].g != 255) || (leds[i].b != 255))           // Don't dim pure white
            leds[i].nscale8(value);
        }
    }
    // write one pixel with the specified color from the current palette to coordinates
    /*
    void Pixel(int x, int y, uint8_t colorIndex) {
      leds[xy(x, y)] = ColorFromCurrentPalette(colorIndex);
      matrix.drawBackgroundPixelRGB888(x,y, leds[xy(x, y)]); // now draw it?
    }
    */

    CRGB ColorFromCurrentPalette(uint8_t index = 0, uint8_t brightness = 255, TBlendType blendType = LINEARBLEND) const
    {
        return ColorFromPalette(_currentPalette, index, brightness, _currentBlendType);
    }

    CRGB HsvToRgb(uint8_t h, uint8_t s, uint8_t v) const
    {
        CHSV hsv = CHSV(h, s, v);
        CRGB rgb;
        hsv2rgb_spectrum(hsv, rgb);
        return rgb;
    }

    #if USE_NOISE
        void NoiseVariablesSetup()
        {
            _ptrNoise->noisesmoothing = 200;

            _ptrNoise->noise_x = random16();
            _ptrNoise->noise_y = random16();
            _ptrNoise->noise_z = random16();
            _ptrNoise->noise_scale_x = 6000;
            _ptrNoise->noise_scale_y = 6000;
        }

        void SetNoise(uint32_t nx, uint32_t ny, uint32_t nz, uint32_t sx, uint32_t sy)
        {
            _ptrNoise->noise_x += nx;
            _ptrNoise->noise_y += ny;
            _ptrNoise->noise_z += nx;
            _ptrNoise->noise_scale_x = sx;
            _ptrNoise->noise_scale_y = sy;
        }

        void FillGetNoise()
        {
            for (uint16_t i = 0; i < _width; i++)
            {
                uint32_t ioffset = _ptrNoise->noise_scale_x * (i - ((_height + 1) / 2));

                for (uint16_t j = 0; j < _height; j++)
                {
                    uint32_t joffset = _ptrNoise->noise_scale_y * (j - ((_height + 1) / 2));

                    uint16_t data = inoise16(_ptrNoise->noise_x + ioffset, _ptrNoise->noise_y + joffset, _ptrNoise->noise_z) >> 8;

                    uint8_t olddata = _ptrNoise->noise[i][j];
                    uint8_t newdata = scale8(olddata, _ptrNoise->noisesmoothing) + scale8(data, 256 - _ptrNoise->noisesmoothing);
                    data = newdata;

                    _ptrNoise->noise[i][j] = data;
                }
            }
        }
    #endif

    virtual void MoveInwardX(int startY = 0, int endY = MATRIX_HEIGHT - 1)
    {
        for (int y = startY; y <= endY; y++)
        {
            for (int x = _width / 2; x > 0; x--)
                leds[xy(x, y)] = leds[xy(x - 1, y)];

            for (int x = _width / 2; x < _width; x++)
                leds[xy(x, y)] = leds[xy(x + 1, y)];
        }
    }

    virtual void MoveOutwardsX(int startY = 0, int endY = MATRIX_HEIGHT - 1)
    {
        for (int y = startY; y <= endY; y++)
        {
            for (int x = 0; x < _width / 2 - 1; x++)
            {
                leds[xy(x, y)] = leds[xy(x + 1, y)];
                leds[xy(_width - x - 1, y)] = leds[xy(_width - x - 2, y)];
            }
        }
    }

    // MoveX - Shift the content on the matrix left or right

    void MoveX(uint8_t delta)
    {
        for (int y = 0; y < _height; y++)
        {
            // First part
            for (int x = 0; x < _width - delta; x++)
                leds[xy(x, y)] = leds[xy(x + delta, y)];
            // Wrap around to second part
            for (int x = _width - delta; x < _width; x++)
                leds[xy(x, y)] = leds[xy(x + delta - _width, y)];
        }
    }

    // MoveY - Shifts the content on the matix up or down

    void MoveY(uint8_t delta)
    {
        CRGB tmp = 0;
        for (int x = 0; x < _width; x++)
        {
            tmp = leds[xy(x, 0)];
            for (int m = 0; m < delta; m++) // moves
            {
                // Do this delta time for each row... computationally expensive potentially.
                for (int y = 0; y < _height - 1; y++)
                    leds[xy(x, y)] = leds[xy(x, y + 1)];

                leds[xy(x, _height - 1)] = tmp;
            }
        } // end column loop
    }     /// MoveY

    #if USE_NOISE
        void MoveFractionalNoiseX(uint8_t amt = 16)
        {
            std::unique_ptr<CRGB[]> ledsTemp = std::make_unique<CRGB[]>(NUM_LEDS);

            // move delta pixelwise
            for (int y = 0; y < _height; y++)
            {
                uint16_t amount = _ptrNoise->noise[0][y] * amt;
                uint8_t delta = _width - 1 - (amount / 256);

                // Process up to the end less the dekta
                for (int x = 0; x < _width - delta; x++)
                    ledsTemp[xy(x, y)] = leds[xy(x + delta, y)];

                // Do the tail portion while wrapping around
                for (int x = _width - delta; x < _width; x++)
                    ledsTemp[xy(x, y)] = leds[xy(x + delta - _width, y)];
            }

            // move fractions
            CRGB PixelA;
            CRGB PixelB;

            for (uint16_t y = 0; y < _height; y++)
            {
                uint16_t amount = _ptrNoise->noise[0][y] * amt;
                uint8_t delta = _height - 1 - (amount / 256);
                uint8_t fractions = amount - (delta * 256);

                for (uint16_t x = 1; x < _width; x++)
                {
                    PixelA = ledsTemp[xy(x, y)];
                    PixelB = ledsTemp[xy(x - 1, y)];

                    PixelA %= 255 - fractions;
                    PixelB %= fractions;

                    leds[xy(x, y)] = PixelA + PixelB;
                }

                PixelA = ledsTemp[xy(0, y)];
                PixelB = ledsTemp[xy(_width - 1, y)];

                PixelA %= 255 - fractions;
                PixelB %= fractions;

                leds[xy(0, y)] = PixelA + PixelB;
            }
        }

        void MoveFractionalNoiseY(uint8_t amt = 16)
        {
            std::unique_ptr<CRGB[]> ledsTemp = std::make_unique<CRGB[]>(NUM_LEDS);

            // move delta pixelwise
            for (int x = 0; x < _width; x++)
            {
                uint16_t amount = _ptrNoise->noise[x][0] * amt;
                uint8_t delta = _height - 1 - (amount / 256);

                for (int y = 0; y < _height - delta; y++)
                {
                    ledsTemp[xy(x, y)] = leds[xy(x, y + delta)];
                }
                for (int y = _height - delta; y < _height; y++)
                {
                    ledsTemp[xy(x, y)] = leds[xy(x, y + delta - _height)];
                }
            }

            // move fractions
            CRGB PixelA;
            CRGB PixelB;

            for (uint16_t x = 0; x < _width; x++)
            {
                uint16_t amount = _ptrNoise->noise[x][0] * amt;
                uint8_t delta = _height - 1 - (amount / 256);
                uint8_t fractions = amount - (delta * 256);

                for (uint16_t y = 1; y < _height; y++)
                {
                    PixelA = ledsTemp[xy(x, y)];
                    PixelB = ledsTemp[xy(x, y - 1)];

                    PixelA %= 255 - fractions;
                    PixelB %= fractions;

                    leds[xy(x, y)] = PixelA + PixelB;
                }

                PixelA = ledsTemp[xy(x, 0)];
                PixelB = ledsTemp[xy(x, _height - 1)];

                PixelA %= 255 - fractions;
                PixelB %= fractions;

                leds[xy(x, 0)] = PixelA + PixelB;
            }
        }
    #endif
};
