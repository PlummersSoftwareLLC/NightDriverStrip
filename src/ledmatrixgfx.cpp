//+--------------------------------------------------------------------------
//
// File:        ledmatrixgfx.cpp
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
//    Code for handling HUB75 matrix panels
//
// History:     May-24-2021         Davepl      Commented
//
//---------------------------------------------------------------------------

#include "globals.h"

#if USE_HUB75

#include "effects/matrix/Boid.h"
#include "effects/matrix/Vector.h"
#include <SmartMatrix.h>
#include "ledmatrixgfx.h"
#include "systemcontainer.h"

const rgb24 defaultBackgroundColor = {0x40, 0, 0};

// The declarations create the "layers" that make up the matrix display

SMLayerBackground<LEDMatrixGFX::SM_RGB, LEDMatrixGFX::kBackgroundLayerOptions> LEDMatrixGFX::backgroundLayer(kMatrixWidth, kMatrixHeight);
SMLayerBackground<LEDMatrixGFX::SM_RGB, LEDMatrixGFX::kBackgroundLayerOptions> LEDMatrixGFX::titleLayer(kMatrixWidth, kMatrixHeight);
SmartMatrixHub75Calc<COLOR_DEPTH, LEDMatrixGFX::kMatrixWidth, LEDMatrixGFX::kMatrixHeight, LEDMatrixGFX::kPanelType, LEDMatrixGFX::kMatrixOptions> LEDMatrixGFX::matrix;

void LEDMatrixGFX::StartMatrix()
{
    matrix.addLayer(&backgroundLayer);
    matrix.addLayer(&titleLayer);

    // When the matrix starts, you can ask it to leave N bytes of memory free, and this amount must be tuned.  Too much free
    // will cause a dim panel with a low refresh, too little will starve other things.  We currently have enough RAM for
    // use so begin() is not being called with a reserve paramter, but it can be if memory becomes scarce.

    matrix.setCalcRefreshRateDivider(MATRIX_CALC_DIVIDER);
    matrix.setRefreshRate(MATRIX_REFRESH_RATE);
    matrix.setMaxCalculationCpuPercentage(95);
    matrix.begin();

    Serial.printf("Matrix Refresh Rate: %d\n", matrix.getRefreshRate());

    //backgroundLayer.setRefreshRate(100);
    backgroundLayer.fillScreen(rgb24(0, 64, 0));
    backgroundLayer.setFont(font6x10);
    backgroundLayer.drawString(8, kMatrixHeight / 2 - 6, rgb24(255, 255, 255), "NightDriver");
    backgroundLayer.swapBuffers(false);

    matrix.setBrightness(255);
}

