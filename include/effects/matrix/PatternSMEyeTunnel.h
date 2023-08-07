#pragma once

#include "effectmanager.h"
#include "effects/strip/musiceffect.h"

// Derived from https://editor.soulmatelights.com/gallery/2238-something
//
// Infinite depth eyeball tunnel.

#if ENABLE_AUDIO
class PatternSMEyeTunnel : public BeatEffectBase,
                           public LEDStripEffect
#else
class PatternSMEyeTunnel : public LEDStripEffect
#endif
{
 private:
#undef WU_WEIGHT
  static inline uint8_t WU_WEIGHT(uint8_t a, uint8_t b) {
    return (uint8_t)(((a) * (b) + (a) + (b)) >> 8);
  }

  void wu_pixel(int16_t x, int16_t y, CRGB* col) {
    // extract the fractional parts and derive their inverses
    uint8_t xx = x & 0xff, yy = y & 0xff, ix = 255 - xx, iy = 255 - yy;
    // calculate the intensities for each affected pixel
    // #define WU_WEIGHT(a, b) ((uint8_t)(((a) * (b) + (a) + (b)) >> 8))
    uint8_t wu[4] = {WU_WEIGHT(ix, iy), WU_WEIGHT(xx, iy), WU_WEIGHT(ix, yy),
                     WU_WEIGHT(xx, yy)};
#undef WU_WEIGHT
    // multiply the intensities by the colour, and saturating-add them to the
    // pixels
    for (uint8_t i = 0; i < 4; i++) {
      int16_t local_x = (x >> 8) + (i & 1);
      int16_t local_y = (y >> 8) + ((i >> 1) & 1);
      int16_t xy = g()->xy(local_x, local_y);
      if (xy < 0 || xy > NUM_LEDS) continue;
      g()->leds[xy].r = qadd8(g()->leds[xy].r, col->r * wu[i] >> 8);
      g()->leds[xy].g = qadd8(g()->leds[xy].g, col->g * wu[i] >> 8);
      g()->leds[xy].b = qadd8(g()->leds[xy].b, col->b * wu[i] >> 8);
    }
  }

  // from: https://github.com/FastLED/FastLED/pull/202
  CRGB ColorFromPaletteExtended(const CRGBPalette16& pal, uint16_t index,
                                uint8_t brightness, TBlendType blendType) {
    // Extract the four most significant bits of the index as a palette index.
    uint8_t index_4bit = (index >> 12);
    // Calculate the 8-bit offset from the palette index.
    uint8_t offset = (uint8_t)(index >> 4);
    // Get the palette entry from the 4-bit index
    const CRGB* entry = &(pal[0]) + index_4bit;
    uint8_t red1 = entry->red;
    uint8_t green1 = entry->green;
    uint8_t blue1 = entry->blue;

    uint8_t blend = offset && (blendType != NOBLEND);
    if (blend) {
      if (index_4bit == 15) {
        entry = &(pal[0]);
      } else {
        entry++;
      }

      // Calculate the scaling factor and scaled values for the lower palette
      // value.
      uint8_t f1 = 255 - offset;
      red1 = scale8_LEAVING_R1_DIRTY(red1, f1);
      green1 = scale8_LEAVING_R1_DIRTY(green1, f1);
      blue1 = scale8_LEAVING_R1_DIRTY(blue1, f1);

      // Calculate the scaled values for the neighbouring palette value.
      uint8_t red2 = entry->red;
      uint8_t green2 = entry->green;
      uint8_t blue2 = entry->blue;
      red2 = scale8_LEAVING_R1_DIRTY(red2, offset);
      green2 = scale8_LEAVING_R1_DIRTY(green2, offset);
      blue2 = scale8_LEAVING_R1_DIRTY(blue2, offset);
      cleanup_R1();

      // These sums can't overflow, so no qadd8 needed.
      red1 += red2;
      green1 += green2;
      blue1 += blue2;
    }
    if (brightness != 255) {
      // nscale8x3_video(red1, green1, blue1, brightness);
      nscale8x3(red1, green1, blue1, brightness);
    }

    return CRGB(red1, green1, blue1);
  }

 public:
  PatternSMEyeTunnel()
      :
#if ENABLE_AUDIO
        BeatEffectBase(1.50, 0.05),
#endif
        LEDStripEffect(EFFECT_MATRIX_SMEYE_TUNNEL, "Eye Tunnel") {
  }

  PatternSMEyeTunnel(const JsonObjectConst& jsonObject)
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
    g()->Clear();
    float ms = millis() / 750.f;
    const float kHalfWidth = MATRIX_WIDTH / 2;
    const float kHalfHeight = MATRIX_HEIGHT / 2;

    const float density = 1.1175f + 0.0275f * sinf(ms * 1.09f);
    for (float y = 0; y <= kHalfHeight + 2; y += density)
      for (float x = 0; x <= kHalfWidth + 2; x += density) {
        float hyp = hypotf(x, y) * -3.f;
        float fx = sinf(ms + hyp / 8.f) * 84.f;
        float fy = cosf(ms * .78f + hyp / 8.f) * 224.f;
        CRGB col = ColorFromPaletteExtended(
            HeatColors_p, (ms * 24.f + hyp * 4.f) * 256.f, 255, LINEARBLEND);
        wu_pixel(256 * (x + kHalfWidth) + fx, 256 * (y + kHalfHeight) + fy,
                 &col);
        if (x)
          wu_pixel(256 * (-x + kHalfWidth) + fx, 256 * (y + kHalfHeight) + fy,
                   &col);
        if (y)
          wu_pixel(256 * (x + kHalfWidth) + fx, 256 * (-y + kHalfHeight) + fy,
                   &col);
        if (x && y)
          wu_pixel(256 * (-x + kHalfWidth) + fx, 256 * (-y + kHalfHeight) + fy,
                   &col);
      }
  }

#if ENABLE_AUDIO
  virtual void HandleBeat(bool bMajor, float elapsed, float span) override {}
#endif
};
