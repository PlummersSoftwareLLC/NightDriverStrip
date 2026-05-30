#pragma once
//+--------------------------------------------------------------------------
//
// File:        fan_geometry.h
//
// NightDriverStrip - (c) 2018 Plummer's Software LLC.  All Rights Reserved.
//
// This file is part of faneffects.h; see that file header for additional context.
//
// Split scope: shared fan/ring pixel ordering and drawing helpers.
//---------------------------------------------------------------------------
//

#include <algorithm>
#include <cmath>

#include "ledstripeffect.h"

enum PixelOrder
{
    Sequential = 0,
    Reverse = 1,
    BottomUp = 2,
    TopDown = 4,
    LeftRight = 8,
    RightLeft = 16
};

inline void RotateForward(int iStart, int length = FAN_SIZE, int count = 1)
{
    std::rotate(&FastLED.leds()[iStart], &FastLED.leds()[iStart + count], &FastLED.leds()[iStart + length]);
}

inline void RotateReverse(int iStart, int length = FAN_SIZE, int count = 1)
{
    std::rotate(&FastLED.leds()[iStart], &FastLED.leds()[iStart + length - count], &FastLED.leds()[iStart + length]);
}

// Rotate all the pixels in the buffer forward or back.
inline void RotateAll(bool bForward = true, int count = 1)
{
    if (bForward)
        RotateForward(0, count);
    else
        RotateReverse(0, count);
}

// Rotate one circular section within itself, like a single fan.
inline void RotateFan(int iFan, bool bForward = true, int count = 1)
{
    if (bForward)
        RotateForward(iFan * FAN_SIZE, FAN_SIZE, count);
    else
        RotateReverse(iFan * FAN_SIZE, FAN_SIZE, count);
}

// For a 24 LED ring this returns 0, 23, 1, 22, 2, 21, ...
inline int16_t GetRingPixelPosition(float fPos, int16_t ringSize)
{
    if (fPos < 0)
    {
        debugW("GetRingPixelPosition called with negative value %f", fPos);
        return 0;
    }

    int pos = fPos;
    if (pos & 1)
        return ringSize - 1 - pos / 2;

    return pos / 2;
}

// Returns the sequential strip position for fan index + direction.
inline int GetFanPixelOrder(int iPos, PixelOrder order = Sequential)
{
    if (iPos < 0)
        debugW("Calling GetFanPixelOrder with negative index: %d", iPos);

    while (iPos < 0)
        iPos += FAN_SIZE;

    if (iPos >= NUM_FANS * FAN_SIZE)
    {
        if (order == TopDown)
            return NUM_LEDS - 1 - (iPos - NUM_FANS * FAN_SIZE);

        return iPos;
    }

    int fanPos = iPos % FAN_SIZE;
    int fanBase = iPos - fanPos;

    switch (order)
    {
        case BottomUp:
            return fanBase + ((GetRingPixelPosition(fanPos, RING_SIZE_0) + LED_FAN_OFFSET_BU) % FAN_SIZE);

        case TopDown:
            return fanBase + ((GetRingPixelPosition(fanPos, RING_SIZE_0) + LED_FAN_OFFSET_TD) % FAN_SIZE);

        case LeftRight:
            return fanBase + ((GetRingPixelPosition(fanPos, RING_SIZE_0) + LED_FAN_OFFSET_LR) % FAN_SIZE);

        case RightLeft:
            return fanBase + ((GetRingPixelPosition(fanPos, RING_SIZE_0) + LED_FAN_OFFSET_RL) % FAN_SIZE);

        case Reverse:
            return NUM_LEDS - 1 - fanPos;

        case Sequential:
        default:
            return fanBase + fanPos;
    }
}

// Clears pixels logically into a fan bank in a selected direction.
inline void ClearFanPixels(float fPos, float count, PixelOrder order = Sequential, int iFan = 0)
{
    fPos += iFan * FAN_SIZE;
    while (count > 0)
    {
        for (int i = 0; i < NUM_CHANNELS; i++)
            FastLED[i][GetFanPixelOrder(fPos + static_cast<int>(count), order)] = CRGB::Black;

        count--;
    }
}

inline int GetRingSize(int iRing)
{
    return g_aRingSizeTable[iRing];
}

inline int GetFanIndex(float fPos)
{
    return fPos / FAN_SIZE;
}

