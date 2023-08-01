//+--------------------------------------------------------------------------
//
// File:        PatternSpiro.h
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
// History:     Jun-25-2022        Davepl      Based on Aurora
//
//---------------------------------------------------------------------------

/*
 * Aurora: https://github.com/pixelmatix/aurora
 * Copyright (c) 2014 Jason Coon
 *
 * Portions of this code are adapted from Andrew: http://pastebin.com/f22bfe94d
 * which, in turn, was "Adapted from the Life example on the Processing.org site"
 *
 * Made much more colorful by J.B. Langston:
 *  https://github.com/jblang/aurora/commit/6db5a884e3df5d686445c4f6b669f1668841929b
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

#ifndef PatternSpiro_H
#define PatternSpiro_H

class PatternSpiro : public LEDStripEffect
{
private:
  uint8_t theta1 = 0;
  uint8_t theta2 = 0;
  uint8_t hueoffset = 0;

  uint8_t radiusx = MATRIX_WIDTH / 4;
  uint8_t radiusy = MATRIX_HEIGHT / 4;
  uint8_t minx = MATRIX_CENTER_X - radiusx;
  uint8_t maxx = MATRIX_CENTER_X + radiusx - 1;
  uint8_t miny = MATRIX_CENTER_Y - radiusy;
  uint8_t maxy = MATRIX_CENTER_Y + radiusy - 1;

  uint8_t spirocount = 1;
  uint8_t spirooffset = 256 / spirocount;
  boolean spiroincrement = false;

  boolean handledChange = false;

public:
  PatternSpiro() : LEDStripEffect(EFFECT_MATRIX_SPIRO, "Spiro")
  {
  }

  PatternSpiro(const JsonObjectConst& jsonObject) : LEDStripEffect(jsonObject)
  {
  }

  virtual size_t DesiredFramesPerSecond() const override
  {
      return 120;
  }
  void Draw() override
  {
    auto graphics = g();
    graphics->DimAll(253);

    // effects.ShowFrame();

    boolean change = false;

    for (int i = 0; i < spirocount; i++)
    {
      uint8_t x = graphics->mapsin8(theta1 + i * spirooffset, minx, maxx);
      uint8_t y = graphics->mapcos8(theta1 + i * spirooffset, miny, maxy);

      uint8_t x2 = graphics->mapsin8(theta2 + i * spirooffset, x - radiusx, x + radiusx);
      uint8_t y2 = graphics->mapcos8(theta2 + i * spirooffset, y - radiusy, y + radiusy);

      CRGB color = graphics->ColorFromCurrentPalette(hueoffset + i * spirooffset, 128);
      graphics->leds[graphics->xy(x2, y2)] += color;

      if (x2 == MATRIX_CENTER_X && y2 == MATRIX_CENTER_Y)
        change = true;
    }

    theta2 += 1;

    EVERY_N_MILLIS(25)
    {
      theta1 += 1;
    }

    EVERY_N_MILLIS(100)
    {
      if (change && !handledChange)
      {
        handledChange = true;

        if (spirocount >= MATRIX_WIDTH || spirocount == 1)
          spiroincrement = !spiroincrement;

        if (spiroincrement)
        {
          if (spirocount >= 4)
            spirocount *= 2;
          else
            spirocount += 1;
        }
        else
        {
          if (spirocount > 4)
            spirocount /= 2;
          else
            spirocount -= 1;
        }

        spirooffset = 256 / spirocount;
      }

      if (!change)
        handledChange = false;
    }

    EVERY_N_MILLIS(33)
    {
      hueoffset += 1;
    }
  }
};

#endif
