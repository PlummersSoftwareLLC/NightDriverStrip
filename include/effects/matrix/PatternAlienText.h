//+--------------------------------------------------------------------------
//
// File:        PatternLife.h
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
//   and
//
// History:     Jun-25-2022         Davepl      Based on Aurora
//
//---------------------------------------------------------------------------

/*
 * Aurora: https://github.com/pixelmatix/aurora
 * Copyright (c) 2014 Jason Coon
 *
 * Inspired by 'Space Invader Generator': https://the8bitpimp.wordpress.com/2013/05/07/space-invader-generator
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

#ifndef PatternAlienText_H
#define PatternAlienText_H

// Description: This file contains the implementation of the PatternAlienText class, 
//              which is a subclass of LEDStripEffect. The class is designed to create 
//              an effect that simulates alien text on an LED matrix. 
// 
//              The PatternAlienText class utilizes two main properties, charWidth and charHeight, 
//              to define the dimensions of each 'character' in the alien text. It also uses leftMargin 
//              and topMargin to define the starting position of the text on the LED matrix.
// 
//              The Draw() method is the core function where the alien text is generated. It uses 
//              randomization to light up LEDs in a way that mimics the appearance of a foreign script. 
//              The color of the text is randomly chosen, and the arrangement of lit LEDs creates 
//              the illusion of alien characters. This process repeats, moving across and then down 
//              the matrix, simulating scrolling text.

class PatternAlienText : public LEDStripEffect
{
private:
  const int charWidth = 6;
  const int charHeight = 6;
  const int leftMargin = 2;
  const int topMargin = 2;
  uint8_t x;
  uint8_t y;

public:

  PatternAlienText() : LEDStripEffect(EFFECT_MATRIX_ALIEN_TEXT, "AlienText")
  {
  }

  PatternAlienText(const JsonObjectConst& jsonObject) : LEDStripEffect(jsonObject)
  {
  }

  void Start() override
  {
      x = leftMargin;
      y = topMargin;
      g()->Clear();
      debugW("Starting AlienText...");
  }

  void Draw() override
  {
    std::shared_ptr<GFXBase> graphics = _GFX[0];

    graphics->DimAll(245);

    CRGB color1 = RandomSaturatedColor();

    for (int i = 0; i < (charWidth / 2 + 1); i++)
    {
      for (int j = 0; j < (charHeight - 1); j++)
      {
        CRGB color = CRGB::Black;

        if (random(0, 2) == 1)
          color = color1;

        graphics->setPixel(x + i, y + j, color);

        if (i < 2)
          graphics->setPixel(x + ((charWidth / 2 + 1) - i), y + j, color);
      }
    }

    x += charWidth;
    if (x > MATRIX_WIDTH - charWidth)
    {
      x = leftMargin;
      y += charHeight;
    }

    if (y > MATRIX_HEIGHT - charHeight)
    {
      x = leftMargin;
      y = topMargin;
    }
  }
};

#endif
