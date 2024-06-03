//+--------------------------------------------------------------------------
//
// File:        ledmatrixgfx.h
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

#if USE_HUB75

#include <SmartMatrix.h>

//
// Matrix Panel
//

#define COLOR_DEPTH 24 // known working: 24, 48 - If the sketch uses type `rgb24` directly, COLOR_DEPTH must be 24

class LEDMatrixGFX : public GFXBase
{
protected:
    String strCaption;
    unsigned long captionStartTime = 0;
    float captionDuration = 0;
    const float captionFadeInTime = 500;
    const float captionFadeOutTime = 1000;

public:
    typedef RGB_TYPE(COLOR_DEPTH) SM_RGB;
    static const uint8_t kMatrixWidth = MATRIX_WIDTH;                                   // known working: 32, 64, 96, 128
    static const uint8_t kMatrixHeight = MATRIX_HEIGHT;                                 // known working: 16, 32, 48, 64
    static const uint8_t kRefreshDepth = COLOR_DEPTH;                                   // known working: 24, 36, 48
    static const uint8_t kDmaBufferRows = 4;                                            // known working: 2-4, use 2 to save memory, more to keep from dropping frames and automatically lowering refresh rate
    static const uint8_t kPanelType = SMARTMATRIX_HUB75_32ROW_MOD16SCAN;                // use SMARTMATRIX_HUB75_16ROW_MOD8SCAN for common 16x32 panels
    static const uint8_t kMatrixOptions = (SMARTMATRIX_OPTIONS_BOTTOM_TO_TOP_STACKING   /* | SMARTMATRIX_OPTIONS_ESP32_CALC_TASK_CORE_1 */); // see http://docs.pixelmatix.com/SmartMatrix for options
    static const uint8_t kBackgroundLayerOptions = (SM_BACKGROUND_OPTIONS_NONE);
    static const uint8_t kDefaultBrightness = 255; // full (100%) brightness
    static const rgb24   defaultBackgroundColor;

    static SMLayerBackground<SM_RGB, kBackgroundLayerOptions> backgroundLayer;
    static SMLayerBackground<SM_RGB, kBackgroundLayerOptions> titleLayer;
    static SmartMatrixHub75Refresh<COLOR_DEPTH, kMatrixWidth, kMatrixHeight, kPanelType, kMatrixOptions> matrixRefresh;
    static SmartMatrixHub75Calc<COLOR_DEPTH, kMatrixWidth, kMatrixHeight, kPanelType, kMatrixOptions> matrix;

    LEDMatrixGFX(size_t w, size_t h) : GFXBase(w, h)
    {
    }

    ~LEDMatrixGFX()
    {
    }

    static void InitializeHardware(std::vector<std::shared_ptr<GFXBase>>& devices)
    {
        StartMatrix();

        for (int i = 0; i < NUM_CHANNELS; i++)
        {
            auto matrix = make_shared_psram<LEDMatrixGFX>(MATRIX_WIDTH, MATRIX_HEIGHT);
            devices.push_back(matrix);
            matrix->loadPalette(0);
        }

        // We don't need color correction on the title layer, but we want it on the main background

        titleLayer.enableColorCorrection(false);
        backgroundLayer.enableColorCorrection(true);

        // Starting an effect might need to draw, so we need to set the leds up before doing so
        std::static_pointer_cast<LEDMatrixGFX>(devices[0])->setLeds(GetMatrixBackBuffer());
    }

    void SetBrightness(byte amount)
    {
        matrix.setBrightness(amount);
    }

    // EstimatePowerDraw
    //
    // Estimate the total power load for the board and matrix by walking the pixels and adding our previously measured
    // power draw per pixel based on what color and brightness each pixel is

    int EstimatePowerDraw()
    {
        constexpr auto kBaseLoad       = 1500;          // Experimentally derived values
        constexpr auto mwPerPixelRed   = 4.10f;
        constexpr auto mwPerPixelGreen = 0.82f;
        constexpr auto mwPerPixelBlue  = 1.75f;

        float totalPower = kBaseLoad;
        for (int i = 0; i < NUM_LEDS; i++)
        {
            const auto pixel = leds[i];
            totalPower += pixel.r * mwPerPixelRed   / 255.0f;
            totalPower += pixel.g * mwPerPixelGreen / 255.0f;
            totalPower += pixel.b * mwPerPixelBlue  / 255.0f;
        }
        return (int) totalPower;
    }

