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

// HexagonGFX
// 
// A version of the LEDStripGFX class that accounts for the layout of the hexagon LEDs
//

class HexagonGFX : public LEDStripGFX
{
  public:

    HexagonGFX(size_t numLeds) : LEDStripGFX(numLeds, 1)
    {
    }

    virtual int getStartIndexOfRow(int row) const
    {
        if (row < 0) 
        {
            // Invalid row
            return -1;
        }
        else if (row < 10)
        {
            // For the top half, we use the formula for the sum of an arithmetic series
            return (row * (row + 19)) / 2;
        } 
        else if (row < 19)
        {
            // For the bottom half, we adjust our row number to start from 1 again (d = row - 9)
            // Then work our way back up from the end
            int d = 19 - row;
            return 271 - (d * (d + 19)) / 2;
        }
        else 
        {
            // Invalid row
            throw std::runtime_error(str_sprintf("Tried to get index of row %d in the hexagon.", row).c_str());
        }
    }


    // Function to calculate the width of a specified row on an LED display.
    virtual int getRowWidth(int row) const
    {
        if (row < 10) 
        {
            // For the top half, the width simply increases from 10 to 19.
            return 10 + row;
        } 
        else 
        {
            // For the bottom half, the width decreases from 19 back to 10.
            return 28 - row;
        }
    }
    

    // xy
    // 
    // Absent a great way to map x/y coords to the hexagon face, we do it by returning 
    // the Xth pixel in row Y.  It's up to you not to overrun the width of that row, but
    // it will just blend into the next row if you do.

    virtual uint16_t xy(uint16_t x, uint16_t y) const override
    {
        auto start = getStartIndexOfRow(y);
        if (y & 0x01)
        {
            auto width = getRowWidth(y);
            return start + width - x;
        }
        else
        {
            return start + x;
        }
    }
};

