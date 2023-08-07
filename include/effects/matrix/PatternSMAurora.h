#pragma once

#include "effectmanager.h"

// Derived from https://editor.soulmatelights.com/gallery/1244-aurora-borealis
// Could use some fine-tuning on timings. speed doesn't work like the original.

class PatternSMAurora : public LEDStripEffect
{
 private:
  const int LED_COLS = MATRIX_WIDTH;
  const int LED_ROWS = MATRIX_HEIGHT;
  unsigned long timer{0};
  byte speed = 7;   // Scale  0-255
  byte scale = 60;  // Speed 0-255

  float adjustHeight = fmap(LED_ROWS, 8, 32, 28, 12);
  uint16_t adjScale = map(LED_COLS, 8, 64, 310, 63);

  static constexpr TProgmemRGBPalette16 GreenAuroraColors_p FL_PROGMEM = {
      0x000000, 0x003300, 0x006600, 0x009900, 0x00cc00, 0x00ff00,
      0x33ff00, 0x66ff00, 0x99ff00, 0xccff00, 0xffff00, 0xffcc00,
      0xff9900, 0xff6600, 0xff3300, 0xff0000};

  float fmap(const float x, const float in_min, const float in_max,
             const float out_min, const float out_max) {
    return (out_max - out_min) * (x - in_min) / (in_max - in_min) + out_min;
  }

 public:
  PatternSMAurora()
      :
        LEDStripEffect(EFFECT_MATRIX_SMAURORA, "Aurora Borealis") {
  }

  PatternSMAurora(const JsonObjectConst& jsonObject)
      :
        LEDStripEffect(jsonObject) {
  }

  void Start() override { g()->Clear(); }

  void Draw() override {
    uint16_t _scale = map(scale, 1, 255, 30, adjScale);
    byte _speed = map(speed, 1, 255, 128, 16);

    EVERY_N_MILLIS(80)
    for (byte x = 0; x < LED_COLS; x++) {
      for (byte y = 0; y < LED_ROWS; y++) {
        timer++;
        g()->leds[g()->xy(x, y)] = ColorFromPalette(
            GreenAuroraColors_p,
            qsub8(inoise8(timer % 2 + x * _scale, y * 16 + timer % 16,
                          timer / _speed),
                  fabs((float)LED_ROWS / 2 - (float)y) * adjustHeight));
      }
    }
  }
};
