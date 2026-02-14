//+--------------------------------------------------------------------------
//
// File:        hub75gfx.cpp
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
// Description:
//
//    Code for handling HUB75 matrix panels with the SmartMatrix library
//
// History:     May-24-2021         Davepl      Commented
//
//---------------------------------------------------------------------------

#include "globals.h"

#if USE_HUB75

#include <algorithm>

#include "deviceconfig.h"
#include "effectmanager.h"
#include "hub75gfx.h"
#include "ledstripeffect.h"
#include "soundanalyzer.h"
#include "systemcontainer.h"
#include "values.h"

// The declarations create the "layers" that make up the matrix display

SMLayerBackground<HUB75GFX::SM_RGB, HUB75GFX::kBackgroundLayerOptions> HUB75GFX::backgroundLayer(kMatrixWidth, kMatrixHeight);
SMLayerBackground<HUB75GFX::SM_RGB, HUB75GFX::kBackgroundLayerOptions> HUB75GFX::titleLayer(kMatrixWidth, kMatrixHeight);
SmartMatrixHub75Calc<COLOR_DEPTH, HUB75GFX::kMatrixWidth, HUB75GFX::kMatrixHeight, HUB75GFX::kPanelType, HUB75GFX::kMatrixOptions> HUB75GFX::matrix;

HUB75GFX::HUB75GFX(size_t w, size_t h) : GFXBase(w, h)
{
}

HUB75GFX::~HUB75GFX()
{
}

void HUB75GFX::InitializeHardware(std::vector<std::shared_ptr<GFXBase>>& devices)
{
    StartMatrix();

    for (int i = 0; i < NUM_CHANNELS; i++)
    {
        auto tmp_matrix = make_shared_psram<HUB75GFX>(MATRIX_WIDTH, MATRIX_HEIGHT);
        devices.push_back(tmp_matrix);
        tmp_matrix->loadPalette(0);
    }

    // We don't need color correction on the title layer, but we want it on the main background

    titleLayer.enableColorCorrection(false);
    backgroundLayer.enableColorCorrection(true);

    // Starting an effect might need to draw, so we need to set the leds up before doing so
    std::static_pointer_cast<HUB75GFX>(devices[0])->setLeds(GetMatrixBackBuffer());
}

void HUB75GFX::SetBrightness(byte amount)
{
    matrix.setBrightness(amount);
}

// EstimatePowerDraw
//
// Estimate the total power load for the board and matrix by walking the pixels and adding our previously measured
// power draw per pixel based on what color and brightness each pixel is

int HUB75GFX::EstimatePowerDraw()
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

// Whereas an WS281xGFX would track its own memory for the CRGB array, we simply point to the buffer already used for
// the matrix display memory.  That also eliminated having a local draw buffer that is then copied, because the effects
// can render directly to the right back buffer automatically.

void HUB75GFX::setLeds(CRGB *pLeds)
{
    leds = pLeds;
}

void HUB75GFX::fillLeds(std::unique_ptr<CRGB []> & pLEDs)
{
    // A mesmerizer panel has the same layout as in memory, so we can memcpy.

    memcpy(leds, pLEDs.get(), sizeof(CRGB) * GetLEDCount());
}

void HUB75GFX::Clear(CRGB color)
{
    // NB: We directly clear the backbuffer because otherwise effects would start with a snapshot of the effect
    //     before them on the next buffer swap.  So we clear the backbuffer and then the leds, which point to
    //     the current front buffer.  TLDR:  We clear both the front and back buffers to avoid flicker between effects.

    if (color.g == color.r && color.r == color.b)
    {
        memset((void *) leds, color.r, sizeof(CRGB) * _ledcount);
        memset((void *) backgroundLayer.backBuffer(), color.r, sizeof(HUB75GFX::SM_RGB) * _ledcount);
    }
    else
    {
        SM_RGB* buf = (SM_RGB*)backgroundLayer.backBuffer();
        for (size_t i = 0; i < _ledcount; ++i)
        {
            buf[i]  = rgb24(color.r, color.g, color.b);
            leds[i] = color;
        }
    }
}

const String & HUB75GFX::GetCaption()
{
    return strCaption;
}

float HUB75GFX::GetCaptionTransparency()
{
    unsigned long now = millis();
    if (strCaption == nullptr)
        return 0.0f;

    if (now > (captionStartTime + totalCaptionDuration))
        return 0.0f;

    float elapsed = now - captionStartTime;

    if (elapsed < captionFadeInTime)
        return elapsed / captionFadeInTime;

    if (elapsed > captionFadeInTime + captionDuration)
        return 1.0f - ((elapsed - captionFadeInTime - captionDuration) / captionFadeOutTime);

    return 1.0f;
}

