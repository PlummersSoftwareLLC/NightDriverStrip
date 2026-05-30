#pragma once
//+--------------------------------------------------------------------------
//
// File:        faneffects_reel.h
//
// NightDriverStrip - (c) 2018 Plummer's Software LLC.  All Rights Reserved.
//
// This file is part of faneffects.h; see that file header for additional context.
//
// Split scope: reel-style fan effects and palette reel/spin variants.
//---------------------------------------------------------------------------
//

#include <cmath>

#include "effects.h"
#include "effects/strip/fan_geometry.h"
#include "paletteeffect.h"
#include "random_utils.h"
#include "soundanalyzer.h"

class TapeReelEffect : public EffectWithId<TapeReelEffect>
{
private:

  float ReelPos[NUM_FANS] = {0};
  float ReelDir[NUM_FANS] = {0};

public:

  TapeReelEffect(const String & strName) : EffectWithId<TapeReelEffect>(strName) {}
  TapeReelEffect(const JsonObjectConst& jsonObject) : EffectWithId<TapeReelEffect>(jsonObject) {}

  void Draw() override
  {
    EVERY_N_MILLISECONDS(250)
    {
      for (int i = 0; i < NUM_FANS; i++)
      {
        if (random(0, 100) < 40)
        {
          int action = random(0, 3);
          if (action == 0)
          {
            ReelDir[i] = 0;
          }
          else if (action == 1)
          {
            if (ReelDir[i] == 0)
            {
              ReelDir[i] = -1;
            }
            else
            {
              ReelDir[i] -= .5f;
            }
          }
          else if (action == 2)
          {
            if (ReelDir[i] == 0)
            {
              ReelDir[i] = 1;
            }
            else
            {
              ReelDir[i] += .5f;
            }
          }
        }
      }
    }

    EVERY_N_MILLISECONDS(20)
    {
      for (int i = 0; i < NUM_FANS; i++)
      {
        ReelPos[i] = (ReelPos[i] + ReelDir[i]);
        if (ReelPos[i] < 0)
          ReelPos[i] += FAN_SIZE;
        if (ReelPos[i] >= FAN_SIZE)
          ReelPos[i] -= FAN_SIZE;
      }
    }

    EVERY_N_MILLISECONDS(20)
    {
      FastLED.clear(false);
      DrawEffect();
    }
  }

  void DrawEffect()
  {
    for (int i = 0; i < NUM_FANS; i++)
    {
      float pos = ReelPos[i];
      DrawFanPixels(i * FAN_SIZE + pos, 1, CRGB::White);
      DrawFanPixels(i * FAN_SIZE + fmod(pos + 1, FAN_SIZE), 1, CRGB::Blue);
      DrawFanPixels(i * FAN_SIZE + fmod(pos + FAN_SIZE / 2, FAN_SIZE), 1, CRGB::White);
      DrawFanPixels(i * FAN_SIZE + fmod(pos + FAN_SIZE / 2 + 1, FAN_SIZE), 1, CRGB::Blue);
    }
  }
};

class PaletteReelEffect : public EffectWithId<PaletteReelEffect>
{
private:
  float ReelPos[NUM_FANS] = {0};
  float ReelDir[NUM_FANS] = {0};
  int ColorOffset[NUM_FANS] = {0};

public:
  PaletteReelEffect(const String & strName) : EffectWithId<PaletteReelEffect>(strName) {}

  PaletteReelEffect(const JsonObjectConst& jsonObject) : EffectWithId<PaletteReelEffect>(jsonObject) {}

