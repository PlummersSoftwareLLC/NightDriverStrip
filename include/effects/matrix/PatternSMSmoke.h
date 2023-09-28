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
    if (deltaHue & 0x01) //((deltaHue >> 2U) == 0U) // (orig) I'd like to connect some kind of multiplier to the color change delay, but I don't know what...

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
    if (random8(WIDTH) != 0U) // (orig) // the counter spiral does not always move synchronously with the main one.
      deltaHue2--;

    // Two diagonal (note Y is used for height AND X offset)
    // "wipers", of different colors. Each leaves a
    // sky-written trailer of color.
    // slew is used to make the lines look less mathematically diagonal
    // and introduce some delay between color1 and color2. This gives a
    // "tear" in the refresh and as a bonus, gives some 'pepper in fire'
    // effect to he colors which gets quickly blended.
    for (uint8_t y = 0; y < HEIGHT; y++)
    {
      uint8_t slew = random8(8) - 8/2 + 1;
      uint8_t slew2 = random8(8) - 8/2 + 1;
      g()->leds[XY((deltaHue + y + slew + 1U) % WIDTH, HEIGHT - 1U - y)] += color;
      g()->leds[XY((deltaHue + y + slew2) % WIDTH, HEIGHT - 1U - y)] += color2; // color2
      g()->leds[XY((deltaHue2 + y + slew2) % WIDTH, y)] += color;
      g()->leds[XY((deltaHue2 + y + slew + 1U) % WIDTH, y)] += color2; // color2
    }

    EVERY_N_MILLISECONDS(100)
    {
      // (orig) "speed of movement through the noise array"
      // Calling SetNoise() in here will index past what was
      // FillGetNoised, which returns slowly scrolling bars
      // of black along X and Y axes.
      g()->FillGetNoise();
      // g()->SetNoise(1, 1, 1, 4, 4);
    }

    // Lower number for thicker, more static fog. Higher for more wisp.
    g()->MoveFractionalNoiseX(3);
    g()->MoveFractionalNoiseY(3);
    // Without this, we get tornadoes where the diagonals cross as there's
    // an excess of set pixels there.
    g()->BlurFrame(10);
  }
};
