#pragma once
//+--------------------------------------------------------------------------
//
// File:        faneffects_fire.h
//
// NightDriverStrip - (c) 2018 Plummer's Software LLC.  All Rights Reserved.
//
// This file is part of faneffects.h; see that file header for additional context.
//
// Split scope: fan fire simulation base and concrete fire variants.
//---------------------------------------------------------------------------
//

#include "effects.h"
#include "effects/strip/fan_geometry.h"
#include "random_utils.h"

template<typename TEffect>
class FireFanEffectBase : public EffectWithId<TEffect>
{
protected:
  CRGBPalette16 Palette;
  int LEDCount;
  int CellsPerLED;
  float Cooling;
  int Sparks;
  int SparkHeight;
  uint8_t Sparking;
  bool bReversed;
  bool bMirrored;
  bool bMulticolor;
  uint8_t MaxSparkTemp;

  PixelOrder Order;

  allocated_unique_ptr<uint8_t[]> abHeat;

  static const uint8_t BlendSelf = 0;
  static const uint8_t BlendNeighbor1 = 1;
  static const uint8_t BlendNeighbor2 = 1;
  static const uint8_t BlendNeighbor3 = 0;

  static const uint8_t BlendTotal = (BlendSelf + BlendNeighbor1 + BlendNeighbor2 + BlendNeighbor3);

  int CellCount() const { return LEDCount * CellsPerLED; }

public:

  FireFanEffectBase(CRGBPalette16 palette,
                    int ledCount,
                    int cellsPerLED = 1,
                    float cooling = 20,
                    uint8_t sparking = 100,
                    int sparks = 3,
                    int sparkHeight = 4,
                    PixelOrder order = Sequential,
                    bool breversed = false,
                    bool bmirrored = false,
                    bool bmulticolor = false,
                    uint8_t maxSparkTemp = 255)
  : EffectWithId<TEffect>("FireFanEffect"),
        Palette(palette),
        LEDCount(ledCount),
        CellsPerLED(cellsPerLED),
        Cooling(cooling),
        Sparks(sparks),
        SparkHeight(sparkHeight),
        Sparking(sparking),
        bReversed(breversed),
        bMirrored(bmirrored),
        Order(order),
        bMulticolor(bmulticolor),
        MaxSparkTemp(maxSparkTemp)
  {
    if (bMirrored)
      LEDCount = LEDCount / 2;
    abHeat = make_unique_psram<uint8_t[]>(CellCount());
  }

  FireFanEffectBase(const JsonObjectConst& jsonObject)
      : EffectWithId<TEffect>(jsonObject),
        Palette(jsonObject[PTY_PALETTE].as<CRGBPalette16>()),
        LEDCount(jsonObject[PTY_LEDCOUNT]),
        CellsPerLED(jsonObject[PTY_CELLSPERLED]),
        Cooling(jsonObject[PTY_COOLING]),
        Sparks(jsonObject[PTY_SPARKS]),
        SparkHeight(jsonObject[PTY_SPARKHEIGHT]),
        Sparking(jsonObject[PTY_SPARKING]),
        bReversed(jsonObject[PTY_REVERSED]),
        bMirrored(jsonObject[PTY_MIRORRED]),
        Order((PixelOrder)jsonObject[PTY_ORDER]),
        bMulticolor(jsonObject[PTY_MULTICOLOR] == 1),
        MaxSparkTemp(jsonObject[PTY_SPARKTEMP])
  {
    abHeat = make_unique_psram<uint8_t[]>(CellCount());
  }

  bool SerializeToJSON(JsonObject& jsonObject) override
  {
    auto jsonDoc = CreateJsonDocument();

    JsonObject root = jsonDoc.to<JsonObject>();
    LEDStripEffect::SerializeToJSON(root);

    jsonDoc[PTY_PALETTE] = Palette;
    jsonDoc[PTY_LEDCOUNT] = LEDCount;
    jsonDoc[PTY_CELLSPERLED] = CellsPerLED;
    jsonDoc[PTY_COOLING] = Cooling;
    jsonDoc[PTY_SPARKS] = Sparks;
    jsonDoc[PTY_SPARKHEIGHT] = SparkHeight;
    jsonDoc[PTY_SPARKTEMP] = MaxSparkTemp;
    jsonDoc[PTY_SPARKING] = Sparking;
    jsonDoc[PTY_REVERSED] = bReversed;
    jsonDoc[PTY_MIRORRED] = bMirrored;
    jsonDoc[PTY_ORDER] = to_value(Order);
    jsonDoc[PTY_MULTICOLOR] = bMulticolor ? 1 : 0;

    return SetIfNotOverflowed(jsonDoc, jsonObject, __PRETTY_FUNCTION__);
  }

