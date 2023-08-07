#pragma once

#include "effectmanager.h"

// Derived from
// https://editor.soulmatelights.com/gallery/552-festive-lighting-green-with-toys

class PatternSMHolidayLights : public LEDStripEffect
{
 private:
  // Holiday lights
  //@stepko
  // Merry Christmas and Happy New Year

  // updated by kostyamat (green with toys and any matrix size support)
  static constexpr int WIDTH = MATRIX_WIDTH;
  static constexpr int HEIGHT = MATRIX_HEIGHT;
  static constexpr int speed = (200 / (HEIGHT - 4));
  byte hue;
  byte effId = 2;  // 1-3

  const byte maxDim = max(WIDTH, HEIGHT);
  const byte minDim = min(WIDTH, HEIGHT);
  const byte width_adj = (WIDTH < HEIGHT ? (HEIGHT - WIDTH) / 2 : 0);
  const byte height_adj = (HEIGHT < WIDTH ? (WIDTH - HEIGHT) / 2 : 0);
  const bool glitch = abs(WIDTH - HEIGHT) >= minDim / 4;

  byte density = 50;      //
  byte fadingSpeed = 10;  //
  byte updateFromRGBWeight = 10;
  const byte scaleToNumLeds = NUM_LEDS / 256;

  CRGB RGBweight(uint16_t idx) {
    return (g()->leds[idx].r + g()->leds[idx].g + g()->leds[idx].b);
  }

  void confetti() {
    uint16_t idx = random16(NUM_LEDS);
    for (byte i = 0; i < scaleToNumLeds; i++)
      if (random8() < density)
        if (RGBweight(idx) < 10) g()->leds[idx] = random(48, 16777216);
  }

  void drawPixelXYF_X(float x, uint16_t y, const CRGB& color) {
    // if (x<0 || y<0 || x>((float)WIDTH) || y>((float)HEIGHT)) return;

    // extract the fractional parts and derive their inverses
    uint8_t xx = (x - (int)x) * 255, ix = 255 - xx;
    // calculate the intensities for each affected pixel
    uint8_t wu[2] = {ix, xx};
    // multiply the intensities by the colour, and saturating-add them to the
    // pixels
    for (int8_t i = 1; i >= 0; i--) {
      int16_t xn = x + (i & 1);
      CRGB clr = g()->leds[g()->xy(xn, HEIGHT - 1 - y)];
      if (xn > 0 && xn < (int)WIDTH - 1) {
        clr.r = qadd8(clr.r, (color.r * wu[i]) >> 8);
        clr.g = qadd8(clr.g, (color.g * wu[i]) >> 8);
        clr.b = qadd8(clr.b, (color.b * wu[i]) >> 8);
      } else if (xn == 0 || xn == (int)WIDTH - 1) {
        clr.r = qadd8(clr.r, (color.r * 85) >> 8);
        clr.g = qadd8(clr.g, (color.g * 85) >> 8);
        clr.b = qadd8(clr.b, (color.b * 85) >> 8);
      }
      g()->leds[g()->xy(xn, HEIGHT - 1 - y)] = clr;
    }
  }

  byte y[HEIGHT];
  float x;

  void addGlitter(uint8_t chanceOfGlitter) {
    if (random8() < chanceOfGlitter)
      g()->leds[random16(NUM_LEDS)] = random(0, 16777215);
  }

  void spruce() {
    hue++;
    // fadeToBlackBy(leds, NUM_LEDS, map(speed, 1, 255, 1, 10));
    fadeAllChannelsToBlackBy(map(speed, 1, 255, 1, 10));
    uint8_t z;
    if (effId == 3)
      z = triwave8(hue);
    else
      z = beatsin8(1, 1, 255);
    for (uint8_t i = 0; i < minDim; i++) {
      x = beatsin16(i * (map(speed, 1, 255, 3, 20) /*(NUM_LEDS/256)*/), i * 2,
                    (minDim * 4 - 2) - (i * 2 + 2));
      if (effId == 2)
        drawPixelXYF_X(x / 4 + height_adj, i,
                       random8(10) == 0
                           ? CHSV(random8(), random8(32, 255), 255)
                           : CHSV(100, 255, map(speed, 1, 255, 128, 100)));
      else
        drawPixelXYF_X(x / 4 + height_adj, i, CHSV(hue + i * z, 255, 255));
    }
    if (!(WIDTH & 0x01))
      g()->leds[g()->xy(WIDTH / 2 - ((millis() >> 9) & 0x01 ? 1 : 0),
                        minDim - 1 - ((millis() >> 8) & 0x01 ? 1 : 0))] =
          CHSV(0, 255, 255);
    else
      g()->leds[g()->xy(WIDTH / 2, minDim - 1)] =
          CHSV(0, (millis() >> 9) & 0x01 ? 0 : 255, 255);

    if (glitch) confetti();
  }

 public:
  PatternSMHolidayLights()
      :
        LEDStripEffect(EFFECT_MATRIX_SMHOLIDAY_LIGHTS, "Holiday Lights") {
  }

  PatternSMHolidayLights(const JsonObjectConst& jsonObject)
      :
        LEDStripEffect(jsonObject) {
  }

  void Start() override { g()->Clear(); }

  void Draw() override {
    spruce();
  }
};
