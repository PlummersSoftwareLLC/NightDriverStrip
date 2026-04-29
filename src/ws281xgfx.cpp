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

#include <algorithm>
#include <cstring>

#include "deviceconfig.h"
#include "effectmanager.h"
#include "systemcontainer.h"
#include "values.h"
#include "ws281xgfx.h"
#if USE_WS281X
#include "ws281xoutputmanager.h"
#endif

namespace
{
    DRAM_ATTR std::mutex g_ws281xTransportMutex;
}

std::mutex& WS281xGFX::TransportMutex()
{
    return g_ws281xTransportMutex;
}

WS281xGFX::WS281xGFX(size_t w, size_t h) : GFXBase(w, h)
{
    debugV("Creating Device of size %zu x %zu", w, h);
    // Strip pixel storage is one of the largest persistent allocations in the system. Prefer PSRAM
    // so live-topology and UI work do not consume the smaller internal heap needed by drivers/tasks.
    leds = static_cast<CRGB *>(PreferPSRAMAlloc(w * h * sizeof(CRGB)));
    if(!leds)
        throw std::runtime_error("Unable to allocate LEDs in WS281xGFX");
    memset(leds, 0, w * h * sizeof(CRGB));
}

WS281xGFX::~WS281xGFX()
{
    free(leds);
    leds = nullptr;
}

void WS281xGFX::ConfigureTopology(size_t width, size_t height, bool serpentine)
{
    const auto newLEDCount = width * height;
    if (newLEDCount != GetLEDCount())
    {
        // Topology changes can resize the per-channel LED backing store significantly; keep that large
        // buffer in PSRAM when available rather than growing pressure on the internal heap.
        auto* newPixels = static_cast<CRGB*>(PreferPSRAMAlloc(newLEDCount * sizeof(CRGB)));
        if (!newPixels)
            throw std::runtime_error("Unable to resize LEDs in WS281xGFX");
        memset(newPixels, 0, newLEDCount * sizeof(CRGB));

        if (leds)
        {
            memcpy(newPixels, leds, std::min(GetLEDCount(), newLEDCount) * sizeof(CRGB));
            free(leds);
        }

        leds = newPixels;
    }

    GFXBase::ConfigureTopology(width, height, serpentine);
}

void WS281xGFX::InitializeHardware(std::vector<std::shared_ptr<GFXBase>>& devices)
{
    // We don't support more than 8 parallel channels
    #if NUM_CHANNELS > 8
        #error The maximum value of NUM_CHANNELS (number of parallel channels) is 8
    #endif

    const auto& deviceConfig = g_ptrSystem->GetDeviceConfig();

    for (int i = 0; i < NUM_CHANNELS; i++)
    {
        debugW("Allocating WS281xGFX for channel %d", i);
        auto device = make_shared_psram<WS281xGFX>(deviceConfig.GetMatrixWidth(), deviceConfig.GetMatrixHeight());
        device->ConfigureTopology(deviceConfig.GetMatrixWidth(), deviceConfig.GetMatrixHeight(), deviceConfig.IsMatrixSerpentine());
        devices.push_back(device);
    }

    // Use a single WS281x transport path for both boot and live reconfiguration. The compiled pin macros still
    // define the default configuration, but driving LEDs through one runtime-capable manager avoids fragile
    // handoffs between different ESP32 RMT implementations.
    #if USE_WS281X
    auto& outputManager = g_ptrSystem->SetupWS281xOutputManager();
    String errorMessage;
    if (!outputManager.ApplyConfig(deviceConfig, devices, &errorMessage))
        throw std::runtime_error(errorMessage.c_str());
    #endif
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

    #if USE_WS281X
    auto& effectManager = g_ptrSystem->GetEffectManager();
    const auto& deviceConfig = g_ptrSystem->GetDeviceConfig();

    if (!g_ptrSystem->HasWS281xOutputManager())
    {
        static auto lastDrawTime = millis();
        g_Values.FPS = 1000.0 / max(1UL, millis() - lastDrawTime);
        lastDrawTime = millis();
        return;
    }

    for (int i = 0; i < NUM_CHANNELS; i++)
    {
        auto& graphics = effectManager.g(i);
        const auto ledCount = graphics.GetLEDCount();
        const auto activePixels = std::min<size_t>(pixelsDrawn, ledCount);
        if (activePixels < ledCount)
            fill_solid(graphics.leds + activePixels, ledCount - activePixels, CRGB::Black);
    }

    static auto lastDrawTime = millis();
    auto& outputManager = g_ptrSystem->GetWS281xOutputManager();
    outputManager.Show(g_ptrSystem->GetDevices(), pixelsDrawn, deviceConfig.GetBrightness(), g_Values.Fader);
    g_Values.FPS = 1000.0 / max(1UL, millis() - lastDrawTime);
    lastDrawTime = millis();
    #ifdef POWER_LIMIT_MW
        g_Values.Brite = 100.0 * calculate_max_brightness_for_power_mW(deviceConfig.GetBrightness(), POWER_LIMIT_MW) / 255;
    #else
        g_Values.Brite = 100.0 * deviceConfig.GetBrightness() / 255;
    #endif
    g_Values.Watts = calculate_unscaled_power_mW(effectManager.g().leds, pixelsDrawn) / 1000; // 1000 for mw->W
    #endif
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

    #if USE_WS281X
    // Hexagon layouts are always driven through the runtime manager because their physical mapping is
    // board-specific and does not benefit from the compiled FastLED fallback assumptions used by strips.
    auto& outputManager = g_ptrSystem->SetupWS281xOutputManager();
    String errorMessage;
    if (!outputManager.ApplyConfig(g_ptrSystem->GetDeviceConfig(), devices, &errorMessage))
        throw std::runtime_error(errorMessage.c_str());
    #endif
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
