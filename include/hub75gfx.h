#pragma once

//+--------------------------------------------------------------------------
//
// File:        hub75gfx.h
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
//
// Description:
//
//   Provides a HUB75 GFX implementation for our RGB LED panel so that
//   we can use primitives such as lines and fills on it.
//
// History:     Oct-9-2018         Davepl      Created from other projects
//
//---------------------------------------------------------------------------

#include "globals.h"

#if USE_HUB75

#include "MatrixHardware_ESP32_Custom.h"

#define SM_INTERNAL     // Silence build messages from their header
#include <SmartMatrix.h>

#include <cmath>
#include <memory>

#include "gfxbase.h"
#include "types.h"

//
// Matrix Panel
//

#define COLOR_DEPTH 24 // known working: 24, 48 - If the sketch uses type `rgb24` directly, COLOR_DEPTH must be 24

class HUB75GFX : public GFXBase
{
protected:
    String strCaption;
    unsigned long captionStartTime = 0;
    float captionDuration = 0;
    const float captionFadeInTime = 500;
    float captionFadeOutTime = 1000;
    float totalCaptionDuration = 0;

public:
    typedef RGB_TYPE(COLOR_DEPTH) SM_RGB;
    static const uint8_t kPanelType = SMARTMATRIX_HUB75_32ROW_MOD16SCAN;                // use SMARTMATRIX_HUB75_16ROW_MOD8SCAN for common 16x32 panels
    static const uint8_t kMatrixOptions = (SMARTMATRIX_OPTIONS_BOTTOM_TO_TOP_STACKING   /* | SMARTMATRIX_OPTIONS_ESP32_CALC_TASK_CORE_1 */); // see http://docs.pixelmatix.com/SmartMatrix for options
    static const uint8_t kBackgroundLayerOptions = (SM_BACKGROUND_OPTIONS_NONE);
    static const uint8_t kDefaultBrightness = 255; // full (100%) brightness

    static SMLayerBackground<SM_RGB, kBackgroundLayerOptions> backgroundLayer;
    static SMLayerBackground<SM_RGB, kBackgroundLayerOptions> titleLayer;
    static SmartMatrixHub75Calc<COLOR_DEPTH, kMatrixWidth, kMatrixHeight, kPanelType, kMatrixOptions> matrix;

    HUB75GFX(size_t w, size_t h);

    ~HUB75GFX() override;

    static void InitializeHardware(std::vector<std::shared_ptr<GFXBase>>& devices);

    static void SetBrightness(byte amount);

    // EstimatePowerDraw
    //
    // Estimate the total power load for the board and matrix by walking the pixels and adding our previously measured
    // power draw per pixel based on what color and brightness each pixel is

    int EstimatePowerDraw();

    __attribute__((always_inline)) uint16_t xy(uint16_t x, uint16_t y) const noexcept override
    {
        // Note the x,y are unsigned so can't be less than zero
        if (x < _width && y < _height)
            return y * MATRIX_WIDTH + x;

        return 0;
    }

    // Whereas an WS281xGFX would track its own memory for the CRGB array, we simply point to the buffer already used for
    // the matrix display memory.  That also eliminated having a local draw buffer that is then copied, because the effects
    // can render directly to the right back buffer automatically.

    void setLeds(CRGB *pLeds);

    void fillLeds(std::unique_ptr<CRGB []> & pLEDs) override;

    void Clear(CRGB color = CRGB::Black) override;

    const String & GetCaption();

    float GetCaptionTransparency();

    void SetCaption(const String & str, uint32_t duration);

    void MoveInwardX(int startY = 0, int endY = MATRIX_HEIGHT - 1) override;

    void MoveOutwardsX(int startY = 0, int endY = MATRIX_HEIGHT - 1) override;

    // PrepareFrame
    //
    // Gets the matrix ready for the effect or wifi to render into

    void PrepareFrame() override;

    // PostProcessFrame
    //
    // Things we do with the matrix after rendering a frame, such as setting the brightness and swapping the backbuffer forward

    void PostProcessFrame(uint16_t localPixelsDrawn, uint16_t wifiPixelsDrawn) override;

    // Matrix interop

    static void StartMatrix();
    static CRGB *GetMatrixBackBuffer();
    static void MatrixSwapBuffers(bool bSwapBackground);
};
#endif
