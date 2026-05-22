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
#include <esp_heap_caps.h>

#include "deviceconfig.h"
#include "effectmanager.h"
#include "pixelformat.h"
#include "systemcontainer.h"
#include "values.h"
#include "ws281xgfx.h"
#if USE_WS281X
#include "ws281xoutputmanager.h"
#endif

namespace
{
    DRAM_ATTR std::mutex g_ws281xTransportMutex;

    constexpr uint8_t kPowerRedMw = 16 * 5;      // FastLED default: 16 mA at 5 V
    constexpr uint8_t kPowerGreenMw = 11 * 5;    // FastLED default: 11 mA at 5 V
    constexpr uint8_t kPowerBlueMw = 15 * 5;     // FastLED default: 15 mA at 5 V
    constexpr uint8_t kPowerDarkMw = 1 * 5;      // FastLED default: 1 mA at 5 V

    #ifndef SK6812_WHITE_MW
        #define SK6812_WHITE_MW (15 * 5)
    #endif
    #ifndef NIGHTDRIVER_DEFAULT_AMBIENT_CW
        #define NIGHTDRIVER_DEFAULT_AMBIENT_CW 0
    #endif
    #ifndef NIGHTDRIVER_DEFAULT_AMBIENT_WW
        #define NIGHTDRIVER_DEFAULT_AMBIENT_WW 0
    #endif
    #ifndef SK6812_WHITE_EXTRACT_RATIO
        #define SK6812_WHITE_EXTRACT_RATIO 128
    #endif

    constexpr uint8_t kPowerWhiteMw = SK6812_WHITE_MW;
    constexpr uint8_t kDefaultAmbientCw = NIGHTDRIVER_DEFAULT_AMBIENT_CW;
    constexpr uint8_t kDefaultAmbientWw = NIGHTDRIVER_DEFAULT_AMBIENT_WW;
    constexpr uint8_t kDefaultWhiteExtractRatio = SK6812_WHITE_EXTRACT_RATIO;

    uint32_t EstimateRGBUnscaledPowerMw(const CRGB* leds, size_t ledCount)
    {
        uint32_t red = 0;
        uint32_t green = 0;
        uint32_t blue = 0;

        for (size_t i = 0; i < ledCount; ++i)
        {
            red += leds[i].r;
            green += leds[i].g;
            blue += leds[i].b;
        }

        return ((red * kPowerRedMw) >> 8)
             + ((green * kPowerGreenMw) >> 8)
             + ((blue * kPowerBlueMw) >> 8)
             + (kPowerDarkMw * ledCount);
    }

    uint32_t EstimateWS281xUnscaledPowerMw(const GFXBase& graphics, size_t ledCount)
    {
        #if defined(USE_SK6812) && USE_SK6812
            uint32_t red = 0;
            uint32_t green = 0;
            uint32_t blue = 0;
            uint32_t white = 0;
            const uint16_t ratio = static_cast<uint16_t>(kDefaultWhiteExtractRatio);
            const uint8_t ambientWhite = PixelFormatHelpers::SaturatingAdd(kDefaultAmbientCw, kDefaultAmbientWw);

            for (size_t i = 0; i < ledCount; ++i)
            {
                CRGB color = graphics.leds[i];
                uint8_t effectWhite = 0;
                if (graphics.whites)
                    effectWhite = PixelFormatHelpers::SaturatingAdd(graphics.whites[i].cw, graphics.whites[i].ww);

                uint8_t pull = 0;
                if (effectWhite == 0)
                {
                    const uint8_t shared = std::min(color.r, std::min(color.g, color.b));
                    pull = static_cast<uint8_t>((static_cast<uint16_t>(shared) * ratio + 127) / 255);
                    color.r -= pull;
                    color.g -= pull;
                    color.b -= pull;
                }

                red += color.r;
                green += color.g;
                blue += color.b;
                white += std::max(PixelFormatHelpers::SaturatingAdd(pull, effectWhite), ambientWhite);
            }

            return ((red * kPowerRedMw) >> 8)
                 + ((green * kPowerGreenMw) >> 8)
                 + ((blue * kPowerBlueMw) >> 8)
                 + ((white * kPowerWhiteMw) >> 8)
                 + (kPowerDarkMw * ledCount);
        #else
            return EstimateRGBUnscaledPowerMw(graphics.leds, ledCount);
        #endif
    }

    uint8_t LimitBrightnessForPower(uint32_t unscaledPowerMw, uint8_t targetBrightness, uint8_t fader, uint32_t maxPowerMw)
    {
        if (unscaledPowerMw == 0 || targetBrightness == 0 || fader == 0)
            return targetBrightness;

        const uint64_t requestedMw =
            (static_cast<uint64_t>(unscaledPowerMw) * targetBrightness * fader) / (256ULL * 256ULL);
        if (requestedMw <= maxPowerMw)
            return targetBrightness;

        return static_cast<uint8_t>((static_cast<uint64_t>(targetBrightness) * maxPowerMw) / requestedMw);
    }

    uint32_t ScalePowerMw(uint32_t unscaledPowerMw, uint8_t brightness, uint8_t fader)
    {
        return static_cast<uint32_t>(
            (static_cast<uint64_t>(unscaledPowerMw) * brightness * fader) / (256ULL * 256ULL));
    }
}

std::mutex& WS281xGFX::TransportMutex()
{
    return g_ws281xTransportMutex;
}

