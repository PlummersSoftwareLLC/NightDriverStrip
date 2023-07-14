#pragma once

#include "effectmanager.h"
#include "effects/strip/musiceffect.h"

// Good freakin' grief.
#define now_weekMs 0 * 1000ul + millis()  // - tmr

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

  const int cfg_length = MATRIX_HEIGHT;
  const int cfg_width = MATRIX_WIDTH;
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

  virtual void Start() override { g()->Clear(); }

  void drawPixelXY(int8_t x, int8_t y, CRGB color) {
    if (x < 0 || x > (MATRIX_WIDTH - 1) || y < 0 || y > (MATRIX_HEIGHT - 1))
      return;
    uint32_t thisPixel = g()->xy(x, MATRIX_HEIGHT - y);
    g()->leds[thisPixel] = color;
  }  // служебные функции

  virtual void Draw() override {
#if ENABLE_AUDIO
    ProcessAudio();
#endif

    fadeAllChannelsToBlackBy(70);
    byte amount = (thisScale >> 3) + 1;
    for (int i = 0; i < amount; i++) {
      int homeX = inoise16(i * 100000000ul + (now_weekMs << 3) * CUR_PRES_speed / 255);
      homeX = map(homeX, 15000, 50000, 0, cfg_length);
      int offsX = inoise8(i * 2500 + (now_weekMs >> 1) * CUR_PRES_speed / 255) - 128;
      offsX = cfg_length / 2 * offsX / 128;
      unsigned int thisX = homeX + offsX;

      int homeY = inoise16(i * 100000000ul + 2000000000ul +
                           (now_weekMs << 3) * CUR_PRES_speed / 255);
      homeY = map(homeY, 15000, 50000, 0, cfg_width);
      int offsY = inoise8(i * 2500 + 30000 + (now_weekMs >> 1) * CUR_PRES_speed / 255) - 128;
      offsY = cfg_length / 2 * offsY / 128;
      int thisY = homeY + offsY;
      drawPixelXY(thisX, thisY, CRGB(CHSV(CUR_PRES_color, 255, 255)));
    }
  }

#if ENABLE_AUDIO
  virtual void HandleBeat(bool bMajor, float elapsed, float span) override {}
#endif
};