inline int GetRingIndex(float fPos)
{
    fPos = fmod(fPos, FAN_SIZE);
    int iRing = 0;
    do
    {
        if (fPos < GetRingSize(iRing))
            return iRing;

        fPos -= GetRingSize(iRing);
        iRing++;
    } while (iRing < NUM_RINGS);

    return iRing;
}

inline int GetRingPos(float fPos)
{
    fPos = fmod(fPos, FAN_SIZE);
    for (int iRing = 0; iRing < NUM_RINGS; iRing++)
    {
        if (fPos < GetRingSize(iRing))
            return fPos;

        fPos -= GetRingSize(iRing);
    }

    return 0;
}

inline void DrawFanPixels(float fPos, float count, CRGB color, PixelOrder order = Sequential, int iFan = 0)
{
    fPos += iFan * FAN_SIZE;

    if (fPos + count > NUM_LEDS)
    {
        debugE("DrawFanPixels called with fPos=%f, count=%f, but there are only %d LEDs", fPos, count, NUM_LEDS);
        return;
    }

    if (count < 0)
    {
        debugE("Negative count in DrawFanPixels");
        return;
    }

    float availFirstPixel = 1.0f - (fPos - static_cast<long>(fPos));
    float amtFirstPixel = min(availFirstPixel, count);
    float remaining = min(count, FastLED.size() - fPos);
    int iPos = fPos;

    if (remaining > 0.0f && amtFirstPixel > 0.0f && iPos < NUM_LEDS)
    {
        for (int i = 0; i < NUM_CHANNELS; i++)
        {
            auto index = GetFanPixelOrder(iPos, order);
            CRGB newColor = LEDStripEffect::ColorFraction(color, amtFirstPixel);
            auto l = FastLED[i][index];
            l += newColor;
            FastLED[i][index] = l;
        }

        iPos++;
        remaining -= amtFirstPixel;
    }

    while (remaining > 1.0f && iPos < NUM_LEDS)
    {
        for (int i = 0; i < NUM_CHANNELS; i++)
            FastLED[i][GetFanPixelOrder(iPos, order)] += color;

        iPos++;
        remaining--;
    }

    if (remaining > 0.0f)
    {
        for (int i = 0; i < NUM_CHANNELS; i++)
            FastLED[i][GetFanPixelOrder(iPos, order)] += LEDStripEffect::ColorFraction(color, remaining);
    }
}

inline void DrawRingPixels(float fPos, float count, CRGB color, int iInsulator, int iRing, bool bMerge = true)
{
    int bPos = 0;
    for (int i = 0; i < iRing; i++)
        bPos += g_aRingSizeTable[i];

    bPos += iInsulator * FAN_SIZE;

    if (bPos + fPos + count > NUM_LEDS + 1)
    {
        debugE("DrawFanPixels called with fPos=%f, count=%f, but there are only %d LEDs", fPos, count, NUM_LEDS);
        return;
    }

    if (count < 0)
    {
        debugE("Negative count in DrawFanPixels");
        return;
    }

    float availFirstPixel = 1.0f - (fPos - static_cast<long>(fPos));
    float amtFirstPixel = min(availFirstPixel, count);
    float remaining = min(count, FastLED.size() - fPos);
    int iPos = fPos;

    iPos %= GetRingSize(iRing);
    if (remaining > 0.0f && amtFirstPixel > 0.0f)
    {
        for (int i = 0; i < NUM_CHANNELS; i++)
        {
            if (!bMerge)
                FastLED[i][bPos + iPos] = CRGB::Black;

            FastLED[i][bPos + iPos++] += LEDStripEffect::ColorFraction(color, amtFirstPixel);
        }

        remaining -= amtFirstPixel;
    }

    while (remaining > 1.0f)
    {
        for (int i = 0; i < NUM_CHANNELS; i++)
        {
            iPos %= GetRingSize(iRing);
            if (!bMerge)
                FastLED[i][bPos + iPos] = CRGB::Black;

            FastLED[i][bPos + iPos++] += color;
        }

        remaining--;
    }

    iPos %= GetRingSize(iRing);
    if (remaining > 0.0f)
    {
        for (int i = 0; i < NUM_CHANNELS; i++)
        {
            if (!bMerge)
                FastLED[i][bPos + iPos] = CRGB::Black;

            FastLED[i][bPos + iPos++] += LEDStripEffect::ColorFraction(color, remaining);
        }
    }
}

inline void FillRingPixels(CRGB color, int iInsulator, int iRing)
{
    DrawRingPixels(0, g_aRingSizeTable[iRing], color, iInsulator, iRing);
}
