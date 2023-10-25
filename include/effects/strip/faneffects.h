//+--------------------------------------------------------------------------
//
// File:        FireEffect.h
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
//    Fan effects generally have attributes specific to being arranged as
//    circles, and this code also provides a way to draw into those circles
//    in a bottom up, top down, or sideways direction as well.
//
// History:     Apr-13-2019         Davepl      Adapted from LEDWifiSocket
//
//---------------------------------------------------------------------------

#pragma once

#include <cmath>
#include "effects.h"
#include "paletteeffect.h"
#include "soundanalyzer.h"

// Simple definitions of what direction we're talking about

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

// Rotate
//
// Rotate all the pixels in the buffer forward or back

inline void RotateAll(bool bForward = true, int count = 1)
{
  if (bForward)
    RotateForward(0, count);
  else
    RotateReverse(0, count);
}

// RotateFan
//
// Rotate one circular section within itself, like a single fan

inline void RotateFan(int iFan, bool bForward = true, int count = 1)
{
  if (bForward)
    RotateForward(iFan * FAN_SIZE, FAN_SIZE, count);
  else
    RotateReverse(iFan * FAN_SIZE, FAN_SIZE, count);
}

// Get the pixel position working our way over a circle, rather than around it.
// For a 24 led ring this return 0, 23, 1, 22, 2, 21, 3, 20, 4, 19, etc...

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
  else
    return pos / 2;
}

// GetFanPixelOrder
//
// Returns the sequential strip postion of a an LED on the fans based
// on the index and direction specified, like 32nd most TopDown pixel.

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
    else
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

// ClearFanPixels
//
// Clears pixels logically into a fan bank in a direction such as top down rather than
// just straight sequential strip order

inline void ClearFanPixels(float fPos, float count, PixelOrder order = Sequential, int iFan = 0)
{
  fPos += iFan * FAN_SIZE;
  while (count > 0)
  {
    for (int i = 0; i < NUM_CHANNELS; i++)
      FastLED[i][GetFanPixelOrder(fPos + (int)count, order)] = CRGB::Black;
    count--;
  }
}

// GetRingSize
//
// Returns the size of the Nth ring

inline int GetRingSize(int iRing)
{
  return g_aRingSizeTable[iRing];
}

// GetFanIndex
//
// Given the index into NUM_LEDS, returns the index of the fan that this must belong to

inline int GetFanIndex(float fPos)
{
  return fPos / FAN_SIZE;
}

// GetRingIndex
//
// Ggiven the index into NUM_LEDS or FAN_SIZE, returns the index of the ring this must be on

inline int GetRingIndex(float fPos)
{
  fPos = fmod(fPos, FAN_SIZE);
  int iRing = 0;
  do
  {
    if (fPos < GetRingSize(iRing))
    {
      return iRing;
    }
    else
    {
      fPos -= GetRingSize(iRing);
      iRing++;
    }
  } while (iRing < NUM_RINGS);
  return iRing;
}

// GetRingPos
//
// Given the index into NUM_LEDS or FAN_SIZE, returns the index of the LED on the current ring

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

// DrawFanPixels
//
// A fan is a ring set with a single ring

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
  // Calculate how much the first pixel will hold

  float availFirstPixel = 1.0f - (fPos - (long)(fPos));
  float amtFirstPixel = min(availFirstPixel, count);
  float remaining = min(count, FastLED.size() - fPos);
  int iPos = fPos;

  // Blend (add) in the color of the first partial pixel

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

  // Now draw any full pixels in the middle

  while (remaining > 1.0f && iPos < NUM_LEDS)
  {
    for (int i = 0; i < NUM_CHANNELS; i++)
      FastLED[i][GetFanPixelOrder(iPos, order)] += color;
    iPos++;
    remaining--;
  }

  // Draw tail pixel, up to a single full pixel

  if (remaining > 0.0f)
  {
    for (int i = 0; i < NUM_CHANNELS; i++)
      FastLED[i][GetFanPixelOrder(iPos, order)] += LEDStripEffect::ColorFraction(color, remaining);
  }
}

