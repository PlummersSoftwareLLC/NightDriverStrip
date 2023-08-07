#pragma once

#include "effectmanager.h"

// Inspired by https://editor.soulmatelights.com/gallery/1620-rainbow-tunel
// Like Hypnosis, a swirling radial rainbow, but entering a black hole.

class PatternSMRainbowTunnel : public LEDStripEffect
{
 private:
  // RadialRainbow
  // Stepko and Sutaburosu
  // 23/12/21
  static constexpr int LED_COLS = MATRIX_WIDTH;
  static constexpr int LED_ROWS = MATRIX_HEIGHT;

  bool setupm = 1;
  const uint8_t C_X = LED_COLS / 2;
  const uint8_t C_Y = LED_ROWS / 2;
  const uint8_t mapp = 255 / LED_COLS;
  struct {
    uint8_t angle;
    uint8_t radius;
  } rMap[LED_COLS][LED_ROWS];

 public:
  PatternSMRainbowTunnel()
      :
        LEDStripEffect(EFFECT_MATRIX_SMRAINBOW_TUNNEL, "Rainbow Tunnel") {
  }

  PatternSMRainbowTunnel(const JsonObjectConst& jsonObject)
      :
        LEDStripEffect(jsonObject) {
  }

  void Start() override {
    g()->Clear();
    for (int8_t x = -C_X; x < C_X + (LED_COLS % 2); x++) {
      for (int8_t y = -C_Y; y < C_Y + (LED_ROWS % 2); y++) {
        rMap[x + C_X][y + C_Y].angle = 128 * (atan2(y, x) / PI);
        rMap[x + C_X][y + C_Y].radius = hypot(x, y) * mapp;  // thanks
                                                             // Sutaburosu
      }
    }
  }

  void Draw() override {
    static byte scaleX = 4;
    static byte scaleY = 4;
    static byte speed = 2;
    static uint16_t t;

    t += speed;
    for (uint8_t x = 0; x < LED_COLS; x++) {
      for (uint8_t y = 0; y < LED_ROWS; y++) {
        byte angle = rMap[x][y].angle;
        byte radius = rMap[x][y].radius;
        g()->leds[g()->xy(x, y)] =
            CHSV((angle * scaleX) - t + (radius * scaleY), 255,
                 constrain(radius * 2, 0, 255));
      }
    }
  }
};
