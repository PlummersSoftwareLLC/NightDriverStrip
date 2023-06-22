//+--------------------------------------------------------------------------
//
// File:        PatternMisc.h
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
*
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

#ifndef PatternMisc_H
#define PatternMisc_H

#include "Geometry.h"

class PatternSunburst : public LEDStripEffect
{
  public:

    PatternSunburst() : LEDStripEffect(EFFECT_MATRIX_SUNBURST, "Sunburst")
    {
    }

    PatternSunburst(const JsonObjectConst& jsonObject) : LEDStripEffect(jsonObject)
    {
    }

    virtual size_t DesiredFramesPerSecond() const override
    {
        return 60;
    }

    virtual void Draw() override
    {
      uint8_t dim = beatsin8(2, 210, 250);
      g()->DimAll(dim);

      for (int i = 2; i <= MATRIX_WIDTH / 2; i++)
      {
        CRGB color = g()->ColorFromCurrentPalette((i - 2) * (240 / (MATRIX_WIDTH / 2)));

        // The LIB8TION library defines beatsin8, but this needed beatcos8 which did not exist, so I
        // added it to the graphics interface rathe than adding it to a custom version of lib8tion

        uint8_t x = g()->beatcos8((17 - i) * 2, MATRIX_CENTER_X - i, MATRIX_CENTER_X + i);
        uint8_t y = beatsin8((17 - i) * 2, MATRIX_CENTER_Y - i, MATRIX_CENTER_Y + i);

        if (color.r != 0 || color.g != 0 || color.b !=0 )
          g()->setPixel(x, y, color);
      }
    }
};

class PatternRose : public LEDStripEffect
{
  public:

    PatternRose() : LEDStripEffect(EFFECT_MATRIX_ROSE, "Rose")
    {
    }

    PatternRose(const JsonObjectConst& jsonObject) : LEDStripEffect(jsonObject)
    {
    }

    virtual size_t DesiredFramesPerSecond() const override
    {
        return 60;
    }

    virtual bool RequiresDoubleBuffering() const override
    {
        return false;
    }

    virtual void Draw() override
    {
      uint8_t dim = beatsin8(2, 170, 250);
      g()->DimAll(dim);


      for (uint16_t i = 0; i < MATRIX_HEIGHT; i++)
      {
        CRGB color;

        uint8_t x = 0;
        uint8_t y = 0;

        if (i < 16) {
          x = g()->beatcos8((i + 1) * 2, i, MATRIX_HEIGHT - i) + 16;
          y = beatsin8((i + 1) * 2, i, MATRIX_HEIGHT - i);
          color = g()->ColorFromCurrentPalette(i * 14);
        }
        else
        {
          x = beatsin8((32 - i) * 2, MATRIX_WIDTH - i, i + 1) + 16;
          y = g()->beatcos8((32 - i) * 2, MATRIX_WIDTH - i, i + 1);
          color = g()->ColorFromCurrentPalette((31 - i) * 14);
        }

        g()->setPixel(x, y, color);
      }
    }
};

class PatternPinwheel : public LEDStripEffect
{
  public:

    PatternPinwheel() : LEDStripEffect(EFFECT_MATRIX_PINWHEEL, "Pinwheel")
    {
    }

    PatternPinwheel(const JsonObjectConst& jsonObject) : LEDStripEffect(jsonObject)
    {
    }

    virtual void Start() override
    {
      g()->Clear();
    }

    virtual size_t DesiredFramesPerSecond() const override
    {
        return 45;
    }

    virtual void Draw() override
    {
      uint8_t dim = beatsin8(2, 30, 70);
      fadeAllChannelsToBlackBy(dim);

      for (uint8_t i = 0; i < 64; i++)
      {
        CRGB color;

        uint8_t x = 0;
        uint8_t y = 0;

        x = beatsin8((64 - i) * 2, MATRIX_HEIGHT - i, i + 1) + 16;
        y = g()->beatcos8((64 - i) * 2, MATRIX_HEIGHT - i, i + 1);
        color = g()->ColorFromCurrentPalette((64 - i) * 14);

        g()->setPixel(x, y, color);
      }
    }
};

class PatternInfinity : public LEDStripEffect
{
public:

    PatternInfinity() : LEDStripEffect(EFFECT_MATRIX_INFINITY, "Infinity")
    {
    }

    PatternInfinity(const JsonObjectConst& jsonObject) : LEDStripEffect(jsonObject)
    {
    }

    virtual size_t DesiredFramesPerSecond() const override
    {
        return 30;
    }

    int _lastX = -1;
    int _lastY = -1;

    virtual void Draw() override
    {
        // dim all pixels on the display slightly
        // to 250/255 (98%) of their current brightness
        g()->DimAll(250);

        // the Effects class has some sample oscillators
        // that move from 0 to 255 at different speeds
        g()->MoveOscillators();

        // the horizontal position of the head of the infinity sign
        // oscillates from 0 to the maximum horizontal and back
        int x = (MATRIX_WIDTH - 1) - g()->p[1];

        // the vertical position of the head oscillates up and down

        const int ymargin = 6;
        int y = map8(sin8(g()->osci[3]), ymargin, MATRIX_HEIGHT - ymargin);

        // the hue oscillates from 0 to 255, overflowing back to 0

        uint8_t hue = sin8(g()->osci[5]);

        // draw a pixel at x,y using a color from the current palette
        if (_lastX == -1)
        {
          _lastX = x;
          _lastY = y;
        }

        g()->drawLine(_lastX, _lastY, x, y, g()->ColorFromCurrentPalette(hue));
        _lastX = x;
        _lastY = y;

        //g()->setPixel(x, y, g()->ColorFromCurrentPalette(hue));

    }
};


class PatternMunch : public LEDStripEffect
{
private:
    uint8_t count = 0;
    uint8_t dir = 1;
    uint8_t flip = 0;
    uint8_t generation = 0;

public:
    PatternMunch() : LEDStripEffect(EFFECT_MATRIX_MUNCH, "Munch")
    {
    }

    PatternMunch(const JsonObjectConst& jsonObject) : LEDStripEffect(jsonObject)
    {
    }

    virtual size_t DesiredFramesPerSecond() const override
    {
        return 16;
    }

    virtual bool RequiresDoubleBuffering() const override
    {
        return false;
    }

    virtual void Draw() override
    {

        for (uint16_t x = 0; x < MATRIX_WIDTH; x++) {
            for (uint16_t y = 0; y < MATRIX_HEIGHT; y++) {
                g()->leds[g()->xy(x, y)] =
                  (x ^ y ^ flip) < count ?
                      g()->ColorFromCurrentPalette(((x ^ y) << 2) + generation)
                    : CRGB::Black;

                // The below is more pleasant
               // effects.leds[XY(x, y)] = effects.ColorFromCurrentPalette(((x ^ y) << 2) + generation) ;
            }
        }

        count += dir;

        if (count <= 0 || count >= MATRIX_WIDTH) {
          dir = -dir;
        }

        if (count <= 0) {
          if (flip == 0)
            flip = MATRIX_WIDTH-1;
          else
            flip = 0;
        }

        generation++;

    }
};

#endif
