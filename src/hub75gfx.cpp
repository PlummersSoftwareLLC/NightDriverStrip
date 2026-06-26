//+--------------------------------------------------------------------------
//
// File:        hub75gfx.cpp
//
// NightDriverStrip - (c) 2018 Plummer's Software LLC.  All Rights Reserved.
//
// Description:
//
//    Generic base class for HUB75 DMA LED panel drivers, providing
//    shared logic for power estimation, brightness bounding, etc.
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

#if USE_MPDMA_HUB75
#include "espmpdmagfx.h"
#endif

#if USE_ESP_HUB75
#include "esphub75gfx.h"
#endif

HUB75GFX::HUB75GFX(size_t w, size_t h) : GFXBase(w, h)
{
}

HUB75GFX::~HUB75GFX()
{
}

void HUB75GFX::InitializeHardware(std::vector<std::shared_ptr<GFXBase>>& devices)
{
#if USE_MPDMA_HUB75
    ESPMPDMAGFX::InitializeHardware(devices);
#elif USE_ESP_HUB75
    ESPHUB75GFX::InitializeHardware(devices);
#else
#error "USE_HUB75 is defined but no specific DMA driver (USE_MPDMA_HUB75 or USE_ESP_HUB75) was selected!"
#endif
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
    if (color.g == color.r && color.r == color.b)
    {
        memset((void *) leds, color.r, sizeof(CRGB) * _ledcount);
    }
    else
    {
        for (size_t i = 0; i < _ledcount; ++i)
        {
            leds[i] = color;
        }
    }
}

void HUB75GFX::MoveInwardX(int startY, int endY)
{
    // Uses knowledge of how the pixels are laid out in order to do the scroll.
    for (int y = startY; y <= endY; y++)
    {
        auto pLinemem = leds + y * MATRIX_WIDTH;
        auto pLinemem2 = pLinemem + (MATRIX_WIDTH / 2);
        memmove(pLinemem + 1, pLinemem, sizeof(CRGB) * (MATRIX_WIDTH / 2));
        memmove(pLinemem2, pLinemem2 + 1, sizeof(CRGB) * (MATRIX_WIDTH / 2));
    }
}

void HUB75GFX::MoveOutwardsX(int startY, int endY)
{
    for (int y = startY; y <= endY; y++)
    {
        auto pLinemem = leds + y * MATRIX_WIDTH;
        auto pLinemem2 = pLinemem + (MATRIX_WIDTH / 2);
        memmove(pLinemem, pLinemem + 1, sizeof(CRGB) * (MATRIX_WIDTH / 2));
        memmove(pLinemem2 + 1, pLinemem2, sizeof(CRGB) * (MATRIX_WIDTH / 2));
    }
}

void HUB75GFX::PrepareFrame()
{
}

// PostProcessFrame
//
// Things we do with the matrix after rendering a frame, such as bounding the power and setting the brightness.
// Derived classes should call this, then do their actual drawing loop.

void HUB75GFX::PostProcessFrame(uint16_t localPixelsDrawn, uint16_t wifiPixelsDrawn)
{
    // If we drew no pixels, there's nothing to post process
    if ((localPixelsDrawn + wifiPixelsDrawn) == 0)
        return;

    constexpr auto kCaptionPower = 500;                                                 // A guess as the power the caption will consume
    g_Values.MatrixPowerMilliwatts = EstimatePowerDraw();                               // What our drawn pixels will consume

    if (GetCaptionTransparency() > 0)
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
    SetBrightness(targetBrightness);
}

#endif
