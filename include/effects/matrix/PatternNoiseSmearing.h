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

class PatternMultipleStream : public LEDStripEffect 
{
public:
  PatternMultipleStream() : LEDStripEffect("MultiStream")
  {
  }

  virtual void Start()
  {
    graphics()->Clear();
  }

  // this pattern draws two points to the screen based on sin/cos if a counter
  // (comment out NoiseSmearWithRadius to see pattern of pixels)
  // these pixels are smeared by a large radius, giving a lot of movement
  // the image is dimmed before each drawing to not saturate the screen with color
  // the smear has an offset so the pixels usually have a trail leading toward the upper left
  virtual void Draw()
  {
    static unsigned long counter = 0;

#if 0
    // this counter lets put delays between each frame and still get the same animation
    counter++;
#else
    // this counter updates in real time and can't be slowed down for debugging
    counter = millis() / 10;
#endif

    uint8_t x1 = 4 + sin8(counter * 2) / 10;
    uint8_t x2 = 8 + sin8(counter * 2) / 16;
    uint8_t y2 = 8 + cos8((counter * 2) / 3) / 16;

    graphics()->leds[graphics()->xy(x1, x2)] = graphics()->ColorFromCurrentPalette(patternNoiseSmearingHue);
    graphics()->leds[graphics()->xy(x2, y2)] = graphics()->ColorFromCurrentPalette(patternNoiseSmearingHue + 128);

    // Noise
    graphics()->noise_x += 1000;
    graphics()->noise_y += 1000;
    graphics()->noise_scale_x = 4000;
    graphics()->noise_scale_y = 4000;

    graphics()->FillNoise();

    graphics()->MoveX(8);
    graphics()->MoveFractionalNoiseX(4);

    graphics()->MoveY(8);
    graphics()->MoveFractionalNoiseY(4);

    patternNoiseSmearingHue++;
  }
};

class PatternMultipleStream2 : public LEDStripEffect 
{
public:
  PatternMultipleStream2() : LEDStripEffect("MultipleStream2")
  {
  }

  virtual void Draw()
  {
    graphics()->DimAll(230);

    uint8_t xx = 4 + sin8(millis() / 9) / 10;
    uint8_t yy = 4 + cos8(millis() / 10) / 10;
    graphics()->leds[graphics()->xy(xx, yy)] += graphics()->ColorFromCurrentPalette(patternNoiseSmearingHue);

    xx = 8 + sin8(millis() / 10) / 16;
    yy = 8 + cos8(millis() / 7) / 16;
    graphics()->leds[graphics()->xy(xx, yy)] += graphics()->ColorFromCurrentPalette(patternNoiseSmearingHue + 80);

    graphics()->leds[graphics()->xy(15, 15)] += graphics()->ColorFromCurrentPalette(patternNoiseSmearingHue + 160);

    graphics()->noise_x += 1000;
    graphics()->noise_y += 1000;
    graphics()->noise_z += 1000;
    graphics()->noise_scale_x = 4000;
    graphics()->noise_scale_y = 4000;
    graphics()->FillNoise();

    graphics()->MoveX(3);
    graphics()->MoveFractionalNoiseY();

    graphics()->MoveY(3);
    graphics()->MoveFractionalNoiseX();

    patternNoiseSmearingHue++;
  }
};

class PatternMultipleStream3 : public LEDStripEffect 
{
public:
  PatternMultipleStream3() : LEDStripEffect("MultipleStream3")
  {
  }

  virtual void Draw()
  {
    //CLS();
    graphics()->DimAll(235); 

    for (uint8_t i = 3; i < 32; i = i + 4) {
      graphics()->leds[graphics()->xy(i, 15)] += graphics()->ColorFromCurrentPalette(i * 8);
    }

    // Noise
    graphics()->noise_x += 1000;
    graphics()->noise_y += 1000;
    graphics()->noise_z += 1000;
    graphics()->noise_scale_x = 4000;
    graphics()->noise_scale_y = 4000;
    graphics()->FillNoise();

    graphics()->MoveX(3);
    graphics()->MoveFractionalNoiseY(4);

    graphics()->MoveY(3);
    graphics()->MoveFractionalNoiseX(4);
  }
};