// DrawRingPixels
//
// With multiple rings, a fan or insulator becomes a ringset, and this function will
// draw to particular ring within a particular insulator.  If merge is true the color
// is added to the cell, if false it is replaced.

inline void DrawRingPixels(float fPos, float count, CRGB color, int iInsulator, int iRing, bool bMerge = true)
{
  // bPos will be the start of this ring (relative to NUM_LEDS)
  int bPos = 0;
  for (int i = 0; i < iRing; i++)
    bPos += g_aRingSizeTable[i];
  bPos += iInsulator * FAN_SIZE;

  if (bPos + fPos + count > NUM_LEDS + 1) // +1 because we work in the 0..1.0 range when drawing
  {
    debugE("DrawFanPixels called with fPos=%f, count=%f, but there are only %d LEDs", fPos, count, NUM_LEDS);
    return;
  }

  if (count < 0)
  {
    debugE("Negative count in DrawFanPixels");
    return;
  }
  // Calculate how much the first pixel will hold

  float availFirstPixel = 1.0f - (fPos - (long)(fPos));
  float amtFirstPixel = min(availFirstPixel, count);
  float remaining = min(count, FastLED.size() - fPos);
  int iPos = fPos;
  // Blend (add) in the color of the first partial pixel

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

  // Now draw any full pixels in the middle

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

  // Draw tail pixel, up to a single full pixel

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

class EmptyEffect : public LEDStripEffect
{
  using LEDStripEffect::LEDStripEffect;

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

class FanBeatEffect : public LEDStripEffect
{
public:

  FanBeatEffect(const String & strName) : LEDStripEffect(EFFECT_STRIP_FAN_BEAT, strName)
  {
  }

  FanBeatEffect(const JsonObjectConst& jsonObject) : LEDStripEffect(jsonObject)
  {
  }

  void Draw() override
  {
    fadeToBlackBy(FastLED.leds(), NUM_LEDS, 20);
    DrawEffect();
    delay(20);
  }

  void OnBeat()
  {
    int passes = g_Analyzer._VURatio;
    for (int iPass = 0; iPass < passes; iPass++)
    {
      int iFan = random(0, NUM_FANS);
      int passes = random(1, g_Analyzer._VURatio);
      CRGB c = CHSV(random(0, 255), 255, 255);

      for (int iPass = 0; iPass < passes; iPass++)
      {
        DrawFanPixels(0, FAN_SIZE, c, Sequential, iFan++);
      }
    }

    CRGB c = CHSV(random(0, 255), 255, 255);
    for (int i = NUM_FANS * FAN_SIZE; i < NUM_LEDS; i++)
    {
      g()->setPixel(i, c);
    }
  }

  void DrawEffect()
  {
    static bool latch = false;
    static float minVUSeen = 0.0;

    if (latch)
    {
      if (g_Analyzer._VURatio < minVUSeen)
        minVUSeen = g_Analyzer._VURatio;
    }

    if (g_Analyzer._VURatio < 0.25f) // Crossing center going up
    {
      latch = true;
      minVUSeen = g_Analyzer._VURatio;
    }

    if (latch)
    {
      if (g_Analyzer._VURatio > 1.5f)
      {
        if (random_range(1.0f, 3.0f) < g_Analyzer._VURatio)
        {
          latch = false;
          OnBeat();
        }
      }
    }
  }
};

class CountEffect : public LEDStripEffect
{
  using LEDStripEffect::LEDStripEffect;

  const int DRAW_LEN = 16;
  const int OPEN_LEN = NUM_FANS * FAN_SIZE - DRAW_LEN;

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

class TapeReelEffect : public LEDStripEffect
{
private:
  float ReelPos[NUM_FANS] = {0};
  float ReelDir[NUM_FANS] = {0};

public:
  TapeReelEffect(const String & strName) : LEDStripEffect(EFFECT_STRIP_TAPE_REEL, strName)
  {
  }

  TapeReelEffect(const JsonObjectConst& jsonObject) : LEDStripEffect(jsonObject)
  {
  }

  void Draw() override
  {
    EVERY_N_MILLISECONDS(250)
    {
      for (int i = 0; i < NUM_FANS; i++)
      {
        if (random(0, 100) < 40) // 40% Chance of attempting to do something
        {
          int action = random(0, 3); // Generate a random outcome
          if (action == 0)
          {
            ReelDir[i] = 0; // 0 -> Stop the Reel
          }
          else if (action == 1)
          {
            if (ReelDir[i] == 0)
            {
              ReelDir[i] = -1; // 1 -> Spin Backwards, or accel if already doing so
            }
            else
            {
              ReelDir[i] -= .5f;
            }
          }
          else if (action == 2)
          {
            if (ReelDir[i] == 0) // 2 -> Spin Forwards, or accel if already doing so
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

    EVERY_N_MILLISECONDS(20) // Update the reels based on the direction
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

    EVERY_N_MILLISECONDS(20) // Draw the Effect
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

class PaletteReelEffect : public LEDStripEffect
{
private:
  float ReelPos[NUM_FANS] = {0};
  float ReelDir[NUM_FANS] = {0};
  int ColorOffset[NUM_FANS] = {0};

public:
  PaletteReelEffect(const String & strName) : LEDStripEffect(EFFECT_STRIP_PALETTE_REEL, strName)
  {
  }

  PaletteReelEffect(const JsonObjectConst& jsonObject) : LEDStripEffect(jsonObject)
  {
  }

  void Draw() override
  {
    EVERY_N_MILLISECONDS(250)
    {
      for (int i = 0; i < NUM_FANS; i++)
      {
        if (random(0, 100) < 50 * g_Analyzer._VURatio) // 40% Chance of attempting to do something
        {
          int action = random(0, 3); // Generate a random outcome
          if (action == 0 || action == 3)
          {
            ReelDir[i] = 0; // 0 -> Stop the Reel
          }
          else if (action == 1)
          {
            if (g_Analyzer._VURatio > 0.5f)
            {
              if (ReelDir[i] == 0)
              {
                ColorOffset[i] = random(0, 255);
                ReelDir[i] = -1; // 1 -> Spin Backwards, or accel if already doing so
              }
              else
              {
                ReelDir[i] -= .5f;
              }
            }
          }
          else if (action == 2)
          {
            if (g_Analyzer._VURatio > 0.5f)
            {
              if (ReelDir[i] == 0) // 2 -> Spin Forwards, or accel if already doing so
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

    EVERY_N_MILLISECONDS(20) // Update the reels based on the direction
    {
      for (int i = 0; i < NUM_FANS; i++)
      {
        ReelPos[i] = (ReelPos[i] + ReelDir[i] * (2 + g_Analyzer._VURatio));
        if (ReelPos[i] < 0)
          ReelPos[i] += FAN_SIZE;
        if (ReelPos[i] >= FAN_SIZE)
          ReelPos[i] -= FAN_SIZE;
      }
    }

    EVERY_N_MILLISECONDS(20) // Draw the Effect
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

class PaletteSpinEffect : public LEDStripEffect
{
  const CRGBPalette16 _Palette;
  bool _bReplaceMagenta;
  float _sparkleChance;

private:
  float ReelPos[NUM_FANS] = {0};
  int ColorOffset[NUM_FANS] = {0};

public:
  PaletteSpinEffect(const String &strName, const CRGBPalette16 &palette, bool bReplace, float sparkleChance = 0.0)
      : LEDStripEffect(EFFECT_STRIP_PALETTE_SPIN, strName),
        _Palette(palette),
        _bReplaceMagenta(bReplace),
        _sparkleChance(sparkleChance)
  {
  }

  PaletteSpinEffect(const JsonObjectConst& jsonObject)
      : LEDStripEffect(jsonObject),
        _Palette(jsonObject[PTY_PALETTE].as<CRGBPalette16>()),
        _bReplaceMagenta(jsonObject["rpm"]),
        _sparkleChance(jsonObject["sch"])
  {
  }

  bool SerializeToJSON(JsonObject& jsonObject) override
  {
    AllocatedJsonDocument jsonDoc(512);

    JsonObject root = jsonDoc.to<JsonObject>();
    LEDStripEffect::SerializeToJSON(root);

    jsonDoc[PTY_PALETTE] = _Palette;
    jsonDoc["rpm"] = _bReplaceMagenta;
    jsonDoc["sch"] = _sparkleChance;

    return jsonObject.set(jsonDoc.as<JsonObjectConst>());
  }

  void Draw() override
  {
    EVERY_N_MILLISECONDS(20) // Update the reels based on the direction
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

    EVERY_N_MILLISECONDS(20) // Draw the Effect
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
class ColorCycleEffect : public LEDStripEffect
{
  PixelOrder _order;
  int _step;

public:
  using LEDStripEffect::LEDStripEffect;

  ColorCycleEffect(PixelOrder order = Sequential, int step = 8)
    : LEDStripEffect(EFFECT_STRIP_COLOR_CYCLE, "ColorCylceEffect"),
      _order(order),
      _step(step)
  {
  }

  ColorCycleEffect(const JsonObjectConst& jsonObject)
    : LEDStripEffect(jsonObject),
      _order((PixelOrder)jsonObject[PTY_ORDER]),
      _step(jsonObject["stp"])
  {
  }

  bool SerializeToJSON(JsonObject& jsonObject) override
  {
    StaticJsonDocument<LEDStripEffect::_jsonSize> jsonDoc;

    JsonObject root = jsonDoc.to<JsonObject>();
    LEDStripEffect::SerializeToJSON(root);

    jsonDoc[PTY_ORDER] = to_value(_order);
    jsonDoc["stp"] = _step;

    assert(!jsonDoc.overflowed());

    return jsonObject.set(jsonDoc.as<JsonObjectConst>());
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

class ColorCycleEffectBottomUp : public LEDStripEffect
{
public:
  using LEDStripEffect::LEDStripEffect;

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

class ColorCycleEffectTopDown : public LEDStripEffect
{
public:
  using LEDStripEffect::LEDStripEffect;

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

class ColorCycleEffectSequential : public LEDStripEffect
{
public:
  using LEDStripEffect::LEDStripEffect;

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

class SpinningPaletteEffect : public PaletteEffect
{
  int iRotate = 0;

public:
  using PaletteEffect::PaletteEffect;

  void Draw() override
  {
    PaletteEffect::Draw();
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

class ColorCycleEffectRightLeft : public LEDStripEffect
{
public:
  using LEDStripEffect::LEDStripEffect;

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

class ColorCycleEffectLeftRight : public LEDStripEffect
{
public:
  using LEDStripEffect::LEDStripEffect;

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

class FireFanEffect : public LEDStripEffect
{
protected:
  CRGBPalette16 Palette;
  int LEDCount; // Number of LEDs total
  int CellsPerLED;
  int Cooling;     // Rate at which the pixels cool off
  int Sparks;      // How many sparks will be attempted each frame
  int SparkHeight; // If created, max height for a spark
  int Sparking;    // Probability of a spark each attempt
  bool bReversed;  // If reversed we draw from 0 outwards
  bool bMirrored;  // If mirrored we split and duplicate the drawing
  bool bMulticolor; // If true each channel spoke will be a different color
  PixelOrder Order;

  std::unique_ptr<uint8_t[]> abHeat; // Heat table to map temp to color

  // When diffusing the fire upwards, these control how much to blend in from the cells below (ie: downward neighbors)
  // You can tune these coefficients to control how quickly and smoothly the fire spreads

  static const uint8_t BlendSelf = 0;      // 2
  static const uint8_t BlendNeighbor1 = 1; // 3
  static const uint8_t BlendNeighbor2 = 1; // 2
  static const uint8_t BlendNeighbor3 = 0; // 1

  static const uint8_t BlendTotal = (BlendSelf + BlendNeighbor1 + BlendNeighbor2 + BlendNeighbor3);

  int CellCount() const { return LEDCount * CellsPerLED; }

public:
  FireFanEffect(CRGBPalette16 palette,
                int ledCount,
                int cellsPerLED = 1,
                int cooling = 20,
                int sparking = 100,
                int sparks = 3,
                int sparkHeight = 4,
                PixelOrder order = Sequential,
                bool breversed = false,
                bool bmirrored = false,
                bool bmulticolor = false)
      : LEDStripEffect(EFFECT_STRIP_FIRE_FAN, "FireFanEffect"),
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
        bMulticolor(bmulticolor)
  {
    if (bMirrored)
      LEDCount = LEDCount / 2;
    abHeat.reset( psram_allocator<uint8_t>().allocate(CellCount()) );
  }

  FireFanEffect(const JsonObjectConst& jsonObject)
      : LEDStripEffect(jsonObject),
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
        bMulticolor(jsonObject[PTY_MULTICOLOR] == 1)
  {
    if (bMirrored)
      LEDCount = LEDCount / 2;
    abHeat.reset( psram_allocator<uint8_t>().allocate(CellCount()) );
  }

  bool SerializeToJSON(JsonObject& jsonObject) override
  {
    AllocatedJsonDocument jsonDoc(512);

    JsonObject root = jsonDoc.to<JsonObject>();
    LEDStripEffect::SerializeToJSON(root);

    jsonDoc[PTY_PALETTE] = Palette;
    jsonDoc[PTY_LEDCOUNT] = LEDCount;
    jsonDoc[PTY_CELLSPERLED] = CellsPerLED;
    jsonDoc[PTY_COOLING] = Cooling;
    jsonDoc[PTY_SPARKS] = Sparks;
    jsonDoc[PTY_SPARKHEIGHT] = SparkHeight;
    jsonDoc[PTY_SPARKING] = Sparking;
    jsonDoc[PTY_REVERSED] = bReversed;
    jsonDoc[PTY_MIRORRED] = bMirrored;
    jsonDoc[PTY_ORDER] = to_value(Order);
    jsonDoc[PTY_MULTICOLOR] = bMulticolor ? 1 : 0;

    return jsonObject.set(jsonDoc.as<JsonObjectConst>());
  }

  CRGB GetBlackBodyHeatColorByte(byte temp) const
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
    // First cool each cell by a litle bit

    EVERY_N_MILLISECONDS(50)
    {
      for (int i = 0; i < CellCount(); i++)
      {
        int coolingAmount = random(0, Cooling);
        abHeat[i] = ::max(0.0, abHeat[i] - coolingAmount * (2.0 - g_Analyzer._VURatio));
      }
    }

    EVERY_N_MILLISECONDS(20)
    {
      // Next drift heat up and diffuse it a little bit
      for (int i = 0; i < CellCount(); i++)
        abHeat[i] = min(255, (abHeat[i] * BlendSelf +
                              abHeat[(i + 1) % CellCount()] * BlendNeighbor1 +
                              abHeat[(i + 2) % CellCount()] * BlendNeighbor2 +
                              abHeat[(i + 3) % CellCount()] * BlendNeighbor3) /
                                 BlendTotal);
    }

    // Randomly ignite new sparks down in the flame kernel

    EVERY_N_MILLISECONDS(20)
    {
      for (int i = 0; i < Sparks; i++)
      {
        if (random(255) < Sparking / 4 + Sparking * (g_Analyzer._VURatio / 2.0) * 0.5)
        {
          int y = CellCount() - 1 - random(SparkHeight * CellsPerLED);
          abHeat[y] = abHeat[y] + random(50, 255); // Can roll over which actually looks good!
        }
      }
    }

    // Finally, convert heat to a color

    for (int i = 0; i < LEDCount; i++)
    {
      // uint8_t maxv = 0;
      // for (int iCell = 0; iCell < CellsPerLED; iCell++)
      //   maxv = max(maxv, heat[i * CellsPerLED + iCell]);

      for (int iChannel = 0; iChannel < NUM_CHANNELS; iChannel++)
      {
        CRGB color = GetBlackBodyHeatColorByte(abHeat[i * CellsPerLED]);

        // If multicolor, we shift the hue based on the channel
        if (bMulticolor)
        {
            CHSV hsv = rgb2hsv_approximate(color);
                 hsv.hue += iChannel * (255/NUM_CHANNELS);
            color = hsv;
        }

        // If we're reversed, we work from the end back.  We don't reverse the bonus pixels

        int j = (!bReversed || i > FAN_SIZE) ? i : LEDCount - 1 - i;
        uint x = GetFanPixelOrder(j, order);
        if (x < NUM_LEDS)
        {
          FastLED[iChannel][x] = color;
          if (bMirrored)
            FastLED[iChannel][!bReversed ? (2 * LEDCount - 1 - i) : LEDCount + i] = color;
        }
      }
    }
  }
};

class BlueFireFanEffect : public FireFanEffect
{
  using FireFanEffect::FireFanEffect;

  virtual CRGB MapHeatToColor(uint8_t temperature, int iChannel = 0)
  {
    uint8_t t192 = round((temperature / 255.0) * 191);
    uint8_t heatramp = t192 & 0x3F; // 0..63
    heatramp <<= 2;                 // scale up to 0..252

    CHSV hsv(HUE_BLUE, 255, heatramp);
    CRGB rgb;
    hsv2rgb_rainbow(hsv, rgb);
    return rgb;
  }
};

class GreenFireFanEffect : public FireFanEffect
{
  using FireFanEffect::FireFanEffect;

  virtual CRGB MapHeatToColor(uint8_t temperature, int iChannel = 0)
  {
    uint8_t t192 = round((temperature / 255.0) * 191);
    uint8_t heatramp = t192 & 0x3F; // 0..63
    heatramp <<= 2;                 // scale up to 0..252

    CHSV hsv(HUE_GREEN, 255, heatramp);
    CRGB rgb;
    hsv2rgb_rainbow(hsv, rgb);
    return rgb;
  }
};

class RGBRollAround : public LEDStripEffect
{
  int iRotate = 0;

public:
  using LEDStripEffect::LEDStripEffect;

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

class HueTest : public LEDStripEffect
{
  int iRotate = 0;

public:
  using LEDStripEffect::LEDStripEffect;

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

class RingTestEffect : public LEDStripEffect
{
private:
public:
  RingTestEffect() : LEDStripEffect(EFFECT_STRIP_RING_TEST, "Ring Test")
  {
  }

  RingTestEffect(const JsonObjectConst& jsonObject) : LEDStripEffect(jsonObject)
  {
  }

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

/*
 * Effects intended for a train-style lantern with concentric rings of 16/12/8/1
 */

// Lantern - A candle-like effect that flickers in the center of an LED disc
//           Inspired by a candle effect I saw done by carangil

class LanternParticle
{
  const int minPeturbation = 100;
  const int maxPeterbation = 3500;
  const int perterbationIncrement = 1;
  const int maxDeviation = 100;

  int centerX = maxDeviation / 2;
  int centerY = maxDeviation / 2;

  int velocityX = 0;
  int velocityY = 0;

  int pertub = minPeturbation;
  int perturbDirection = perterbationIncrement;

  float rotation = 0.0f;

protected:

  float distance(float x1, float y1, float x2, float y2)
  {
    return std::sqrt(std::pow(x1-x2, 2) + std::pow(y1 - y2, 2));
  }

  CRGB flameColor(int val)
  {
    val = min(val, 255);
    val = max(val, 0);

    return CRGB(val, val * .30, val * .05);
  }

  // Generate a vector of how bright each of the surrounding 8 LEDs on the unit circle should be

  std::vector<float> led_brightness(float wandering_x, float wandering_y)
  {
    const float sqrt2 = std::sqrt(2);

    const std::vector<std::pair<float, float>> unit_circle_coords = {
        {1, 0},
        { 1 / sqrt2,  1 / sqrt2},
        {0, 1},
        {-1 / sqrt2,  1 / sqrt2},
        {-1, 0},
        {-1 / sqrt2, -1 / sqrt2},
        {0, -1},
        { 1 / sqrt2, -1 / sqrt2}
    };

    std::vector<float> brightness_values;

    for (const auto& coord : unit_circle_coords) {
        float d = distance(wandering_x, wandering_y, coord.first, coord.second);
        float brightness = std::max(1.0f - d, 0.0f);
        brightness_values.push_back(brightness);
    }

    return brightness_values;
}

public:
  void Draw()
  {
    // random trigger brightness oscillation, if at least half uncalm

      int movx = 0;
      int movy = 0;

      if (pertub > (maxPeterbation / 2))
        if (random(2000) < 5)
          pertub = maxPeterbation; // occasional 'bonus' wind

      // random poke, intensity determined by uncalm value (0 is perfectly calm)

      movx = random(pertub >> 7) - (pertub >> 9);
      movy = random(pertub >> 7) - (pertub >> 9);

      // if reach most calm value, start moving towards uncalm
      if (pertub < minPeturbation)
        perturbDirection = perterbationIncrement;

      // if reach most uncalm value, start going towards calm
      if (pertub > maxPeterbation)
        perturbDirection = -perterbationIncrement;

      pertub += perturbDirection;

      // Move center of flame around by the current velocity

      centerX += movx + (velocityX / 7);
      centerY += movy + (velocityY / 7);

      // Enforce some range limits
      if (centerX < -maxDeviation)
      {
        centerX = -maxDeviation;
        velocityX *= -0.5;
      }

      if (centerX > maxDeviation)
      {
        centerX = maxDeviation;
        velocityX *= -0.5;
      }

      if (centerY < -maxDeviation)
      {
        centerY = -maxDeviation;
        velocityY *= -0.5;
      }

      if (centerY > maxDeviation)
      {
        centerY = maxDeviation;
        velocityY *= -0.5;
      }

      // Dampen the velocity down a fraction

      velocityX = (velocityX * 999) / 1000;
      velocityY = (velocityY * 999) / 1000;

      // Apply Hooke's law of spring motion to accelerate back towards rest/center

      velocityX -= centerX;
      velocityY -= centerY;


    rotation += 0.0;

    // Draw four outer pixels in second ring outwards.  We draw 1.05 to take advantage of the non-linear red response in
    // the second pixels (when drawn at 5%, the red will show up more, depending on color correction).

    float xRatio = map(centerX, 0.0f, maxDeviation, -1.0f, 1.0f);
    float yRatio = map(centerY, 0.0f, maxDeviation, -1.0f, 1.0f);

    auto brightness = led_brightness(xRatio, yRatio);
    for (int i = 0; i < 8; i++)
    {
      CRGB pixelColor = flameColor(255 * brightness[i]);
      pixelColor.fadeToBlackBy(255 * (3.0 - brightness[i]));
      DrawRingPixels(i, 1, pixelColor, 0, 2, true);
    }

    // Now draw a center pixel which is dimmed proportional to the distance the center is from actual

    CRGB centerColor = CRGB(255, 12, 0);
    centerColor.fadeToBlackBy(distance(xRatio, yRatio, 0, 0) * 128);
    DrawRingPixels(0, 1.0, centerColor, 0, 3);

    debugV("X,Y = %f, %f\n", xRatio, yRatio);
  }
};

class LanternEffect : public LEDStripEffect
{
  static const int _maxParticles = 1;

private:
  LanternParticle _particles[_maxParticles];

public:
  LanternEffect() : LEDStripEffect(EFFECT_STRIP_LANTERN, "LanternEffect")
  {
  }

  LanternEffect(const JsonObjectConst& jsonObject) : LEDStripEffect(jsonObject)
  {
  }

  size_t DesiredFramesPerSecond() const override
  {
    return 30;
  }

  void Draw() override
  {
    fadeAllChannelsToBlackBy(20);
    for (int i = 0; i < _maxParticles; i++)
      _particles[i].Draw();
  }
};
