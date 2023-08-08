#pragma once

#include "effectmanager.h"

class PatternSMStrobeDiffusion : public LEDStripEffect
{
 private:
 public:
  PatternSMStrobeDiffusion()
      :
        LEDStripEffect(EFFECT_MATRIX_SMSTROBE_DIFFUSION, "Strobe Diffusion") {
  }

  PatternSMStrobeDiffusion(const JsonObjectConst& jsonObject)
      :
        LEDStripEffect(jsonObject) {
  }

  void Start() override { g()->Clear(); }

  void Draw() override {
#if 0
  uint16_t _scale = map(scale, 1, 255, 30, adjScale);
  byte _speed = map(speed, 1, 255, 128, 16);
  for (byte x = 0; x < LED_COLS; x++)
  {
    for (byte y = 0; y < LED_ROWS; y++)
    {
      timer++;
      leds[XY(x, y)] =
          ColorFromPalette(GreenAuroraColors_p,
          qsub8(
          inoise8(timer % 2 + x * _scale,
          y * 16 + timer % 16,
          timer / _speed),
          fabs((float)LED_ROWS / 2 - (float)y) * adjastHeight));
    }
  }
#endif
  }
};
