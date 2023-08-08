#pragma once

#include "effectmanager.h"
#include "effects/strip/musiceffect.h"

// Derived from https://editor.soulmatelights.com/gallery/1133-spiro
// Honestly, I wonder if I should have just used the Aurora base...

#if ENABLE_AUDIO
class PatternSMSpiro : public BeatEffectBase,
                       public LEDStripEffect
#else
class PatternSMSpiro : public LEDStripEffect
#endif
{
 private:
  uint8_t Speed = 46;  // 1-255 is length Setting
  uint8_t Scale = 3;   // 1-100 is palette Setting
  uint8_t hue;  // постепенный сдвиг оттенка или какой-нибудь другой цикличный
                // счётчик

  const int WIDTH = MATRIX_WIDTH;

#if ENABLE_AUDIO
  // Just lie to the whole project about height to keep it out of the VU meter.
  // This is one of the rare effects where that top line actually matters.
  const int HEIGHT = MATRIX_HEIGHT - 1;
#else
  const int HEIGHT = MATRIX_HEIGHT;
#endif

  // --------------------------- эффект спирали ----------------------
  /*
   * Aurora: https://github.com/pixelmatix/aurora
   * https://github.com/pixelmatix/aurora/blob/sm3.0-64x64/PatternSpiro.h
   * Copyright (c) 2014 Jason Coon
   * Неполная адаптация SottNick
   */
  byte spirotheta1 = 0;
  byte spirotheta2 = 0;
  //    byte spirohueoffset = 0; // будем использовать переменную сдвига оттенка
  //    hue из эффектов Радуга

  const uint8_t spiroradiusx = WIDTH / 4;   // - 1;
  const uint8_t spiroradiusy = HEIGHT / 4;  // - 1;

  const uint8_t spirocenterX = WIDTH / 2;
  const uint8_t spirocenterY = HEIGHT / 2;

  const uint8_t spirominx = spirocenterX - spiroradiusx;
  const uint8_t spiromaxx =
      spirocenterX + spiroradiusx - (WIDTH % 2 == 0 ? 1 : 0);  //+ 1;
  const uint8_t spirominy = spirocenterY - spiroradiusy;
  const uint8_t spiromaxy =
      spirocenterY + spiroradiusy - (HEIGHT % 2 == 0 ? 1 : 0);  //+ 1;

  uint8_t spirocount = 1;
  uint8_t spirooffset = 256 / spirocount;
  boolean spiroincrement = false;
  boolean spirohandledChange = false;

  const TProgmemRGBPalette16* curPalette = &PartyColors_p;

  uint8_t mapsin8(uint8_t theta, uint8_t lowest = 0, uint8_t highest = 255) {
    uint8_t beatsin = sin8(theta);
    uint8_t rangewidth = highest - lowest;
    uint8_t scaledbeat = scale8(beatsin, rangewidth);
    uint8_t result = lowest + scaledbeat;
    return result;
  }

  uint8_t mapcos8(uint8_t theta, uint8_t lowest = 0, uint8_t highest = 255) {
    uint8_t beatcos = cos8(theta);
    uint8_t rangewidth = highest - lowest;
    uint8_t scaledbeat = scale8(beatcos, rangewidth);
    uint8_t result = lowest + scaledbeat;
    return result;
  }

 public:
  PatternSMSpiro()
      :
#if ENABLE_AUDIO
        BeatEffectBase(1.50, 0.05),
#endif
        LEDStripEffect(EFFECT_MATRIX_SMSPIRO, "Spiro") {
  }

  PatternSMSpiro(const JsonObjectConst& jsonObject)
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
    g()->BlurFrame(20);  // @Palpalych советует делать размытие
    g()->DimAll(255U - Speed / 10);

    boolean change = false;

    for (uint8_t i = 0; i < spirocount; i++) {
      uint8_t x = mapsin8(spirotheta1 + i * spirooffset, spirominx, spiromaxx);
      uint8_t y = mapcos8(spirotheta1 + i * spirooffset, spirominy, spiromaxy);

      uint8_t x2 = mapsin8(spirotheta2 + i * spirooffset, x - spiroradiusx,
                           x + spiroradiusx);
      uint8_t y2 = mapcos8(spirotheta2 + i * spirooffset, y - spiroradiusy,
                           y + spiroradiusy);

      // CRGB color = ColorFromPalette( PartyColors_p, (hue + i * spirooffset),
      // 128U); // вообще-то палитра должна постоянно меняться, но до адаптации
      // этого руки уже не дошли CRGB color = ColorFromPalette(*curPalette, hue +
      // i * spirooffset, 128U); // вот так уже прикручена к бегунку Масштаба. за
      // leds[XY(x2, y2)] += color;
      if (x2 < WIDTH && y2 < HEIGHT)  // добавил проверки. не знаю, почему
                                      // эффект подвисает без них
        g()->leds[XY(x2, y2)] +=
            (CRGB)ColorFromPalette(*curPalette, hue + i * spirooffset);

      if ((x2 == spirocenterX && y2 == spirocenterY) ||
          (x2 == spirocenterX && y2 == spirocenterY))
        change = true;
    }

    spirotheta2 += 2;

    //      EVERY_N_MILLIS(12) { маловата задержочка
    spirotheta1 += 1;
    //      }

    EVERY_N_MILLIS(75) {
      if (change && !spirohandledChange) {
        spirohandledChange = true;

        if (spirocount >= WIDTH || spirocount == 1)
          spiroincrement = !spiroincrement;

        if (spiroincrement) {
          if (spirocount >= 4)
            spirocount *= 2;
          else
            spirocount += 1;
        } else {
          if (spirocount > 4)
            spirocount /= 2;
          else
            spirocount -= 1;
        }

        spirooffset = 256 / spirocount;
      }

      if (!change) spirohandledChange = false;
    }

    //      EVERY_N_MILLIS(33) { маловата задержочка
    hue += 1;
    //      }
  }

#if ENABLE_AUDIO
  void HandleBeat(bool bMajor, float elapsed, float span) override {}
#endif
};
