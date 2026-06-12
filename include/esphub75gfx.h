//+--------------------------------------------------------------------------
//
// File:        esphub75gfx.h
//
// NightDriverStrip - (c) 2026 Plummer's Software LLC.  All Rights Reserved.
//
// This file is part of the NightDriver software project.
//
// Description:
//
//   Provides a modern HUB75 GFX implementation using esphome-libs/esp-hub75.
//
//---------------------------------------------------------------------------

#pragma once

#if USE_ESP_HUB75

#include "globals.h"
#include "gfxbase.h"
#include <hub75.h>
#include <memory>
#include <vector>

class ESPHUB75GFX : public GFXBase
{
protected:
    static std::unique_ptr<Hub75Driver> driver;
    static std::unique_ptr<CRGB[]> drawBuffer;

public:
    ESPHUB75GFX(size_t w, size_t h) : GFXBase(w, h)
    {
    }

    ~ESPHUB75GFX() override = default;

    __attribute__((always_inline)) uint16_t xy(uint16_t x, uint16_t y) const noexcept override
    {
        if (x < _width && y < _height)
            return y * _width + x;
        return 0;
    }

    void drawPixel(int16_t x, int16_t y, uint16_t color) override
    {
        if (isValidPixel(x, y))
            leds[xy(x, y)] = from16Bit(color);
    }

    static void InitializeHardware(std::vector<std::shared_ptr<GFXBase>>& devices);

    void setLeds(CRGB *pLeds)
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
