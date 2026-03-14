//+--------------------------------------------------------------------------
//
// File:        ws281xgfx.cpp
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
//    Code for handling LED strips with the FastLED library
//
// History:     Jul-22-2023         Rbergen      Created
//
//---------------------------------------------------------------------------

#include "globals.h"

#include "deviceconfig.h"
#include "effectmanager.h"
#include "systemcontainer.h"
#include "values.h"
#include "ws281xgfx.h"

void WS281xGFX::AddLEDs(std::vector<std::shared_ptr<GFXBase>>& devices)
{
    // Macro to add LEDs to a channel

    #if FASTLED_EXPERIMENTAL_ESP32_RGBW_ENABLED
        #define ADD_CHANNEL(channel) \
            debugI("Adding %zu LEDs to pin %d from channel %d on FastLED.", devices[channel]->GetLEDCount(), LED_PIN ## channel, channel); \
            FastLED.addLeds<WS2812, LED_PIN ## channel, COLOR_ORDER>(devices[channel]->leds, devices[channel]->GetLEDCount()).setRgbw(Rgbw(kRGBWDefaultColorTemp, FASTLED_EXPERIMENTAL_ESP32_RGBW_MODE )); \
            pinMode(LED_PIN ## channel, OUTPUT)
    #else
        #define ADD_CHANNEL(channel) \
            debugI("Adding %zu LEDs to pin %d from channel %d on FastLED.", devices[channel]->GetLEDCount(), LED_PIN ## channel, channel); \
            FastLED.addLeds<WS2812B, LED_PIN ## channel, COLOR_ORDER>(devices[channel]->leds, devices[channel]->GetLEDCount()); \
            pinMode(LED_PIN ## channel, OUTPUT)
    #endif

    debugI("Adding LEDs to FastLED...");

    // The following "unrolled conditional compile loop" to set up the channels is needed because the LED pin
    //   is a template parameter to FastLED.addLeds()

    #if NUM_CHANNELS >= 1 && LED_PIN0 >= 0
        ADD_CHANNEL(0);
    #endif

    #if NUM_CHANNELS >= 2 && LED_PIN1 >= 0
        ADD_CHANNEL(1);
    #endif

    #if NUM_CHANNELS >= 3 && LED_PIN2 >= 0
        ADD_CHANNEL(2);
    #endif

    #if NUM_CHANNELS >= 4 && LED_PIN3 >= 0
        ADD_CHANNEL(3);
    #endif

    #if NUM_CHANNELS >= 5 && LED_PIN4 >= 0
        ADD_CHANNEL(4);
    #endif

    #if NUM_CHANNELS >= 6 && LED_PIN5 >= 0
        ADD_CHANNEL(5);
    #endif

    #if NUM_CHANNELS >= 7 && LED_PIN6 >= 0
        ADD_CHANNEL(6);
    #endif

    #if NUM_CHANNELS >= 8 && LED_PIN7 >= 0
        ADD_CHANNEL(7);
    #endif

    #ifdef POWER_LIMIT_MW
        set_max_power_in_milliwatts(POWER_LIMIT_MW);                // Set brightness limit
    #endif
}

WS281xGFX::WS281xGFX(size_t w, size_t h) : GFXBase(w, h)
{
    debugV("Creating Device of size %zu x %zu", w, h);
    leds = static_cast<CRGB *>(calloc(w * h, sizeof(CRGB)));
    if(!leds)
        throw std::runtime_error("Unable to allocate LEDs in WS281xGFX");
}

WS281xGFX::~WS281xGFX()
{
    free(leds);
    leds = nullptr;
}

void WS281xGFX::InitializeHardware(std::vector<std::shared_ptr<GFXBase>>& devices)
{
    // We don't support more than 8 parallel channels
    #if NUM_CHANNELS > 8
        #error The maximum value of NUM_CHANNELS (number of parallel channels) is 8
    #endif

    for (int i = 0; i < NUM_CHANNELS; i++)
    {
        debugW("Allocating WS281xGFX for channel %d", i);
        devices.push_back(make_shared_psram<WS281xGFX>(MATRIX_WIDTH, MATRIX_HEIGHT));
    }

    AddLEDs(devices);
}

// PostProcessFrame
//
// PostProcessFrame sends the data to the LED strip.  If it's fewer than the size of the strip, we only send that many.

void WS281xGFX::PostProcessFrame(uint16_t localPixelsDrawn, uint16_t wifiPixelsDrawn)
{
    auto pixelsDrawn = wifiPixelsDrawn > 0 ? wifiPixelsDrawn : localPixelsDrawn;

    // If we drew no pixels, there's nothing to post process
    if (pixelsDrawn == 0)
    {
        debugV("Frame draw ended without any pixels drawn.");
        return;
    }

    // If there are no LEDs to show, we can just return now

    if (FastLED.count() == 0)
    {
        static auto lastDrawTime = millis();
        g_Values.FPS = 1000.0 / max(1UL, millis() - lastDrawTime);
        lastDrawTime = millis();
        return;
    }

    auto& effectManager = g_ptrSystem->GetEffectManager();

    for (int i = 0; i < NUM_CHANNELS; i++)
    {
        FastLED[i].setLeds(effectManager.g(i)->leds, pixelsDrawn);
        fadeLightBy(FastLED[i].leds(), FastLED[i].size(), 255 - g_ptrSystem->GetDeviceConfig().GetBrightness());
    }
    FastLED.show(g_Values.Fader); //Shows the pixels

    g_Values.FPS = FastLED.getFPS();
    #ifdef POWER_LIMIT_MW
        g_Values.Brite = 100.0 * calculate_max_brightness_for_power_mW(g_ptrSystem->GetDeviceConfig().GetBrightness(), POWER_LIMIT_MW) / 255;
    #else
        g_Values.Brite = 100.0 * g_ptrSystem->GetDeviceConfig().GetBrightness() / 255;
    #endif
    g_Values.Watts = calculate_unscaled_power_mW(effectManager.g()->leds, pixelsDrawn) / 1000; // 1000 for mw->W
}

#if HEXAGON

HexagonGFX::HexagonGFX(size_t numLeds) : WS281xGFX(numLeds, 1)
{
}

void HexagonGFX::InitializeHardware(std::vector<std::shared_ptr<GFXBase>>& devices)
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

    AddLEDs(devices);
}

// filHexRing
//
// Fills a ring around the hexagon, inset by the indent specified and in the color provided

void HexagonGFX::fillHexRing(uint16_t indent, CRGB color)
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

#endif
