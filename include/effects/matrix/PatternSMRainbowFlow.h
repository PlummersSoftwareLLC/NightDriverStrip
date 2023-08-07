#pragma once

#include "effectmanager.h"

// Derived from https://editor.soulmatelights.com/gallery/1236-rainbow-flow
// Another super-simple one.

class PatternSMRainbowFlow : public LEDStripEffect
{
 private:
  byte hue = 0;

 public:
  PatternSMRainbowFlow()
      :
        LEDStripEffect(EFFECT_MATRIX_SMRAINBOW_FLOW, "Rainbow Flow") {
  }

  PatternSMRainbowFlow(const JsonObjectConst& jsonObject)
      :
        LEDStripEffect(jsonObject) {
  }

  void Start() override { g()->Clear(); }

  void Draw() override {
    const float hl = NUM_LEDS / 1.3;

    EVERY_N_MILLISECONDS(80) { hue++; }
    int t = millis() / 40;
    for (int i = 0; i < NUM_LEDS; i++) {
      int c = (abs(i - hl) / hl) * 127;
      c = sin8(c);
      c = sin8(c / 2 + t);
      byte b = sin8(c + t / 8);

      g()->leds[i] = CHSV(b + hue, 255, 255);
    }
  }
};
