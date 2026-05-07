//+--------------------------------------------------------------------------
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

#include <gfxfont.h>
#include <memory>
#include <stdexcept>
#include <unordered_map>

#include "effectmanager.h"
#include "effects/matrix/Boid.h"
#include "gfxbase.h"
#include "systemcontainer.h"

// 32 Entries in the 5-bit gamma table
const uint8_t GFXBase::gamma5[32] =
{
    0x00, 0x01, 0x02, 0x03, 0x05, 0x07, 0x09, 0x0b,
    0x0e, 0x11, 0x14, 0x18, 0x1d, 0x22, 0x28, 0x2e,
    0x36, 0x3d, 0x46, 0x4f, 0x59, 0x64, 0x6f, 0x7c,
    0x89, 0x97, 0xa6, 0xb6, 0xc7, 0xd9, 0xeb, 0xff
};

// 64 Entries in the 6-bit gamma table
const uint8_t GFXBase::gamma6[64] =
{
    0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x08,
    0x09, 0x0a, 0x0b, 0x0d, 0x0e, 0x10, 0x12, 0x13,
    0x15, 0x17, 0x19, 0x1b, 0x1d, 0x20, 0x22, 0x25,
    0x27, 0x2a, 0x2d, 0x30, 0x33, 0x37, 0x3a, 0x3e,
    0x41, 0x45, 0x49, 0x4d, 0x52, 0x56, 0x5b, 0x5f,
    0x64, 0x69, 0x6e, 0x74, 0x79, 0x7f, 0x85, 0x8b,
    0x91, 0x97, 0x9d, 0xa4, 0xab, 0xb2, 0xb9, 0xc0,
    0xc7, 0xcf, 0xd6, 0xde, 0xe6, 0xee, 0xf7, 0xff
};

GFXBase::~GFXBase()
{
}

uint8_t GFXBase::beatcos8(accum88 beats_per_minute, uint8_t lowest, uint8_t highest, uint32_t timebase, uint8_t phase_offset)
{
    uint8_t beat = beat8(beats_per_minute, timebase);
    uint8_t beatCos = cos8(beat + phase_offset);
    uint8_t rangeWidth = highest - lowest;
    uint8_t scaledBeat = scale8(beatCos, rangeWidth);
    uint8_t result = lowest + scaledBeat;
    return result;
}

uint8_t GFXBase::mapsin8(uint8_t theta, uint8_t lowest, uint8_t highest)
{
    uint8_t beatSin = sin8(theta);
    uint8_t rangeWidth = highest - lowest;
    uint8_t scaledBeat = scale8(beatSin, rangeWidth);
    uint8_t result = lowest + scaledBeat;
    return result;
}

uint8_t GFXBase::mapcos8(uint8_t theta, uint8_t lowest, uint8_t highest)
{
    uint8_t beatCos = cos8(theta);
    uint8_t rangeWidth = highest - lowest;
    uint8_t scaledBeat = scale8(beatCos, rangeWidth);
    uint8_t result = lowest + scaledBeat;
    return result;
}

CRGB GFXBase::from16Bit(uint16_t color)
{
    uint8_t r = gamma5[color >> 11];
    uint8_t g = gamma6[(color >> 5) & 0x3F];
    uint8_t b = gamma5[color & 0x1F];

    return CRGB(r, g, b);
}

uint16_t GFXBase::to16bit(uint8_t r, uint8_t g, uint8_t b)
{
    return ((r / 8) << 11) | ((g / 4) << 5) | (b / 8);
}

uint16_t GFXBase::to16bit(const CRGB rgb)
{
    return ((rgb.r / 8) << 11) | ((rgb.g / 4) << 5) | (rgb.b / 8);
}

uint16_t GFXBase::to16bit(CRGB::HTMLColorCode code)
{
    return to16bit(CRGB(code));
}

void GFXBase::Clear(CRGB color)
{
    if (color == CRGB::Black)
        memset(leds, 0, sizeof(CRGB) * _width * _height);
    else
        fill_solid(leds, _width * _height, color);
}

// getPixel
//
// Retrieves the color of a pixel at the specified X and Y coordinates.

CRGB GFXBase::getPixel(int16_t x, int16_t y) const
{
    if (isValidPixel(x, y))
        return leds[XY(x, y)];
    else
        throw std::runtime_error(str_sprintf("Invalid index in getPixel: x=%d, y=%d, LEDCount=%zu", x, y, GetLEDCount()).c_str());
}

// getPixel
//
// Retrieves the color of a pixel at the specified linear index.

CRGB GFXBase::getPixel(int16_t i) const
{
    if (isValidPixel(i))
        return leds[i];
    else
        throw std::runtime_error(str_sprintf("Invalid index in getPixel: i=%d, LEDCount=%zu", i, GetLEDCount()).c_str());
}

