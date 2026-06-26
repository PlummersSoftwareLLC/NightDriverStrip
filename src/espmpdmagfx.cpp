//+--------------------------------------------------------------------------
//
// File: espmpdmagfx.cpp
//
// NightDriverStrip - (c) 2026 Robert Lipe. All Rights Reserved.
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
// Description: Optimized HUB75 DMA driver (mrcodetastic) for ESP32-S3,
//              conserving SRAM for network tasks via lower bit-depth and
//              manual, single-buffered memory management.
//
//---------------------------------------------------------------------------

#include "globals.h"

#if USE_MPDMA_HUB75
#include <algorithm>
#include <ESP32-HUB75-MatrixPanel-I2S-DMA.h>
#include "deviceconfig.h"
#include "espmpdmagfx.h"
#include "systemcontainer.h"
#include "values.h"

std::unique_ptr<MatrixPanel_I2S_DMA> ESPMPDMAGFX::driver;
std::unique_ptr<CRGB[]> ESPMPDMAGFX::drawBuffer;

void ESPMPDMAGFX::InitializeHardware(std::vector<std::shared_ptr<GFXBase>>& devices)
{
    HUB75_I2S_CFG config(MATRIX_WIDTH, MATRIX_HEIGHT, 1);

#if WAVESHARE_ESP32_S3_RGB_MATRIX
    // Waveshare ESP32-S3-RGB-Matrix
    config.gpio = {
        .r1 = 4, .g1 = 5, .b1 = 6,
        .r2 = 7, .g2 = 15, .b2 = 16,
        .a = 18, .b = 8, .c = 3, .d = 42, .e = (MATRIX_HEIGHT > 32 ? 9 : -1),
        .lat = 40, .oe = 2, .clk = 41
    };
#elif MATRIX_S3
    // Adafruit MatrixPortal S3 physical pinout mapping
    config.gpio = {
        .r1 = 42, .g1 = 41, .b1 = 40,
        .r2 = 38, .g2 = 39, .b2 = 37,
        .a = 45, .b = 36, .c = 48, .d = 35, .e = 21,
        .lat = 47, .oe = 14, .clk = 2
    };
#else
    // Default fallback to Mesmerizer ESP32 WROVER pinout
    config.gpio = {
        .r1 = 2, .g1 = 0, .b1 = 32,
        .r2 = 25, .g2 = 33, .b2 = 27,
        .a = 5, .b = 4, .c = 19, .d = 18, .e = 26,
        .lat = 21, .oe = 23, .clk = 22
    };
#endif

    // SRAM Optimization: We disabled library double-buffer to save RAM, but keep 8-bit color depth for fidelity
    config.setPixelColorDepthBits(8);
    config.double_buff    = false;

    // --- Optional Performance & Fidelity Tunings ---
    // config.i2sspeed = HUB75_I2S_CFG::HZ_20M; // Up to HZ_40M on S3. Increases refresh rate, reduces flicker, frees CPU, but cheap panels may glitch.
    // config.latch_blanking = 2;               // Default is 1. Increase to fix "ghosting" (faint lines bleeding across the matrix).
    // config.min_refresh_rate = 120;           // Default is 60Hz. Force higher refresh for camera filming, at cost of CPU/DMA bandwidth.
    // config.clkphase = false;                 // Flip clock phase if you see a "shimmering" effect or single pixels jumping around.


    driver = std::make_unique<MatrixPanel_I2S_DMA>(config);
    if (!driver->begin()) debugE("MatrixPanel_I2S_DMA::begin() FAILED!");

    // Allocate draw buffer in PSRAM
    drawBuffer = make_unique_psram<CRGB[]>(MATRIX_WIDTH * MATRIX_HEIGHT);

    // ... (Device initialization code)
    auto tmp_matrix = make_shared_psram<ESPMPDMAGFX>(MATRIX_WIDTH, MATRIX_HEIGHT);
    devices.push_back(tmp_matrix);
    std::static_pointer_cast<ESPMPDMAGFX>(devices[0])->setLeds(drawBuffer.get());
}

void ESPMPDMAGFX::SetBrightness(byte amount)
{
    if (driver)
        driver->setBrightness8(amount);
}

void ESPMPDMAGFX::PostProcessFrame(uint16_t localPixelsDrawn, uint16_t wifiPixelsDrawn)
{
    // Determine the active brightness level (honoring the cross-fade fader)
    uint8_t targetBrightness = g_ptrSystem->GetDeviceConfig().GetBrightness();
    if (g_Values.Fader < targetBrightness)
        targetBrightness = g_Values.Fader;
    SetBrightness(targetBrightness);

    // Render CRGB buffer to DMA using the XY() macro to match how effects populated it
    for (int y = 0; y < MATRIX_HEIGHT; y++)
        for (int x = 0; x < MATRIX_WIDTH; x++)
        {
            CRGB color = leds[XY(x, y)];
            driver->drawPixelRGB888(x, y, color.r, color.g, color.b);
        }

    FastLED.countFPS();
}
#endif