void LEDMatrixGFX::PrepareFrame()
{
    // We treat the internal matrix buffer as our own little playground to draw in, but that assumes they're
    // both 24-bits RGB triplets.  Or at least the same size!

    static_assert(sizeof(CRGB) == sizeof(SM_RGB), "Code assumes 24 bits in both places");

    EVERY_N_MILLIS(MILLIS_PER_FRAME)
    {

        #if SHOW_FPS_ON_MATRIX
            backgroundLayer.setFont(gohufont11);
            // 3 is half char width at curret font size, 5 is half the height.
            string output = "FPS: " + std::to_string(g_Values.FPS);
            backgroundLayer.drawString(MATRIX_WIDTH / 2 - (3 * output.length()), MATRIX_HEIGHT / 2 - 5, rgb24(255, 255, 255), rgb24(0, 0, 0), output.c_str());
        #endif

        auto graphics = g_ptrSystem->EffectManager().g();

        matrix.setCalcRefreshRateDivider(MATRIX_CALC_DIVIDER);
        matrix.setRefreshRate(MATRIX_REFRESH_RATE);

        auto pMatrix = std::static_pointer_cast<LEDMatrixGFX>(g_ptrSystem->EffectManager().GetBaseGraphics());
        pMatrix->setLeds(GetMatrixBackBuffer());

        // We set ourselves to the lower of the fader value or the brightness value,
        // so that we can fade between effects without having to change the brightness
        // setting.

        if (g_ptrSystem->EffectManager().GetCurrentEffect().ShouldShowTitle() && pMatrix->GetCaptionTransparency() > 0.00)
        {
            titleLayer.setFont(font3x5);
            uint8_t brite = (uint8_t)(pMatrix->GetCaptionTransparency() * 255.0);
            debugV("Caption: %d", brite);

            rgb24 chromaKeyColor = rgb24(255, 0, 255);
            rgb24 shadowColor = rgb24(0, 0, 0);
            rgb24 titleColor = rgb24(255, 255, 255);

            titleLayer.setChromaKeyColor(chromaKeyColor);
            titleLayer.setFont(font6x10);
            titleLayer.fillScreen(chromaKeyColor);

            const size_t kCharWidth = 6;
            const size_t kCharHeight = 10;

            const auto caption = pMatrix->GetCaption();

            int y = MATRIX_HEIGHT - 2 - kCharHeight;
            int w = caption.length() * kCharWidth;
            int x = (MATRIX_WIDTH / 2) - (w / 2) + 1;

            auto szCaption = caption.c_str();
            titleLayer.drawString(x - 1, y, shadowColor, szCaption);
            titleLayer.drawString(x + 1, y, shadowColor, szCaption);
            titleLayer.drawString(x, y - 1, shadowColor, szCaption);
            titleLayer.drawString(x, y + 1, shadowColor, szCaption);
            titleLayer.drawString(x, y, titleColor, szCaption);

            // We enable the chromakey overlay just for the strip of screen where it appears.  This support is only
            // present in the private fork of SmartMatrix that is linked to the mesermizer project.

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

void LEDMatrixGFX::PostProcessFrame(uint16_t localPixelsDrawn, uint16_t wifiPixelsDrawn)
{
    // If we drew no pixels, there's nothing to post process
    if ((localPixelsDrawn + wifiPixelsDrawn) == 0)
        return;

    auto pMatrix = std::static_pointer_cast<LEDMatrixGFX>(g_ptrSystem->EffectManager().g());

    constexpr auto kCaptionPower = 500;                                                 // A guess as the power the caption will consume
    g_Values.MatrixPowerMilliwatts = pMatrix->EstimatePowerDraw();                             // What our drawn pixels will consume

    if (pMatrix->GetCaptionTransparency() > 0)
        g_Values.MatrixPowerMilliwatts += kCaptionPower;

    const double kMaxPower = g_ptrSystem->DeviceConfig().GetPowerLimit();
    uint8_t scaledBrightness = std::clamp(kMaxPower / g_Values.MatrixPowerMilliwatts, 0.0, 1.0) * 255;

    // If the target brightness is lower than current, we drop to it immediately, but if its higher, we ramp the brightness back in
    // somewhat slowly to avoid flicker.  We do this by using a weighted average of the current and former brightness.  To avoid
    // an ansymptote near the max, we always increase by at least one step if we're lower than the target.

    constexpr auto kWeightedAverageAmount = 10;
    if (scaledBrightness <= g_Values.MatrixScaledBrightness)
        g_Values.MatrixScaledBrightness = scaledBrightness;
    else
        g_Values.MatrixScaledBrightness = std::max(g_Values.MatrixScaledBrightness + 1,
                                                    (( g_Values.MatrixScaledBrightness * (kWeightedAverageAmount-1) ) + scaledBrightness) / kWeightedAverageAmount);

    // We set ourselves to the lower of the fader value or the brightness value, or the power constrained value,
    // whichever is lowest, so that we can fade between effects without having to change the brightness setting.

    auto targetBrightness = min({ g_ptrSystem->DeviceConfig().GetBrightness(), g_Values.Fader, g_Values.MatrixScaledBrightness });

    debugV("MW: %d, Setting Scaled Brightness to: %d", g_Values.MatrixPowerMilliwatts, targetBrightness);
    pMatrix->SetBrightness(targetBrightness);

    MatrixSwapBuffers(g_ptrSystem->EffectManager().GetCurrentEffect().RequiresDoubleBuffering() || pMatrix->GetCaptionTransparency() > 0.0, false);

    FastLED.countFPS();
}

CRGB *LEDMatrixGFX::GetMatrixBackBuffer()
{
    for (auto& device : g_ptrSystem->Devices())
        device->UpdatePaletteCycle();

    return (CRGB *)backgroundLayer.getRealBackBuffer();
}

void LEDMatrixGFX::MatrixSwapBuffers(bool bSwapBackground, bool bSwapTitle)
{
    // If an effect redraws itself entirely ever frame, it can skip saving the most recent buffer, so
    // can swap without waiting for a copy.
    matrix.setCalcRefreshRateDivider(MATRIX_CALC_DIVIDER);
    matrix.setRefreshRate(MATRIX_REFRESH_RATE);
    matrix.setMaxCalculationCpuPercentage(95);

    backgroundLayer.swapBuffers(bSwapBackground);
}

#endif