void GFXBase::drawPixel(int16_t x, int16_t y, uint16_t color)
{
    if (isValidPixel(x, y))
        leds[XY(x, y)] = from16Bit(color);
}

// drawPixelXY_Blend
//
// Blends a color with the existing pixel color.

void GFXBase::drawPixelXY_Blend(uint8_t x, uint8_t y, CRGB color, uint8_t blend_amount)
{
    if (isValidPixel(x, y)) {
        nblend(leds[XY(x,y)], color, blend_amount);
    }
}

// drawPixelXYF_Wu
//
// Draws an anti-aliased pixel using Wu's algorithm, blending with the background.

void GFXBase::drawPixelXYF_Wu(float x, float y, CRGB color)
{
    uint8_t xx = (x - (int)x) * 255, yy = (y - (int)y) * 255, ix = 255 - xx, iy = 255 - yy;
    uint8_t wu[4] = {WU_WEIGHT(ix, iy), WU_WEIGHT(xx, iy), WU_WEIGHT(ix, yy), WU_WEIGHT(xx, yy)};
    for (uint8_t i = 0; i < 4; i++)
    {
        int16_t xn = x + (i & 1), yn = y + ((i >> 1) & 1);
        if (isValidPixel(xn, yn)) {
            CRGB clr = leds[XY(xn, yn)];
            clr.r = qadd8(clr.r, (color.r * wu[i]) >> 8);
            clr.g = qadd8(clr.g, (color.g * wu[i]) >> 8);
            clr.b = qadd8(clr.b, (color.b * wu[i]) >> 8);
            leds[XY(xn, yn)] = clr;
        }
    }
}

// drawLineF
//
// Draws a gradient line using floating point coordinates (DDA algorithm).

void GFXBase::drawLineF(float x1, float y1, float x2, float y2, const CRGB &col1, const CRGB &col2)
{
    CRGB c2 = (col2 == CRGB::Black) ? col1 : col2;
    float dx = x2 - x1;
    float dy = y2 - y1;
    float steps = fmax(fabs(dx), fabs(dy));
    if (steps == 0) {
        drawPixelXYF_Wu(x1, y1, col1);
        return;
    }
    float xinc = dx / steps;
    float yinc = dy / steps;
    float x = x1;
    float y = y1;
    for (int i = 0; i <= steps; i++) {
        uint8_t blend_amount = (uint8_t)((i / steps) * 255);
        CRGB color = blend(col1, c2, blend_amount);
        drawPixelXYF_Wu(x, y, color);
        x += xinc;
        y += yinc;
    }
}

// drawSafeFilledCircleF
//
// Draws a solid circle using floating point coordinates.

void GFXBase::drawSafeFilledCircleF(float cx, float cy, float radius, CRGB col)
{
    for (int8_t y = -radius; y <= radius; y++)
    {
        for (int8_t x = -radius; x <= radius; x++)
        {
            if (x * x + y * y <= radius * radius)
                drawPixelXYF_Wu(cx + x, cy + y, col);
        }
    }
}

void GFXBase::fillLeds(std::unique_ptr<CRGB[]> &pLEDs)
{
    for (int x = 0; x < _width; x++)
        for (int y = 0; y < _height; y++)
            setPixel(x, y, pLEDs[y * _width + x]);
}

void GFXBase::setPixel(int16_t x, int16_t y, uint16_t color)
{
    if (isValidPixel(x, y))
        leds[XY(x, y)] = from16Bit(color);
    else
        debugE("Invalid setPixel request: x=%d, y=%d, LEDCount=%zu", x, y, GetLEDCount());
}

void GFXBase::setPixel(int16_t x, int16_t y, CRGB color)
{
    if (isValidPixel(x, y))
        leds[XY(x, y)] = color;
    else
        debugE("Invalid setPixel request: x=%d, y=%d, LEDCount=%zu", x, y, GetLEDCount());
}

void GFXBase::setPixel(int16_t x, int r, int g, int b)
{
    if (isValidPixel(x))
        setPixel(x, CRGB(r, g, b));
    else
        debugE("Invalid setPixel request: x=%d, LEDCount=%zu", x, GetLEDCount());
}

// fadePixelToBlackBy
//
// Fast per-pixel fade toward black by 'fadeValue' (0..255).
// Applies scale = 255 - fadeValue to the pixel's RGB in-place.

void GFXBase::fadePixelToBlackBy(int16_t x, int16_t y, uint8_t fadeValue) noexcept
{
    FadePixelInPlace(leds[XY(x, y)], fadeValue);
}

void GFXBase::fadePixelToBlackBy(int16_t i, uint8_t fadeValue) noexcept
{
    FadePixelInPlace(leds[i], fadeValue);
}