void HUB75GFX::SetCaption(const String & str, uint32_t duration)
{
    captionDuration = (float)duration;
    totalCaptionDuration = captionFadeInTime + captionDuration + captionFadeOutTime;
    strCaption = str;
    captionStartTime = millis();
}

void HUB75GFX::MoveInwardX(int startY, int endY)
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

void HUB75GFX::MoveOutwardsX(int startY, int endY)
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

void HUB75GFX::StartMatrix()
{
    matrix.addLayer(&backgroundLayer);
    matrix.addLayer(&titleLayer);

    // When the matrix starts, you can ask it to leave N bytes of memory free, and this amount must be tuned.  Too much free
    // will cause a dim panel with a low refresh, too little will starve other things.  We currently have enough RAM for
    // use so begin() is not being called with a reserve parameter, but it can be if memory becomes scarce.

    matrix.setCalcRefreshRateDivider(MATRIX_CALC_DIVIDER);
    matrix.setRefreshRate(MATRIX_REFRESH_RATE);
    matrix.setMaxCalculationCpuPercentage(95);
    matrix.begin();

    Serial.printf("Matrix Refresh Rate: %lu\n", (unsigned long)matrix.getRefreshRate());

    //backgroundLayer.setRefreshRate(100);
    backgroundLayer.fillScreen(rgb24(0, 64, 0));
    backgroundLayer.setFont(font6x10);
    backgroundLayer.drawString(8, kMatrixHeight / 2 - 6, rgb24(255, 255, 255), "NightDriver");
    backgroundLayer.swapBuffers(false);

    matrix.setBrightness(255);
}

void HUB75GFX::PrepareFrame()
{
    // We treat the internal matrix buffer as our own little playground to draw in, but that assumes they're
    // both 24-bits RGB triplets.  Or at least the same size!

    static_assert(sizeof(CRGB) == sizeof(SM_RGB), "Code assumes 24 bits in both places");

    EVERY_N_MILLIS(MILLIS_PER_FRAME)
    {
        auto graphics = g_ptrSystem->GetEffectManager().g();

        matrix.setCalcRefreshRateDivider(MATRIX_CALC_DIVIDER);
        matrix.setRefreshRate(MATRIX_REFRESH_RATE);

        auto pMatrix = std::static_pointer_cast<HUB75GFX>(g_ptrSystem->GetEffectManager().GetBaseGraphics()[0]);
        pMatrix->setLeds(GetMatrixBackBuffer());

        // We set ourselves to the lower of the fader value or the brightness value,
        // so that we can fade between effects without having to change the brightness
        // setting.

        if (g_ptrSystem->GetEffectManager().GetCurrentEffect().ShouldShowTitle() && pMatrix->GetCaptionTransparency() > 0.00)
        {
            titleLayer.setFont(font3x5);
            uint8_t brite = (uint8_t)(pMatrix->GetCaptionTransparency() * 255.0);
            debugV("Caption: %d", brite);

            rgb24 chromaKeyColor = rgb24(255, 0, 255);
            rgb24 shadowColor = rgb24(0, 0, 0);
            rgb24 titleColor = rgb24(255, 255, 255);

            titleLayer.setChromaKeyColor(chromaKeyColor);
            titleLayer.setFont(font6x10);

            const size_t kCharWidth = 6;
            const size_t kCharHeight = 10;

            const auto caption = pMatrix->GetCaption();

            int y = MATRIX_HEIGHT - 2 - kCharHeight;
            int w = caption.length() * kCharWidth;

            unsigned long elapsed = millis() - captionStartTime;

            int x;
            if (w > MATRIX_WIDTH)
            {
                // Scroll if too wide to fit
                float progress = (float)elapsed / totalCaptionDuration;
                x = MATRIX_WIDTH - (int)(progress * (w + MATRIX_WIDTH));
            }

            else
            {
                // Center if it fits
                x = (MATRIX_WIDTH / 2) - (w / 2) + 1;
            }

            // Generic fill that's way faster than the rectangle base impl
            for (int i = y * _width; i < (y + 1 + kCharHeight) * _width; ++i)
                titleLayer.backBuffer()[i] = chromaKeyColor;

            auto szCaption = caption.c_str();
            titleLayer.drawString(x - 1, y, shadowColor, szCaption);
            titleLayer.drawString(x + 1, y, shadowColor, szCaption);
            titleLayer.drawString(x, y - 1, shadowColor, szCaption);
            titleLayer.drawString(x, y + 1, shadowColor, szCaption);
            titleLayer.drawString(x, y, titleColor, szCaption);

            // We enable the chromakey overlay just for the strip of screen where it appears.  This support is only
            // present in the private fork of SmartMatrix that is linked to the mesmerizer project.

            titleLayer.swapBuffers(false);
            titleLayer.enableChromaKey(true, y, y + kCharHeight);
            titleLayer.setBrightness(brite); // 255 would obscure it entirely
        }
        else
        {
            titleLayer.enableChromaKey(false);
            titleLayer.setBrightness(0);
        }


    }
}

