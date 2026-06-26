#pragma once

//+--------------------------------------------------------------------------
//
// File:        hub75gfx.h
//
// NightDriverStrip - (c) 2018 Plummer's Software LLC.  All Rights Reserved.
//
// Description:
//
//   Provides a generic base class for HUB75 DMA LED panel drivers, 
//   containing shared logic for power estimation, brightness bounding, 
//   and primitive hardware effects.
//
//---------------------------------------------------------------------------

#include "globals.h"

#if USE_HUB75

#include <memory>
#include "gfxbase.h"
#include "types.h"

#define COLOR_DEPTH 24

class HUB75GFX : public GFXBase
{
public:
    HUB75GFX(size_t w, size_t h);
    ~HUB75GFX() override;

    // Factory initializer based on compiler flags
    static void InitializeHardware(std::vector<std::shared_ptr<GFXBase>>& devices);

    // Derived classes must implement this to actually talk to the hardware
    virtual void SetBrightness(byte amount) = 0;

    // EstimatePowerDraw
    // Estimate the total power load for the board and matrix
    int EstimatePowerDraw();

    __attribute__((always_inline)) uint16_t xy(uint16_t x, uint16_t y) const noexcept override
    {
        if (x < _width && y < _height)
            return y * MATRIX_WIDTH + x;
        return 0;
    }

    void setLeds(CRGB *pLeds);
    void fillLeds(std::unique_ptr<CRGB []> & pLEDs) override;
    void Clear(CRGB color = CRGB::Black) override;

    void MoveInwardX(int startY = 0, int endY = MATRIX_HEIGHT - 1) override;
    void MoveOutwardsX(int startY = 0, int endY = MATRIX_HEIGHT - 1) override;

    void PrepareFrame() override;

    // Shared brightness and power bounding logic
    void PostProcessFrame(uint16_t localPixelsDrawn, uint16_t wifiPixelsDrawn) override;
};
#endif

