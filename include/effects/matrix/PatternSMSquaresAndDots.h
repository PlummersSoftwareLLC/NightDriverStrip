#pragma once

#include "effects/strip/musiceffect.h"
#include "effectmanager.h"
#include <algorithm>

// Inspired from https://editor.soulmatelights.com/gallery/843-squares-and-dots
// This looks better on 2812's than on HUB75.

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
    LEDStripEffect(EFFECT_MATRIX_SMSQUARES_AND_DOTS, "Squares and Dots")
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
    int hue = random8();
    int y1 = y;
    for (int j = 0; j < 3; j++) {
      int x1 = x;
      for (int i = 0; i < 3; i++) {
        x1 = std::clamp(x1, 3, MATRIX_WIDTH-3);
        y1 = std::clamp(y1, 3, MATRIX_HEIGHT-3);
        uint16_t index = g()->xy(x1, y1);
        if (sprites[numSpr][i][j])
        {
          g()->leds[index].setHue(hue);
        }
        else
        {
            g()->leds[index] = 0;
        }
        x1++;
      }
      y1++;
    }
}

  void Start() override
  {
    g()->Clear();
    for (byte x = 0; x < MATRIX_WIDTH / 3 + 1; x++) {
      for (byte y = 0; y < MATRIX_HEIGHT / 3 + 1; y++) {
        printSpr(x * 3, y * 3, random8(2));
      }
    }
  }

  void Draw() override
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
