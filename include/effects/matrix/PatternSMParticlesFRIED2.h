#pragma once

#include "effectmanager.h"
#include "effects/strip/musiceffect.h"

//#define cfg_deviceType 2   // 1 - это лента, 2 - это матрица
//#define thisScale 254      // 254 - максимальный, наверное, масштаб
//#define CUR_PRES_speed 15  // 15 - скорость, на которой частицы пляшут
//туда-сюда #define CUR_PRES_color 1   // 1 - цвет частиц, чтобы не
//заморачиваться с палитрами

#define cfg_length MATRIX_HEIGHT
#define cfg_width MATRIX_WIDTH
#define now_weekMs 0 * 1000ul + millis()  // - tmr

//#define getPix XY
//#define CUR_PRES_fromPal false

#define FOR_i(x, y) for (int i = (x); i < (y); i++)
#define CUR_PRES preset[cfg_curPreset

#if ENABLE_AUDIO
class PatternSMParticles : public BeatEffectBase,
                           public LEDStripEffect
#else
class PatternSMParticles : public LEDStripEffect
#endif
{
 private:
  const int thisScale = 254;  // 254 - максимальный, наверное, масштаб
  const int CUR_PRES_speed =
      15;  // 15 - скорость, на которой частицы пляшут туда-сюда
  const int CUR_PRES_color =
      1;  // 1 - цвет частиц, чтобы не заморачиваться с палитрами

  const int Xcfg_length = MATRIX_HEIGHT;
  const int Xcfg_width = MATRIX_WIDTH;
  const int Xnow_weekMs = 0 * 1000ul + millis();  // - tmr

 public:
  PatternSMParticles()
      :
#if ENABLE_AUDIO
        BeatEffectBase(1.50, 0.05),
#endif
        LEDStripEffect(EFFECT_MATRIX_SMPARTICLES, "Particles") {
  }

  PatternSMParticles(const JsonObjectConst& jsonObject)
      :
#if ENABLE_AUDIO
        BeatEffectBase(1.50, 0.05),
#endif
        LEDStripEffect(jsonObject) {
  }

  void Start() override { g()->Clear(); }

  void drawPixelXY(int8_t x, int8_t y, CRGB color) {
    if (x < 0 || x > (MATRIX_WIDTH - 1) || y < 0 || y > (MATRIX_HEIGHT - 1))
      return;
    uint32_t thisPixel = XY((uint8_t)x, (uint8_t)y);  // * SEGMENTS;
    // for (uint8_t i = 0; i < SEGMENTS; i++)
    //{
    g()->leds[thisPixel] = color;
    //}
  }  // служебные функции

  void Draw() override {
#if ENABLE_AUDIO
    ProcessAudio();
#endif

    //   FOR_i(0, cfg_length * cfg_width) leds[i].fadeToBlackBy(70);
    fadeAllChannelsToBlackBy(70);

    {
      uint16_t rndVal = 0;
      byte amount = (thisScale >> 3) + 1;
      // amount = 8;
      FOR_i(0, amount) {
        rndVal = rndVal * 2053 + 13849;  // random2053 алгоритм
        int homeX = inoise16(i * 100000000ul +
                             (now_weekMs << 3) * CUR_PRES_speed / 255);
        homeX = map(homeX, 15000, 50000, 0, cfg_length);
        int offsX =
            inoise8(i * 2500 + (now_weekMs >> 1) * CUR_PRES_speed / 255) - 128;
        offsX = cfg_length / 2 * offsX / 128;
        unsigned int thisX = homeX + offsX;

        int homeY = inoise16(i * 100000000ul + 2000000000ul +
                             (now_weekMs << 3) * CUR_PRES_speed / 255);
        homeY = map(homeY, 15000, 50000, 0, cfg_width);
        int offsY = inoise8(i * 2500 + 30000 +
                            (now_weekMs >> 1) * CUR_PRES_speed / 255) -
                    128;
        offsY = cfg_length / 2 * offsY / 128;
        int thisY = homeY + offsY;
        drawPixelXY(thisX, thisY, 255);
        // setPix(thisX, thisY,
        //       CRGB(CHSV(CUR_PRES_color, 255, 255))
        //      );
      }
    }
  }

#if ENABLE_AUDIO
  void HandleBeat(bool bMajor, float elapsed, float span) override {}
#endif
};