void GFXBase::DrawSafeCircle(int centerX, int centerY, int radius, CRGB color) noexcept
{
    int x = radius;
    int y = 0;
    int err = 0;

    while (x >= y)
    {
        if (isValidPixel(centerX+x, centerY+y)) setPixel(centerX+x, centerY+y, color);
        if (isValidPixel(centerX+y, centerY+x)) setPixel(centerX+y, centerY+x, color);
        if (isValidPixel(centerX-y, centerY+x)) setPixel(centerX-y, centerY+x, color);
        if (isValidPixel(centerX-x, centerY+y)) setPixel(centerX-x, centerY+y, color);
        if (isValidPixel(centerX-x, centerY-y)) setPixel(centerX-x, centerY-y, color);
        if (isValidPixel(centerX-y, centerY-x)) setPixel(centerX-y, centerY-x, color);
        if (isValidPixel(centerX+y, centerY-x)) setPixel(centerX+y, centerY-x, color);
        if (isValidPixel(centerX+x, centerY-y)) setPixel(centerX+x, centerY-y, color);

        if (err <= 0)
        {
            y += 1;
            err += 2*y + 1;
        }
        if (err > 0)
        {
            x -= 1;
            err -= 2*x + 1;
        }
    }
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
//
//   We are now at pixel 5, frac2 = .75
//   We fill pixel with .75 worth of color

void GFXBase::setPixelsF(float fPos, float count, CRGB c, bool bMerge) const
{
    float frac1 = fPos - floor(fPos);
    float frac2 = fPos + count - floor(fPos + count);

    uint8_t fade1 = (uint8_t) ((std::max(frac1, 1.0f - count)) * 255); // Fraction is how far past pixel boundary we are (up to our total size) so larger fraction is more dimming
    uint8_t fade2 = (uint8_t) ((1.0f - frac2) * 255);                   // Fraction is how far we are poking into this pixel, so larger fraction is less dimming
    CRGB c1 = c;
    CRGB c2 = c;
    c1 = c1.fadeToBlackBy(fade1);
    c2 = c2.fadeToBlackBy(fade2);

    // These assignments use the + operator of CRGB to merge the colors when requested, and it's pretty
    // naive, just saturating each color element at 255, so the operator could be improved or replaced
    // if needed...

    float p = fPos;
    if (p >= 0 && isValidPixel(p))
        leds[(int)p] = bMerge ? leds[(int)p] + c1 : c1;

    p = fPos + (1.0f - frac1);
    count -= (1.0f - frac1);

    // Middle (body) pixels

    while (count >= 1)
    {
        if (p >= 0 && isValidPixel(p))
            leds[(int)p] = bMerge ? leds[(int)p] + c : c;
        count--;
        p++;
    };

    // Final pixel, if in bounds
    if (count > 0)
        if (p >= 0 && isValidPixel(p))
            leds[(int)p] = bMerge ? leds[(int)p] + c2 : c2;
}

void GFXBase::blurRows(CRGB *leds, uint16_t width, uint16_t height, uint16_t first, fract8 blur_amount)
{
    // blur rows same as columns, for irregular matrix
    uint8_t keep = 255 - blur_amount;
    uint8_t seep = blur_amount >> 1;
    for (uint16_t row = 0; row < height; row++)
    {
        CRGB carryover = CRGB::Black;
        for (uint16_t i = first; i < width; i++)
        {
            CRGB cur = leds[XY(i, row)];
            CRGB part = cur;
            part.nscale8(seep);
            cur.nscale8(keep);
            cur += carryover;
            if (i)
                leds[XY(i - 1, row)] += part;
            leds[XY(i, row)] = cur;
            carryover = part;
        }
    }
}

// blurColumns: perform a blur1d on each column of a rectangular matrix

void GFXBase::blurColumns(CRGB *leds, uint16_t width, uint16_t height, uint16_t first, fract8 blur_amount)
{
    // blur columns
    uint8_t keep = 255 - blur_amount;
    uint8_t seep = blur_amount >> 1;
    for (uint16_t col = 0; col < width; ++col)
    {
        CRGB carryover = CRGB::Black;
        for (uint16_t i = first; i < height; ++i)
        {
            CRGB cur = leds[XY(col, i)];
            CRGB part = cur;
            part.nscale8(seep);
            cur.nscale8(keep);
            cur += carryover;
            if (i)
                leds[XY(col, i - 1)] += part;
            leds[XY(col, i)] = cur;
            carryover = part;
        }
    }
}

void GFXBase::blur2d(CRGB *leds, uint16_t width, uint16_t firstColumn, uint16_t height, uint16_t firstRow, fract8 blur_amount)
{
    blurRows(leds, width, height, firstColumn, blur_amount);
    blurColumns(leds, width, height, firstRow, blur_amount);
}

void GFXBase::BlurFrame(int amount)
{
    // BUGBUG (davepl) Needs to call isVuVisible on the effects manager to find out if it starts at row 1 or 0
    blur2d(leds, _width, 0, _height, 1, amount);
}

void GFXBase::CyclePalette(int offset)
{
    loadPalette(_paletteIndex + offset);
}

void GFXBase::ChangePalettePeriodically()
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

// Cross-fade current palette slowly toward the target palette
//
// Each time that nblendPaletteTowardPalette is called, small changes
// are made to currentPalette to bring it closer to matching targetPalette.
// You can control how many changes are made in each call:
//   - the default of 24 is a good balance
//   - meaningful values are 1-48.  1=very very slow, 48=quickest
//   - "0" means do not change the currentPalette at all; freeze

void GFXBase::PausePalette(bool bPaused)
{
    _palettePaused = bPaused;
}

void GFXBase::UpdatePaletteCycle()
{
    ChangePalettePeriodically();
    uint8_t maxChanges = 24;
    nblendPaletteTowardPalette(_currentPalette, _targetPalette, maxChanges);
}

void GFXBase::RandomPalette()
{
    loadPalette(_randomPaletteIndex);
}

void GFXBase::fillRectangle(int x0, int y0, int x1, int y1, CRGB color)
{
    for (int x = x0; x < x1; x++)
        for (int y = y0; y < y1; y++)
            drawPixel(x, y, color);
}

void GFXBase::setPalette(const CRGBPalette16& palette)
{
    _currentPalette = palette;
    _targetPalette = palette;
    _currentPaletteName = "Custom";
}

// loadPalette
//
// Note that this function may recurse without
// bound if your random() is very very dumb.

void GFXBase::loadPalette(int index)
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

void GFXBase::setPalette(const String& paletteName)
{
    static const std::unordered_map<const char*, int> paletteMap = {
        {"Rainbow", 0},
        {"Ocean", 1},
        {"Cloud", 2},
        {"Forest", 3},
        {"Party", 4},
        {"Grayscale", 5},
        {"Heat", 6},
        {"Lava", 7},
        {"Ice", 8}
    };

    auto it = paletteMap.find(paletteName.c_str());
    if (it != paletteMap.end()) {
        loadPalette(it->second);
    } else if (paletteName == "Random") {
        RandomPalette();
    }
}

void GFXBase::listPalettes()
{
    Serial.println("{");
    Serial.print("  \"count\": ");
    Serial.print(_paletteCount);
    Serial.println(",");
    Serial.println("  \"results\": [");

    static constexpr const char* paletteNames[] =
    {
        "Rainbow", "Ocean", "Cloud", "Forest", "Party",
        "Grayscale", "Heat", "Lava", "Ice", "Random"
    };

    for (int i = 0; i < _paletteCount; i++)
    {
        Serial.print("    \"");
        Serial.print(paletteNames[i]);
        if (i == _paletteCount - 1)
            Serial.println("\"");
        else
            Serial.println("\",");
    }

    Serial.println("  ]");
    Serial.println("}");
}

void GFXBase::setupGrayscalePalette()
{
    _targetPalette = CRGBPalette16(CRGB::Black, CRGB::White);
}

void GFXBase::setupIcePalette()
{
    _targetPalette = CRGBPalette16(CRGB::Black, CRGB::Blue, CRGB::Aqua, CRGB::White);
}

// Oscillators and Emitters

// set the speeds (and by that ratios) of the oscillators here

void GFXBase::MoveOscillators()
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
        p[i] = map8(sin8(osci[i]), 0, std::min(255U, (unsigned int)_width - 1)); // why? to keep the result in the range of 0-_width (matrix size)
    }
}

