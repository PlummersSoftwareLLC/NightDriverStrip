//+--------------------------------------------------------------------------
//
// File: esphub75gfx.cpp
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
// Description:
//
//   Provides a modern HUB75 GFX implementation using esphome-libs/esp-hub75.
//
//---------------------------------------------------------------------------

#include "globals.h"

#if USE_ESP_HUB75

#include "esphub75gfx.h"
#include "systemcontainer.h"

std::unique_ptr<Hub75Driver> ESPHUB75GFX::driver;
std::unique_ptr<CRGB[]> ESPHUB75GFX::drawBuffer;

void ESPHUB75GFX::InitializeHardware(std::vector<std::shared_ptr<GFXBase>>& devices)
{
    // Set up configuration for the HUB75 driver
    Hub75Config config{};
    config.panel_width = MATRIX_WIDTH;
    config.panel_height = MATRIX_HEIGHT;
    config.scan_pattern = Hub75ScanPattern::SCAN_1_16;
    config.double_buffer = true;
    config.shift_driver = Hub75ShiftDriver::GENERIC;

#if WAVESHARE_ESP32_S3_RGB_MATRIX
    // Waveshare ESP32-S3-RGB-Matrix
    config.pins.r1 = 4;
    config.pins.g1 = 5;
    config.pins.b1 = 6;
    config.pins.r2 = 7;
    config.pins.g2 = 15;
    config.pins.b2 = 16;
    config.pins.a = 18;
    config.pins.b = 8;
    config.pins.c = 3;
    config.pins.d = 42;
#if MATRIX_HEIGHT > 32
    config.pins.e = 9;
#else
    config.pins.e = -1; // 64x32 is 1/16 scan, no E pin
#endif
    config.pins.lat = 40;
    config.pins.oe = 2;
    config.pins.clk = 41;
#else
    // Adafruit MatrixPortal S3 physical pinout mapping
    config.pins.r1 = 42;
    config.pins.g1 = 41;
    config.pins.b1 = 40;
    config.pins.r2 = 38;
    config.pins.g2 = 39;
    config.pins.b2 = 37;
    config.pins.a = 45;
    config.pins.b = 36;
    config.pins.c = 48;
    config.pins.d = 35;
#if MATRIX_HEIGHT > 32
    config.pins.e = 21;
#else
    config.pins.e = -1; // 64x32 is 1/16 scan, no E pin
#endif
    config.pins.lat = 47;
    config.pins.oe = 14;
    config.pins.clk = 2;
#endif

    driver = std::make_unique<Hub75Driver>(config);
    driver->begin();

    // Allocate frame drawing buffer in PSRAM
    drawBuffer = make_unique_psram<CRGB[]>(MATRIX_WIDTH * MATRIX_HEIGHT);

    for (int i = 0; i < NUM_CHANNELS; i++)
    {
        auto tmp_matrix = make_shared_psram<ESPHUB75GFX>(MATRIX_WIDTH, MATRIX_HEIGHT);
        devices.push_back(tmp_matrix);
        tmp_matrix->loadPalette(0);
    }

    std::static_pointer_cast<ESPHUB75GFX>(devices[0])->setLeds(drawBuffer.get());
}

void ESPHUB75GFX::PostProcessFrame(uint16_t localPixelsDrawn, uint16_t wifiPixelsDrawn)
{
    if ((localPixelsDrawn + wifiPixelsDrawn) == 0)
        return;

    // Stream our CRGB buffer directly into the active HUB75 drawing buffer
    driver->draw_pixels(0, 0, _width, _height, (const uint8_t*)leds, Hub75PixelFormat::RGB888, Hub75ColorOrder::RGB, false);

    // Swap buffers to display the rendered frame atomically
    driver->flip_buffer();

    FastLED.countFPS();
}



#endif
