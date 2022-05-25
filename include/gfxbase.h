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
#include <stdexcept>

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

    CRGB * _pLEDs;

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

    inline virtual uint16_t xy( uint8_t x, uint8_t y) const
    {
        if( x >= MATRIX_WIDTH || x < 0) 
          return 0;
        if( y >= MATRIX_HEIGHT || y < 0) 
          return 0;  
        return (y * MATRIX_WIDTH) + x; 
    }
    
    virtual void Clear()
    {
      memset(_pLEDs, 0, sizeof(CRGB) * _width * _height);
    }
    
    virtual uint16_t getPixelIndex(int16_t x, int16_t y)
    {
      return y * _width + x;
    }

    virtual CRGB getPixel(int16_t x) const 
    {
      return _pLEDs[x];
    }

    inline virtual CRGB getPixel(int16_t x, int16_t y)
    {
        return getPixel(getPixelIndex(x,y));
    }
    
    inline virtual void drawPixel(int16_t x, int16_t y, uint16_t color)
    {
      setPixel(x, y, color);
    }

    inline virtual void scrollLeft()
    {
      for (int i = 0; i < NUM_LEDS - 1; i++)
      {
        setPixel(i, getPixel(i+1));
      }
    }

    inline virtual void fillLeds(const CRGB * pLEDs)
    {
      memcpy(_pLEDs, pLEDs, sizeof(CRGB) * _width * _height);
    }

    virtual void setPixel(int16_t x, int16_t y, uint16_t color)
    {
        if (x >= 0 && x <= MATRIX_WIDTH && y >= 0 && y <= MATRIX_HEIGHT)
            _pLEDs[getPixelIndex(x, y)] = from16Bit(color);
    }

    inline virtual void setPixel(int16_t x, int16_t y, CRGB color)
    {
        if (x >= 0 && x <= MATRIX_WIDTH && y >= 0 && y <= MATRIX_HEIGHT)
            _pLEDs[getPixelIndex(x, y)] = color;
    }

    inline virtual void setPixel(int16_t x, int r, int g, int b)
    {
        setPixel(x, CRGB(r,g,b));
    }

    inline virtual void setPixel(int x, CRGB color)
    {
        if (x >= 0 && x <= MATRIX_WIDTH * MATRIX_HEIGHT)
            _pLEDs[x] = color;
    }

    inline virtual CRGB getPixel(int x)
    {
        if (x >= 0 && x <= MATRIX_WIDTH * MATRIX_HEIGHT)
            return _pLEDs[x];
        throw std::runtime_error("Pixel out of bounds in getPixel");
    }

    inline void setPixels(float fPos, float count, CRGB c, bool bMerge = false) const
    {		
        float frac1 = fPos - floor(fPos);							// eg:   3.25 becomes 0.25
        float frac2 = fPos + count - floor(fPos + count);			// eg:   3.25 + 1.5 yields 4.75 which becomes 0.75

      /* Example:
      
        Starting at 3.25, draw for 1.5:
        We start at pixel 3.
        We fill pixel with .75 worth of color
        We advance to next pixel
        
        We fill one pixel and advance to next pixel

        We are now at pixel 5, frac2 = .75
        We fill pixel with .75 worth of color
      */

      uint8_t fade1 = (std::max(frac1, 1.0f-count)) * 255;					// Fraction is how far past pixel boundary we are (up to our total size) so larger fraction is more dimming
      uint8_t fade2 = (1.0 - frac2) * 255;		// Fraction is how far we are poking into this pixel, so larger fraction is less dimming
      CRGB c1 = c;
      CRGB c2 = c;
      c1 = c1.fadeToBlackBy(fade1);
      c2 = c2.fadeToBlackBy(fade2);

      float p = fPos;
      if (p >= 0 && p < GetLEDCount())
        for (int i = 0; i < NUM_CHANNELS; i++)
            _pLEDs[(int)p] = bMerge ? _pLEDs[(int)p]  + c1 : c1;  
      p=fPos+(1.0 - frac1);
      count -= (1.0 - frac1);

      // Middle (body) pixels

      while (count >= 1)
      {
        if (p >= 0 && p < GetLEDCount())
            for (int i = 0; i < NUM_CHANNELS; i++)
              _pLEDs[(int)p] = bMerge ? _pLEDs[(int)p]  + c : c;  
        count--;
        p++;
      };

      // Final pixel, if in bounds
      if (count > 0)
        if (p >= 0 && p < GetLEDCount())
          for (int i = 0; i < NUM_CHANNELS; i++)
              _pLEDs[(int)p] = bMerge ? _pLEDs[(int)p]  + c2 : c2;  
    }
    
    
};
