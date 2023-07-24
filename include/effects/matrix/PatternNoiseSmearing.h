//+--------------------------------------------------------------------------
//
// File:        PatternNoiseSmearing.h
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
* Aurora: https://github.com/pixelmatix/aurora
* Copyright (c) 2014 Jason Coon
*
* Portions of this code are adapted from "Noise Smearing" by Stefan Petrick: https://gist.githubusercontent.com/embedded-creations/5cd47d83cb0e04f4574d/raw/ebf6a82b4755d55cfba3bf6598f7b19047f89daf/NoiseSmearing.ino
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

#ifndef PatternNoiseSmearing_H
#define PatternNoiseSmearing_H

class PatternCurtain : public LEDStripEffect
{
public:
  PatternCurtain() : LEDStripEffect(EFFECT_MATRIX_CURTAIN, "Curtain")
  {
  }

  PatternCurtain(const JsonObjectConst& jsonObject) : LEDStripEffect(jsonObject)
  {
  }

  virtual void Draw() override
  {
    g()->DimAll(235);
    g()->BlurFrame(50);

    // Clear our area potentially drawn by the VU meter last frame; copy Row1 onto Row0 so it usually goes unnoticed

    for (int x = 0; x < MATRIX_WIDTH; x++)
      g()->setPixel(x, 0, g()->getPixel(x, 1));

    for (uint16_t i = 3; i < MATRIX_WIDTH - 3; i = i + 3)
    {
      uint16_t color = g()->to16bit(g()->ColorFromCurrentPalette(i * 4));
      g()->drawCircle(i, 2, 1, color);
      g()->setPixel(i, 2, color);
    }


    // Noise
    g()->SetNoise(3000, 3000, 3000, 2000 *(2.0 - g_Analyzer._VURatio), 2000 *(2.0 - g_Analyzer._VURatio));
    g()->FillGetNoise();

    //g()->MoveX(3);
    g()->MoveFractionalNoiseY(8);

    g()->MoveY(3);
    //g()->MoveFractionalNoiseX(8);
  }
};

class PatternGridLights : public LEDStripEffect {
public:
  PatternGridLights() : LEDStripEffect(EFFECT_MATRIX_GRID_LIGHTS, "Grid Dots")
  {
  }

  PatternGridLights(const JsonObjectConst& jsonObject) : LEDStripEffect(jsonObject)
  {
  }

  virtual void Draw() override
  {
    g()->DimAll(230);

    // Clear our area potentially drawn by the VU meter last frame; copy Row1 onto Row0 so it usually goes unnoticed

    for (int x = 0; x < MATRIX_WIDTH; x++)
      g()->setPixel(x, 0, g()->getPixel(x, 1));

    // draw grid of rainbow dots on top of the dimmed image
    for (uint16_t y = 1; y < MATRIX_HEIGHT - 6; y = y + 6)
    {
      for (uint16_t x = 1; x < MATRIX_WIDTH - 6; x = x + 6)
      {
        g()->leds[g()->xy(x, y)] += g()->ColorFromCurrentPalette((x * y) / 2);
      }
    }

    // Noise
    g()->SetNoise(1000, 1000, 1000, 4000, 4000);
    g()->FillGetNoise();

    g()->MoveX(3);
    g()->MoveFractionalNoiseX(4);

    g()->MoveY(3);
    g()->MoveFractionalNoiseY(4);
  }
};

class PatternPaletteSmear : public LEDStripEffect
{
public:
  PatternPaletteSmear() : LEDStripEffect(EFFECT_MATRIX_PALETTE_SMEAR, "Smear")
  {
  }

  PatternPaletteSmear(const JsonObjectConst& jsonObject) : LEDStripEffect(jsonObject)
  {
  }

  virtual void Draw() override
  {
    g()->DimAll(20);

    // draw a rainbow color palette
    for (uint16_t y = 0; y < MATRIX_HEIGHT; y++)
    {
      for (uint16_t x = 0; x < MATRIX_CENTER_X; x++)
      {
        g()->leds[XY(x, y)] += g()->ColorFromCurrentPalette(x * 8, y * 8 + 7);
      }
      for (uint16_t x = 0; x < MATRIX_CENTER_X; x++)
      {
        g()->leds[XY(MATRIX_WIDTH - 1 - x, y)] += g()->ColorFromCurrentPalette(x * 8, y * 8 + 7);
      }

    }

    // Clear our area potentially drawn by the VU meter last frame; copy Row1 onto Row0 so it usually goes unnoticed

    // Noise
    g()->SetNoise(3000, 3000, 0, 4000, 4000);

    g()->FillGetNoise();

    g()->MoveX(6);
    g()->MoveFractionalNoiseY(4);

    g()->MoveY(12);
    g()->MoveFractionalNoiseX(16);

    for (int x = 0; x < MATRIX_WIDTH; x++)
      g()->setPixel(x, 0, g()->getPixel(x, 1));

  }
};

class PatternRainbowFlag : public LEDStripEffect
{
public:
  PatternRainbowFlag() : LEDStripEffect(EFFECT_MATRIX_RAINBOW_FLAG, "RainbowFlag")
  {
  }

  PatternRainbowFlag(const JsonObjectConst& jsonObject) : LEDStripEffect(jsonObject)
  {
  }

  virtual void Draw() override
  {
    g()->DimAll(10);

    CRGB rainbow[7] = {
      CRGB::Red,
      CRGB::Orange,
      CRGB::Yellow,
      CRGB::Green,
      CRGB::Blue,
      CRGB::Violet
    };

    uint8_t y = 2;

    for (uint8_t c = 0; c < 6; c++) {
      for (uint8_t j = 0; j < 5; j++) {
        for (uint16_t x = 0; x < MATRIX_WIDTH; x++)
        {
          g()->leds[g()->xy(x, y)] += rainbow[c];
        }

        y++;
        if (y >= MATRIX_HEIGHT)
          break;
      }
    }

    // Noise
    g()->SetNoise(1000, 1000, 0, 4000, 4000);
    g()->FillGetNoise();

    g()->MoveX(8);
    g()->MoveFractionalNoiseY(8);

    g()->MoveY(3);
    g()->MoveFractionalNoiseX(8);


  }
};
#endif
