#pragma once

#include "effectmanager.h"
#include "effects/strip/musiceffect.h"

// Derived from https://editor.soulmatelights.com/gallery/2265-stepkos-psp
//
// Horizontally scrolling cloud.

#if ENABLE_AUDIO
class PatternSMPSPCloud : public BeatEffectBase,
                          public LEDStripEffect
#else
class PatternSMPSPCloud : public LEDStripEffect
#endif
{
 private:
  uint8_t col = 150;
  int xadj = (256 / MATRIX_HEIGHT) << 8;

 public:
  PatternSMPSPCloud()
      :
#if ENABLE_AUDIO
        BeatEffectBase(1.50, 0.05),
#endif
        LEDStripEffect(EFFECT_MATRIX_SMPSP_CLOUD, "PSP Cloud") {
  }

  PatternSMPSPCloud(const JsonObjectConst& jsonObject)
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
    uint32_t t = millis() << 5;
    for (uint8_t x = 0; x < MATRIX_WIDTH; x++) {
      uint16_t h1 =
          map(inoise16(x * xadj + t), 0, 65535, 0, MATRIX_HEIGHT << 8);
      uint16_t h2 = map(inoise16(0, 35550, x * xadj + t), 0, 65535, 0,
                        MATRIX_HEIGHT << 8);
      uint8_t bh1 = uint8_t(h1 >> 8);
      uint8_t bh2 = uint8_t(h2 >> 8);
      for (uint8_t y = 0; y < MATRIX_HEIGHT; y++) {
        g()->leds[g()->xy(x, y)] =
            CHSV(col, map(y + x, 0, MATRIX_HEIGHT + MATRIX_WIDTH - 1, 255, 32),
                 map(x - (MATRIX_HEIGHT - 1 - y), 0, MATRIX_WIDTH - 1, 196,
                     255)) +
            CHSV(0, 0, (y < bh1) ? map(y, 0, bh1, 64, 256) : 0) +
            CHSV(0, 0, (y < bh2) ? map(y, 0, bh2, 64, 256) : 0);
      }
      g()->leds[g()->xy(x, bh1)] += CHSV(0, 0, (h1 % 256));
      g()->leds[g()->xy(x, bh2)] += CHSV(0, 0, (h2 % 256));
    }
  }

#if ENABLE_AUDIO
  virtual void HandleBeat(bool bMajor, float elapsed, float span) override {}
#endif
};
