#pragma once

#include "effectmanager.h"

// Adapted from https://editor.soulmatelights.com/gallery/1004-amazing-lightning
//
// Probably best with a diffuser.

class PatternSMLightning : public LEDStripEffect
{
 private:
  uint8_t call = 0;    // time
  uint8_t aux0 = 127;  // root position
  uint8_t aux1 = 4;    // how many lines

  // all based on perlin noise - nice!
  int RANDOM(const uint8_t i, const int y) {
    return inoise8_raw(aux0 * 77 + (i + 1) * 199, y * 33) * 2;
  }

 public:
  PatternSMLightning()
      :
        LEDStripEffect(EFFECT_MATRIX_SMLIGHTNING, "Lightning") {
  }

  PatternSMLightning(const JsonObjectConst& jsonObject)
      :
        LEDStripEffect(jsonObject) {
  }

  void Start() override { g()->Clear(); }

  void Draw() override {
    uint8_t matrixWidth = MATRIX_WIDTH;
    uint8_t matrixHeight = MATRIX_HEIGHT;

    g()->Clear();

    call += aux1;
    if (call > 250) {
      call = 0;
      if (aux1 > 1) {
        aux1 = 1;
      } else {
        aux1 = 4;
        aux0 = map8(random8(), 64, 192);
        fill_solid(g()->leds, matrixWidth * matrixHeight, CRGB::Black);
      }
    }

    if (call == 0 || call == 20 || call == 50 && aux1 == 1) {  // draw main line
      int y = matrixHeight;
      int16_t x0 = aux0 - RANDOM(0, y);
      for (; y >= 0; y--) {
        auto x = x0 + RANDOM(0, y);
        g()->leds[XY(x * matrixWidth / 255, y)] = CRGB::White;
      }

    } else if (aux1 != 1) {  // draw only parts of 'searching' lines
      for (int i = 0; i < aux1; i++) {
        int y = matrixHeight;
        int16_t x0 =
            aux0 - RANDOM(i, y);  // substract the first offset so that all
                                  // lines come from the same place (aux0)
        y = map8(255 - call, 0, matrixHeight);
        auto x = x0 + RANDOM(i, y);
        g()->leds[XY(x * matrixWidth / 255, y)] = CRGB::Gray;
      }
    }

    nscale8(g()->leds, matrixWidth * matrixHeight, 250);
  }
};