void GFXBase::ResetOscillators()
{
    std::fill_n(osci, 6, 0);
    std::fill_n(p, 6, 0);
}

// All the Caleidoscope functions work directly within the screenbuffer (leds array).
// Draw whatever you like in the area x(0-15) and y (0-15) and then copy it around.

// rotates the first 16x16 quadrant 3 times onto a 32x32 (+90 degrees rotation for each one)

void GFXBase::Caleidoscope1() const
{
    for (int x = 0; x < ((_width + 1) / 2); x++)
    {
        for (int y = 0; y < ((_height + 1) / 2); y++)
        {
            leds[XY(_width - 1 - x, y)] = leds[XY(x, y)];
            leds[XY(_width - 1 - x, _height - 1 - y)] = leds[XY(x, y)];
            leds[XY(x, _height - 1 - y)] = leds[XY(x, y)];
        }
    }
}

// mirror the first 16x16 quadrant 3 times onto a 32x32

void GFXBase::Caleidoscope2() const
{
    for (int x = 0; x < ((_width + 1) / 2); x++)
    {
        for (int y = 0; y < ((_height + 1) / 2); y++)
        {
            leds[XY(_width - 1 - x, y)] = leds[XY(y, x)];
            leds[XY(x, _height - 1 - y)] = leds[XY(y, x)];
            leds[XY(_width - 1 - x, _height - 1 - y)] = leds[XY(x, y)];
        }
    }
}

