#pragma once

#include "effectmanager.h"
#include "effects/strip/musiceffect.h"

#if ENABLE_AUDIO
class PatternSMSunRadiation : public BeatEffectBase,
                              public LEDStripEffect
#else
class PatternSMSunRadiation : public LEDStripEffect
#endif
{
 private:
  static constexpr int LED_ROWS = MATRIX_HEIGHT;
  static constexpr int LED_COLS = MATRIX_WIDTH;
  // sun radiation
  // fastled 16x16 matrix demo (work for any size too)
  // Yaroslaw Turbin 19.01.2020
  // https://vk.com/ldirko
  // https://www.reddit.com/user/ldirko/

  // https://wokwi.com/arduino/projects/288134661735973389
  CRGB chsvLut[256];
  byte bump[(LED_COLS + 2) * (LED_ROWS + 2)];

  void generateCHSVlut() {
    for (int j = 0; j < 256; j++)
      chsvLut[j] = HeatColor(j / 1.4);  // 256 pallette color
  }

  void generatebump() {
    int t = millis() / 4;
    int index = 0;
    for (byte j = 0; j < (LED_ROWS + 2); j++) {
      for (byte i = 0; i < (LED_COLS + 2); i++) {
        byte col = (inoise8_raw(i * 25, j * 25, t)) / 2;
        bump[index++] = col;
      }
    }
  }

  void Bumpmap() {
    int yindex = LED_COLS + 3;
    int8_t vly = -(LED_ROWS / 2 + 1);
    for (byte y = 0; y < LED_ROWS; y++) {
      ++vly;
      int8_t vlx = -(LED_COLS / 2 + 1);
      for (byte x = 0; x < LED_COLS; x++) {
        ++vlx;
        int8_t nx = bump[x + yindex + 1] - bump[x + yindex - 1];
        int8_t ny = bump[x + yindex + (LED_COLS + 2)] -
                    bump[x + yindex - (LED_COLS + 2)];
        // Mesmerizer: changed scales to 4 from 7 to take up more of our larger
        // screen w/o repeats.
        const int8_t scaleX = 4;
        const int8_t scaleY = 4;
        byte difx = abs8(vlx * scaleX - nx);
        byte dify = abs8(vly * scaleY - ny);
        int temp = difx * difx + dify * dify;
        int col = 255 - temp / 8;  // 8 its a size of effect

        if (col < 0) col = 0;
        g()->leds[g()->xy(x, y)] = chsvLut[col];  // thx satubarosu ))
      }
      yindex += (LED_COLS + 2);
    }
  }

 public:
  PatternSMSunRadiation()
      :
#if ENABLE_AUDIO
        BeatEffectBase(1.50, 0.05),
#endif
        LEDStripEffect(EFFECT_MATRIX_SMSUN_RADIATION, "Sun Radiation") {
  }

  PatternSMSunRadiation(const JsonObjectConst& jsonObject)
      :
#if ENABLE_AUDIO
        BeatEffectBase(1.50, 0.05),
#endif
        LEDStripEffect(jsonObject) {
  }

  virtual void Start() override {
    g()->Clear();
    generateCHSVlut();
  }

  virtual void Draw() override {
#if ENABLE_AUDIO
    ProcessAudio();
#endif
    generatebump();
    Bumpmap();
  }

#if ENABLE_AUDIO
  virtual void HandleBeat(bool bMajor, float elapsed, float span) override {}
#endif
};
