#pragma once

#include "effectmanager.h"
#include "effects/strip/musiceffect.h"

// Derived from https://editor.soulmatelights.com/gallery/388-fire2021

#if ENABLE_AUDIO
class PatternSMFire2021 : public BeatEffectBase,
                          public LEDStripEffect
#else
class PatternSMFire2021 : public LEDStripEffect
#endif
{
 private:
  uint8_t Speed = 150;  // 1-252 ...why is not 255?! // Setting
  uint8_t Scale = 9;    // 1-99 is palette and scale // Setting

  uint8_t pcnt;  // какой-то счётчик какого-то прогресса
  uint8_t deltaValue;  // просто повторно используемая переменная
  uint16_t ff_x, ff_y, ff_z;  // большие счётчики
  uint8_t step;  // какой-нибудь счётчик кадров или последовательностей операций

  const TProgmemRGBPalette16* curPalette;

 public:
  PatternSMFire2021()
      :
#if ENABLE_AUDIO
        BeatEffectBase(1.50, 0.05),
#endif
        LEDStripEffect(EFFECT_MATRIX_SMFIRE2021, "Fire 2021") {
  }

  PatternSMFire2021(const JsonObjectConst& jsonObject)
      :
#if ENABLE_AUDIO
        BeatEffectBase(1.50, 0.05),
#endif
        LEDStripEffect(jsonObject) {
  }

  void Start() override {
    g()->Clear();
    if (Scale > 100U)
      Scale = 100U;  // чтобы не было проблем при прошивке без очистки памяти
    deltaValue = Scale * 0.0899;  // /100.0F * ((sizeof(palette_arr)
                                  // /sizeof(TProgmemRGBPalette16 *))-0.01F));
#if LATER
    if (deltaValue == 3U || deltaValue == 4U)
      curPalette = palette_arr
          [deltaValue];  // (uint8_t)(Scale/100.0F * ((sizeof(palette_arr)
                         // /sizeof(TProgmemRGBPalette16 *))-0.01F))];
    else
#endif
      curPalette = firePalettes[deltaValue];  // (uint8_t)(Scale/100.0F *
                                              // ((sizeof(firePalettes)/sizeof(TProgmemRGBPalette16
                                              // *))-0.01F))];
    deltaValue = (((Scale - 1U) % 11U + 1U));
    step =
        map(Speed * Speed, 1U, 65025U, (deltaValue - 1U) / 2U + 1U,
            deltaValue * 18U +
                44);  // корректируем скорость эффекта в наш диапазон допустимых
    // deltaValue = (((Scale - 1U) % 11U + 2U) << 4U); // ширина языков пламени
    // (масштаб шума Перлина)
    deltaValue = 0.7 * deltaValue * deltaValue +
                 31.3;  // ширина языков пламени (масштаб шума Перлина)
    pcnt = map(step, 1U, 255U, 20U, 128U);  // nblend 3th param
  }

  void Draw() override {
#if ENABLE_AUDIO
    ProcessAudio();
#endif

    ff_x += step;  // static uint32_t t += speed;
    for (byte x = 0; x < MATRIX_WIDTH; x++) {
      for (byte y = 0; y < MATRIX_HEIGHT; y++) {
        int16_t Bri = inoise8(x * deltaValue, (y * deltaValue) - ff_x, ff_z) -
                      (y * (255 / MATRIX_HEIGHT));
        byte Col = Bri;  // inoise8(x * deltaValue, (y * deltaValue) - ff_x,
                         // ff_z) - (y * (255 / MATRIX_HEIGHT));
        if (Bri < 0) Bri = 0;
        if (Bri != 0) Bri = 256 - (Bri * 0.2);
        // NightDriver mod - invert Y argument.
        nblend(g()->leds[g()->xy(x, MATRIX_HEIGHT - 1 - y)],
               ColorFromPalette(*curPalette, Col, Bri), pcnt);
      }
    }

    if (!random8()) ff_z++;
  }

#if ENABLE_AUDIO
  void HandleBeat(bool bMajor, float elapsed, float span) override {}
#endif
};