// copy one diagonal triangle into the other one within a 16x16

void GFXBase::Caleidoscope3() const
{
    for (int x = 0; x < ((_width + 1) / 2); x++)
    {
        for (int y = 0; y <= x; y++)
        {
            leds[XY(x, y)] = leds[XY(y, x)];
        }
    }
}

// copy one diagonal triangle into the other one within a 16x16 (90 degrees rotated compared to Caleidoscope3)

void GFXBase::Caleidoscope4() const
{
    for (int x = 0; x < ((_width + 1) / 2); x++)
    {
        for (int y = 0; y <= ((_height + 1) / 2) - x; y++)
        {
            leds[XY(((_height + 1) / 2) - y, ((_width + 1) / 2) - x)] = leds[XY(x, y)];
        }
    }
}

// copy one diagonal triangle into the other one within a 8x8

void GFXBase::Caleidoscope5() const
{
    for (int x = 0; x < _width / 4; x++)
    {
        for (int y = 0; y <= x; y++)
        {
            leds[XY(x, y)] = leds[XY(y, x)];
        }
    }

    for (int x = _width / 4; x < _width / 2; x++)
    {
        for (int y = _height / 4; y >= 0; y--)
        {
            leds[XY(x, y)] = leds[XY(y, x)];
        }
    }
}

// rotates the first 8x8 quadrant 3 times onto a 16x16

void GFXBase::Caleidoscope6() const
{
    for (int x = 1; x < ((_width + 1) / 2); x++)
    {
        leds[XY(7 - x, 7)] = leds[XY(x, 0)];
    } // a
    for (int x = 2; x < ((_width + 1) / 2); x++)
    {
        leds[XY(7 - x, 6)] = leds[XY(x, 1)];
    } // b
    for (int x = 3; x < ((_width + 1) / 2); x++)
    {
        leds[XY(7 - x, 5)] = leds[XY(x, 2)];
    } // c
    for (int x = 4; x < ((_width + 1) / 2); x++)
    {
        leds[XY(7 - x, 4)] = leds[XY(x, 3)];
    } // d
    for (int x = 5; x < ((_width + 1) / 2); x++)
    {
        leds[XY(7 - x, 3)] = leds[XY(x, 4)];
    } // e
    for (int x = 6; x < ((_width + 1) / 2); x++)
    {
        leds[XY(7 - x, 2)] = leds[XY(x, 5)];
    } // f
    for (int x = 7; x < ((_width + 1) / 2); x++)
    {
        leds[XY(7 - x, 1)] = leds[XY(x, 6)];
    } // g
}

// SpiralStream
//
// create a square twister to the left or counter-clockwise
// x and y for center, r for radius

void GFXBase::SpiralStream(int x, int y, int r, uint8_t dimm) const
{
    for (int d = r; d >= 0; d--)
    { // from the outside to the inside
        for (int i = x - d; i <= x + d; i++)
        {
            leds[XY(i, y - d)] += leds[XY(i + 1, y - d)]; // lowest row to the right
            leds[XY(i, y - d)].nscale8(dimm);
        }
        for (int i = y - d; i <= y + d; i++)
        {
            leds[XY(x + d, i)] += leds[XY(x + d, i + 1)]; // right colum up
            leds[XY(x + d, i)].nscale8(dimm);
        }
        for (int i = x + d; i >= x - d; i--)
        {
            leds[XY(i, y + d)] += leds[XY(i - 1, y + d)]; // upper row to the left
            leds[XY(i, y + d)].nscale8(dimm);
        }
        for (int i = y + d; i >= y - d; i--)
        {
            leds[XY(x - d, i)] += leds[XY(x - d, i - 1)]; // left colum down
            leds[XY(x - d, i)].nscale8(dimm);
        }
    }
}

// expand everything within a circle

