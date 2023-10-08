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
//   Provides an Adafruit_GFX implementation for our RGB LED panel so that
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
protected:
    static void AddLEDsToFastLED(std::vector<std::shared_ptr<GFXBase>>& devices)
    {
        // Macro to add LEDs to a channel

        #define ADD_CHANNEL(channel) \
            debugI("Adding %zu LEDs to pin %d from channel %d on FastLED.", devices[channel]->GetLEDCount(), LED_PIN ## channel, channel); \
            FastLED.addLeds<WS2812B, LED_PIN ## channel, COLOR_ORDER>(devices[channel]->leds, devices[channel]->GetLEDCount()); \
            pinMode(LED_PIN ## channel, OUTPUT)

        debugI("Adding LEDs to FastLED...");

        // The following "unrolled conditional compile loop" to set up the channels is needed because the LED pin
        //   is a template parameter to FastLED.addLeds()

        #if NUM_CHANNELS >= 1
            ADD_CHANNEL(0);
        #endif

        #if NUM_CHANNELS >= 2
            ADD_CHANNEL(1);
        #endif

        #if NUM_CHANNELS >= 3
            ADD_CHANNEL(2);
        #endif

        #if NUM_CHANNELS >= 4
            ADD_CHANNEL(3);
        #endif

        #if NUM_CHANNELS >= 5
            ADD_CHANNEL(4);
        #endif

        #if NUM_CHANNELS >= 6
            ADD_CHANNEL(5);
        #endif

        #if NUM_CHANNELS >= 7
            ADD_CHANNEL(6);
        #endif

        #if NUM_CHANNELS >= 8
            ADD_CHANNEL(7);
        #endif

        #ifdef POWER_LIMIT_MW
            set_max_power_in_milliwatts(POWER_LIMIT_MW);                // Set brightness limit
        #endif
    }

public:

    LEDStripGFX(size_t w, size_t h) : GFXBase(w, h)
    {
        debugV("Creating Device of size %zu x %zu", w, h);
        leds = static_cast<CRGB *>(calloc(w * h, sizeof(CRGB)));
        if(!leds)
            throw std::runtime_error("Unable to allocate LEDs in LEDStripGFX");
    }

    ~LEDStripGFX() override
    {
        free(leds);
        leds = nullptr;
    }

    static void InitializeHardware(std::vector<std::shared_ptr<GFXBase>>& devices)
    {
        // We don't support more than 8 parallel channels
        #if NUM_CHANNELS > 8
            #error The maximum value of NUM_CHANNELS (number of parallel channels) is 8
        #endif

        for (int i = 0; i < NUM_CHANNELS; i++)
        {
            debugW("Allocating LEDStripGFX for channel %d", i);
            devices.push_back(make_shared_psram<LEDStripGFX>(MATRIX_WIDTH, MATRIX_HEIGHT));
        }

        AddLEDsToFastLED(devices);

    }

    // PostProcessFrame
    //
    // PostProcessFrame sends the data to the LED strip.  If it's fewer than the size of the strip, we only send that many.

    void PostProcessFrame(uint16_t wifiPixelsDrawn, uint16_t localPixelsDrawn) override;
};

#if HEXAGON

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

    static void InitializeHardware(std::vector<std::shared_ptr<GFXBase>>& devices)
    {
        // We don't support more than 8 parallel channels
        #if NUM_CHANNELS > 8
            #error The maximum value of NUM_CHANNELS (number of parallel channels) is 8
        #endif

        for (int i = 0; i < NUM_CHANNELS; i++)
        {
            debugW("Allocating HexagonGFX for channel %d", i);
            devices.push_back(make_shared_psram<HexagonGFX>(NUM_LEDS));
        }

        AddLEDsToFastLED(devices);
    }

    virtual int getStartIndexOfRow(int row) const
    {
        if (row < 0)
        {
            // Invalid row
            return -1;
        }
        else if (row < HEX_HALF_DIMENSION)
        {
            // For the top half, we use the formula for the sum of an arithmetic series.  It's ok to stare.
            return (row * (row + HEX_MAX_DIMENSION)) / 2;
        }
        else if (row < HEX_MAX_DIMENSION)
        {
            // For the bottom half, we adjust our row number to start from 1 again (d = row - 9)
            // Then work our way back up from the end.  ChatGPT couldn't solve the series so I decided to
            // just work backwards from the end using the same formula as the top half, and it works.
            int d = HEX_MAX_DIMENSION - row;
            return NUM_LEDS - (d * (d + HEX_MAX_DIMENSION)) / 2;
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
        if (row < HEX_HALF_DIMENSION)
        {
            // For the top half, the width simply increases from 10 to 19.
            return HEX_HALF_DIMENSION + row;
        }
        else
        {
            // For the bottom half, the width decreases from 19 back to 10.
            return HEX_MAX_DIMENSION + HEX_HALF_DIMENSION - 1 - row;
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

    // filHexRing
    //
    // Fills a ring around the hexagon, inset by the indent specified and in the color provided

    virtual void fillHexRing(uint16_t indent, CRGB color)
    {
        for (int i = indent; i < getRowWidth(indent)-indent; ++i)
            setPixel(getStartIndexOfRow(indent) + i, color);

        // Iterate over all rows
        for (int row = indent; row < HEX_MAX_DIMENSION - indent; ++row)
        {
            // Get the start index of the current row
            int startIndex = getStartIndexOfRow(row);

            setPixel(startIndex + indent, color);                           // first pixel
            setPixel(startIndex + getRowWidth(row) - 1 - indent, color);    // last pixel
        }

        for (int i = indent; i < getRowWidth(HEX_MAX_DIMENSION-indent-1)-indent; ++i)
            setPixel(getStartIndexOfRow(HEX_MAX_DIMENSION-indent-1) + i, color);

    }
};

#endif
