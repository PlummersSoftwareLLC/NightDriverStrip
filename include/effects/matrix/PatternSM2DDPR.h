#pragma once

#include "effects/strip/musiceffect.h"
#include "effectmanager.h"

// Inspired by https://editor.soulmatelights.com/gallery/1479-2dd-pr-centering
// Looks best on a square display, but OK on rectangles.
// I'll admit this math may as well be magic, but it's pretty.

#if USE_AUDIO
class PatternSM2DDPR : public BeatEffectBase, public LEDStripEffect
#else
class PatternSM2DDPR : public LEDStripEffect
#endif
{
private:
  uint8_t ZVoffset = 0;

  const int Scale = 127;
  const int Speed = 215;
  uint32_t effTimer;

  float HALF_WIDTH = MATRIX_WIDTH * .5;
  float HALF_HEIGHT = MATRIX_HEIGHT * .5;
  float radius = HALF_WIDTH;
//   byte effect = 1;

public:
  PatternSM2DDPR() :
#if USE_AUDIO
    BeatEffectBase(1.50, 0.05),
#endif
    LEDStripEffect(EFFECT_MATRIX_SM2DDPR, "2DD PR")
    {
    }

  PatternSM2DDPR(const JsonObjectConst& jsonObject) :
#if USE_AUDIO
    BeatEffectBase(1.50, 0.05),
#endif
    LEDStripEffect(jsonObject)
  {
  }

  virtual void Start() override
  {
  }


  // Use integer-only Pythagorean to compute the radius from x^2 and y^2.
  int16_t ZVcalcRadius(int16_t x, int16_t y)
  {
    x *= x;
    y *= y;
    int16_t radi = sin8(x + y);
    return radi;
  }

  // From point X,Y, find is the distance to center(X,Y)
  int16_t ZVcalcDist(uint8_t x, uint8_t y, float center_x, float center_y)
  {
    int16_t a = (center_y - y - .5);
    int16_t b = (center_x - x - .5);
    int16_t dist = ZVcalcRadius(a, b);
    return dist;
  }

  virtual void Draw() override
  {
    effTimer =sin8 (millis()/6000)/10;

    EVERY_N_MILLISECONDS(1) {
      ZVoffset += 4;
    }

    for (int x = 0; x < MATRIX_WIDTH; x++) {
      for (int y = 0; y < MATRIX_HEIGHT; y++) {
        int dist = ZVcalcDist(x, y, HALF_WIDTH, HALF_HEIGHT);

        // exclude outside of circle
        int brightness = 1;
        if (dist += radius) {
          brightness = map(dist, -effTimer,radius, 255, 110);
          brightness += ZVoffset;
          brightness = sin8(brightness);
        }

        int index = g()->xy(x, y);
        int hue = map(dist, radius,-3,  125, 255);
        g()->leds[index] = CHSV(hue, 255, brightness);
      }
    }
  }
#if USE_AUDIO
	virtual void HandleBeat(bool bMajor, float elapsed, float span) override
	{

	}
#endif


};
