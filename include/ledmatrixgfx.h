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
//   Provides an Adafruit_GFX implementation for our RGB LED panel so that
//   we can use primitives such as lines and fills on it.
//
// History:     Oct-9-2018         Davepl      Created from other projects
//
//---------------------------------------------------------------------------

#pragma once

#if USE_HUB75

#include "globals.h"
#include <cmath>
#include <memory>
#include <mutex>
#include "types.h"

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
    static const uint8_t kPanelType = SMARTMATRIX_HUB75_32ROW_MOD16SCAN;                // use SMARTMATRIX_HUB75_16ROW_MOD8SCAN for common 16x32 panels
    static const uint8_t kMatrixOptions = (SMARTMATRIX_OPTIONS_BOTTOM_TO_TOP_STACKING   /* | SMARTMATRIX_OPTIONS_ESP32_CALC_TASK_CORE_1 */); // see http://docs.pixelmatix.com/SmartMatrix for options
    static const uint8_t kBackgroundLayerOptions = (SM_BACKGROUND_OPTIONS_NONE);
    static const uint8_t kDefaultBrightness = 255; // full (100%) brightness

    static SMLayerBackground<SM_RGB, kBackgroundLayerOptions> backgroundLayer;
    static SMLayerBackground<SM_RGB, kBackgroundLayerOptions> titleLayer;
    static SmartMatrixHub75Calc<COLOR_DEPTH, kMatrixWidth, kMatrixHeight, kPanelType, kMatrixOptions> matrix;

    LEDMatrixGFX(size_t w, size_t h) : GFXBase(w, h)
    {
    }

    ~LEDMatrixGFX() override
    = default;

    // A 3-byte struct will have one byte of padding so each element
    // begins on a NA boundary. Making this
    // struct __attribute__((packed)) PolarMap
    // might conserve 25% of this buffer, but it might also force
    // single elements to be split across a (locked) cache line.
    // We'll thus leave this naturally aligned unless we have a
    // really great reason not to.
    struct PolarMap {
        uint8_t angle;
        uint8_t scaled_radius;
        uint8_t unscaled_radius;
    };

    using PolarMapArray = PolarMap[kMatrixWidth][kMatrixHeight];

    static const PolarMapArray& getPolarMap() {
        static std::unique_ptr<PolarMapArray> rMap_ptr;
        static std::mutex rMap_mutex;

        // Double-checked locking for thread-safe, on-demand initialization
        if (!rMap_ptr) {
            std::lock_guard<std::mutex> lock(rMap_mutex);
            if (!rMap_ptr) {
                // Allocate from PSRAM using the project's helper
                rMap_ptr = make_unique_psram<PolarMapArray>();

                auto& rMap = *rMap_ptr;
                const uint8_t C_X = kMatrixWidth / 2;
                const uint8_t C_Y = kMatrixHeight / 2;
                const float mapp = 255.0f / kMatrixWidth;

                for (int8_t x = -C_X; x < C_X + (kMatrixWidth % 2); x++) {
                    for (int8_t y = -C_Y; y < C_Y + (MATRIX_HEIGHT % 2); y++) {
                        float angle_rad = atan2f(static_cast<float>(y), static_cast<float>(x));
                        float radius_float = hypotf(static_cast<float>(x), static_cast<float>(y));

                        rMap[x + C_X][y + C_Y].angle = 128.0f * (angle_rad / (float)M_PI);
                        rMap[x + C_X][y + C_Y].scaled_radius = radius_float * mapp;
                        rMap[x + C_X][y + C_Y].unscaled_radius = radius_float;
                    }
                }

                // A note on the radius calculations:
                //
                // `unscaled_radius` is the true geometric distance from the center of the
                // matrix to the pixel. This is useful for effects that need the real
                // physical distance.
                //
                // `scaled_radius` maps the geometric radius to a range that is more
                // suitable for use with 8-bit FastLED functions (like inoise8).
                // The scaling is normalized by the matrix width, which is a common
                // technique to make radial effects work consistently across different
                // matrix sizes.
            }
        }
        return *rMap_ptr;
    }

    static void InitializeHardware(std::vector<std::shared_ptr<GFXBase>>& devices)
    {
        StartMatrix();

        for (int i = 0; i < NUM_CHANNELS; i++)
        {
            auto tmp_matrix = make_shared_psram<LEDMatrixGFX>(MATRIX_WIDTH, MATRIX_HEIGHT);
            devices.push_back(tmp_matrix);
            tmp_matrix->loadPalette(0);
        }

        // We don't need color correction on the title layer, but we want it on the main background

        titleLayer.enableColorCorrection(false);
        backgroundLayer.enableColorCorrection(true);

        // Starting an effect might need to draw, so we need to set the leds up before doing so
        std::static_pointer_cast<LEDMatrixGFX>(devices[0])->setLeds(GetMatrixBackBuffer());
    }

    static void SetBrightness(byte amount)
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

    __attribute__((always_inline)) uint16_t xy(uint16_t x, uint16_t y) const noexcept override
    {
        // Note the x,y are unsigned so can't be less than zero
        if (x < _width && y < _height)
            return y * MATRIX_WIDTH + x;

        debugE("%s", str_sprintf("Invalid index in xy: x=%d, y=%d, NUM_LEDS=%d", x, y, NUM_LEDS).c_str());
        return 0;
    }

    // Whereas an LEDStripGFX would track its own memory for the CRGB array, we simply point to the buffer already used for
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

        if (color.g == color.r && color.r == color.b)
        {
            memset((void *) leds, color.r, sizeof(CRGB) * _ledcount);
            memset((void *) backgroundLayer.backBuffer(), color.r, sizeof(LEDMatrixGFX::SM_RGB) * _ledcount);
        }
        else 
        {
            SM_RGB* buf = (SM_RGB*)backgroundLayer.backBuffer();
            for (int i = 0; i < _ledcount; ++i) 
            {
                buf[i]  = rgb24(color.r, color.g, color.b);
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