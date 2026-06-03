#pragma once
//+--------------------------------------------------------------------------
//
// File:        faneffects_colorcycle.h
//
// NightDriverStrip - (c) 2018 Plummer's Software LLC.  All Rights Reserved.
//
// This file is part of faneffects.h; see that file header for additional context.
//
// Split scope: fan color-cycle and rotating palette effects.
//---------------------------------------------------------------------------
//

#include "effects.h"
#include "effects/strip/fan_geometry.h"
#include "paletteeffect.h"

class ColorCycleEffect : public EffectWithId<ColorCycleEffect>
{
private:
  PixelOrder _order;
  int _step;

public:
  ColorCycleEffect(PixelOrder order = Sequential, int step = 8)
  : EffectWithId<ColorCycleEffect>("ColorCylceEffect"),
      _order(order),
      _step(step)
  {
  }

  ColorCycleEffect(const JsonObjectConst& jsonObject)
    : EffectWithId<ColorCycleEffect>(jsonObject),
      _order((PixelOrder)jsonObject[PTY_ORDER]),
      _step(jsonObject["stp"])
  {
  }

  bool SerializeToJSON(JsonObject& jsonObject) override
  {
    auto jsonDoc = CreateJsonDocument();

    JsonObject root = jsonDoc.to<JsonObject>();
    LEDStripEffect::SerializeToJSON(root);

    jsonDoc[PTY_ORDER] = to_value(_order);
    jsonDoc["stp"] = _step;

    return SetIfNotOverflowed(jsonDoc, jsonObject, __PRETTY_FUNCTION__);
  }

  void Draw() override
  {
    FastLED.clear(false);
    DrawEffect();
  }

  void DrawEffect()
  {
    static uint8_t basehue = 0;
    uint8_t hue = basehue;
    EVERY_N_MILLISECONDS(20)
    {
      basehue += 1;
    }
    for (int i = 0; i < NUM_LEDS; i++)
      DrawFanPixels(i, 1, CHSV(hue += _step, 255, 255), _order);
  }
};

class ColorCycleEffectBottomUp : public EffectWithId<ColorCycleEffectBottomUp>
{
public:
  using EffectWithId<ColorCycleEffectBottomUp>::EffectWithId;

  void Draw() override
  {
    FastLED.clear(false);
    DrawEffect();
  }

  void DrawEffect()
  {
    static uint8_t basehue = 0;
    uint8_t hue = basehue;
    EVERY_N_MILLISECONDS(20)
    {
      basehue += 2;
    }
    for (int i = 0; i < NUM_LEDS; i++)
      DrawFanPixels(i, 1, CHSV(hue += 8, 255, 255), BottomUp);
  }
};

class ColorCycleEffectTopDown : public EffectWithId<ColorCycleEffectTopDown>
{
public:
  using EffectWithId<ColorCycleEffectTopDown>::EffectWithId;

  void Draw() override
  {
    FastLED.clear(false);
    DrawEffect();
  }

  void DrawEffect()
  {
    static uint8_t basehue = 0;
    uint8_t hue = basehue;
    EVERY_N_MILLISECONDS(30)
    {
      basehue += 1;
    }
    for (int i = 0; i < NUM_LEDS; i++)
      DrawFanPixels(i, 1, CHSV(hue += 4, 255, 255), TopDown);
  }
};

class ColorCycleEffectSequential : public EffectWithId<ColorCycleEffectSequential>
{
public:
  using EffectWithId<ColorCycleEffectSequential>::EffectWithId;

  void Draw() override
  {
    FastLED.clear(false);
    DrawEffect();
  }

  void DrawEffect()
  {
    static uint8_t basehue = 0;
    uint8_t hue = basehue;
    EVERY_N_MILLISECONDS(30)
    {
      basehue += 1;
    }
    for (int i = 0; i < NUM_LEDS; i++)
      DrawFanPixels(i, 1, CHSV(hue += 4, 255, 255), Sequential);
  }
};

class SpinningPaletteEffect : public PaletteEffectBase<SpinningPaletteEffect>
{
private:

  int iRotate = 0;

public:

  using PaletteEffectBase<SpinningPaletteEffect>::PaletteEffectBase;

  void Draw() override
  {
    PaletteEffectBase<SpinningPaletteEffect>::Draw();
    for (int i = 0; i < NUM_FANS; i++)
    {
      RotateFan(i, (i / 2) * 2 == i ? true : false, iRotate);
    }
    delay(10);

    EVERY_N_MILLISECONDS(25)
    {
      iRotate = (iRotate + 1) % FAN_SIZE;
    }
  }
};

class ColorCycleEffectRightLeft : public EffectWithId<ColorCycleEffectRightLeft>
{
public:

  using EffectWithId<ColorCycleEffectRightLeft>::EffectWithId;

  void Draw() override
  {
    FastLED.clear(false);
    DrawEffect();
    delay(20);
  }

  void DrawEffect()
  {
    static uint8_t basehue = 0;
    uint8_t hue = basehue;
    basehue += 8;
    for (int i = 0; i < NUM_LEDS; i++)
      DrawFanPixels(i, 1, CHSV(hue += 16, 255, 255), RightLeft);
  }
};

class ColorCycleEffectLeftRight : public EffectWithId<ColorCycleEffectLeftRight>
{
public:

  using EffectWithId<ColorCycleEffectLeftRight>::EffectWithId;

  void Draw() override
  {
    FastLED.clear(false);
    DrawEffect();
    delay(20);
  }

  void DrawEffect()
  {
    static uint8_t basehue = 0;
    uint8_t hue = basehue;
    basehue += 8;
    for (int i = 0; i < NUM_LEDS; i++)
      DrawFanPixels(i, 1, CHSV(hue += 16, 255, 255), LeftRight);
  }
};
