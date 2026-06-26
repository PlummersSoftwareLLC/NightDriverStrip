#pragma once

//+--------------------------------------------------------------------------
//
// File:        esphub75gfx.h
//
// NightDriverStrip - (c) 2026 Robert Lipe.  All Rights Reserved.
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

#if USE_ESP_HUB75

#include "globals.h"
#include <hub75.h>
#include <memory>
#include <vector>
#include "hub75gfx.h"

class ESPHUB75GFX : public HUB75GFX
{
protected:
    static std::unique_ptr<Hub75Driver> driver;
    static std::unique_ptr<CRGB[]> drawBuffer;

public:
    ESPHUB75GFX(size_t w, size_t h) : HUB75GFX(w, h)
    {
    }

    ~ESPHUB75GFX() override = default;

    __attribute__((always_inline)) uint16_t xy(uint16_t x, uint16_t y) const noexcept override
    {
        if (x < _width && y < _height)
            return y * _width + x;
        return 0;
    }

    __attribute__((always_inline)) inline void drawPixel(int16_t x, int16_t y, uint16_t color) override
    {
        if (isValidPixel(x, y))
            leds[xy(x, y)] = from16Bit(color);
    }

    static void InitializeHardware(std::vector<std::shared_ptr<GFXBase>>& devices);

    void SetBrightness(byte amount) override;

    void PostProcessFrame(uint16_t localPixelsDrawn, uint16_t wifiPixelsDrawn) override;
};

#endif
