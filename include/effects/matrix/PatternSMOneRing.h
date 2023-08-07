#pragma once

#include "effectmanager.h"
#include "effects/strip/musiceffect.h"

// Inspired by https://editor.soulmatelights.com/gallery/1455-one-ring

#if USE_AUDIO
// class PatternSMOneRing : public BeatEffectBase, public LEDStripEffect
#else
class PatternSMOneRing : public LEDStripEffect
#endif
{
 private:
  const int RINGSIZE = (MATRIX_HEIGHT / 2);
  const float INCLINE = (1.5);  // 1.0-?.0;

 public:
  PatternSMOneRing()
      :
#if SOUND
        BeatEffectBase(1.50, 0.05),
#endif
        LEDStripEffect(EFFECT_MATRIX_SMONE_RING, "One Ring") {
  }

  PatternSMOneRing(const JsonObjectConst& jsonObject)
      :
#if USE_AUDIO
        BeatEffectBase(1.50, 0.05),
#endif
        LEDStripEffect(jsonObject) {
  }

  void Start() override {}

  float code(double t, double i, double x, double y) {
    return sin(x * 3 / RINGSIZE - t) * INCLINE - y;
  }

  void processFrame(double t, double x, double y) {
    float tempcode = code(t, 0, x, y) + MATRIX_HEIGHT / RINGSIZE * 1.5;
    double frame = constrain(tempcode + RINGSIZE * 0.4, -1, 1) * 255;
    uint8_t bri = 0;
    if (frame >= 0) bri = frame;
    frame = constrain(tempcode + RINGSIZE * 1.1, -1, 1) * 255;
    if (frame <= 0) bri = -frame;
    frame = fabs(constrain(tempcode + RINGSIZE * 0.75, -1, 1) * 255);
    if (frame <= 254) bri = -frame;
    bri = 255 - bri;
    g()->leds[g()->xy(x, y)].setRGB(bri, bri, 0);
  }

  void Draw() override {
    double t =
        millis() /
        1000.0;  // some formulas is hardcoded and fps get down. this speedup it
    for (byte x = 0; x < MATRIX_WIDTH; x++) {
      for (byte y = 0; y < MATRIX_HEIGHT; y++) {
        processFrame(t, x, y);
      }
    }
  }

#if USE_AUDIO
  void HandleBeat(bool bMajor, float elapsed, float span) override {}
#endif
};
