#pragma once

#include "effectmanager.h"
#include "effects/strip/musiceffect.h"
// Derived from https://editor.soulmatelights.com/gallery/1516-mirage

#if ENABLE_AUDIO
class PatternSMMirage : public BeatEffectBase,
                        public LEDStripEffect
#else
class PatternSMMirage : public LEDStripEffect
#endif
{
 private:
  uint8_t speed = 128, hue = 70;
  const uint8_t div = 4, val = 255;
  static constexpr int LED_COLS = MATRIX_WIDTH;
  static constexpr int LED_ROWS = MATRIX_HEIGHT;

  byte buff[LED_COLS + 2][LED_ROWS + 2];

  void blur() {
    uint16_t sum;
    for (byte x = 1; x < LED_COLS + 1; x++) {
      for (byte y = 1; y < LED_ROWS + 1; y++) {
        sum = buff[x][y];
        sum += buff[x + 1][y];
        sum += buff[x][y - 1];
        sum += buff[x][y + 1];
        sum += buff[x - 1][y];
        sum /= 5;
        buff[x][y] = sum;
      }
    }
  }

#undef WU_WEIGHT
  // static inline uint8_t WU_WEIGHT(uint8_t a, uint8_t b) {return
  // (uint8_t)(((a) * (b) + (a) + (b)) >> 8);}

  void drawDot(float x, float y, byte a) {
    uint8_t xx = (x - (int)x) * 255, yy = (y - (int)y) * 255, ix = 255 - xx,
            iy = 255 - yy;
#define WU_WEIGHT(a, b) ((uint8_t)(((a) * (b) + (a) + (b)) >> 8))
    uint8_t wu[4] = {WU_WEIGHT(ix, iy), WU_WEIGHT(xx, iy), WU_WEIGHT(ix, yy),
                     WU_WEIGHT(xx, yy)};
#undef WU_WEIGHT
    // multiply the intensities by the colour, and saturating-add them to the
    // pixels
    for (uint8_t i = 0; i < 4; i++) {
      int16_t xn = x + (i & 1), yn = y + ((i >> 1) & 1);
      byte clr = buff[xn][yn];
      clr = constrain(qadd8(clr, (a * wu[i]) >> 8), 0, 255);
      buff[xn][yn] = clr;
    }
  }

 public:
  PatternSMMirage()
      :
#if ENABLE_AUDIO
        BeatEffectBase(1.50, 0.05),
#endif
        LEDStripEffect(EFFECT_MATRIX_SMMIRAGE, "Mirage") {
  }

  PatternSMMirage(const JsonObjectConst& jsonObject)
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
    blur();
    float x1 = (float)beatsin88(15 * speed, div, (LED_COLS - 1) * div) / div;
    float y1 = (float)beatsin88(20 * speed, div, (LED_ROWS)*div) / div;
    float x2 = (float)beatsin88(16 * speed, div, (LED_COLS - 1) * div) / div;
    float y2 = (float)beatsin88(14 * speed, div, (LED_ROWS)*div) / div;
    float x3 = (float)beatsin88(12 * speed, div, (LED_COLS - 1) * div) / div;
    float y3 = (float)beatsin88(16 * speed, div, (LED_ROWS)*div) / div;
    drawDot(x1, y1, val);
    drawDot(x1 + 1, y1, val);
    drawDot(x2, y2, val);
    drawDot(x2 + 1, y2, val);
    drawDot(x3, y3, val);
    drawDot(x3 + 1, y3, val);
    hue++;
    for (byte x = 1; x < LED_COLS + 1; x++) {
      for (byte y = 1; y < LED_ROWS + 1; y++) {
        g()->leds[g()->xy(x - 1, y - 1)] = CHSV(hue, buff[x][y], 255);
      }
    }
  }

#if ENABLE_AUDIO
  virtual void HandleBeat(bool bMajor, float elapsed, float span) override {}
#endif
};
