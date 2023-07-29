#pragma once

#include "effects/strip/musiceffect.h"
#include "effectmanager.h"

// Derived from https://editor.soulmatelights.com/gallery/1110-prismata

#if ENABLE_AUDIO
class PatternSMPrismata : public BeatEffectBase, public LEDStripEffect
#else
class PatternSMPrismata : public LEDStripEffect
#endif
{
private:
  uint8_t Speed = 30U;       // 1-255
  uint8_t Scale = 8;         // 1-100 is palette
  uint8_t hue;

  const TProgmemRGBPalette16 *curPalette = &PartyColors_p;

  void drawPixelXY(int8_t x, int8_t y, CRGB color)
  {
    if (x < 0 || x > (MATRIX_WIDTH - 1) || y < 0 || y > (MATRIX_HEIGHT - 1)) return;
    // Mesmerizer flips the Y axis here.
    uint32_t thisPixel = g()->xy((uint8_t)x, MATRIX_HEIGHT -1  - (uint8_t)y);
    g()->leds[thisPixel] = color;
  }


public:
  PatternSMPrismata() :
#if ENABLE_AUDIO
    BeatEffectBase(1.50, 0.05),
#endif
    LEDStripEffect(EFFECT_MATRIX_SMPRISMATA, "Prismata")
    {
    }

  PatternSMPrismata(const JsonObjectConst& jsonObject) :
#if ENABLE_AUDIO
    BeatEffectBase(1.50, 0.05),
#endif
    LEDStripEffect(jsonObject)
  {
  }

  void Start() override
  {
    g()->Clear();
  }

  void Draw() override
  {
#if ENABLE_AUDIO
    ProcessAudio();
#endif

  g()->BlurFrame(20); // @Palpalych посоветовал делать размытие
  g()->DimAll(255U - (Scale - 1U) % 11U * 3U);

  for (uint8_t x = 0; x < MATRIX_WIDTH; x++)
  {
    //uint8_t y = beatsin8(x + 1, 0, HEIGHT-1); // это я попытался распотрошить данную функцию до исходного кода и вставить в неё регулятор скорости
    // вместо 28 в оригинале было 280, умножения на .Speed не было, а вместо >>17 было (<<8)>>24. короче, оригинальная скорость достигается при бегунке .Speed=20
    uint8_t beat = (GET_MILLIS() * (accum88(x + 1)) * 28 * Speed) >> 17;
    uint8_t y = scale8(sin8(beat), MATRIX_HEIGHT-1);
    //и получилось!!!

    drawPixelXY(x, y, ColorFromPalette(*curPalette, x * 7 + hue));
  }
  }

#if ENABLE_AUDIO
  virtual void HandleBeat(bool bMajor, float elapsed, float span) override
  {

  }
#endif
};
