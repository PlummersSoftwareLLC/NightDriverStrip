#pragma once
//+--------------------------------------------------------------------------
//
// File:        faneffects_core.h
//
// NightDriverStrip - (c) 2018 Plummer's Software LLC.  All Rights Reserved.
//
// This file is part of faneffects.h; see that file header for additional context.
//
// Split scope: foundational fan effects and beat/count primitives.
//---------------------------------------------------------------------------
//

#include "effects.h"
#include "effects/strip/fan_geometry.h"
#include "random_utils.h"
#include "soundanalyzer.h"

class EmptyEffect : public EffectWithId<EmptyEffect>
{
  using EffectWithId<EmptyEffect>::EffectWithId;

  void Draw() override
  {
    FastLED.clear(false);
    DrawEffect();
    delay(20);
  }

  void DrawEffect()
  {
  }
};

class FanBeatEffect : public EffectWithId<FanBeatEffect>
{
  public:

  FanBeatEffect(const String & strName) : EffectWithId<FanBeatEffect>(strName) {}

  FanBeatEffect(const JsonObjectConst& jsonObject) : EffectWithId<FanBeatEffect>(jsonObject) {}

  void Draw() override
  {
    fadeToBlackBy(FastLED.leds(), NUM_LEDS, 20);
    DrawEffect();
    delay(20);
  }

  void OnBeat()
  {
    int passes = (int)g_Analyzer.VURatio();
    for (int iPass = 0; iPass < passes; iPass++)
    {
      int iFan = random(0, NUM_FANS);
      int innerPasses = random(1, (int)g_Analyzer.VURatio());
      CRGB c = CHSV(random(0, 255), 255, 255);

      for (int iInnerPass = 0; iInnerPass < innerPasses; iInnerPass++)
      {
        DrawFanPixels(0, FAN_SIZE, c, Sequential, iFan++);
      }
    }

    CRGB c = CHSV(random(0, 255), 255, 255);
    for (int i = NUM_FANS * FAN_SIZE; i < NUM_LEDS; i++)
      g().setPixel(i, c);
  }

  void DrawEffect()
  {
    static bool latch = false;
    static float minVUSeen = 0.0;

    if (latch)
    {
      if (g_Analyzer.VURatio() < minVUSeen)
        minVUSeen = g_Analyzer.VURatio();
    }

    if (g_Analyzer.VURatio() < 0.25f)
    {
      latch = true;
      minVUSeen = g_Analyzer.VURatio();
    }

    if (latch)
    {
      if (g_Analyzer.VURatio() > 1.5f)
      {
        if (random_range(1.0f, 3.0f) < g_Analyzer.VURatio())
        {
          latch = false;
          OnBeat();
        }
      }
    }
  }
};

class CountEffect : public EffectWithId<CountEffect>
{
    using EffectWithId<CountEffect>::EffectWithId;

private:

  const int DRAW_LEN = 16;
  const int OPEN_LEN = NUM_FANS * FAN_SIZE - DRAW_LEN;

public:

  void Draw() override
  {
    static float i = 0;
    EVERY_N_MILLISECONDS(30)
    {
      i += 0.5f;

      if (i >= OPEN_LEN)
        i -= OPEN_LEN;

      FastLED.clear();
      float t = i;
      for (int z = 0; z < NUM_FANS; z += 3)
      {
        CRGB c = CHSV(z * 48, 255, 255);
        DrawFanPixels(t, DRAW_LEN, c, BottomUp);
        t += FAN_SIZE * 3;
        if (t >= OPEN_LEN)
          t -= OPEN_LEN;
      }

      FastLED.show();
    }
  }
};
