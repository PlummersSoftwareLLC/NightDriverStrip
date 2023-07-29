#pragma once

#include "effectmanager.h"
#include "effects/matrix/Boid.h"
#include "effects/matrix/Vector.h"
#include "effects/strip/musiceffect.h"

// Derived from https://editor.soulmatelights.com/gallery/2132-F
// This makes a very cool green vine that grows up the display.

#if ENABLE_AUDIO
class PatternSMFlocking : public BeatEffectBase,
                          public LEDStripEffect
#else
class PatternSMFlocking : public LEDStripEffect
#endif
{
 private:
  const int WIDTH = MATRIX_WIDTH;
  const int HEIGHT = MATRIX_HEIGHT;
  const int COLS = MATRIX_WIDTH;
  const int ROWS = MATRIX_HEIGHT;
  static const int NUM_PARTICLES =
      20;  // set this to the number of particles. the varialbe describes what
           // it's supposed to be. it works with 50 but it's a little slow. on
           // an esp32 it looks pretty nice at that number 15 is a safe number

  CRGB getPixColorXY(uint8_t x, uint8_t y) {
    return g()->leds[g()->xy(x, MATRIX_HEIGHT - 1 - y)];
  }

  void drawPixelXY(int8_t x, int8_t y, CRGB color) {
    if (x < 0 || x > (MATRIX_WIDTH - 1) || y < 0 || y > (MATRIX_HEIGHT - 1))
      return;
    // Mesmerizer flips the Y axis here.
    uint32_t thisPixel = g()->xy((uint8_t)x, MATRIX_HEIGHT - 1 - (uint8_t)y);
    g()->leds[thisPixel] = color;
  }

#undef WU_WEIGHT
static inline uint8_t WU_WEIGHT(uint8_t a, uint8_t b) {return (uint8_t)(((a) * (b) + (a) + (b)) >> 8);}

  void drawPixelXYF(float x, float y, CRGB color) {
    // if (x<0 || y<0) return; //не похоже, чтобы отрицательные значения хоть
    // как-нибудь учитывались тут // зато с этой строчкой пропадает нижний ряд
    // extract the fractional parts and derive their inverses
    uint8_t xx = (x - (int)x) * 255, yy = (y - (int)y) * 255, ix = 255 - xx,
            iy = 255 - yy;
// calculate the intensities for each affected pixel
// #define WU_WEIGHT(a, b) ((uint8_t)(((a) * (b) + (a) + (b)) >> 8))
    std::array<uint8_t, 4> wu{WU_WEIGHT(ix, iy), WU_WEIGHT(xx, iy),
                              WU_WEIGHT(ix, yy), WU_WEIGHT(xx, yy)};
    for (uint8_t i = 0; i < 4; i++) {
      int16_t xn = x + (i & 1), yn = y + ((i >> 1) & 1);
      CRGB clr = getPixColorXY(xn, yn);
      clr.r = qadd8(clr.r, (color.r * wu[i]) >> 8);
      clr.g = qadd8(clr.g, (color.g * wu[i]) >> 8);
      clr.b = qadd8(clr.b, (color.b * wu[i]) >> 8);
      drawPixelXY(xn, yn, clr);
    }
  }

  byte hue = 0;
  std::array<Boid, NUM_PARTICLES> boids;  // this makes the boids

  uint16_t x;
  uint16_t y;
  uint16_t z;

  uint16_t speed = 10;
  uint16_t scale = 30;

 public:
  PatternSMFlocking()
      :
#if ENABLE_AUDIO
        BeatEffectBase(1.50, 0.05),
#endif
        LEDStripEffect(EFFECT_MATRIX_SMFLOCKING, "Flocking") {
  }

  PatternSMFlocking(const JsonObjectConst &jsonObject)
      :
#if ENABLE_AUDIO
        BeatEffectBase(1.50, 0.05),
#endif
        LEDStripEffect(jsonObject) {
  }

  void Start() override {
    g()->Clear();

    x = random16();
    y = random16();
    z = random16();

    for (auto &boid : boids) {
      boid = Boid(random(COLS), 0);
    }
  }

  void Draw() override {
#if ENABLE_AUDIO
    ProcessAudio();
#endif
    for (auto &boid : boids) {
      boid.avoidBorders();
      boid.update(boids, NUM_PARTICLES);

      drawPixelXYF(
          boid.location.x, boid.location.y,
          ColorFromPalette(PartyColors_p, boid.hue * 15, 255, LINEARBLEND));
    }
    fadeAllChannelsToBlackBy(75);
    x += speed;
    y += speed;
    z += speed;
  }

#if ENABLE_AUDIO
  virtual void HandleBeat(bool bMajor, float elapsed, float span) override {}
#endif
};
