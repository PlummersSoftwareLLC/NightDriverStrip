#pragma once

#include "effectmanager.h"

// Derived from https://editor.soulmatelights.com/gallery/1116-smoke

class PatternSMSmoke : public LEDStripEffect
{
private:
  uint8_t Scale = 50; // 1-100. SettingA

  static constexpr int WIDTH = MATRIX_WIDTH;
  static constexpr int HEIGHT = MATRIX_HEIGHT;

  uint8_t hue, hue2;           // постепенный сдвиг оттенка или какой-нибудь другой
                               // цикличный счётчик
  uint8_t deltaHue, deltaHue2; // ещё пара таких же, когда нужно много

public:
  PatternSMSmoke()
      : LEDStripEffect(EFFECT_MATRIX_SMSMOKE, "Smoke")
  {
  }

  PatternSMSmoke(const JsonObjectConst &jsonObject)
      : LEDStripEffect(jsonObject)
  {
  }

  void Start() override
  {
    g()->Clear();
  }

  void Draw() override
  {
    deltaHue++;
    CRGB color, color2;

    if (hue2 == Scale)
    {
      hue2 = 0U;
      hue = random8();
    }
    if (deltaHue & 0x01) //((deltaHue >> 2U) == 0U) // какой-то умножитель охота, подключить к задержке смены цвета, но хз какой...
      hue2++;

    if (g()->IsPalettePaused())
    {
      color = g()->ColorFromCurrentPalette(hue);
      color2 = g()->ColorFromCurrentPalette(hue + 127);
    }
    else
    {
      hsv2rgb_spectrum(CHSV(hue, 255U, 127U), color);
      hsv2rgb_spectrum(CHSV(hue + 127, 255U, 127U), color2);
    }

    // deltaHue2--;
    if (random8(WIDTH) != 0U) // встречная спираль движется не всегда синхронно основной
      deltaHue2--;

    for (uint8_t y = 0; y < HEIGHT; y++)
    {
      g()->leds[XY((deltaHue + y + 1U) % WIDTH, HEIGHT - 1U - y)] += color;
      g()->leds[XY((deltaHue + y) % WIDTH, HEIGHT - 1U - y)] += color2; // color2
      g()->leds[XY((deltaHue2 + y) % WIDTH, y)] += color;
      g()->leds[XY((deltaHue2 + y + 1U) % WIDTH, y)] += color2; // color2
    }

    EVERY_N_MILLISECONDS(100)
    {
      g()->SetNoise(1000, 1000, 1000, 4000, 4000);
      g()->FillGetNoise();
    }

    g()->MoveFractionalNoiseX(3);
    g()->MoveFractionalNoiseY(3);
    g()->BlurFrame(20);
  }
};
