#pragma once

#include "effectmanager.h"
#include "effects/matrix/Boid.h"
#include "effects/matrix/Vector.h"
#include "effects/strip/musiceffect.h"

// Inspired by
// https://editor.soulmatelights.com/gallery/2252-overcomplicatedbubbles The
// flocking code gives the bubbles a bit of a swarming effect

#if ENABLE_AUDIO
class PatternSMBubbles : public BeatEffectBase,
                         public LEDStripEffect
#else
class PatternSMBubbles : public LEDStripEffect
#endif
{
 private:
  static constexpr int NUM_PARTICLES = 80;
  struct xboid {
    float scale{1};
    float mass{1};
    int scaledir{1};
    int bbrightness{0};
  };
  Boid boids[NUM_PARTICLES];
  xboid xboids[NUM_PARTICLES];
  uint16_t x;
  uint16_t y;
  uint16_t z;
  int speed = 1;
  int count = NUM_PARTICLES;

  [[nodiscard]] CRGB getPixColorXY(uint8_t x, uint8_t y) const {
    return g()->leds[g()->xy(x, MATRIX_HEIGHT - 1 - y)];
    // return g()->leds[g()->xy(x, y)];
  }

  void drawPixelXY(int8_t x, int8_t y, CRGB color) {
    if (x < 0 || x > (MATRIX_WIDTH - 1) || y < 0 || y > (MATRIX_HEIGHT - 1))
      return;
    // Mesmerizer flips the Y axis here.
    uint32_t thisPixel = g()->xy((uint8_t)x, MATRIX_HEIGHT - 1 - (uint8_t)y);
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
    // #define WU_WEIGHT(a,b) ((uint8_t) (((a)*(b)+(a)+(b))>>8))
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
  PatternSMBubbles()
      :
#if ENABLE_AUDIO
        BeatEffectBase(1.50, 0.05),
#endif
        LEDStripEffect(EFFECT_MATRIX_SMBUBBLES, "Bubbles") {
  }

  PatternSMBubbles(const JsonObjectConst& jsonObject)
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

    for (int i = 0; i < count; i++) {
      boids[i] = Boid(random(MATRIX_WIDTH), 0);
      boids[i].hue = random(40, 255);
      ;
      xboids[i].mass = random(1.0, 2.5);
      xboids[i].scale = random(0.5, 68.5);
    }
  }

  void Draw() override {
#if ENABLE_AUDIO
    ProcessAudio();
#endif

    EVERY_N_MILLISECONDS(2000) {
      if (count <= 5 || count >= 79) {
        speed = -speed;
      }
      count += speed;
    }

    for (int i = 5; i < count; i++) {
      Boid* boid = &boids[i];
      xboid* xboid = &xboids[i];
      xboid->scale += xboid->scaledir;
#if 0
   xboid->bbrightness = boid->location.y * 15;
   if   (xboid->bbrightness >= 255) xboid-> bbrightness = 255;
#else
      xboid->bbrightness = map(boid->location.y, 0, MATRIX_HEIGHT, 65, 255);
#endif
      if (xboid->scale > 200 || xboid->scale < 10) {
        xboid->scaledir = -xboid->scaledir;
      }
      int ioffset = xboid->scale * boid->location.x;
      int joffset = xboid->scale * boid->location.y;
      float angle = inoise8(x + ioffset, y + joffset, z);

      boid->velocity.x = .2 * (float)sin8(angle) * 0.007 - 1.0;
      boid->velocity.y = -((float)cos8(angle) * 0.006 - 1.0);
      boid->update();

      drawPixelXYF(boid->location.x, boid->location.y,
                   ColorFromPalette(OceanColors_p, boid->hue * 15,
                                    xboid->bbrightness, LINEARBLEND));

      if (boid->location.x < 0 || boid->location.x >= MATRIX_WIDTH ||
          boid->location.y < 0 || boid->location.y >= MATRIX_HEIGHT) {
        boid->location.x = random(MATRIX_WIDTH);
        boid->location.y = 0;
      }
    }
    fadeAllChannelsToBlackBy(105);
    x += speed;
    y += speed;
    z += speed;
  }

#if ENABLE_AUDIO
  void HandleBeat(bool bMajor, float elapsed, float span) override {}
#endif
};