  void Draw() override
  {
    EVERY_N_MILLISECONDS(250)
    {
      for (int i = 0; i < NUM_FANS; i++)
      {
        if (random(0, 100) < 50 * g_Analyzer.VURatio())
        {
          int action = random(0, 3);
          if (action == 0 || action == 3)
          {
            ReelDir[i] = 0;
          }
          else if (action == 1)
          {
            if (g_Analyzer.VURatio() > 0.5f)
            {
              if (ReelDir[i] == 0)
              {
                ColorOffset[i] = random(0, 255);
                ReelDir[i] = -1;
              }
              else
              {
                ReelDir[i] -= .5f;
              }
            }
          }
          else if (action == 2)
          {
            if (g_Analyzer.VURatio() > 0.5f)
            {
              if (ReelDir[i] == 0)
              {
                ColorOffset[i] = random(0, 255);
                ReelDir[i] = 1;
              }
              else
              {
                ReelDir[i] += .5f;
              }
            }
          }
        }
      }
    }

    EVERY_N_MILLISECONDS(20)
    {
      for (int i = 0; i < NUM_FANS; i++)
      {
        ReelPos[i] = (ReelPos[i] + ReelDir[i] * (2 + g_Analyzer.VURatio()));
        if (ReelPos[i] < 0)
          ReelPos[i] += FAN_SIZE;
        if (ReelPos[i] >= FAN_SIZE)
          ReelPos[i] -= FAN_SIZE;
      }
    }

    EVERY_N_MILLISECONDS(20)
    {
      fadeAllChannelsToBlackBy(20);
      DrawEffect();
    }
  }

  void DrawEffect()
  {
    for (int i = 0; i < NUM_FANS; i++)
    {
      if (ReelDir[i] != 0)
      {
        int pos = ReelPos[i];
        ClearFanPixels(0, 16, Sequential, i);
        for (int x = 0; x < FAN_SIZE; x++)
        {
          DrawFanPixels(i * FAN_SIZE + ((pos + x) % FAN_SIZE), 1, ColorFromPalette(RainbowColors_p, ColorOffset[i] + x * 4, 255, NOBLEND));
        }
      }
    }
  }
};

class PaletteSpinEffect : public EffectWithId<PaletteSpinEffect>
{
private:
    const CRGBPalette16 _Palette;
    bool _bReplaceMagenta;
    float _sparkleChance;
    float ReelPos[NUM_FANS] = {0};
    int ColorOffset[NUM_FANS] = {0};

public:
  PaletteSpinEffect(const String &strName, const CRGBPalette16 &palette, bool bReplace, float sparkleChance = 0.0)
  : EffectWithId<PaletteSpinEffect>(strName),
        _Palette(palette),
        _bReplaceMagenta(bReplace),
        _sparkleChance(sparkleChance)
  {
  }

  PaletteSpinEffect(const JsonObjectConst& jsonObject)
      : EffectWithId<PaletteSpinEffect>(jsonObject),
        _Palette(jsonObject[PTY_PALETTE].as<CRGBPalette16>()),
        _bReplaceMagenta(jsonObject["rpm"]),
        _sparkleChance(jsonObject["sch"])
  {
  }

  bool SerializeToJSON(JsonObject& jsonObject) override
  {
    auto jsonDoc = CreateJsonDocument();

    JsonObject root = jsonDoc.to<JsonObject>();
    LEDStripEffect::SerializeToJSON(root);

    jsonDoc[PTY_PALETTE] = _Palette;
    jsonDoc["rpm"] = _bReplaceMagenta;
    jsonDoc["sch"] = _sparkleChance;

    return SetIfNotOverflowed(jsonDoc, jsonObject, __PRETTY_FUNCTION__);
  }

  void Draw() override
  {
    EVERY_N_MILLISECONDS(20)
    {
      for (int i = 0; i < NUM_FANS; i++)
      {
        ReelPos[i] = (ReelPos[i] + 0.25f);
        if (ReelPos[i] < 0)
          ReelPos[i] += FAN_SIZE;
        if (ReelPos[i] >= FAN_SIZE)
          ReelPos[i] -= FAN_SIZE;
      }
    }

    EVERY_N_MILLISECONDS(20)
    {
      fadeAllChannelsToBlackBy(20);
      DrawEffect();
    }
  }

  void DrawEffect()
  {
    for (int i = 0; i < NUM_FANS; i++)
    {
      ClearFanPixels(0, FAN_SIZE, Sequential, i);
      for (int x = 0; x < FAN_SIZE; x++)
      {
        float q = fmod(ReelPos[i] + x, FAN_SIZE);
        CRGB c = ColorFromPalette(_Palette, 255.0f * q / FAN_SIZE, 255, NOBLEND);
        if (_bReplaceMagenta && c == CRGB(CRGB::Magenta))
          c = CRGB(CHSV(beatsin8(2, 0, 255), 255, 255));
        if (random_range(0.0f, 10.f) < _sparkleChance)
          c = CRGB::White;
        DrawFanPixels(x, 1, c, Sequential, i);
      }
    }
  }
};
