#pragma once

#include "effectmanager.h"
#include "effects/matrix/Boid.h"
#include "effects/matrix/Vector.h"

// Inspired by https://editor.soulmatelights.com/gallery/2254-flocking
// Almost a textbook Boid demo: Separation, Cohesion, Alignment.

class PatternSMFlocking : public LEDStripEffect
{
 private:
  // With 10 they have just about enough time to spread after a collision on a
  // 64x32.
  static constexpr int NUM_PARTICLES = 10;
  Boid boids[NUM_PARTICLES];

  [[nodiscard]] CRGB getPixColorXY(uint8_t x, uint8_t y) const {
    return g()->leds[XY(x, MATRIX_HEIGHT - 1 - y)];
    // return g()->leds[XY(x, y)];
  }

  void drawPixelXY(int8_t x, int8_t y, CRGB color) {
    if (x < 0 || x > (MATRIX_WIDTH - 1) || y < 0 || y > (MATRIX_HEIGHT - 1))
      return;
    // Mesmerizer flips the Y axis here.
    uint32_t thisPixel = XY((uint8_t)x, MATRIX_HEIGHT - 1 - (uint8_t)y);
    g()->leds[thisPixel] = color;
  }

#undef WU_WEIGHT
  static inline uint8_t WU_WEIGHT(uint8_t a, uint8_t b) {
    return (uint8_t)(((a) * (b) + (a) + (b)) >> 8);
  }

  void drawPixelXYF(float x, float y, CRGB color)  //, uint8_t darklevel = 0U)
  {
    //  if (x<0 || y<0) return; //не похоже, чтобы отрицательные значения хоть
    //  как-нибудь учитывались тут // зато с этой строчкой пропадает нижний ряд
    // extract the fractional parts and derive their inverses
    uint8_t xx = (x - (int)x) * 255, yy = (y - (int)y) * 255, ix = 255 - xx,
            iy = 255 - yy;
    // calculate the intensities for each affected pixel
    // #define WU_WEIGHT(a, b) ((uint8_t)(((a) * (b) + (a) + (b)) >> 8))
    uint8_t wu[4] = {WU_WEIGHT(ix, iy), WU_WEIGHT(xx, iy), WU_WEIGHT(ix, yy),
                     WU_WEIGHT(xx, yy)};
    // multiply the intensities by the colour, and saturating-add them to the
    // pixels
    for (uint8_t i = 0; i < 4; i++) {
      int16_t xn = x + (i & 1), yn = y + ((i >> 1) & 1);
      CRGB clr = getPixColorXY(xn, yn);
      clr.r = qadd8(clr.r, (color.r * wu[i]) >> 8);
      clr.g = qadd8(clr.g, (color.g * wu[i]) >> 8);
      clr.b = qadd8(clr.b, (color.b * wu[i]) >> 8);
      drawPixelXY(xn, yn, clr);
    }
  }

 public:
  PatternSMFlocking()
      :
        LEDStripEffect(EFFECT_MATRIX_SMFLOCKING, "Flocking") {
  }

  PatternSMFlocking(const JsonObjectConst& jsonObject)
      :
        LEDStripEffect(jsonObject) {
  }

  void Start() override {
    g()->Clear();
    for (int i = 0; i < NUM_PARTICLES; i++) {
      boids[i] = Boid(random(MATRIX_WIDTH), 0);
    }
  }

  void Draw() override {
    for (auto& boid : boids) {
      boid.flock(boids, NUM_PARTICLES);
      //   	   	boid.avoidBorders();
      boid.bounceOffBorders(.7);
      boid.update();
      drawPixelXYF(
          boid.location.x, boid.location.y,
          ColorFromPalette(PartyColors_p, boid.hue * 15, 255, LINEARBLEND));
    }
    // fadeToBlackBy(g()->leds, NUM_LEDS,75);
    fadeAllChannelsToBlackBy(75);
  }
};