void GFXBase::Expand(int centerX, int centerY, int radius, uint8_t dimm)
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
            leds[XY(a + centerX, b + centerY)]   = leds[XY(nextA + centerX, nextB + centerY)];
            leds[XY(b + centerX, a + centerY)]   = leds[XY(nextB + centerX, nextA + centerY)];
            leds[XY(-a + centerX, b + centerY)]  = leds[XY(-nextA + centerX, nextB + centerY)];
            leds[XY(-b + centerX, a + centerY)]  = leds[XY(-nextB + centerX, nextA + centerY)];
            leds[XY(-a + centerX, -b + centerY)] = leds[XY(-nextA + centerX, -nextB + centerY)];
            leds[XY(-b + centerX, -a + centerY)] = leds[XY(-nextB + centerX, -nextA + centerY)];
            leds[XY(a + centerX, -b + centerY)]  = leds[XY(nextA + centerX, -nextB + centerY)];
            leds[XY(b + centerX, -a + centerY)]  = leds[XY(nextB + centerX, -nextA + centerY)];

            // dim them
            leds[XY(a + centerX, b + centerY)].nscale8(dimm);
            leds[XY(b + centerX, a + centerY)].nscale8(dimm);
            leds[XY(-a + centerX, b + centerY)].nscale8(dimm);
            leds[XY(-b + centerX, a + centerY)].nscale8(dimm);
            leds[XY(-a + centerX, -b + centerY)].nscale8(dimm);
            leds[XY(-b + centerX, -a + centerY)].nscale8(dimm);
            leds[XY(a + centerX, -b + centerY)].nscale8(dimm);
            leds[XY(b + centerX, -a + centerY)].nscale8(dimm);

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

void GFXBase::StreamRight(uint8_t scale, int fromX, int toX, int fromY, int toY)
{
    for (int x = fromX + 1; x < toX; x++)
    {
        for (int y = fromY; y < toY; y++)
        {
            leds[XY(x, y)] += leds[XY(x - 1, y)];
            leds[XY(x, y)].nscale8(scale);
        }
    }
    for (int y = fromY; y < toY; y++)
        leds[XY(0, y)].nscale8(scale);
}

// give it a linear tail to the left

void GFXBase::StreamLeft(uint8_t scale, int fromX, int toX, int fromY, int toY)
{
    for (int x = toX; x < fromX; x++)
    {
        for (int y = fromY; y < toY; y++)
        {
            leds[XY(x, y)] += leds[XY(x + 1, y)];
            leds[XY(x, y)].nscale8(scale);
        }
    }
    for (int y = fromY; y < toY; y++)
        leds[XY(0, y)].nscale8(scale);
}

// give it a linear tail downwards

void GFXBase::StreamDown(uint8_t scale)
{
    for (int x = 0; x < _width; x++)
    {
        for (int y = 1; y < _height; y++)
        {
            leds[XY(x, y)] += leds[XY(x, y - 1)];
            leds[XY(x, y)].nscale8(scale);
        }
    }
    for (int x = 0; x < _width; x++)
        leds[XY(x, 0)].nscale8(scale);
}

// give it a linear tail upwards

void GFXBase::StreamUp(uint8_t scale)
{
    for (int x = 0; x < _width; x++)
    {
        for (int y = (int)_height - 2; y >= 0; y--)
        {
            leds[XY(x, y)] += leds[XY(x, y + 1)];
            leds[XY(x, y)].nscale8(scale);
        }
    }
    for (int x = 0; x < _width; x++)
        leds[XY(x, (int)_height - 1)].nscale8(scale);
}

// give it a linear tail up and to the left

void GFXBase::StreamUpAndLeft(uint8_t scale)
{
    for (int x = 0; x < (int)_width - 1; x++)
    {
        for (int y = (int)_height - 2; y >= 0; y--)
        {
            leds[XY(x, y)] += leds[XY(x + 1, y + 1)];
            leds[XY(x, y)].nscale8(scale);
        }
    }
    for (int x = 0; x < (int)_width; x++)
        leds[XY(x, (int)_height - 1)].nscale8(scale);
    for (int y = 0; y < (int)_height; y++)
        leds[XY((int)_width - 1, y)].nscale8(scale);
}

// give it a linear tail up and to the right

void GFXBase::StreamUpAndRight(uint8_t scale)
{
    for (int x = 0; x < (int)_width - 1; x++)
    {
        for (int y = (int)_height - 2; y >= 0; y--)
        {
            leds[XY(x + 1, y)] += leds[XY(x, y + 1)];
            leds[XY(x, y)].nscale8(scale);
        }
    }
    for (int x = 0; x < (int)_width; x++)
        leds[XY(x, (int)_height - 1)].nscale8(scale);

    for (int y = 0; y < (int)_height; y++)
        leds[XY((int)_width - 1, y)].nscale8(scale);
}

// just move everything one line down

void GFXBase::MoveDown()
{
    for (int y = (int)_height - 1; y > 0; y--)
    {
        for (int x = 0; x < (int)_width; x++)
        {
            leds[XY(x, y)] = leds[XY(x, y - 1)];
        }
    }
}

// just move everything one line down

void GFXBase::VerticalMoveFrom(int start, int end)
{
    for (int y = end; y > start; y--)
    {
        for (int x = 0; x < (int)_width; x++)
        {
            leds[XY(x, y)] = leds[XY(x, y - 1)];
        }
    }
}