class PatternMultipleStream4 : public LEDStripEffect 
{
public:
  PatternMultipleStream4() : LEDStripEffect("MultipleStream4")
  {
  }

  virtual void Draw()
  {
    //CLS();
    graphics()->DimAll(235); 

    graphics()->leds[graphics()->xy(15, 15)] += graphics()->ColorFromCurrentPalette(patternNoiseSmearingHue);


    // Noise
    graphics()->noise_x += 1000;
    graphics()->noise_y += 1000;
    graphics()->noise_scale_x = 4000;
    graphics()->noise_scale_y = 4000;
    graphics()->FillNoise();

    graphics()->MoveX(8);
    graphics()->MoveFractionalNoiseX();

    graphics()->MoveY(8);
    graphics()->MoveFractionalNoiseY();

    patternNoiseSmearingHue++;
  }
};

class PatternMultipleStream5 : public LEDStripEffect 
{
public:
  PatternMultipleStream5() : LEDStripEffect("MultipleStream5")
  {
  }

  virtual void Draw()
  {

    //CLS();
    graphics()->DimAll(235);


    for (uint8_t i = 3; i < 32; i = i + 4) {
      graphics()->leds[graphics()->xy(i, 31)] += graphics()->ColorFromCurrentPalette(i * 8);
    }

    // Noise
    graphics()->noise_x += 1000;
    graphics()->noise_y += 1000;
    graphics()->noise_z += 1000;
    graphics()->noise_scale_x = 4000;
    graphics()->noise_scale_y = 4000;
    graphics()->FillNoise();

    graphics()->MoveX(3);
    graphics()->MoveFractionalNoiseY(4);

    graphics()->MoveY(4);
    graphics()->MoveFractionalNoiseX(4);
  }
};

class PatternMultipleStream8 : public LEDStripEffect {
public:
  PatternMultipleStream8() : LEDStripEffect("MultipleStream8")
  {
  }

  virtual void Draw()
  {
    graphics()->DimAll(230); 

    // draw grid of rainbow dots on top of the dimmed image
    for (uint8_t y = 1; y < 32; y = y + 6) {
      for (uint8_t x = 1; x < 32; x = x + 6) {

        graphics()->leds[graphics()->xy(x, y)] += graphics()->ColorFromCurrentPalette((x * y) / 4);
      }
    }

    // Noise
    graphics()->noise_x += 1000;
    graphics()->noise_y += 1000;
    graphics()->noise_z += 1000;
    graphics()->noise_scale_x = 4000;
    graphics()->noise_scale_y = 4000;
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
    graphics()->DimAll(170);
   
    // draw a rainbow color palette
    for (uint8_t y = 1; y < MATRIX_HEIGHT; y++) {
      for (uint8_t x = 0; x < MATRIX_WIDTH; x++) {
        graphics()->leds[graphics()->xy(x, y)] += graphics()->ColorFromCurrentPalette(x * 8, y * 8 + 7);
      }
    }
 
  
    // Noise
    graphics()->noise_x += 1000;
    graphics()->noise_y += 1000;
    graphics()->noise_scale_x = 4000;
    graphics()->noise_scale_y = 4000;
 
    graphics()->FillNoise();

    graphics()->MoveX(3);
    //graphics()->MoveFractionalNoiseY(4);

    graphics()->MoveY(3);
    graphics()->MoveFractionalNoiseX(4);
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
    graphics()->noise_x += 1000;
    graphics()->noise_y += 1000;
    graphics()->noise_scale_x = 4000;
    graphics()->noise_scale_y = 4000;
    graphics()->FillNoise();

    graphics()->MoveX(8);
    graphics()->MoveFractionalNoiseY(8);

    graphics()->MoveY(3);
    graphics()->MoveFractionalNoiseX(8);


  }
};
#endif