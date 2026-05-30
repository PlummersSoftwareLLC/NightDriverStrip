#pragma once
//+--------------------------------------------------------------------------
//
// File:        faneffects_test.h
//
// NightDriverStrip - (c) 2018 Plummer's Software LLC.  All Rights Reserved.
//
// This file is part of faneffects.h; see that file header for additional context.
//
// Split scope: fan diagnostics, color sanity checks, and ring test effects.
//---------------------------------------------------------------------------
//

#include "effects.h"
#include "effects/strip/fan_geometry.h"

class RGBRollAround : public EffectWithId<RGBRollAround>
{
private:
    int iRotate = 0;

public:
  using EffectWithId<RGBRollAround>::EffectWithId;

  virtual void DrawColor(CRGB color, int phase)
  {
    const int lineLen = FAN_SIZE;
    int q = beatsin16(24, 0, NUM_LEDS - lineLen, 0, phase);
    DrawFanPixels(q, lineLen, color, BottomUp);
  }

  void Draw() override
  {
    FastLED.clear();
    DrawColor(CRGB::Red, 0);
    DrawColor(CRGB::Green, 16383);
    DrawColor(CRGB::Blue, 32767);
  }
};

class HueTest : public EffectWithId<HueTest>
{
private:

    int iRotate = 0;

public:
  using EffectWithId<HueTest>::EffectWithId;

  void Draw() override
  {
    FastLED.clear();
    int iFan = 0;
    for (int sat = 255; sat >= 0 && iFan < NUM_FANS; sat -= 32)
    {
      DrawFanPixels(0, FAN_SIZE, CRGB(CHSV(HUE_RED, sat, 255)), Sequential, iFan++);
    }
  }
};

class RingTestEffect : public EffectWithId<RingTestEffect>
{
  public:
  // ID provided by EffectWithId

    RingTestEffect() : EffectWithId<RingTestEffect>("Ring Test") {}
    RingTestEffect(const JsonObjectConst& jsonObject) : EffectWithId<RingTestEffect>(jsonObject) {}

    void Draw() override
    {
      for (int i = 0; i < NUM_FANS; i++)
      {
        for (int c = 0; c < NUM_RINGS; c++)
        {
          FillRingPixels(CRGB(CHSV(c * 16, 255, 255)), i, c);
        }
      }
    }
};
