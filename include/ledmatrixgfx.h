//+--------------------------------------------------------------------------
//
// File:        ledmatrixgfx.h
//
// File:        NTPTimeClient.h
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
//   Provides a Adafruit_GFX implementation for our RGB LED panel so that
//   we can use primitives such as lines and fills on it.
//
// History:     Oct-9-2018         Davepl      Created from other projects
//
//---------------------------------------------------------------------------

#pragma once
#include "gfxbase.h"
#include <SmartMatrix.h>

//
// Matrix Panel
//

#define COLOR_DEPTH   24                  // known working: 24, 48 - If the sketch uses type `rgb24` directly, COLOR_DEPTH must be 24

const uint8_t kMatrixWidth = MATRIX_WIDTH;          // known working: 32, 64, 96, 128
const uint8_t kMatrixHeight = MATRIX_HEIGHT;        // known working: 16, 32, 48, 64
const uint8_t kRefreshDepth = COLOR_DEPTH;                   // known working: 24, 36, 48
const uint8_t kDmaBufferRows = 4;                   // known working: 2-4, use 2 to save memory, more to keep from dropping frames and automatically lowering refresh rate
const uint8_t kPanelType = SMARTMATRIX_HUB75_32ROW_MOD16SCAN;   // use SMARTMATRIX_HUB75_16ROW_MOD8SCAN for common 16x32 panels
const uint8_t kMatrixOptions = (SMARTMATRIX_OPTIONS_BOTTOM_TO_TOP_STACKING);      // see http://docs.pixelmatix.com/SmartMatrix for options
const uint8_t kBackgroundLayerOptions = (SM_BACKGROUND_OPTIONS_NONE);
//const uint8_t kScrollingLayerOptions = (SM_SCROLLING_OPTIONS_NONE);

const uint8_t kDefaultBrightness = (100*255)/100;        // full (100%) brightness
const rgb24   defaultBackgroundColor = {0x40, 0, 0};

class LEDMatrixGFX : public GFXBase
{
private:
  
public:

  LEDMatrixGFX(size_t w, size_t h) : GFXBase(w, h)
  {
  }

  ~LEDMatrixGFX()
  {
  }

  inline uint16_t getPixelIndex(int16_t x, int16_t y) const
  {
    return y * _width + x;
  }

  inline CRGB getPixel(int16_t x) const
  {
    if (x >= 0 && x <= MATRIX_WIDTH * MATRIX_HEIGHT)
      return CRGB::Orange;
    else
    {
      throw std::runtime_error("Invalid index in getPixel: " + x);
      return CRGB::Black;
    }
  }

  inline CRGB getPixel(int16_t x, int16_t y) const
  {
    if (x >= 0 && x <= MATRIX_WIDTH && y >= 0 && y <= MATRIX_HEIGHT)
    {
      return getPixel(getPixelIndex(x, y));
    }
    else
    {
      char szBuffer[80];
      snprintf(szBuffer, 80, "Invalid index in getPixel: x=%d, y=%d, NUM_LEDS=%d", x, y, NUM_LEDS);
      throw std::runtime_error(szBuffer);
    }
  }

  inline virtual void setPixel(int16_t x, int16_t y, uint16_t color)
  {
  }

  inline virtual void setPixel(int16_t x, int16_t y, CRGB color)
  {
  }

  inline virtual void setPixel(int x, CRGB color)
  {
  }

  inline void setPixels(float fPos, float count, CRGB c, bool bMerge = false) const
	{		
  }

};