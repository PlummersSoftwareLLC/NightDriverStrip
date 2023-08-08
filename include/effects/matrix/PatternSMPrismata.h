#pragma once

#include "effectmanager.h"

// Derived from https://editor.soulmatelights.com/gallery/1110-prismata
// Sine waves oscillate against each other over a swirling rainbow background.

class PatternSMPrismata : public LEDStripEffect
{
 private:
  uint8_t Speed = 30U;  // 1-255
  uint8_t Scale = 8;    // 1-100 is palette
  uint8_t hue;

  const TProgmemRGBPalette16* curPalette = &PartyColors_p;

  void drawPixelXY(int8_t x, int8_t y, CRGB color) {
    if (x < 0 || x > (MATRIX_WIDTH - 1) || y < 0 || y > (MATRIX_HEIGHT - 1))
      return;
    // Mesmerizer flips the Y axis here.
    uint32_t thisPixel = g()->xy((uint8_t)x, MATRIX_HEIGHT - 1 - (uint8_t)y);
    g()->leds[thisPixel] = color;
  }

 public:
  PatternSMPrismata()
      :
        LEDStripEffect(EFFECT_MATRIX_SMPRISMATA, "Prismata") {
  }

  PatternSMPrismata(const JsonObjectConst& jsonObject)
      :
        LEDStripEffect(jsonObject) {
  }

  void Start() override { g()->Clear(); }

  void Draw() override {

    g()->BlurFrame(20);  // @Palpalych посоветовал делать размытие
    g()->DimAll(255U - (Scale - 1U) % 11U * 3U);

    for (uint8_t x = 0; x < MATRIX_WIDTH; x++) {
      // uint8_t y = beatsin8(x + 1, 0, HEIGHT-1); 
      // In English: I tried to disassemble this function to the source code and
      // insert a speed controller into it instead of 28 in the original, there
      // was 280, there was no multiplication by .Speed, and instead of >> 17
      // there was (<< 8) >> 24. In short, the original speed is achieved with
      // the .Speed = 20 slider

      uint8_t beat = (GET_MILLIS() * (accum88(x + 1)) * 28 * Speed) >> 17;
      uint8_t y = scale8(sin8(beat), MATRIX_HEIGHT - 1);
      //и получилось!!!

      drawPixelXY(x, y, ColorFromPalette(*curPalette, x * 7 + hue));
    }
  }
};