// AllocLedBuffer
//
// FastLED's RMT path on ESP32 hands the CRGB array directly to rmt_write_sample,
// which DMAs from it. DMA cannot reach PSRAM on the ESP32, so the strip's pixel
// storage must live in internal RAM with the DMA-capable cap. Allocating this
// from PSRAM (which is what the project's default allocator now does for any
// allocation above PSRAM_DEFAULT_THRESHOLD) breaks every frame write with
// "rmt: Using buffer allocated from psram" / ESP_ERR_INVALID_ARG.

static CRGB* AllocLedBuffer(size_t count)
{
    void* mem = heap_caps_malloc(count * sizeof(CRGB),
                                 MALLOC_CAP_DMA | MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
    if (!mem)
        throw std::runtime_error("Unable to allocate DMA-capable LED buffer in WS281xGFX");
    memset(mem, 0, count * sizeof(CRGB));
    return static_cast<CRGB*>(mem);
}

#if defined(USE_SK6812) && USE_SK6812
// The whites plane is consumed by PixelFormat::Pack at frame-build time
// but never DMA'd directly, so we don't need DMA-capable memory. Pinning
// it to internal RAM still keeps the per-frame access path off PSRAM,
// which matters under the project's PSRAM-default allocator policy.
static CRGBW* AllocWhitesBuffer(size_t count)
{
    void* mem = heap_caps_malloc(count * sizeof(CRGBW),
                                 MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
    if (!mem)
        throw std::runtime_error("Unable to allocate whites plane in WS281xGFX");
    memset(mem, 0, count * sizeof(CRGBW));
    return static_cast<CRGBW*>(mem);
}
#endif

WS281xGFX::WS281xGFX(size_t w, size_t h) : GFXBase(w, h)
{
    debugV("Creating Device of size %zu x %zu", w, h);
    leds = AllocLedBuffer(w * h);
    #if defined(USE_SK6812) && USE_SK6812
        whites = AllocWhitesBuffer(w * h);
    #endif
}

WS281xGFX::~WS281xGFX()
{
    heap_caps_free(leds);
    leds = nullptr;
    #if defined(USE_SK6812) && USE_SK6812
        if (whites)
        {
            heap_caps_free(whites);
            whites = nullptr;
        }
    #endif
}

void WS281xGFX::ConfigureTopology(size_t width, size_t height, bool serpentine)
{
    const auto newLEDCount = width * height;
    if (newLEDCount != GetLEDCount())
    {
        CRGB* newPixels = AllocLedBuffer(newLEDCount);

        if (leds)
        {
            memcpy(newPixels, leds, std::min(GetLEDCount(), newLEDCount) * sizeof(CRGB));
            heap_caps_free(leds);
        }

        leds = newPixels;

        #if defined(USE_SK6812) && USE_SK6812
            CRGBW* newWhites = AllocWhitesBuffer(newLEDCount);
            if (whites)
            {
                memcpy(newWhites, whites,
                       std::min(GetLEDCount(), newLEDCount) * sizeof(CRGBW));
                heap_caps_free(whites);
            }
            whites = newWhites;
        #endif
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
        auto device = std::make_shared<WS281xGFX>(deviceConfig.GetMatrixWidth(), deviceConfig.GetMatrixHeight());
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
        return;
    }

    for (int i = 0; i < NUM_CHANNELS; i++)
    {
        auto& graphics = effectManager.g(i);
        const auto ledCount = graphics.GetLEDCount();
        const auto activePixels = std::min<size_t>(pixelsDrawn, ledCount);
        if (activePixels < ledCount)
        {
            fill_solid(graphics.leds + activePixels, ledCount - activePixels, CRGB::Black);
            // Zero the tail of the whites plane too (if allocated) so a
            // previously-CCT-lit pixel that's now beyond pixelsDrawn doesn't
            // stay lit on the next frame.
            if (graphics.whites)
                memset(graphics.whites + activePixels, 0,
                       (ledCount - activePixels) * sizeof(CRGBW));
        }
    }

    auto& outputManager = g_ptrSystem->GetWS281xOutputManager();
    uint32_t unscaledPowerMw = 0;
    const size_t activeChannelCount = std::min<size_t>(outputManager.GetActiveChannelCount(), NUM_CHANNELS);
    const size_t activeLEDCount = outputManager.GetActiveLEDCount();
    for (size_t i = 0; i < activeChannelCount; ++i)
    {
        auto& graphics = effectManager.g(i);
        const size_t ledCount = std::min(activeLEDCount, graphics.GetLEDCount());
        unscaledPowerMw += EstimateWS281xUnscaledPowerMw(graphics, ledCount);
    }

    uint8_t outputBrightness = deviceConfig.GetBrightness();
    #ifdef POWER_LIMIT_MW
        outputBrightness = LimitBrightnessForPower(unscaledPowerMw, outputBrightness, g_Values.Fader, POWER_LIMIT_MW);
    #endif
    outputManager.Show(g_ptrSystem->GetDevices(), pixelsDrawn, outputBrightness, g_Values.Fader);

    g_Values.Brite = 100.0 * outputBrightness / 255;
    g_Values.Watts = ScalePowerMw(unscaledPowerMw, outputBrightness, g_Values.Fader) / 1000; // 1000 for mW->W
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
        devices.push_back(std::make_shared<HexagonGFX>(NUM_LEDS));
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
