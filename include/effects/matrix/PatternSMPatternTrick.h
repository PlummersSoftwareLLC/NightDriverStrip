#pragma once

#include "effectmanager.h"
#include "effects/strip/musiceffect.h"
// Inspired by https://editor.soulmatelights.com/gallery/2105-pattern-trick

#if ENABLE_AUDIO
class PatternSMPatternTrick : public BeatEffectBase,
                              public LEDStripEffect
#else
class PatternSMPatternTrick : public LEDStripEffect
#endif
{
 private:
 public:
  PatternSMPatternTrick()
      :
#if ENABLE_AUDIO
        BeatEffectBase(1.50, 0.05),
#endif
        LEDStripEffect(EFFECT_MATRIX_SMPATTERN_TRICK, "PatternTrick") {
  }

  PatternSMPatternTrick(const JsonObjectConst& jsonObject)
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
    int a = millis() / 10;

    for (int x = 0; x < MATRIX_WIDTH; x++) {
      for (int y = 0; y < MATRIX_HEIGHT; y++) {
        if (!(((x + sin8(a / 2) / 24) % MATRIX_WIDTH * 6 ^
               (y + cos8(a / 3) / 8) % MATRIX_HEIGHT * 6) %
              5))
          g()->leds[g()->xy(x, y)].setHue(
              sin8(x * 4 + cos8(y * 2 + a / 4) + a / 3));
        else
          g()->leds[g()->xy(x, y)] = 0;
      }
    }
  }

#if ENABLE_AUDIO
  virtual void HandleBeat(bool bMajor, float elapsed, float span) override {}
#endif
};