// copy the rectangle defined with 2 points x0, y0, x1, y1
// to the rectangle beginning at x2, x3

void GFXBase::Copy(uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1, uint8_t x2, uint8_t y2)
{
    for (int y = y0; y < y1 + 1; y++)
    {
        for (int x = x0; x < x1 + 1; x++)
        {
            leds[XY(x + x2 - x0, y + y2 - y0)] = leds[XY(x, y)];
        }
    }
}

void GFXBase::BresenhamLine(int x0, int y0, int x1, int y1, CRGB color, bool bMerge)
{
    int dx = abs(x1 - x0); // Delta in x direction
    int dy = abs(y1 - y0); // Delta in y direction
    int sx = (x0 < x1) ? 1 : -1; // Step in x direction
    int sy = (y0 < y1) ? 1 : -1; // Step in y direction

    int err = dx - dy; // Initial error term

    while (true)
    {
        int index = XY(x0, y0);
        if (isValidPixel(index))
        {
            // Optimization opportunity: unswtitch bMerge into another function
            leds[index] = bMerge ? leds[index] + color : color;
        }

        if (x0 == x1 && y0 == y1)
            break; // Exit the loop once we've reached the destination

        int e2 = 2 * err; // Error term multiplied by 2 for efficiency. Saves second test for Y.
        if (e2 > -dy) // Move in the x direction if needed
        {
            err -= dy;
            x0 += sx;
        }

        if (e2 < dx) // Move in the y direction if needed
        {
            err += dx;
            y0 += sy;
        }
    }
}

void GFXBase::BresenhamLine(int x0, int y0, int x1, int y1, uint8_t colorIndex, bool bMerge)
{
    BresenhamLine(x0, y0, x1, y1, ColorFromCurrentPalette(colorIndex), bMerge);
}

void GFXBase::drawLine(int x0, int y0, int x1, int y1, CRGB color)
{
    BresenhamLine(x0, y0, x1, y1, color);
}

void GFXBase::DimAll(uint8_t value)
{
    for (int i = 0; i < (int)_ledcount; i++)
        fadePixelToBlackBy(i, 255 - value);
}

CRGB GFXBase::ColorFromCurrentPalette(uint8_t index, uint8_t brightness, TBlendType blendType) const
{
    return ColorFromPalette(_currentPalette, index, brightness, _currentBlendType);
}

CRGB GFXBase::HsvToRgb(uint8_t h, uint8_t s, uint8_t v)
{
    CHSV hsv = CHSV(h, s, v);
    CRGB rgb;
    hsv2rgb_spectrum(hsv, rgb);
    return rgb;
}

#if USE_NOISE
    // The following functions are specializations of noise-related member function
    // templates declared in gfxbase.h.

    void GFXBase::NoiseVariablesSetup() const
    {
        _ptrNoise->noisesmoothing = 200;

        _ptrNoise->noise_x = random16();
        _ptrNoise->noise_y = random16();
        _ptrNoise->noise_z = random16();
        _ptrNoise->noise_scale_x = 6000;
        _ptrNoise->noise_scale_y = 6000;
    }

    void GFXBase::SetNoise(uint32_t nx, uint32_t ny, uint32_t nz, uint32_t sx, uint32_t sy)
    {
        _ptrNoise->noise_x += nx;
        _ptrNoise->noise_y += ny;
        _ptrNoise->noise_z += nx;
        _ptrNoise->noise_scale_x = sx;
        _ptrNoise->noise_scale_y = sy;
    }

    void GFXBase::FillGetNoise() const
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
void GFXBase::MoveInwardX(int startY, int endY)
{
    for (int y = startY; y <= endY; y++)
    {
        for (int x = _width / 2; x > 0; x--)
            leds[XY(x, y)] = leds[XY(x - 1, y)];

        for (int x = _width / 2; x < (int)_width; x++)
            leds[XY(x, y)] = leds[XY(x + 1, y)];
    }
}

void GFXBase::MoveOutwardsX(int startY, int endY)
{
    for (int y = startY; y <= endY; y++)
    {
        for (int x = 0; x < (int)_width / 2 - 1; x++)
        {
            leds[XY(x, y)] = leds[XY(x + 1, y)];
            leds[XY((int)_width - x - 1, y)] = leds[XY((int)_width - x - 2, y)];
        }
    }
}

void GFXBase::MoveX(uint8_t delta) const
{
    for (int y = 0; y < (int)_height; y++)
    {
        for (int x = 0; x < (int)_width - delta; x++)
            leds[XY(x, y)] = leds[XY(x + delta, y)];
        for (int x = (int)_width - delta; x < (int)_width; x++)
            leds[XY(x, y)] = leds[XY(x + delta - (int)_width, y)];
    }
}