// PostProcessFrame
//
// Things we do with the matrix after rendering a frame, such as setting the brightness and swapping the backbuffer forward

void HUB75GFX::PostProcessFrame(uint16_t localPixelsDrawn, uint16_t wifiPixelsDrawn)
{
    // If we drew no pixels, there's nothing to post process
    if ((localPixelsDrawn + wifiPixelsDrawn) == 0)
        return;

    auto pMatrix = std::static_pointer_cast<HUB75GFX>(g_ptrSystem->GetEffectManager().g());

    constexpr auto kCaptionPower = 500;                                                 // A guess as the power the caption will consume
    g_Values.MatrixPowerMilliwatts = pMatrix->EstimatePowerDraw();                             // What our drawn pixels will consume

    if (pMatrix->GetCaptionTransparency() > 0)
        g_Values.MatrixPowerMilliwatts += kCaptionPower;

    const double kMaxPower = g_ptrSystem->GetDeviceConfig().GetPowerLimit();
    uint8_t scaledBrightness = std::clamp(kMaxPower / g_Values.MatrixPowerMilliwatts, 0.0, 1.0) * 255;

    // If the target brightness is lower than current, we drop to it immediately, but if it is higher, we ramp the brightness back in
    // somewhat slowly to avoid flicker.  We do this by using a weighted average of the current and former brightness.  To avoid
    // an asymptote near the max, we always increase by at least one step if we're lower than the target.

    constexpr auto kWeightedAverageAmount = 10;
    if (scaledBrightness <= g_Values.MatrixScaledBrightness)
        g_Values.MatrixScaledBrightness = scaledBrightness;
    else
        g_Values.MatrixScaledBrightness = std::max(g_Values.MatrixScaledBrightness + 1,
                                                    (( g_Values.MatrixScaledBrightness * (kWeightedAverageAmount-1) ) + scaledBrightness) / kWeightedAverageAmount);

    // We set ourselves to the lower of the fader value or the brightness value, or the power constrained value,
    // whichever is lowest, so that we can fade between effects without having to change the brightness setting.

    auto targetBrightness = min({ g_ptrSystem->GetDeviceConfig().GetBrightness(), g_Values.Fader, g_Values.MatrixScaledBrightness });

    debugV("MW: %lu, Setting Scaled Brightness to: %lu", (unsigned long)g_Values.MatrixPowerMilliwatts, (unsigned long)targetBrightness);
    pMatrix->SetBrightness(targetBrightness);

    #if SHOW_FPS_ON_MATRIX
        // Display status on bottom of matrix in format FPS: 00 CPU0: 000 CPU1: 000 Aud: 00
        backgroundLayer.setFont(font3x5);
        auto& taskManager = g_ptrSystem->GetTaskManager();
        String output = "LED: " + String(g_Values.FPS) + " AUD: " + String(g_Analyzer.AudioFPS());
        backgroundLayer.drawString(2, MATRIX_HEIGHT  - 12, rgb24(255, 255, 255), rgb24(0, 0, 0), output.c_str());
        output = "CP0: " + String((int)taskManager.GetCPUUsagePercent(0)) + " CP1: " + String((int)taskManager.GetCPUUsagePercent(1));
        backgroundLayer.drawString(2, MATRIX_HEIGHT  - 6, rgb24(255, 255, 255), rgb24(0, 0, 0), output.c_str());
    #endif

    MatrixSwapBuffers((wifiPixelsDrawn > 0) || g_ptrSystem->GetEffectManager().GetCurrentEffect().RequiresDoubleBuffering() || pMatrix->GetCaptionTransparency() > 0.0);

    FastLED.countFPS();
}

CRGB *HUB75GFX::GetMatrixBackBuffer()
{
    for (auto& device : g_ptrSystem->GetDevices())
        device->UpdatePaletteCycle();

    return (CRGB *)backgroundLayer.backBuffer();
}

void HUB75GFX::MatrixSwapBuffers(bool bSwapBackground)
{
    // If an effect redraws itself entirely ever frame, it can skip saving the most recent buffer, so
    // can swap without waiting for a copy.
    matrix.setCalcRefreshRateDivider(MATRIX_CALC_DIVIDER);
    matrix.setRefreshRate(MATRIX_REFRESH_RATE);
    matrix.setMaxCalculationCpuPercentage(95);

    backgroundLayer.swapBuffers(bSwapBackground);
}

#endif
