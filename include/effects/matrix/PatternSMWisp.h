#pragma once

#include "effectmanager.h"

// Derived from https://editor.soulmatelights.com/gallery/1151-comet

class PatternSMWisp : public LEDStripEffect
{
 private:
  uint8_t Scale = 28;  // 1-100 - Should be a setting

  const uint8_t noisesmooth = 200;

  // matrix size constants are calculated only here and do not change in effects
  const uint8_t CENTER_X_MINOR =
      (MATRIX_WIDTH / 2) -
      ((MATRIX_WIDTH - 1) &
       0x01);  // the center of the matrix according to ICSU, shifted to the
               // smaller side, if the width is even
  const uint8_t CENTER_Y_MINOR =
      (MATRIX_HEIGHT / 2) -
      ((MATRIX_HEIGHT - 1) &
       0x01);  // center of the YGREK matrix, shifted down if the height is even
  const uint8_t CENTER_X_MAJOR =
      MATRIX_WIDTH / 2 +
      (MATRIX_WIDTH % 2);  // the center of the matrix according to IKSU,
                           // shifted to a larger side, if the width is even
  const uint8_t CENTER_Y_MAJOR =
      MATRIX_HEIGHT / 2 +
      (MATRIX_HEIGHT %
       2);  // center of the YGREK matrix, shifted up if the height is even

  CRGB ledsbuff[NUM_LEDS];  // копия массива leds[] целиком

#define NUM_LAYERS 1  // в кометах используется 1 слой, но для огня 2018 нужно 2
  uint32_t noise32_x[NUM_LAYERSMAX];
  uint32_t noise32_y[NUM_LAYERSMAX];
  uint32_t noise32_z[NUM_LAYERSMAX];
  uint32_t scale32_x[NUM_LAYERSMAX];
  uint32_t scale32_y[NUM_LAYERSMAX];

#define NUM_LAYERSMAX 2
  uint8_t noise3d[NUM_LAYERSMAX][MATRIX_WIDTH]
                 [MATRIX_HEIGHT];  // двухслойная маска или хранилище свойств в
                                   // размер всей матрицы

  int8_t zD;
  int8_t zF;

 public:
  PatternSMWisp()
      :
        LEDStripEffect(EFFECT_MATRIX_SMWISP, "Wisp") {
  }

  PatternSMWisp(const JsonObjectConst& jsonObject)
      :
        LEDStripEffect(jsonObject) {
  }

  void MoveFractionalNoiseX(int8_t amplitude = 1, float shift = 0) {
    for (uint8_t y = 0; y < MATRIX_HEIGHT; y++) {
      int16_t amount =
          ((int16_t)noise3d[0][0][y] - 128) * 2 * amplitude + shift * 256;
      int8_t delta = abs(amount) >> 8;
      int8_t fraction = abs(amount) & 255;
      for (uint8_t x = 0; x < MATRIX_WIDTH; x++) {
        if (amount < 0) {
          zD = x - delta;
          zF = zD - 1;
        } else {
          zD = x + delta;
          zF = zD + 1;
        }
        CRGB PixelA = CRGB::Black;
        if ((zD >= 0) && (zD < MATRIX_WIDTH))
          PixelA = g()->leds[XY(zD, y)];
        CRGB PixelB = CRGB::Black;
        if ((zF >= 0) && (zF < MATRIX_WIDTH))
          PixelB = g()->leds[XY(zF, y)];
        ledsbuff[XY(x, y)] =
            (PixelA.nscale8(ease8InOutApprox(255 - fraction))) +
            (PixelB.nscale8(ease8InOutApprox(
                fraction)));  // lerp8by8(PixelA, PixelB, fraction );
      }
    }
    memcpy(g()->leds, ledsbuff, sizeof(CRGB) * NUM_LEDS);
  }

  void MoveFractionalNoiseY(int8_t amplitude = 1, float shift = 0) {
    for (uint8_t x = 0; x < MATRIX_WIDTH; x++) {
      int16_t amount =
          ((int16_t)noise3d[0][x][0] - 128) * 2 * amplitude + shift * 256;
      int8_t delta = abs(amount) >> 8;
      int8_t fraction = abs(amount) & 255;
      // for (uint8_t y = 0 ; y < MATRIX_HEIGHT; y++) {
      for (uint8_t y = 0; y < MATRIX_HEIGHT; y++) {
        if (amount < 0) {
          zD = y - delta;
          zF = zD - 1;
        } else {
          zD = y + delta;
          zF = zD + 1;
        }
        CRGB PixelA = CRGB::Black;
        // NightDriver change: Keep it out of the AUDIO line or you'll get green
        // ghost wisps (which are kinda cool)
        if ((zD >= 1) && (zD < MATRIX_HEIGHT))
          PixelA = g()->leds[XY(x, zD)];
        CRGB PixelB = CRGB::Black;
        if ((zF >= 0) && (zF < MATRIX_HEIGHT))
          PixelB = g()->leds[XY(x, zF)];
        ledsbuff[XY(x, y)] =
            (PixelA.nscale8(ease8InOutApprox(255 - fraction))) +
            (PixelB.nscale8(ease8InOutApprox(fraction)));
      }
    }
    memcpy(g()->leds, ledsbuff, sizeof(CRGB) * NUM_LEDS);
  }

  void FillNoise(int8_t layer) {
    for (uint8_t i = 0; i < MATRIX_WIDTH; i++) {
      int32_t ioffset = scale32_x[layer] * (i - CENTER_X_MINOR);
      for (uint8_t j = 0; j < MATRIX_HEIGHT; j++) {
        int32_t joffset = scale32_y[layer] * (j - CENTER_Y_MINOR);
        int8_t data = inoise16(noise32_x[layer] + ioffset,
                               noise32_y[layer] + joffset, noise32_z[layer]) >>
                      8;
        int8_t olddata = noise3d[layer][i][j];
        int8_t newdata =
            scale8(olddata, noisesmooth) + scale8(data, 255 - noisesmooth);
        data = newdata;
        noise3d[layer][i][j] = data;
      }
    }
  }

  void Start() override { g()->Clear(); }

  void Draw() override {
    g()->DimAll(254U);  // < -- затухание эффекта для последующего кадра
    CRGB _eNs_color = CHSV(millis() / Scale * 2, 255, 255);
    g()->leds[XY(CENTER_X_MINOR, CENTER_Y_MINOR)] += _eNs_color;
    g()->leds[XY(CENTER_X_MINOR + 1, CENTER_Y_MINOR)] += _eNs_color;
    g()->leds[XY(CENTER_X_MINOR, CENTER_Y_MINOR + 1)] += _eNs_color;
    g()->leds[XY(CENTER_X_MINOR + 1, CENTER_Y_MINOR + 1)] += _eNs_color;

    // Noise
    noise32_x[0] += 1500;
    noise32_y[0] += 1500;
    noise32_z[0] += 1500;
    scale32_x[0] = 8000;
    scale32_y[0] = 8000;
    FillNoise(0);
    MoveFractionalNoiseX(MATRIX_WIDTH / 2U - 1U);
    MoveFractionalNoiseY(MATRIX_HEIGHT / 2U - 1U);
  }
};
