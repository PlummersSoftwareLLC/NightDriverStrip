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

#include "globals.h"
#include "ledstripeffect.h"

uint8_t patternNoiseSmearingHue = 0;


class PatternCurtain : public LEDStripEffect 
{
public:
  PatternCurtain() : LEDStripEffect("Curtain")
  {
  }

  virtual void Draw()
  {
    graphics()->DimAll(235); 
    graphics()->BlurFrame(50);

    // Clear our area potentially drawn by the VU meter last frame; copy Row1 onto Row0 so it usually goes unnoticed

    for (int x = 0; x < MATRIX_WIDTH; x++)
      graphics()->setPixel(x, 0, graphics()->getPixel(x, 1));

    for (uint8_t i = 3; i < MATRIX_WIDTH - 3; i = i + 3) 
    {
      uint16_t color = graphics()->to16bit(graphics()->ColorFromCurrentPalette(i * 4));
      graphics()->drawCircle(i, 2, 1, color);
      graphics()->setPixel(i, 2, color);
    }

     
    // Noise
    graphics()->SetNoise(3000, 3000, 3000, 2000 *(2.0 - gVURatio), 2000 *(2.0 - gVURatio));
    graphics()->FillNoise();

    //graphics()->MoveX(3);
    graphics()->MoveFractionalNoiseY(8);

    graphics()->MoveY(3);
    //graphics()->MoveFractionalNoiseX(8);
  }
};

class PatternGridLights : public LEDStripEffect {
public:
  PatternGridLights() : LEDStripEffect("Grid Lights")
  {
  }

  virtual void Draw()
  {
    graphics()->DimAll(230); 

    // Clear our area potentially drawn by the VU meter last frame; copy Row1 onto Row0 so it usually goes unnoticed

    for (int x = 0; x < MATRIX_WIDTH; x++)
      graphics()->setPixel(x, 0, graphics()->getPixel(x, 1));

    // draw grid of rainbow dots on top of the dimmed image
    for (uint8_t y = 1; y < MATRIX_HEIGHT - 6; y = y + 6) 
    {
      for (uint8_t x = 1; x < MATRIX_WIDTH - 6; x = x + 6) 
      {
        graphics()->leds[graphics()->xy(x, y)] += graphics()->ColorFromCurrentPalette((x * y) / 2);
      }
    }

    // Noise
    graphics()->SetNoise(1000, 1000, 1000, 4000, 4000);
    graphics()->FillNoise();

    graphics()->MoveX(3);
    graphics()->MoveFractionalNoiseX(4);

    graphics()->MoveY(3);
    graphics()->MoveFractionalNoiseY(4);
  }
};

class PatternPaletteSmear : public LEDStripEffect 
{
public:
  PatternPaletteSmear() : LEDStripEffect("PaletteSmear")
  {
  }

  virtual void Draw()
  {
    graphics()->DimAll(10);
   
    // draw a rainbow color palette
    for (uint8_t y = 0; y < MATRIX_HEIGHT; y++) 
    {
      for (uint8_t x = 0; x < MATRIX_CENTER_X; x++) 
      {
        graphics()->leds[graphics()->xy(x, y)] += graphics()->ColorFromCurrentPalette(x * 8, y * 8 + 7);
      }
      for (uint8_t x = 0; x < MATRIX_CENTER_X; x++) 
      {
        graphics()->leds[graphics()->xy(MATRIX_WIDTH - 1 - x, y)] += graphics()->ColorFromCurrentPalette(x * 8, y * 8 + 7);
      }

    }
 
    // Clear our area potentially drawn by the VU meter last frame; copy Row1 onto Row0 so it usually goes unnoticed

    // Noise
graphics()->SetNoise(3000, 3000, 0, 4000, 4000);
 
    graphics()->FillNoise();

    graphics()->MoveX(6);
    graphics()->MoveFractionalNoiseY(4);

    graphics()->MoveY(12);
    graphics()->MoveFractionalNoiseX(16);

    for (int x = 0; x < MATRIX_WIDTH; x++)
      graphics()->setPixel(x, 0, graphics()->getPixel(x, 1));

  }
};

class PatternRainbowFlag : public LEDStripEffect 
{
public:
  PatternRainbowFlag() : LEDStripEffect("RainbowFlag")
  {
  }

  virtual void Draw()
  {
    graphics()->DimAll(10); 

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
        for (uint8_t x = 0; x < MATRIX_WIDTH; x++) 
        {
          graphics()->leds[graphics()->xy(x, y)] += rainbow[c];
        }

        y++;
        if (y >= MATRIX_HEIGHT)
          break;
      }
    }

    // Noise
    graphics()->SetNoise(1000, 1000, 0, 4000, 4000);
    graphics()->FillNoise();

    graphics()->MoveX(8);
    graphics()->MoveFractionalNoiseY(8);

    graphics()->MoveY(3);
    graphics()->MoveFractionalNoiseX(8);


  }
};
#endif
