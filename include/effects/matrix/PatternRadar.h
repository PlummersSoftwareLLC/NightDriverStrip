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
// History:     Jun-25-2022         Davepl      Based on Aurora
//
//---------------------------------------------------------------------------

/*
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

#ifndef PatternRadar_H
#define PatternRadar_H

class PatternRadar : public LEDStripEffect
{
private:
  uint8_t theta = 0;
  uint8_t hueoffset = 0;

public:
  PatternRadar() : LEDStripEffect(EFFECT_MATRIX_RADAR, "Radar")
  {
  }

  PatternRadar(const JsonObjectConst& jsonObject) : LEDStripEffect(jsonObject)
  {
  }

  void Draw() override
  {
    auto graphics = (GFXBase *)_GFX[0].get();
    graphics->DimAll(254);

    for (int offset = 0; offset < MATRIX_CENTER_X; offset++)
    {
      uint8_t hue = 255 - (offset * 16 + hueoffset);
      CRGB color = graphics->ColorFromCurrentPalette(hue);
      uint8_t x = graphics->mapcos8(theta, offset, (MATRIX_WIDTH - 1) - offset);
      uint8_t y = graphics->mapsin8(theta, offset, (MATRIX_HEIGHT - 1) - offset);
      uint16_t xzy = graphics->xy(x, y);
      graphics->leds[xzy] = color;

      EVERY_N_MILLIS(25)
      {
        theta += 2;
        hueoffset += 1;
      }
    }
  }
};

#endif
