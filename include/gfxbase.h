//+--------------------------------------------------------------------------
//
// File:        gfxbase.h
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
#define fastled_internal 1
#include "globals.h"
#include "FastLED.h"
#include "Adafruit_GFX.h"

// 5:6:5 Color definitions
#define BLACK16 0x0000
#define BLUE16 0x001F
#define RED16 0xF800
#define GREEN16 0x07E0
#define CYAN16 0x07FF
#define MAGENTA16 0xF81F
#define YELLOW16 0xFFE0
#define WHITE16 0xFFFF

#include "screen.h"

class GFXBase : public Adafruit_GFX
{
  protected:

    size_t _width;
    size_t _height;

    static const uint8_t gamma5[];
    static const uint8_t gamma6[];

  public:
        
    GFXBase(int w, int h) : Adafruit_GFX(w, h),         
                            _width(w),
                            _height(h)
    {
    }
    
    inline static CRGB from16Bit(uint16_t color) // Convert 16bit 5:6:5 to 24bit color using lookup table for gamma
    {
      uint8_t r = gamma5[color >> 11];
      uint8_t g = gamma6[(color >> 5) & 0x3F];
      uint8_t b = gamma5[color & 0x1F];

      return CRGB(r, g, b);
    }

    static inline uint16_t to16bit(uint8_t r, uint8_t g, uint8_t b) // Convert RGB -> 16bit 5:6:5
    {
      return ((r / 8) << 11) | ((g / 4) << 5) | (b / 8);
    }

    static inline uint16_t to16bit(const CRGB rgb) // Convert CRGB -> 16 bit 5:6:5
    {
      return ((rgb.r / 8) << 11) | ((rgb.g / 4) << 5) | (rgb.b / 8);
    }

    static inline uint16_t to16bit(CRGB::HTMLColorCode code) // Convert HtmlColorCode -> 16 bit 5:6:5
    {
      return to16bit(CRGB(code));
    }

    virtual size_t GetLEDCount() const
    {
      return _width * _height;
    }

    virtual uint16_t xy( uint8_t x, uint8_t y) const = 0;
    virtual uint16_t getPixelIndex(int16_t x, int16_t y) const = 0;
    virtual CRGB getPixel(int16_t x) const = 0;
    virtual void setPixel(int16_t x, int16_t y, uint16_t color) = 0;
    virtual void setPixel(int16_t x, int16_t y, CRGB color) = 0;
    virtual void setPixel(int x, CRGB color) = 0;
    virtual void setPixels(float fPos, float count, CRGB c, bool bMerge = false) const = 0;

    inline virtual CRGB getPixel(int16_t x, int16_t y)
    {
        return getPixel(getPixelIndex(x,y));
    }
    
    inline virtual void drawPixel(int16_t x, int16_t y, uint16_t color)
    {
      setPixel(x, y, color);
    }

    inline virtual void ScrollLeft()
    {
      for (int i = 0; i < NUM_LEDS - 1; i++)
      {
        setPixel(i, getPixel(i+1));
      }
    }

};