void GFXBase::MoveY(uint8_t delta) const
{
    CRGB tmp = 0;
    for (int x = 0; x < (int)_width; x++)
    {
        tmp = leds[XY(x, 0)];
        for (int m = 0; m < delta; m++)
        {
            for (int y = 0; y < (int)_height - 1; y++)
                leds[XY(x, y)] = leds[XY(x, y + 1)];

            leds[XY(x, (int)_height - 1)] = tmp;
        }
    }
}

void GFXBase::PrepareFrame()
{
}

void GFXBase::PostProcessFrame(uint16_t, uint16_t)
{
}

GFXBase::GFXBase(int w, int h) : Adafruit_GFX(w, h),
                        _width(w),
                        _height(h),
                        _ledcount(w*h)
{
    // Allocate boids for matrix effects (like PatternBounce) when we have matrix dimensions
    #if MATRIX_HEIGHT > 1
        debugV("Allocating boids for matrix effects");
        // Boid state scales with width and can become large on matrix targets, so keep it in PSRAM.
        _boids = make_unique_psram_constructed<Boid>(_width);
        assert(_boids);
    #endif

    debugV("Setting up palette");
    loadPalette(0);
    ResetOscillators();
}

void GFXBase::ConfigureTopology(size_t width, size_t height, bool serpentine)
{
    // The runtime topology work is intentionally routed through GFXBase so effects that already ask g()
    // for geometry start honoring live strip layouts without each effect learning about DeviceConfig.
    #if MATRIX_HEIGHT > 1
    if (width != _width)
    {
        debugV("Resizing boid state to match runtime topology width");
        // Topology changes invalidate width-sized boid caches. Rebuild them in PSRAM before effects restart.
        _boids = make_unique_psram_constructed<Boid>(width);
        assert(_boids);
    }
    #endif

    _width      = width;
    _height     = height;
    _ledcount   = width * height;
    _serpentine = serpentine;

    WIDTH  = width;
    HEIGHT = height;
    
    Adafruit_GFX::_width = width;
    Adafruit_GFX::_height = height;
}

#if USE_NOISE
void GFXBase::EnsureNoise() const
{
    std::call_once(_noiseInitOnce, [this]()
    {
        // Noise is large and only used by a subset of effects. Lazy allocation keeps the boot path leaner
        // and still allows runtime topology to stay within the build-time maximum noise backing store.
        _ptrNoise = std::make_unique<Noise>();
        assert(_ptrNoise);
        NoiseVariablesSetup();
        FillGetNoise();
    });
}
#endif

// Dirty hack to support FastLED, which calls out of band to get the pixel index for "the" array, without
// any indication of which array or who's asking, so we assume the first matrix. If you have trouble with
// more than one matrix and some FastLED functions like blur2d, this would be why.
uint16_t XY(uint16_t x, uint16_t y)
{
    auto& g = g_ptrSystem->GetEffectManager().g();
    return g.xy(x, y);
}

const GFXBase::PolarMapArray& GFXBase::getPolarMap()
{
    static std::unique_ptr<PolarMapArray> rMap_ptr;
    static std::mutex rMap_mutex;

    // Double-checked locking for thread-safe, on-demand initialization
    if (!rMap_ptr)
    {
        std::lock_guard lock(rMap_mutex);
        if (!rMap_ptr)
        {
            // Allocate from PSRAM using the project's helper
            rMap_ptr = make_unique_psram<PolarMapArray>();

            auto& rMap = *rMap_ptr;
            const uint16_t C_X = kMatrixWidth / 2;
            const uint16_t C_Y = kMatrixHeight / 2;
            const float mapp = 255.0f / kMatrixWidth;

            for (int16_t x = -C_X; x < C_X + (kMatrixWidth % 2); x++)
            {
                for (int16_t y = -C_Y; y < C_Y + (kMatrixHeight% 2); y++)
                {
                    float angle_rad = atan2f(static_cast<float>(y), static_cast<float>(x));
                    float radius_float = hypotf(static_cast<float>(x), static_cast<float>(y));

                    rMap[x + C_X][y + C_Y].angle = 128.0f * (angle_rad / (float)M_PI);
                    rMap[x + C_X][y + C_Y].scaled_radius = radius_float * mapp;
                    rMap[x + C_X][y + C_Y].unscaled_radius = radius_float;
                }
            }

            // A note on the radius calculations:
            //
            // `unscaled_radius` is the true geometric distance from the center of the
            // matrix to the pixel. This is useful for effects that need the real
            // physical distance.
            //
            // `scaled_radius` maps the geometric radius to a range that is more
            // suitable for use with 8-bit FastLED functions (like inoise8).
            // The scaling is normalized by the matrix width, which is a common
            // technique to make radial effects work consistently across different
            // matrix sizes.
        }
    }

    return *rMap_ptr;
}
