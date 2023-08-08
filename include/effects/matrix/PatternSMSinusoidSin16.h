#pragma once

#include "effectmanager.h"
#include "effects/strip/musiceffect.h"

// Taken from https://editor.soulmatelights.com/gallery/1192-sinusoid-sin16

#define SQRT_VARIANT sqrtf // sqrtf or sqrt or sqrt3 (quick variant)

#if ENABLE_AUDIO
class PatternSMSinusoidSin16 : public BeatEffectBase, public LEDStripEffect
#else
class PatternSMSinusoidSin16 : public LEDStripEffect
#endif
{
  private:
    // Good settings
#if 0
  // 2 rings spinning
  uint8_t Speed = 1 + (9*6);      // is speed 1-255 and type (every 9)
  uint8_t Scale = 3 + (9*6);      // is scale (every 9) and amplitude 1-100
#else
    // (Pastel) Hurricane warning!
    uint8_t Speed = 7 + (90 * 1);  // is speed 1-255 and type (every 9)
    uint8_t Scale = 1 + (180 * 1); // is scale (every 9) and amplitude 1-100
#endif

    // несколько общих переменных и буферов, которые могут использоваться в любом
    // эффекте
    uint8_t hue, hue2; // постепенный сдвиг оттенка или какой-нибудь другой
                       // цикличный счётчик
    uint8_t deltaHue, deltaHue2; // ещё пара таких же, когда нужно много
    uint8_t step; // какой-нибудь счётчик кадров или последовательностей операций
    uint8_t pcnt;       // какой-то счётчик какого-то прогресса
    uint8_t deltaValue; // просто повторно используемая переменная
    float speedfactor;  // регулятор скорости в эффектах реального времени
    float emitterX, emitterY; // какие-то динамичные координаты

    // функция отрисовки точки по координатам X Y
    void drawPixelXY(int8_t x, int8_t y, CRGB color)
    {
        if (x < 0 || x > (MATRIX_WIDTH - 1) || y < 0 || y > (MATRIX_HEIGHT - 1))
            return;
        uint32_t thisPixel = XY((uint8_t)x, (uint8_t)y);
        g()->leds[thisPixel] = color;
    }

  public:
    PatternSMSinusoidSin16()
        :
#if ENABLE_AUDIO
          BeatEffectBase(1.50, 0.05),
#endif
          LEDStripEffect(EFFECT_MATRIX_SMSINUSOIDSIN16, "Sinusoid Sin16")
    {
    }

    PatternSMSinusoidSin16(const JsonObjectConst &jsonObject)
        :
#if ENABLE_AUDIO
          BeatEffectBase(1.50, 0.05),
#endif
          LEDStripEffect(jsonObject)
    {
    }

    void Start() override
    {
        g()->Clear();
        // deltaHue = (Scale - 1U) % ... + 1U;
        deltaValue = (Speed - 1U) % 9U; // количество режимов

        emitterX = MATRIX_WIDTH * 0.5;
        emitterY = MATRIX_HEIGHT * 0.5;
        // speedfactor = 0.004 * Speed + 0.015; // speed of the movement along the
        // Lissajous curves //const float speedfactor =
        speedfactor = 0.00145 * Speed + 0.015;
    }

    // ***** SINUSOID3 / СИНУСОИД3 *****

    /*
      Sinusoid3 by Stefan Petrick (mod by Palpalych for GyverLamp 27/02/2020)
      read more about the concept: https://www.youtube.com/watch?v=mubH-w_gwdA
      https://gist.github.com/StefanPetrick/dc666c1b4851d5fb8139b73719b70149
    */
    // v1.7.0 - Updating for GuverLamp v1.7 by PalPalych 12.03.2020
    // 2nd upd by Stepko https://wokwi.com/arduino/projects/287675911209222664
    // 3rd proper by SottNick

    void Draw() override
    {
        float e_s3_size = 3. * Scale / 100.0 + 2; // amplitude of the curves

        // float time_shift =
        // float(millis()%(uint32_t)(30000*(1.0/((float)Speed/255))));
        uint32_t time_shift = millis() & 0xFFFFFF; // overflow protection

        uint16_t _scale = (((Scale - 1U) % 9U) * 10U + 80U) << 7U; // = fmap(scale, 1, 255, 0.1, 3);
        float _scale2 = (float)((Scale - 1U) % 9U) * 0.2 + 0.4;    // для спиралей на sinf
        uint16_t _scale3 = ((Scale - 1U) % 9U) * 1638U + 3276U;    // для спиралей на sin16

        CRGB color;

        float center1x = float(e_s3_size * cos16(speedfactor * 72.0874 * time_shift)) / 0x7FFF - emitterX;
        float center1y = float(e_s3_size * sin16(speedfactor * 98.301 * time_shift)) / 0x7FFF - emitterY;
        float center2x = float(e_s3_size * sin16(speedfactor * 68.8107 * time_shift)) / 0x7FFF - emitterX;
        float center2y = float(e_s3_size * cos16(speedfactor * 65.534 * time_shift)) / 0x7FFF - emitterY;
        float center3x = float(e_s3_size * sin16(speedfactor * 134.3447 * time_shift)) / 0x7FFF - emitterX;
        // float center3x3x = float(e_s3_size * sin16(float(speedfactor * 134.3447 *
        // time_shift))) / 0x7FFF - xemitterX;
        float center3y = float(e_s3_size * cos16(speedfactor * 170.3884 * time_shift)) / 0x7FFF - emitterY;

        ProcessAudio();

        switch (deltaValue)
        {
        case 0: // Sinusoid I
            for (uint8_t y = 0; y < MATRIX_HEIGHT; y++)
            {
                for (uint8_t x = 0; x < MATRIX_WIDTH; x++)
                {
                    float cx = x + center1x;
                    float cy = y + center1y;
                    int8_t v = 127 * (1 + float(sin16(_scale * SQRT_VARIANT(cx * cx + cy * cy))) / 0x7FFF);
                    color.r = v;
                    cx = x + center3x;
                    cy = y + center3y;
                    v = 127 * (1 + float(sin16(_scale * SQRT_VARIANT(cx * cx + cy * cy))) / 0x7FFF);
                    color.b = v;
                    drawPixelXY(x, y, color);
                }
            }
        case 1: // Sinusoid II ???
            for (uint8_t y = 0; y < MATRIX_HEIGHT; y++)
            {
                for (uint8_t x = 0; x < MATRIX_WIDTH; x++)
                {
                    float cx = x + center1x;
                    float cy = y + center1y;
                    // int8_t v = 127 * (0.001 * time_shift * speedfactor +
                    // float(sin16(_scale * SQRT_VARIANT(cx * cx + cy * cy))) / 32767.0);
                    int8_t v = 127 * (1 + float(sin16(_scale * SQRT_VARIANT(cx * cx + cy * cy))) / 0x7FFF);
                    color.r = v;

                    cx = x + center2x;
                    cy = y + center2y;
                    // v = 127 * (float(0.001 * time_shift * speedfactor) +
                    // float(sin16(_scale * SQRT_VARIANT(cx * cx + cy * cy))) / 32767.0);
                    v = 127 * (1 + float(sin16(_scale * SQRT_VARIANT(cx * cx + cy * cy))) / 0x7FFF);
                    color.r = max((uint8_t)v, color.r);
                    color.g = (uint8_t)v >> 1;
                    color.b = (uint8_t)v >> 3;
                    drawPixelXY(x, y, color);
                }
            }
            break;
        case 2: // Sinusoid III
            for (uint8_t y = 0; y < MATRIX_HEIGHT; y++)
            {
                for (uint8_t x = 0; x < MATRIX_WIDTH; x++)
                {
                    float cx = x + center1x;
                    float cy = y + center1y;
                    int8_t v = 127 * (1 + float(sin16(_scale * SQRT_VARIANT(cx * cx + cy * cy))) / 0x7FFF);
                    color.r = v;

                    cx = x + center2x;
                    cy = y + center2y;
                    v = 127 * (1 + float(sin16(_scale * SQRT_VARIANT(cx * cx + cy * cy))) / 0x7FFF);
                    color.b = v;

                    cx = x + center3x;
                    cy = y + center3y;
                    v = 127 * (1 + float(sin16(_scale * SQRT_VARIANT(cx * cx + cy * cy))) / 0x7FFF);
                    color.g = v;
                    drawPixelXY(x, y, color);
                }
            }
            break;
        case 3: // Sinusoid IV
            for (uint8_t y = 0; y < MATRIX_HEIGHT; y++)
            {
                for (uint8_t x = 0; x < MATRIX_WIDTH; x++)
                {
                    float cx = x + center1x;
                    float cy = y + center1y;
                    int8_t v =
                        127 *
                        (1 + float(sin16(_scale * SQRT_VARIANT(cx * cx + cy * cy) + time_shift * speedfactor * 100)) /
                                 0x7FFF);
                    color.r = ~v;

                    cx = x + center2x;
                    cy = y + center2y;
                    v = 127 *
                        (1 + float(sin16(_scale * SQRT_VARIANT(cx * cx + cy * cy) + time_shift * speedfactor * 100)) /
                                 0x7FFF);
                    color.g = ~v;

                    cx = x + center3x;
                    cy = y + center3y;
                    v = 127 *
                        (1 + float(sin16(_scale * SQRT_VARIANT(cx * cx + cy * cy) + time_shift * speedfactor * 100)) /
                                 0x7FFF);
                    color.b = ~v;
                    drawPixelXY(x, y, color);
                }
            }

            break;
            /*    case 4: //changed by stepko //anaglyph sinusoid
                  for (uint8_t y = 0; y < MATRIX_HEIGHT; y++) {
                    for (uint8_t x = 0; x < MATRIX_WIDTH; x++) {
                      float cx = x + center1x;
                      float cy = y + center1y;
                      int8_t v = 127 * (1 + float(sin16(_scale * SQRT_VARIANT(cx *
               cx + cy * cy))) / 0x7FFF);// + time_shift * speedfactor)) / 0x7FFF);
                      color.r = ~v;

                      v = 127 * (1 + float(sin16(_scale * SQRT_VARIANT(cx * cx + cy
               * cy) + time_shift * speedfactor * 10)) / 0x7FFF); color.b = ~v;

                      //same v = 127 * (1 + float(sin16(_scale * SQRT_VARIANT(cx *
               cx + cy * cy) + time_shift * speedfactor * 10)) / 0x7FFF); color.g =
               ~v; drawPixelXY(x, y, color);
                    }
                  }
                  break;
            */
        case 4: // changed by stepko //colored sinusoid
            for (uint8_t y = 0; y < MATRIX_HEIGHT; y++)
            {
                for (uint8_t x = 0; x < MATRIX_WIDTH; x++)
                {
                    float cx = x + center1x;
                    float cy = y + center1y;
                    int8_t v = 127 * (1 + float(sin16(_scale * (beatsin16(2, 1000, 1750) / 2550.) *
                                                      SQRT_VARIANT(cx * cx + cy * cy))) /
                                              0x7FFF); // + time_shift * speedfactor * 5 // mass
                                                       // colors plus by SottNick
                    color.r = v;

                    // v = 127 * (1 + float(sin16(_scale * SQRT_VARIANT(cx * cx + cy *
                    // cy) + time_shift * speedfactor * 7)) / 0x7FFF); v = 127 * (1 +
                    // sinf (_scale2 * SQRT_VARIANT(((cx * cx) + (cy * cy)))  + 0.001 *
                    // time_shift * speedfactor));
                    v = 127 * (1 + float(sin16(_scale * (beatsin16(1, 570, 1050) / 2250.) *
                                                   SQRT_VARIANT(((cx * cx) + (cy * cy))) +
                                               13 * time_shift * speedfactor)) /
                                       0x7FFF); // вместо beatsin сперва ставил просто * 0.41
                    color.b = v;

                    // v = 127 * (1 + float(sin16(_scale * SQRT_VARIANT(cx * cx + cy *
                    // cy) + time_shift * speedfactor * 19)) / 0x7FFF); v = 127 * (1 +
                    // sinf (_scale2 * SQRT_VARIANT(((cx * cx) + (cy * cy)))  + 0.0025 *
                    // time_shift * speedfactor));
                    v = 127 * (1 + float(cos16(_scale * (beatsin16(3, 1900, 2550) / 2550.) *
                                                   SQRT_VARIANT(((cx * cx) + (cy * cy))) +
                                               41 * time_shift * speedfactor)) /
                                       0x7FFF); // вместо beatsin сперва ставил просто * 0.53
                    color.g = v;
                    drawPixelXY(x, y, color);
                }
            }
            break;
        case 5: // changed by stepko //sinusoid in net
            for (uint8_t y = 0; y < MATRIX_HEIGHT; y++)
            {
                for (uint8_t x = 0; x < MATRIX_WIDTH; x++)
                {
                    float cx = x + center1x;
                    float cy = y + center1y;
                    int8_t v =
                        127 *
                        (1 + float(sin16(_scale * SQRT_VARIANT(cx * cx + cy * cy) + time_shift * speedfactor * 5)) /
                                 0x7FFF);
                    color.g = ~v;

                    // v = 127 * (1 + float(sin16(_scale * x) + 0.01 * time_shift *
                    // speedfactor) / 0x7FFF);
                    v = 127 * (1 + float(sin16(_scale * (x + 0.005 * time_shift * speedfactor))) /
                                       0x7FFF); // proper by SottNick

                    color.b = ~v;

                    // v = 127 * (1 + float(sin16(_scale * y * 127 + float(0.011 *
                    // time_shift * speedfactor))) / 0x7FFF);
                    v = 127 * (1 + float(sin16(_scale * (y + 0.0055 * time_shift * speedfactor))) /
                                       0x7FFF); // proper by SottNick
                    color.r = ~v;
                    drawPixelXY(x, y, color);
                }
            }
            break;
        case 6: // changed by stepko //spiral
            for (uint8_t y = 0; y < MATRIX_HEIGHT; y++)
            {
                for (uint8_t x = 0; x < MATRIX_WIDTH; x++)
                {
                    float cx = x + center1x;
                    float cy = y + center1y;
                    // uint8_t v = 127 * (1 + float(sin16(_scale * (2 * atan2(cy, cx) +
                    // hypot(cy, cx)) + time_shift * speedfactor * 5)) / 0x7FFF);
                    uint8_t v = 127 * (1 + sinf(3 * atan2(cy, cx) + _scale2 * hypot(cy, cx))); // proper by SottNick
                    // uint8_t v = 127 * (1 + float(sin16(atan2(cy, cx) * 31255  +
                    // _scale3 *  hypot(cy, cx))) / 0x7FFF); // proper by SottNick
                    color.r = v;

                    cx = x + center2x;
                    cy = y + center2y;
                    // v = 127 * (1 + float(sin16(_scale * (2 * atan2(cy, cx) +
                    // hypot(cy, cx)) + time_shift * speedfactor * 5)) / 0x7FFF);
                    v = 127 * (1 + sinf(3 * atan2(cy, cx) + _scale2 * hypot(cy, cx))); // proper by SottNick
                    // v = 127 * (1 + float(sin16(atan2(cy, cx) * 31255  + _scale3 *
                    // hypot(cy, cx))) / 0x7FFF); // proper by SottNick
                    color.b = v;

                    cx = x + center3x;
                    cy = y + center3y;
                    // v = 127 * (1 + float(sin16(_scale * (2 * atan2(cy, cx) +
                    // hypot(cy, cx)) + time_shift * speedfactor * 5)) / 0x7FFF); v = 127
                    // * (1 + sinf (3* atan2(cy, cx)  + _scale2 *  hypot(cy, cx))); //
                    // proper by SottNick
                    v = 127 * (1 + float(sin16(atan2(cy, cx) * 31255 + _scale3 * hypot(cy, cx))) /
                                       0x7FFF); // proper by SottNick
                    color.g = v;
                    drawPixelXY(x, y, color);
                }
            }
            break;
        case 7: // variant by SottNick
            for (uint8_t y = 0; y < MATRIX_HEIGHT; y++)
            {
                for (uint8_t x = 0; x < MATRIX_WIDTH; x++)
                {
                    float cx = x + center1x;
                    float cy = y + center1y;
                    // uint8_t v = 127 * (1 + float(sin16(_scale * (2 * atan2(cy, cx) +
                    // hypot(cy, cx)) + time_shift * speedfactor * 5)) / 0x7FFF); uint8_t
                    // v = 127 * (1 + float(sin16(3* atan2(cy, cx) + _scale *  hypot(cy,
                    // cx) + time_shift * speedfactor * 5)) / 0x7FFF); uint8_t v = 127 *
                    // (1 + sinf (3* atan2(cy, cx)  + _scale2 *  hypot(cy, cx))); //
                    // proper by SottNick
                    uint8_t v = 127 * (1 + float(sin16(atan2(cy, cx) * 31255 + _scale3 * hypot(cy, cx))) /
                                               0x7FFF); // proper by SottNick
                    //вырезаем центр спирали
                    float d = SQRT_VARIANT(cx * cx + cy * cy);
                    if (d < 0.5)
                        d = 0.5;
                    d /= 10.;
                    int16_t v2 = 1 / d / d;
                    v = constrain(v - v2, 0, 255);
                    //вырезали
                    color.g = v;

                    cx = x + center3x;
                    cy = y + center3y;
                    // v = 127 * (1 + sinf (3* atan2(cy, cx)  + _scale2 *  hypot(cy,
                    // cx))); // proper by SottNick
                    v = 127 * (1 + float(sin16(atan2(cy, cx) * 31255 + _scale3 * hypot(cy, cx))) /
                                       0x7FFF); // proper by SottNick
                    //вырезаем центр спирали
                    d = SQRT_VARIANT(cx * cx + cy * cy);
                    if (d < 0.5)
                        d = 0.5;
                    d /= 10.;
                    v2 = 1 / d / d;
                    v = constrain(v - v2, 0, 255);
                    //вырезали
                    color.r = v;

                    drawPixelXY(x, y, color);
                }
            }
            break;
        case 8: // variant by SottNick
            for (uint8_t y = 0; y < MATRIX_HEIGHT; y++)
            {
                for (uint8_t x = 0; x < MATRIX_WIDTH; x++)
                {
                    float cx = x + center1x;
                    float cy = y + center1y;
                    // uint8_t v = 127 * (1 + float(sin16(_scale * (2 * atan2(cy, cx) +
                    // hypot(cy, cx)) + time_shift * speedfactor * 5)) / 0x7FFF); uint8_t
                    // v = 127 * (1 + sinf (3* atan2(cy, cx)  + _scale2 *  hypot(cy,
                    // cx))); // proper by SottNick
                    uint8_t v = 127 * (1 + float(sin16(_scale * SQRT_VARIANT(cx * cx + cy * cy))) / 0x7FFF);
                    color.g = v;

                    cx = x + center2x;
                    cy = y + center2y;
                    // v = 127 * (1 + float(sin16(_scale * (2 * atan2(cy, cx) +
                    // hypot(cy, cx)) + time_shift * speedfactor * 5)) / 0x7FFF); v = 127
                    // * (1 + sinf (3* atan2(cy, cx)  + _scale2 *  hypot(cy, cx))); //
                    // proper by SottNick v = 127 * (1 + float(sin16(atan2(cy, cx) *
                    // 31255  + _scale3 *  hypot(cy, cx))) / 0x7FFF); // proper by
                    // SottNick
                    v = 127 * (1 + float(sin16(atan2(cy, cx) * 31255 + _scale3 * hypot(cy, cx))) /
                                       0x7FFF); // proper by SottNick
                    //вырезаем центр спирали
                    float d = SQRT_VARIANT(cx * cx + cy * cy);
                    if (d < 0.5)
                        d = 0.5;
                    d /= 12.;
                    int16_t v2 = 1 / d / d;
                    v = constrain(v - v2, 0, 255);
                    //вырезали
                    color.g = max(v, color.g);
                    color.b = v; // >> 1;
                    // color.r = v >> 1;

                    drawPixelXY(x, y, color);
                    // nblend(leds[XY(x, y)], color, 150);
                }
            }
            break;

            /*    case 8: //changed by stepko //blobs
                  for (uint8_t y = 0; y < MATRIX_HEIGHT; y++) {
                    for (uint8_t x = 0; x < MATRIX_WIDTH; x++) {
                      float cx = x + center1x;
                      float cy = y + center1y;
                      uint8_t v = 30 * (max(
                      cy = y + center2y;
                      v = 30 * (max(0., -hypot(cx, cy) + (_scale >> 7U) * 0.07));
                      color.b = v;

                      cx = x + center3x;
                      cy = y + center3y;
                      v = 30 * (max(0., -hypot(cx, cy) + (_scale >> 7U) * 0.07));
                      color.g = v;
                      drawPixelXY(x, y, color);
                    }
                  }
                  break;
            */
        }
    }

#if ENABLE_AUDIO
    void HandleBeat(bool bMajor, float elapsed, float span) override
    {
        // Hacky. Just an external control to turn the knob without a debugger...
        deltaValue = (deltaValue + 1) % 9U;
        debugI("HandleBeat - New delta: %d", deltaValue);
    }
#endif
};
