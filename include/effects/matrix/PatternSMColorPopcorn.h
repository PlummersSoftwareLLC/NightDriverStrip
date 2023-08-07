#pragma once

#include "effectmanager.h"
#include "effects/strip/musiceffect.h"
// Derived from
// https://editor.soulmatelights.com/gallery/2269-aaron-gotwalts-unknown-pleasure
//
// Colored, accelerated, gravity popcorn balls.

#if ENABLE_AUDIO
class PatternSMColorPopcorn : public BeatEffectBase,
                              public LEDStripEffect
#else
class PatternSMColorPopcorn : public LEDStripEffect
#endif
{
 private:
  CRGBPalette16 currentPalette = RainbowColors_p;

  uint8_t gravity = 16;
#define NUM_ROCKETS 8

  using Rocket = struct { int32_t x, y, xd, yd; };

  Rocket rockets[NUM_ROCKETS];

  void restart_rocket(uint8_t r) {
    // rockets[r].x = random16() >> 4;
    rockets[r].xd = random8() + 32;
    if (rockets[r].x > (MATRIX_WIDTH / 2 * 256)) {
      // leap towards the centre of the screen
      rockets[r].xd = -rockets[r].xd;
    }
    // controls the leap height
    rockets[r].yd = random8() * 5 + (MATRIX_HEIGHT - 1) * 1;
  }

  void move() {
    for (uint8_t r = 0; r < NUM_ROCKETS; r++) {
      // add the X & Y velocities to the positions
      rockets[r].x += rockets[r].xd;
      rockets[r].y += rockets[r].yd;

      // bounce off the floor?
      if (rockets[r].y < 0) {
        rockets[r].yd = (-rockets[r].yd * 240) >> 8;
        rockets[r].y = rockets[r].yd;
        // settled on the floor?
        if (rockets[r].y <= 200) {  // if you change gravity, this will probably
                                    // need changing too
          restart_rocket(r);
        }
      }

      // bounce off the sides of the screen?
      if (rockets[r].x < 0 || rockets[r].x > MATRIX_WIDTH * 256) {
        rockets[r].xd = (-rockets[r].xd * 248) >> 8;
        // force back onto the screen, otherwise they eventually sneak away
        if (rockets[r].x < 0) {
          rockets[r].x = rockets[r].xd;
          rockets[r].yd += rockets[r].xd;
        } else {
          rockets[r].x = (MATRIX_WIDTH * 256) - rockets[r].xd;
        }
      }

      // gravity
      rockets[r].yd -= gravity;

      // viscosity
      rockets[r].xd = (rockets[r].xd * 224) >> 8;
      rockets[r].yd = (rockets[r].yd * 224) >> 8;
    }
  }

  uint8_t wu_weight(uint8_t a, uint8_t b) {
    // This idea came from Xiaolin Wu.
    return ((a * b) + (a + b)) >> 8;
  }

  void paint() {
    // fill_solid(g()->leds, N_LEDS, CRGB::Black);
    for (uint8_t r = 0; r < NUM_ROCKETS; r++) {
      CRGB rgb = ColorFromPalette(currentPalette, r * (256 / NUM_ROCKETS), 255,
                                  LINEARBLEND);

      // make the acme pink, because why not
      if (-1 > rockets[r].yd < 1) rgb = CRGB::White;

      // extract the fractional parts and derive their inverses
      uint8_t xx = rockets[r].x & 0xff;
      uint8_t yy = rockets[r].y & 0xff;
      uint8_t ix = 255 - xx;
      uint8_t iy = 255 - yy;
      uint8_t wu[4] = {wu_weight(ix, iy), wu_weight(xx, iy), wu_weight(ix, yy),
                       wu_weight(xx, yy)};

      // multiply the intensities by the colour, and saturating-add them to the
      // pixels
      for (uint8_t i = 0; i < 4; i++) {
        uint8_t x = (rockets[r].x >> 8) + (i & 1);
        uint8_t y = (rockets[r].y >> 8) + ((i >> 1) & 1);
        // Mesmerizer has swapped Y axis.
        int32_t index = g()->xy(x, MATRIX_HEIGHT - 1 - y);
        if (index < NUM_LEDS) {
          g()->leds[index].r = qadd8(g()->leds[index].r, rgb.r * wu[i] >> 8);
          g()->leds[index].g = qadd8(g()->leds[index].g, rgb.g * wu[i] >> 8);
          g()->leds[index].b = qadd8(g()->leds[index].b, rgb.b * wu[i] >> 8);
        }  // else warn...
      }
    }
  }

  bool isSetup = false;

 public:
  PatternSMColorPopcorn()
      :
#if ENABLE_AUDIO
        BeatEffectBase(1.50, 0.05),
#endif
        LEDStripEffect(EFFECT_MATRIX_SMCOLOR_POPCORN, "Color Popcorn") {
  }

  PatternSMColorPopcorn(const JsonObjectConst& jsonObject)
      :
#if ENABLE_AUDIO
        BeatEffectBase(1.50, 0.05),
#endif
        LEDStripEffect(jsonObject) {
  }

  void Start() override {
    g()->Clear();
    for (uint8_t r = 0; r < NUM_ROCKETS; r++) {
      rockets[r].x = random8() * MATRIX_WIDTH - 1;
      rockets[r].y = random8() * MATRIX_HEIGHT - 1;
    }
  }

  void Draw() override {
#if ENABLE_AUDIO
    ProcessAudio();
#endif
    fadeToBlackBy(g()->leds, NUM_LEDS, 60);

    EVERY_N_MILLISECONDS(16) {
      move();
      paint();
    }
  }

#if ENABLE_AUDIO
  virtual void HandleBeat(bool bMajor, float elapsed, float span) override {}
#endif
};
