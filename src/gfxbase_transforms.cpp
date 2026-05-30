//+--------------------------------------------------------------------------
//
// File:        gfxbase_transforms.cpp
//
// This file is part of gfxbase.cpp; see that file header for additional context.
//
// Split scope: GFXBase geometric transforms, streams, kaleidoscope, and draw-shape helpers.
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
