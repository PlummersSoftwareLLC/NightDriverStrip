//+--------------------------------------------------------------------------
//
// File:        LEDStripGFX.h
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

class LEDStripGFX : public GFXBase 
{
  private:

    CRGB *_pLEDs;
  
public:

    LEDStripGFX(size_t w, size_t h) : GFXBase(w, h)
    {
        _pLEDs = static_cast<CRGB *>(calloc(w * h, sizeof(CRGB)));
        if(!_pLEDs)
        {
            throw std::runtime_error("Unable to allocate LEDs in LEDStripGFX");
        }
    }

    CRGB * GetLEDBuffer() const
    {
        return _pLEDs;
    }


    ~LEDStripGFX()
    {
        free(_pLEDs);
        _pLEDs = nullptr;
    }

    virtual size_t GetLEDCount() const
    {
        return NUM_LEDS;
    }
    
    
    inline uint16_t getPixelIndex(int16_t x, int16_t y) const
    {
        if (x & 0x01)
        {
          // Odd rows run backwards
          uint8_t reverseY = (_height - 1) - y;
          return (x * _height) + reverseY;
        }
        else
        {
          // Even rows run forwards
          return (x * _height) + y;
        }
    }

    inline CRGB getPixel(int16_t x) const
    {
        if (x >= 0 && x <= MATRIX_WIDTH * MATRIX_HEIGHT)
            return _pLEDs[x];
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
            return _pLEDs[getPixelIndex(x, y)];
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
        if (x >= 0 && x <= MATRIX_WIDTH && y >= 0 && y <= MATRIX_HEIGHT)
            _pLEDs[getPixelIndex(x, y)] = from16Bit(color);
    }

    inline virtual void setPixel(int16_t x, int16_t y, CRGB color)
    {
        if (x >= 0 && x <= MATRIX_WIDTH && y >= 0 && y <= MATRIX_HEIGHT)
            _pLEDs[getPixelIndex(x, y)] = color;
    }

    inline virtual void setPixel(int x, CRGB color)
    {
        if (x >= 0 && x <= MATRIX_WIDTH * MATRIX_HEIGHT)
            _pLEDs[x] = color;
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

    inline uint16_t xy( uint8_t x, uint8_t y) const
    {
        if( x >= MATRIX_WIDTH || x < 0) 
          return 0;
        if( y >= MATRIX_HEIGHT || y < 0) 
          return 0;  
        return (y * MATRIX_WIDTH) + x; 
    }
    
};

extern DRAM_ATTR std::shared_ptr<LEDStripGFX> g_pStrands[NUM_CHANNELS];
