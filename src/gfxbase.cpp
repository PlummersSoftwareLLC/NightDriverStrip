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

#include <algorithm>
#include <cmath>
#include <gfxfont.h>
#include <memory>
#include <mutex>
#include <stdexcept>
#include <unordered_map>

#include "effectmanager.h"
#include "effects/matrix/Boid.h"
#include "gfxbase.h"
#include "systemcontainer.h"

namespace
{
    DRAM_ATTR allocated_unique_ptr<GFXBase::PolarMapArray> g_polarMap;
    DRAM_ATTR std::mutex g_polarMapMutex;
}

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

// Returns the longest substring of 'text' that fits within 'maxWidth' when rendered with 
// the current font

String GFXBase::FitTextToWidth(const String& text, int maxWidth)
{
    if (maxWidth <= 0)
        return "";

    String fitted = text;
    int16_t x1, y1;
    uint16_t textWidth, textHeight;

    while (!fitted.isEmpty())
    {
        getTextBounds(fitted, 0, 0, &x1, &y1, &textWidth, &textHeight);
        if (textWidth <= maxWidth)
            break;

        fitted.remove(fitted.length() - 1);
    }

    return fitted;
}

// Draws the specified text centered within the given rectangle, using the current font 
// and text color.

void GFXBase::DrawTextInRect(const String& text, int x, int y, int width, int height, uint16_t color)
{
    if (text.isEmpty() || width <= 0 || height <= 0)
        return;

    const String fitted = FitTextToWidth(text, width);
    if (fitted.isEmpty())
        return;

    int16_t x1, y1;
    uint16_t textWidth, textHeight;
    getTextBounds(fitted, 0, 0, &x1, &y1, &textWidth, &textHeight);

    const int drawX = x + std::max(0, (width - static_cast<int>(textWidth)) / 2 - x1);
    const int drawY = y + (height - static_cast<int>(textHeight)) / 2 - y1;

    setTextColor(color);
    setCursor(drawX, drawY);
    print(fitted);
}

void GFXBase::DrawTextInRect(const String& text, int x, int y, int width, int height, const CRGB& color)
{
    DrawTextInRect(text, x, y, width, height, to16bit(color));
}

void GFXBase::DrawTextInRect(const String& text, int x, int y, int width, int height, CRGB::HTMLColorCode color)
{
    DrawTextInRect(text, x, y, width, height, to16bit(color));
}

void GFXBase::DrawTextInBand(const String& text, int bandTop, int bandHeight, uint16_t color)
{
    DrawTextInRect(text, 0, bandTop, static_cast<int>(GetMatrixWidth()), bandHeight, color);
}

void GFXBase::DrawTextInBand(const String& text, int bandTop, int bandHeight, const CRGB& color)
{
    DrawTextInBand(text, bandTop, bandHeight, to16bit(color));
}

void GFXBase::DrawTextInBand(const String& text, int bandTop, int bandHeight, CRGB::HTMLColorCode color)
{
    DrawTextInBand(text, bandTop, bandHeight, to16bit(color));
}

