//+--------------------------------------------------------------------------
//
// File:        esphub75gfx.cpp
//
// NightDriverStrip - (c) 2026 Plummer's Software LLC.  All Rights Reserved.
//
// This file is part of the NightDriver software project.
//
// Description:
//
//    Code for handling HUB75 matrix panels with the modern esp-hub75 library
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