    uint16_t xy(uint16_t x, uint16_t y) const override
    {
        // Note the x,y are unsigned so can't be less than zero
        if (x < _width && y < _height)
            return y * MATRIX_WIDTH + x;

        debugE("%s", str_sprintf("Invalid index in xy: x=%d, y=%d, NUM_LEDS=%d", x, y, NUM_LEDS).c_str());
        return 0;
    }

    // Whereas an LEDStripGFX would track it's own memory for the CRGB array, we simply point to the buffer already used for
    // the matrix display memory.  That also eliminated having a local draw buffer that is then copied, because the effects
    // can render directly to the right back buffer automatically.

    void setLeds(CRGB *pLeds)
    {
        leds = pLeds;
    }

    void fillLeds(std::unique_ptr<CRGB []> & pLEDs) override
    {
        // A mesmerizer panel has the same layout as in memory, so we can memcpy.

        memcpy(leds, pLEDs.get(), sizeof(CRGB) * GetLEDCount());
    }

    void Clear(CRGB color = CRGB::Black) override
    {
        // NB: We directly clear the backbuffer because otherwise effects would start with a snapshot of the effect
        //     before them on the next buffer swap.  So we clear the backbuffer and then the leds, which point to
        //     the current front buffer.  TLDR:  We clear both the front and back buffers to avoid flicker between effects.

        if (color == CRGB::Black)
        {
            memset((void *) backgroundLayer.backBuffer(), 0, sizeof(LEDMatrixGFX::SM_RGB) * _width * _height);
            memset((void *) leds, 0, sizeof(CRGB) * _width * _height);
        }
        else
        {
            for (int i = 0; i < NUM_LEDS; i++)
            {
                backgroundLayer.backBuffer()[i] = rgb24(color.r, color.g, color.b);
                leds[i] = color;
            }
        }
    }

    const String & GetCaption()
    {
        return strCaption;
    }

    float GetCaptionTransparency()
    {
        unsigned long now = millis();
        if (strCaption == nullptr)
            return 0.0f;

        if (now > (captionStartTime + captionDuration + captionFadeInTime + captionFadeOutTime))
            return 0.0f;

        float elapsed = now - captionStartTime;

        if (elapsed < captionFadeInTime)
            return elapsed / captionFadeInTime;

        if (elapsed > captionFadeInTime + captionDuration)
            return 1.0f - ((elapsed - captionFadeInTime - captionDuration) / captionFadeOutTime);

        return 1.0f;
    }

    void SetCaption(const String & str, uint32_t duration)
    {
        captionDuration = duration;
        strCaption = str;
        captionStartTime = millis();
    }

    void MoveInwardX(int startY = 0, int endY = MATRIX_HEIGHT - 1) override
    {
        // Optimized for Smartmatrix matrix - uses knowledge of how the pixels are laid
        // out in order to do the scroll.  We should technically use memmove instead
        // of memcpy since the regions are overlapping but this is faster and seems
        // to work!

        for (int y = startY; y <= endY; y++)
        {
            auto pLinemem = leds + y * MATRIX_WIDTH;
            auto pLinemem2 = pLinemem + (MATRIX_WIDTH / 2);
            memcpy(pLinemem + 1, pLinemem, sizeof(CRGB) * (MATRIX_WIDTH / 2));
            memcpy(pLinemem2, pLinemem2 + 1, sizeof(CRGB) * (MATRIX_WIDTH / 2));
        }
    }

    void MoveOutwardsX(int startY = 0, int endY = MATRIX_HEIGHT - 1) override
    {
        // Optimized for Smartmatrix matrix - uses knowledge of how the pixels are laid
        // out in order to do the scroll.  We should technically use memmove instead
        // of memcpy since the regions are overlapping but this is faster and seems
        // to work!

        for (int y = startY; y <= endY; y++)
        {
            auto pLinemem = leds + y * MATRIX_WIDTH;
            auto pLinemem2 = pLinemem + (MATRIX_WIDTH / 2);
            memcpy(pLinemem, pLinemem + 1, sizeof(CRGB) * (MATRIX_WIDTH / 2));
            memcpy(pLinemem2 + 1, pLinemem2, sizeof(CRGB) * (MATRIX_WIDTH / 2));
        }
    }

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