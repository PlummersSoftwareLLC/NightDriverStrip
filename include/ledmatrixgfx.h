//+--------------------------------------------------------------------------
//
// File:        ledmatrixgfx.h
//
// File:        NTPTimeClient.h
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
//   Provides a Adafruit_GFX implementation for our RGB LED panel so that
//   we can use primitives such as lines and fills on it.
//
// History:     Oct-9-2018         Davepl      Created from other projects
//
//---------------------------------------------------------------------------

#pragma once

#if USE_MATRIX

#include <SmartMatrix.h>

//
// Matrix Panel
//

#define COLOR_DEPTH 24 // known working: 24, 48 - If the sketch uses type `rgb24` directly, COLOR_DEPTH must be 24

class LEDMatrixGFX : public GFXBase
{
protected:
    String strCaption;
    unsigned long captionStartTime;
    float captionDuration;
    const float captionFadeInTime = 500;
    const float captionFadeOutTime = 1000;

public:
    typedef RGB_TYPE(COLOR_DEPTH) SM_RGB;
    static const uint8_t kMatrixWidth = MATRIX_WIDTH;                                   // known working: 32, 64, 96, 128
    static const uint8_t kMatrixHeight = MATRIX_HEIGHT;                                 // known working: 16, 32, 48, 64
    static const uint8_t kRefreshDepth = COLOR_DEPTH;                                   // known working: 24, 36, 48
    static const uint8_t kDmaBufferRows = 4;                                            // known working: 2-4, use 2 to save memory, more to keep from dropping frames and automatically lowering refresh rate
    static const uint8_t kPanelType = SMARTMATRIX_HUB75_32ROW_MOD16SCAN;                // use SMARTMATRIX_HUB75_16ROW_MOD8SCAN for common 16x32 panels
    static const uint8_t kMatrixOptions = (SMARTMATRIX_OPTIONS_BOTTOM_TO_TOP_STACKING /* | SMARTMATRIX_OPTIONS_ESP32_CALC_TASK_CORE_1 */); // see http://docs.pixelmatix.com/SmartMatrix for options
    static const uint8_t kBackgroundLayerOptions = (SM_BACKGROUND_OPTIONS_NONE);
    static const uint8_t kDefaultBrightness = (100 * 255) / 100; // full (100%) brightness
    static const rgb24 defaultBackgroundColor;

    #if USE_MATRIX
    static SMLayerBackground<SM_RGB, kBackgroundLayerOptions> backgroundLayer;
    static SMLayerBackground<SM_RGB, kBackgroundLayerOptions> titleLayer;
    static SmartMatrixHub75Refresh<COLOR_DEPTH, kMatrixWidth, kMatrixHeight, kPanelType, kMatrixOptions> matrixRefresh;
    static SmartMatrixHub75Calc<COLOR_DEPTH, kMatrixWidth, kMatrixHeight, kPanelType, kMatrixOptions> matrix;
    #endif

    LEDMatrixGFX(size_t w, size_t h) : GFXBase(w, h)
    {
    }

    ~LEDMatrixGFX()
    {
    }

    void SetBrightness(byte percent)
    {
        matrix.setBrightness(percent);
    }
    
    virtual uint16_t xy(uint16_t x, uint16_t y) const
    {
        return y * MATRIX_WIDTH + x;    
    }

    void setLeds(CRGB *pLeds)
    {
        leds = pLeds;
    }

    const String & GetCaption()
    {
        return strCaption;
    }

    float GetCaptionTransparency()
    {
        unsigned long now = millis();
        if (strCaption == nullptr)
            return 0;

        if (now > (captionStartTime + captionDuration + captionFadeInTime + captionFadeOutTime))
            return 0;

        float elapsed = now - captionStartTime;

        if (elapsed < captionFadeInTime)
            return elapsed / captionFadeInTime;

        if (elapsed > captionFadeInTime + captionDuration)
            return 1.0 - ((elapsed - captionFadeInTime - captionDuration) / captionFadeOutTime);

        return 1.0;
    }

    void SetCaption(const String & str, uint32_t duration)
    {
        captionDuration = duration;
        strCaption = str;
        captionStartTime = millis();
    }

    virtual void MoveInwardX(int startY = 0, int endY = MATRIX_HEIGHT - 1)
    {
        // Optimized for Smartmatrix matrix - uses knowledge of how the pixels are laid
        // out in order to do the scroll with memmove rather than row by column pixel
        // lookups.          
        for (int y = startY; y <= endY; y++)
        {
            auto pLinemem = leds + y * MATRIX_WIDTH;
            auto pLinemem2 = pLinemem + (MATRIX_WIDTH / 2);
            memmove(pLinemem + 1, pLinemem, sizeof(CRGB) * (MATRIX_WIDTH / 2));
            memmove(pLinemem2, pLinemem2 + 1, sizeof(CRGB) * (MATRIX_WIDTH / 2));                
        }
    }

    virtual void MoveOutwardsX(int startY = 0, int endY = MATRIX_HEIGHT - 1)
    {
        // Optimized for Smartmatrix matrix - uses knowledge of how the pixels are laid
        // out in order to do the scroll with memmove rather than row by column pixel
        // lookups.  
        for (int y = startY; y <= endY; y++)
        {
            auto pLinemem = leds + y * MATRIX_WIDTH;
            auto pLinemem2 = pLinemem + (MATRIX_WIDTH / 2);
            memmove(pLinemem, pLinemem + 1, sizeof(CRGB) * (MATRIX_WIDTH / 2));
            memmove(pLinemem2 + 1, pLinemem2, sizeof(CRGB) * (MATRIX_WIDTH / 2));
        }
    }

    virtual void fillLeds(const CRGB *pLEDs)
    {
        // A mesmerizer panel has the same layout as in memory, so we can memcpy.  Others may require transposition,
        // so we do it the "slow" way for other matrices

        memcpy(leds, pLEDs, sizeof(CRGB) * _width * _height);
    }

    // Matrix interop

    static void StartMatrix();
    static CRGB *GetMatrixBackBuffer();
    static void MatrixSwapBuffers(bool bSwapBackground, bool bSwapTitle);

    SMLayerBackground<SM_RGB, kBackgroundLayerOptions> GetBackgroundLayer()
    {
        return backgroundLayer;
    }
};
#endif