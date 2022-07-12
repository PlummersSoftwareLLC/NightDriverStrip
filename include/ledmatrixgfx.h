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
#include "gfxbase.h"

#if USEMATRIX
#include <SmartMatrix.h>
#include "effects/matrix/Boid.h"
#include "effects/matrix/Vector.h"

//
// Matrix Panel
//

#define COLOR_DEPTH 24 // known working: 24, 48 - If the sketch uses type `rgb24` directly, COLOR_DEPTH must be 24

class LEDMatrixGFX : public GFXBase
{
protected:
    const char *pszCaption = nullptr;
    unsigned long captionStartTime;
    unsigned long captionDuration;
    const unsigned long captionFadeInTime = 500;
    const unsigned long captionFadeOutTime = 1000;

public:
    typedef RGB_TYPE(COLOR_DEPTH) SM_RGB;
    static const uint8_t kMatrixWidth = MATRIX_WIDTH;                                   // known working: 32, 64, 96, 128
    static const uint8_t kMatrixHeight = MATRIX_HEIGHT;                                 // known working: 16, 32, 48, 64
    static const uint8_t kRefreshDepth = COLOR_DEPTH;                                   // known working: 24, 36, 48
    static const uint8_t kDmaBufferRows = 4;                                            // known working: 2-4, use 2 to save memory, more to keep from dropping frames and automatically lowering refresh rate
    static const uint8_t kPanelType = SMARTMATRIX_HUB75_32ROW_MOD16SCAN;                // use SMARTMATRIX_HUB75_16ROW_MOD8SCAN for common 16x32 panels
    static const uint8_t kMatrixOptions = (SMARTMATRIX_OPTIONS_BOTTOM_TO_TOP_STACKING); // see http://docs.pixelmatix.com/SmartMatrix for options
    static const uint8_t kBackgroundLayerOptions = (SM_BACKGROUND_OPTIONS_NONE);
    static const uint8_t kDefaultBrightness = (100 * 255) / 100; // full (100%) brightness
    static const rgb24 defaultBackgroundColor;

    #if USEMATRIX
    static SMLayerBackground<SM_RGB, kBackgroundLayerOptions> backgroundLayer;
    static SMLayerBackground<SM_RGB, kBackgroundLayerOptions> titleLayer;
    static SmartMatrixHub75Refresh<COLOR_DEPTH, kMatrixWidth, kMatrixHeight, kPanelType, kMatrixOptions> matrixRefresh;
    static SmartMatrixHub75Calc<COLOR_DEPTH, kMatrixWidth, kMatrixHeight, kPanelType, kMatrixOptions> matrix;
    #endif

    static Boid *boids;

    LEDMatrixGFX(size_t w, size_t h) : GFXBase(w, h)
    {
        if (boids == nullptr)
            boids = new Boid[MATRIX_WIDTH]();
    }

    ~LEDMatrixGFX()
    {
        // We're going to leak the static instance of boids on exit because there's no cheap and easy
        // way to know when the last instance is being cleaned up.
    }

    inline void setLeds(CRGB *pLeds)
    {
        leds = pLeds;
    }

    const char *GetCaption()
    {
        return pszCaption;
    }

    double GetCaptionTransparency()
    {
        unsigned long now = millis();
        if (pszCaption == nullptr)
            return 0;

        if (now > (captionStartTime + captionDuration + captionFadeInTime + captionFadeOutTime))
            return 0;

        double elapsed = now - captionStartTime;

        if (elapsed < captionFadeInTime)
            return elapsed / captionFadeInTime;

        if (elapsed > captionFadeInTime + captionDuration)
            return 1 - ((elapsed - captionFadeInTime - captionDuration) / captionFadeOutTime);

        return 1;
    }

    void SetCaption(const char *psz, uint32_t duration)
    {
        captionDuration = duration;
        pszCaption = psz;
        captionStartTime = millis();
    }

    // Matrix interop

    static void StartMatrix();
    static CRGB *GetMatrixBackBuffer();
    static void MatrixSwapBuffers();
    static void PresentFrame();

    SMLayerBackground<SM_RGB, kBackgroundLayerOptions> GetBackgroundLayer()
    {
        return backgroundLayer;
    }
};
#endif