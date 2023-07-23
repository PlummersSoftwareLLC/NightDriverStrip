#pragma once

#include "effectmanager.h"
#include "effects/strip/musiceffect.h"
// Derived from https://www.soulmatelights.com/gallery/434-f-lying

#if ENABLE_AUDIO
class PatternSMFlying : public BeatEffectBase,
                        public LEDStripEffect
#else
class PatternSMFlying : public LEDStripEffect
#endif
{
 private:
 public:
  PatternSMFlying()
      :
#if ENABLE_AUDIO
        BeatEffectBase(1.50, 0.05),
#endif
        LEDStripEffect(EFFECT_MATRIX_SMFLYING, "Flying") {
  }

  PatternSMFlying(const JsonObjectConst& jsonObject)
      :
#if ENABLE_AUDIO
        BeatEffectBase(1.50, 0.05),
#endif
        LEDStripEffect(jsonObject) {
  }

  virtual void Start() override { g()->Clear(); }

  virtual void Draw() override {
#if ENABLE_AUDIO
    ProcessAudio();
#endif
    static byte hue = 0;
    EVERY_N_MILLISECONDS(30) { hue++; }  // 30 - speed of hue change
#define speed -5                         // global speed of dots move

    byte x1 = beatsin8(18 + speed, 0, (MATRIX_WIDTH - 1));
    byte x2 = beatsin8(23 + speed, 0, (MATRIX_WIDTH - 1));
    byte x3 = beatsin8(27 + speed, 0, (MATRIX_WIDTH - 1));
    byte x4 = beatsin8(31 + speed, 0, (MATRIX_WIDTH - 1));
    byte x5 = beatsin8(33 + speed, 0, (MATRIX_WIDTH - 1));

    byte y1 = beatsin8(20 + speed, 0, (MATRIX_HEIGHT - 1));
    byte y2 = beatsin8(26 + speed, 0, (MATRIX_HEIGHT - 1));
    byte y3 = beatsin8(15 + speed, 0, (MATRIX_HEIGHT - 1));
    byte y4 = beatsin8(27 + speed, 0, (MATRIX_HEIGHT - 1));
    byte y5 = beatsin8(30 + speed, 0, (MATRIX_HEIGHT - 1));

    CRGB color = CHSV(hue, 255, 255);

    fadeAllChannelsToBlackBy(80);

    g()->drawLine(x1, y1, x2, y2, color);
    g()->drawLine(x2, y2, x3, y3, color);
    g()->drawLine(x3, y3, x1, y1, color);
    g()->drawLine(x4, y4, x1, y1, color);
    g()->drawLine(x4, y4, x2, y2, color);
    g()->drawLine(x4, y4, x3, y3, color);

    g()->blur2d(g()->leds, MATRIX_WIDTH - 1, 0, MATRIX_HEIGHT - 1, 0, 8);
  }

#if ENABLE_AUDIO
  virtual void HandleBeat(bool bMajor, float elapsed, float span) override {}
#endif
};
