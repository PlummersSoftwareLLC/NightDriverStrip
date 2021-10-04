//+--------------------------------------------------------------------------
//
// File:        PaletteEffect.h
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
// Description:
//
//    Fills spokes from a predefined palette
//
// History:     Apr-13-2019         Davepl      Adapted from LEDWifiSocket
//              
//---------------------------------------------------------------------------

#pragma once

#include <sys/types.h>
#include <errno.h>
#include <iostream>
#include <vector>
#include <math.h>
#define  FASTLED_INTERNAL 1           // Suppress the FASTLED compilation banner
#include "FastLED.h"                 
#include "colorutils.h"
#include "globals.h"
#include "ledstripeffect.h"
#include "colordata.h"


extern AppTime g_AppTime;


class PaletteEffect : public LEDStripEffect
{
  private:

    float _startIndex;
    float _paletteIndex;
    const CRGBPalette256 _palette;
    const float _density;
    const float _paletteSpeed;
    const float  _lightSize;
    const float _gapSize;
    const float _LEDSPerSecond;
    const TBlendType  _blend;
    const bool  _bErase;
    const float _brightness;

  public:

    PaletteEffect(const CRGBPalette256 & palette, 
                  float density = 4.0,                
                  float paletteSpeed = 0.25, 
                  float ledsPerSecond = 0.1, 
                  float lightSize = 1, 
                  float gapSize = 0,
                  TBlendType blend = LINEARBLEND, 
                  bool  bErase = true,
                  float brightness = 1.0)
      : LEDStripEffect("Palette Effect"),
        _startIndex(0.0f),
        _paletteIndex(0.0f),
        _palette(palette),
        _density(density),
        _paletteSpeed(paletteSpeed),
        _lightSize(lightSize),
        _gapSize(gapSize),
        _LEDSPerSecond(ledsPerSecond),
        _blend(blend),
        _bErase(bErase),
        _brightness(brightness)
    {
        _paletteIndex = 0.0f;
    }

    ~PaletteEffect()
    {
    }
	
	  virtual void Draw() 
    {
        if (_bErase)
          setAllOnAllChannels(0,0,0);

        float deltaTime = g_AppTime.DeltaTime();
        float increment = (deltaTime * _LEDSPerSecond);      
        const int totalSize = _gapSize + _lightSize + 1;
        _startIndex   = totalSize > 1 ? fmodf(_startIndex + increment, totalSize) : 0;
        
        // A single color step in a palette is 32 increments.  There are 256 total in a palette, and 144 pixels per meter typical, so this
        // scaling yields a color rotation of "one full palette per meter" by default.  We go backwards (-1) to match pixel scrolling direction.

        _paletteIndex = _paletteIndex - deltaTime * _paletteSpeed * 32 * _density * 256/144.0;    

        float iColor = fmodf(_paletteIndex + _startIndex * _density, 256);

        if (_gapSize == 0)
        {
          for (int i = 0; i < _cLEDs; i+=_lightSize)
          {
            iColor = fmodf(iColor + _density, 256);
            setPixels(i, _lightSize, ColorFromPalette(_palette, iColor, 255 * _brightness, _blend), false);
          }
        }
        else
        {
          // Start far enough "back" to have one off-strip light and gap, and then we need to draw at least as far as the last light.
          // This prevents sticks of light from "appearing" or "disappearing" at the ends

          for (float i = 0-totalSize; i < _cLEDs+_lightSize; i++)
          {
              // We look for each pixel where we cross an even multiple of the light+gap size, which means it's time to start the drawing
              // of the light here

              iColor = fmodf(iColor + _density, 256);
              int index = fmodf(i, totalSize);
              if (index == 0)
              {
                  CRGB c = ColorFromPalette(_palette, iColor, 255 * _brightness, _blend);
                  setPixels(i+_startIndex, _lightSize, c,false);
              }
          }
        }
    }

    virtual const char * FriendlyName() const
    {
        return "Palette Effect";
    }    
};
