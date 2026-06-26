#pragma once

//+--------------------------------------------------------------------------
//
// File:        espmpdmagfx.h
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
//   Provides a modern HUB75 GFX implementation using ESP32-HUB75-MatrixPanel-DMA.
//
//---------------------------------------------------------------------------


#if USE_MPDMA_HUB75

#include "globals.h"
#include <ESP32-HUB75-MatrixPanel-I2S-DMA.h>
#include <memory>
#include <vector>
#include "gfxbase.h"

class ESPMPDMAGFX : public GFXBase
{
protected:
    static std::unique_ptr<MatrixPanel_I2S_DMA> driver;
    static std::unique_ptr<CRGB[]> drawBuffer;

public:
    ESPMPDMAGFX(size_t w, size_t h) : GFXBase(w, h)
    {
    }

    ~ESPMPDMAGFX() override = default;

    __attribute__((always_inline)) uint16_t xy(uint16_t x, uint16_t y) const noexcept override
    {
        if (x < _width && y < _height)
            return XY(x, y);
        return 0;
    }

    __attribute__((always_inline)) inline void drawPixel(int16_t x, int16_t y, uint16_t color) override
    {
        if (isValidPixel(x, y))
            leds[xy(x, y)] = from16Bit(color);
    }

    static void InitializeHardware(std::vector<std::shared_ptr<GFXBase>>& devices);

    __attribute__((always_inline)) inline void setLeds(CRGB *pLeds)
    {
        leds = pLeds;
    }

    void fillLeds(std::unique_ptr<CRGB []> & pLEDs) override
    {
        memcpy(leds, pLEDs.get(), sizeof(CRGB) * GetLEDCount());
    }

    void PrepareFrame() override
    {
    }

    static void SetBrightness(byte amount);

    void PostProcessFrame(uint16_t localPixelsDrawn, uint16_t wifiPixelsDrawn) override;
};

#endif
