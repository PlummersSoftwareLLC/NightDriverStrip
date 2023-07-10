#pragma once

#include "effects/strip/musiceffect.h"
#include "effectmanager.h"

#if USE_AUDIO
class PatternSMSquaresAndDots : public BeatEffectBase, public LEDStripEffect
#else
class PatternSMSquaresAndDots : public LEDStripEffect
#endif
{
private:
   const byte sprites[2][3][3] = {1,1,1,1,0,1,1,1,1,0,0,0,0,1,0,0,0,0,};

public:
  PatternSMSquaresAndDots() :
#if USE_AUDIO
    BeatEffectBase(1.50, 0.05),
#endif
    LEDStripEffect(EFFECT_MATRIX_SMSTROBE_DIFFUSION, "Strobe Diffusion")
    {
    }

  PatternSMSquaresAndDots(const JsonObjectConst& jsonObject) :
#if USE_AUDIO
    BeatEffectBase(1.50, 0.05),
#endif
    LEDStripEffect(jsonObject)
  {
  }

  void printSpr(byte x, byte y, byte numSpr) {
    byte hue = random8();
      byte y1 = y;
    for (int j = 0; j < 3; j++) {
      byte x1 = x;
      for (int i = 0; i < 3; i++) {
        uint16_t index = g()->xy(x1, y1);
        if (sprites[numSpr][i][j]) g()->leds[index].setHue(hue);
        else g()->leds[index] = 0;
        x1++;
      }
      y1++;
    }
}

  virtual void Start() override
  {
    for (byte x = 0; x < MATRIX_WIDTH / 3 + 1; x++) {
      for (byte y = 0; y < MATRIX_HEIGHT / 3 + 1; y++) {
        printSpr(x * 3, y * 3, random8(2));
      }
    }
  }

  virtual void Draw() override
  {
    EVERY_N_MILLISECONDS(300) {
    printSpr((random8(MATRIX_WIDTH) % (MATRIX_WIDTH / 3 + 1)) * 3, (random8(MATRIX_HEIGHT) % (MATRIX_HEIGHT / 3 + 1)) * 3, random8(2));
  }

  }
#if USE_AUDIO
	virtual void HandleBeat(bool bMajor, float elapsed, float span) override
	{

	}
#endif


};