void GFXBase::Clear(CRGB color)
{
    const size_t count = _width * _height;
    if (color == CRGB::Black)
        memset(leds, 0, sizeof(CRGB) * count);
    else
        fill_solid(leds, count, color);

    // Also zero the whites plane (if allocated). Without this, a previous
    // effect that lit the dedicated W LEDs (e.g. WarmGlowEffect or any
    // setPixelWhite/setPixelCCT caller) leaves them lit across the
    // effect-switch boundary - the next effect writes only leds[] and the
    // stale W content washes its colors out. Effects that want to preserve
    // whites can rewrite them after the Clear() call; that's how the leds
    // plane already behaves.

    if (whites)
        memset(whites, 0, sizeof(CRGBW) * count);
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

void GFXBase::fillLeds(const CRGB* pLEDs)
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

uint8_t GFXBase::ScaleWhiteCoverage(float coverage)
{
    return static_cast<uint8_t>(std::clamp(coverage, 0.0f, 1.0f) * 255.0f);
}

void GFXBase::setWhitePixelsF(float fPos, float count, CRGBW white, bool bMerge) const
{
    if (!whites || count <= 0.0f)
        return;

    const float start = fPos;
    const float end = fPos + count;
    const int firstPixel = static_cast<int>(floorf(start));
    const int lastPixel = static_cast<int>(floorf(end));

    for (int pixel = firstPixel; pixel <= lastPixel; ++pixel)
    {
        const float pixelStart = static_cast<float>(pixel);
        const float pixelEnd = pixelStart + 1.0f;
        const float coverage = std::min(end, pixelEnd) - std::max(start, pixelStart);
        if (coverage <= 0.0f || !isValidPixel(pixel))
            continue;

        const uint8_t scale = ScaleWhiteCoverage(coverage);
        CRGBW scaled(
            scale8_video(white.cw, scale),
            scale8_video(white.ww, scale));

        if (bMerge)
        {
            whites[pixel].cw = qadd8(whites[pixel].cw, scaled.cw);
            whites[pixel].ww = qadd8(whites[pixel].ww, scaled.ww);
        }
        else
        {
            whites[pixel] = scaled;
        }
    }
}

CRGB GFXBase::ApproximateRgbForKelvin(uint16_t kelvin, uint8_t brightness)
{
    constexpr uint16_t kKelvinWarm = 2700;
    constexpr uint16_t kKelvinCool = 6500;
    constexpr uint16_t kKelvinSpan = kKelvinCool - kKelvinWarm;

    const uint16_t clamped = std::min<uint16_t>(std::max<uint16_t>(kelvin, kKelvinWarm), kKelvinCool);
    const uint16_t coolPart = static_cast<uint16_t>(clamped - kKelvinWarm);
    const uint16_t warmPart = static_cast<uint16_t>(kKelvinSpan - coolPart);

    const auto mix = [&](uint8_t warm, uint8_t cool) -> uint8_t {
        const uint32_t channel = (static_cast<uint32_t>(warm) * warmPart)
                               + (static_cast<uint32_t>(cool) * coolPart)
                               + (kKelvinSpan / 2);
        return static_cast<uint8_t>((channel / kKelvinSpan) * brightness / 255);
    };

    return CRGB(mix(255, 205), mix(180, 225), mix(100, 255));
}

CRGB GFXBase::ScaleRgbToMax(CRGB color, uint8_t brightness)
{
    const uint8_t maxChannel = std::max(color.r, std::max(color.g, color.b));
    if (maxChannel == 0)
        return CRGB::Black;

    color.r = static_cast<uint8_t>((static_cast<uint16_t>(color.r) * 255U) / maxChannel);
    color.g = static_cast<uint8_t>((static_cast<uint16_t>(color.g) * 255U) / maxChannel);
    color.b = static_cast<uint8_t>((static_cast<uint16_t>(color.b) * 255U) / maxChannel);
    color.nscale8_video(brightness);
    return color;
}

CRGB GFXBase::MaximumRgbForKelvin(uint16_t kelvin, uint8_t brightness)
{
    return ScaleRgbToMax(ApproximateRgbForKelvin(kelvin, 255), brightness);
}

CRGBW GFXBase::MaximumWhiteForKelvin(uint16_t kelvin, uint8_t brightness)
{
    CRGBW split = SplitByCct(kelvin, 255);
    const uint8_t maxChannel = std::max(split.cw, split.ww);
    if (maxChannel == 0 || brightness == 0)
        return CRGBW::Black();

    split.cw = static_cast<uint8_t>((static_cast<uint32_t>(split.cw) * 255U * brightness) / (maxChannel * 255U));
    split.ww = static_cast<uint8_t>((static_cast<uint32_t>(split.ww) * 255U * brightness) / (maxChannel * 255U));
    return split;
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

GFXBase::GFXBase(int w, int h) : Adafruit_GFX(w, h),
                        _width(w),
                        _height(h),
                        _ledcount(w*h)
{
    // Allocate boids for matrix effects (like PatternBounce) when we have matrix dimensions
    #if MATRIX_HEIGHT > 1
        debugV("Allocating boids for matrix effects");
        // Boid array is small (a few KB at most) and touched in animation hot paths,
        // so keep it on the default heap instead of explicitly pinning it to PSRAM.
        _boids = std::make_unique<Boid[]>(_width);
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
        // See note in the constructor: keep this on the default heap.
        _boids = std::make_unique<Boid[]>(width);
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
bool GFXBase::EnsureNoise() const
{
    bool justInitialized = false;
    std::call_once(_noiseInitOnce, [&]()
    {
        // Noise is large and only used by a subset of effects. Lazy allocation keeps the boot path leaner
        // and still allows runtime topology to stay within the build-time maximum noise backing store.
        _ptrNoise = std::make_unique<Noise>();
        assert(_ptrNoise);
        _ptrNoise->noisesmoothing = 200;

        _ptrNoise->noise_x = random16();
        _ptrNoise->noise_y = random16();
        _ptrNoise->noise_z = random16();
        _ptrNoise->noise_scale_x = 6000;
        _ptrNoise->noise_scale_y = 6000;
        FillGetNoiseImpl();
        justInitialized = true;
    });

    return justInitialized;
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
    std::lock_guard guard(g_polarMapMutex);
    if (!g_polarMap)
    {
        // Allocate from PSRAM using the project's helper.
        g_polarMap = make_unique_psram<PolarMapArray>();

        auto& rMap = *g_polarMap;
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

    return *g_polarMap;
}
