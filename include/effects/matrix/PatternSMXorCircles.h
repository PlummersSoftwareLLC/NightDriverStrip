#pragma once

#include "effectmanager.h"
#include "effects/strip/musiceffect.h"

#if ENABLE_AUDIO
class PatternSMXorCircles : public BeatEffectBase,
                            public LEDStripEffect
#else
class PatternSMXorCircles : public LEDStripEffect
#endif
{
 private:
  static constexpr int LED_COLS = MATRIX_WIDTH;
  static constexpr int LED_ROWS = MATRIX_HEIGHT;
  /*
      double log2(double num){
        double number=log(num)/log(2);
        return (number);
      }
  */
#define log2(num) log(num) / log(2)
  static constexpr uint8_t scale_x = log2(64 / LED_COLS);
  static constexpr uint8_t scale_y = log2(64 / LED_ROWS);

 public:
  PatternSMXorCircles()
      :
#if ENABLE_AUDIO
        BeatEffectBase(1.50, 0.05),
#endif
        LEDStripEffect(EFFECT_MATRIX_SMXOR_CIRCLES, "Xor Circles") {
  }

  PatternSMXorCircles(const JsonObjectConst& jsonObject)
      :
#if ENABLE_AUDIO
        BeatEffectBase(1.50, 0.05),
#endif
        LEDStripEffect(jsonObject) {
  }

  void Start() override { g()->Clear(); }

  void Draw() override {
#if ENABLE_AUDIO
    ProcessAudio();
#endif

    uint8_t x1sh = beatsin8(5, 0, LED_COLS);
    uint8_t y1sh = beatsin8(6, 0, LED_ROWS);
    uint8_t x2sh = beatsin8(7, 0, LED_COLS);
    uint8_t y2sh = beatsin8(4, 0, LED_ROWS);
    for (uint8_t y = 0; y < LED_ROWS; y++) {
      for (uint8_t x = 0; x < LED_COLS; x++) {
        int8_t cx = x - x1sh;
        int8_t cy = y - y1sh;
        uint8_t a = sqrt16(((cx * cx) + (cy * cy))) << scale_x;
        cx = x - x2sh;
        cy = y - y2sh;
        uint8_t v = sqrt16(((cx * cx) + (cy * cy))) << scale_y;
        g()->leds[g()->xy(x, y)] = (((a ^ v) >> 4) & 1) * 255;
      }
    }
  }

#if ENABLE_AUDIO
  virtual void HandleBeat(bool bMajor, float elapsed, float span) override {}
#endif
};

#undef log2
