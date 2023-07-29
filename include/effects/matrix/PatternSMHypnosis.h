#pragma once

#include "effects/strip/musiceffect.h"
#include "effectmanager.h"

#if ENABLE_AUDIO
class PatternSMHypnosis : public BeatEffectBase, public LEDStripEffect
#else
class PatternSMHypnosis : public LEDStripEffect
#endif
{
private:
    static constexpr int LED_COLS = MATRIX_WIDTH;
  static constexpr int LED_ROWS = MATRIX_HEIGHT;
const uint8_t C_X = LED_COLS / 2;
const uint8_t C_Y = LED_ROWS / 2;
const uint8_t mapp = 255 / LED_COLS;
struct{
  uint8_t angle;
  uint8_t radius;
} rMap[LED_COLS][LED_ROWS];
public:
  PatternSMHypnosis() :
#if ENABLE_AUDIO
    BeatEffectBase(1.50, 0.05),
#endif
    LEDStripEffect(EFFECT_MATRIX_SMHYPNOSIS, "Hypnosis")
    {
    }

  PatternSMHypnosis(const JsonObjectConst& jsonObject) :
#if ENABLE_AUDIO
    BeatEffectBase(1.50, 0.05),
#endif
    LEDStripEffect(jsonObject)
  {
  }

  void Start() override
  {
    g()->Clear();
      for (int8_t x = -C_X; x < C_X + (LED_COLS % 2); x++) {
      for (int8_t y = -C_Y; y < C_Y + (LED_ROWS % 2); y++) {
        rMap[x + C_X][y + C_Y].angle = 128 * (atan2(y, x) / PI);
        rMap[x + C_X][y + C_Y].radius = hypot(x, y) * mapp; //thanks Sutaburosu
      }
    }
  }

  void Draw() override
  {
#if ENABLE_AUDIO
    ProcessAudio();
#endif
  static uint16_t t;
  t += 4;
  for (uint8_t x = 0; x < LED_COLS; x++) {
    for (uint8_t y = 0; y < LED_ROWS; y++) {
      byte angle = rMap[x][y].angle;
      byte radius = rMap[x][y].radius;
      g()->leds[g()->xy(x, y)] = ColorFromPalette(RainbowStripeColors_p, t / 2 + radius + angle, sin8(angle + (radius * 2) - t));
    }
  }
  }

#if ENABLE_AUDIO
  virtual void HandleBeat(bool bMajor, float elapsed, float span) override
  {

  }
#endif
};
