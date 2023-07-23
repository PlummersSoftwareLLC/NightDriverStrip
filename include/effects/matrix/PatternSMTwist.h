#pragma once

#include "effectmanager.h"
#include "effects/strip/musiceffect.h"
// Derived from https://editor.soulmatelights.com/gallery/2110-twist
// Honestly, it looks better on their emulator...probably something
// to improve here.
//
// A fine color-waving matrix of oscillating grids.

#if ENABLE_AUDIO
class PatternSMTwist : public BeatEffectBase,
                       public LEDStripEffect
#else
class PatternSMTwist : public LEDStripEffect
#endif
{
 private:
  const int BRIGHTNESS = 255;
  byte hue = 0;

  void patt1(uint8_t i, uint8_t j, uint8_t color1, uint8_t color2) {
    //  leds[XY(i, j)] = CHSV(0, 255, 0);
    g()->leds[g()->xy(i + 1, j)] = CHSV(color1, 255, BRIGHTNESS);
    g()->leds[g()->xy(i + 1, j + 1)] = CHSV(color1, 255, BRIGHTNESS);
    g()->leds[g()->xy(i, j + 1)] = CHSV(color2, 255, BRIGHTNESS);
  }

  void patt2(uint8_t i, uint8_t j, uint8_t color1, uint8_t color2) {
    //  leds[XY(i, j)] = CHSV(0, 255, 0);
    g()->leds[g()->xy(i + 1, j)] = CHSV(color1, 255, BRIGHTNESS);
    g()->leds[g()->xy(i + 1, j + 1)] = CHSV(color2, 255, BRIGHTNESS);
    g()->leds[g()->xy(i, j + 1)] = CHSV(color2, 255, BRIGHTNESS);
  }

 public:
  PatternSMTwist()
      :
#if ENABLE_AUDIO
        BeatEffectBase(1.50, 0.05),
#endif
        LEDStripEffect(EFFECT_MATRIX_SMTWIST, "Twist") {
  }

  PatternSMTwist(const JsonObjectConst& jsonObject)
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
    EVERY_N_MILLISECONDS(50) { hue++; }

    for (byte i = 0; i < MATRIX_WIDTH; i += 4) {
      for (byte j = 0; j < MATRIX_HEIGHT; j += 4) {
        patt1(i, j, 64 + j + hue, i + hue);
        patt1(i + 2, j + 2, 64 + j + hue, i + hue);
        patt2(i, j + 2, 64 + j + hue, i + hue);
        patt2(i + 2, j, 64 + j + hue, i + hue);
      }
    }
    // Without an aggressive fade, they all run together into squares.
    fadeAllChannelsToBlackBy(200);
  }

#if ENABLE_AUDIO
  virtual void HandleBeat(bool bMajor, float elapsed, float span) override {}
#endif
};
