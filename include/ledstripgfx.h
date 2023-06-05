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

// LEDStripGFX
// 
// A derivation of GFXBase that adds LED-strip-specific functionality

class LEDStripGFX : public GFXBase 
{
public:

    LEDStripGFX(size_t w, size_t h) : GFXBase(w, h)
    {
        debugV("Creating Device of size %d x %d", w, h);
        leds = static_cast<CRGB *>(calloc(w * h, sizeof(CRGB)));
        if(!leds)
            throw std::runtime_error("Unable to allocate LEDs in LEDStripGFX");
    }

    virtual ~LEDStripGFX()
    {
        free(leds);
        leds = nullptr;
    }
};
