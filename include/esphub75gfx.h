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

    void PostProcessFrame(uint16_t localPixelsDrawn, uint16_t wifiPixelsDrawn) override;
};

#endif
