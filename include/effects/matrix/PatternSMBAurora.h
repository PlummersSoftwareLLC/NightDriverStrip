#pragma once

#include "effectmanager.h"
#include "effects/strip/musiceffect.h"

#if ENABLE_AUDIO
class PatternSMStrobeDiffusion : public BeatEffectBase,
                                 public LEDStripEffect
#else
class PatternSMStrobeDiffusion : public LEDStripEffect
#endif
{
 private:
 public:
  PatternSMStrobeDiffusion()
      :
#if ENABLE_AUDIO
        BeatEffectBase(1.50, 0.05),
#endif
        LEDStripEffect(EFFECT_MATRIX_SMSTROBE_DIFFUSION, "Strobe Diffusion") {
  }

  PatternSMStrobeDiffusion(const JsonObjectConst& jsonObject)
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

#if ENABLE_AUDIO
  virtual void HandleBeat(bool bMajor, float elapsed, float span) override {}
#endif
};