  CRGB GetBlackBodyHeatColorByte(uint8_t temp) const
  {
    return ColorFromPalette(Palette, temp, 255);
  }

  void Draw() override
  {
    FastLED.clear(false);
    DrawFire(Order);
  }

  size_t DesiredFramesPerSecond() const override
  {
    return 60;
  }

  virtual void DrawFire(PixelOrder order = Sequential)
  {
    EVERY_N_MILLISECONDS(50)
    {
      for (int i = 0; i < CellCount(); i++)
      {
        float coolingAmount = random_range(0.0f, Cooling);
        abHeat[i] = ::max(0.0, abHeat[i] - (double) coolingAmount);
      }
    }

    EVERY_N_MILLISECONDS(20)
    {
      for (int i = 0; i < CellCount(); i++)
        abHeat[i] = min(255, (abHeat[i] * BlendSelf +
                              abHeat[(i + 1) % CellCount()] * BlendNeighbor1 +
                              abHeat[(i + 2) % CellCount()] * BlendNeighbor2 +
                              abHeat[(i + 3) % CellCount()] * BlendNeighbor3) /
                                 BlendTotal);
    }

    EVERY_N_MILLISECONDS(20)
    {
      for (int i = 0; i < Sparks; i++)
      {
        if (random(255) < Sparking)
        {
          int y = CellCount() - 1 - random(SparkHeight * CellsPerLED);
          abHeat[y] = ::min((long)MaxSparkTemp, abHeat[y] + random(0, MaxSparkTemp));
        }
      }
    }

    constexpr auto num_channels = max(1, NUM_CHANNELS);

    for (int i = 0; i < LEDCount; i++)
    {
      for (int iChannel = 0; iChannel < num_channels; iChannel++)
      {
        CRGB color = GetBlackBodyHeatColorByte(abHeat[i * CellsPerLED]);

        if (bMulticolor)
        {
            CHSV hsv = rgb2hsv_approximate(color);
                 hsv.hue += iChannel * (255/num_channels);
            color = hsv;
        }

        int j = (!bReversed || i > FAN_SIZE) ? i : LEDCount - 1 - i;
        uint x = GetFanPixelOrder(j, order);
        if (x < NUM_LEDS)
        {
            FastLED[iChannel][x] = color;

            if (bMirrored)
            {
                FastLED[iChannel][bReversed ? (2 * LEDCount - 1 - i) : LEDCount + i] = color;
            }
        }

      }
    }
  }
};

class FireFanEffect : public FireFanEffectBase<FireFanEffect>
{
public:
    using FireFanEffectBase<FireFanEffect>::FireFanEffectBase;
};

class BlueFireFanEffect : public FireFanEffectBase<BlueFireFanEffect>
{
public:
  using FireFanEffectBase<BlueFireFanEffect>::FireFanEffectBase;

  virtual CRGB MapHeatToColor(uint8_t temperature, int iChannel = 0)
  {
    uint8_t t192 = round((temperature / 255.0) * 191);
    uint8_t heatramp = t192 & 0x3F;
    heatramp <<= 2;

    CHSV hsv(HUE_BLUE, 255, heatramp);
    CRGB rgb;
    hsv2rgb_rainbow(hsv, rgb);
    return rgb;
  }
};

class GreenFireFanEffect : public FireFanEffectBase<GreenFireFanEffect>
{
public:
  using FireFanEffectBase<GreenFireFanEffect>::FireFanEffectBase;
  virtual CRGB MapHeatToColor(uint8_t temperature, int iChannel = 0)
  {
    uint8_t t192 = round((temperature / 255.0) * 191);
    uint8_t heatramp = t192 & 0x3F;
    heatramp <<= 2;

    CHSV hsv(HUE_GREEN, 255, heatramp);
    CRGB rgb;
    hsv2rgb_rainbow(hsv, rgb);
    return rgb;
  }
};
