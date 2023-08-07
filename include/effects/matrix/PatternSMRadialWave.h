#pragma once

#include "effectmanager.h"
#include "effects/strip/musiceffect.h"
// Derived from https://editor.soulmatelights.com/gallery/1090-radialwave

#if ENABLE_AUDIO
class PatternSMRadialWave : public BeatEffectBase,
                            public LEDStripEffect
#else
class PatternSMRadialWave : public LEDStripEffect
#endif
{
 private:
  // RadialWave
  // Stepko and Sutaburosu
  // 22/05/22
  static constexpr int LED_COLS = MATRIX_WIDTH;
  static constexpr int LED_ROWS = MATRIX_HEIGHT;

  bool setupm = 1;
  const uint8_t C_X = LED_COLS / 2;
  const uint8_t C_Y = LED_ROWS / 2;
  const uint8_t mapp = 255 / LED_COLS;
  struct {
    uint8_t angle;
    uint8_t radius;
  } rMap[LED_COLS][LED_ROWS];

 public:
  PatternSMRadialWave()
      :
#if ENABLE_AUDIO
        BeatEffectBase(1.50, 0.05),
#endif
        LEDStripEffect(EFFECT_MATRIX_SMRADIALWAVE, "Radial Wave") {
  }

  PatternSMRadialWave(const JsonObjectConst& jsonObject)
      :
#if ENABLE_AUDIO
        BeatEffectBase(1.50, 0.05),
#endif
        LEDStripEffect(jsonObject) {
  }

  void Start() override {
    g()->Clear();
    for (int8_t x = -C_X; x < C_X + (LED_COLS % 2); x++) {
      for (int8_t y = -C_Y; y < C_Y + (LED_ROWS % 2); y++) {
        rMap[x + C_X][y + C_Y].angle = 128 * (atan2(y, x) / PI);
        rMap[x + C_X][y + C_Y].radius = hypot(x, y) * mapp;  // thanks
                                                             // Sutaburosu
      }
    }
  }

  void Draw() override {
#if ENABLE_AUDIO
    ProcessAudio();
#endif
    static byte speed = 1;
    static uint32_t t;
    t += speed;
    for (uint8_t x = 0; x < LED_COLS; x++) {
      for (uint8_t y = 0; y < LED_ROWS; y++) {
        byte angle = rMap[x][y].angle;
        byte radius = rMap[x][y].radius;
        g()->leds[g()->xy(x, y)] = CHSV(
            t + radius, 255, sin8(t * 4 + sin8(t * 4 - radius) + angle * 3));
      }
    }
    //  delay(16);
  }

#if ENABLE_AUDIO
  virtual void HandleBeat(bool bMajor, float elapsed, float span) override {}
#endif
};
