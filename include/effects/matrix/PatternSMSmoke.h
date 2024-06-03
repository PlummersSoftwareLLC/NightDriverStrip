#pragma once

#include "effectmanager.h"

// Derived from https://editor.soulmatelights.com/gallery/1116-smoke

class PatternSMSmoke : public LEDStripEffect
{
private:
  static constexpr uint8_t Scale = 50; // 1-100. Setting

  static constexpr int WIDTH = MATRIX_WIDTH;
  static constexpr int HEIGHT = MATRIX_HEIGHT;

  uint8_t hue {0}, hue2 {0};   // gradual shift in hue or some other
                               // cyclic counter
  uint8_t deltaHue {0}, deltaHue2 {0};

public:
  PatternSMSmoke()
      : LEDStripEffect(EFFECT_MATRIX_SMSMOKE, "Smoke")
  {
  }

  PatternSMSmoke(const JsonObjectConst &jsonObject)
      : LEDStripEffect(jsonObject)
  {
  }

  virtual size_t DesiredFramesPerSecond() const           // Desired framerate of the LED drawing
  {
      return 24;
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
    for (uint8_t y = 0; y < HEIGHT; y++)
    {
      g()->leds[XY((deltaHue + y + 1U) % WIDTH, HEIGHT - 1U - y)] += color;
      g()->leds[XY((deltaHue + y) % WIDTH, HEIGHT - 1U - y)] += color2; // color2
      g()->leds[XY((deltaHue2 + y) % WIDTH, y)] += color;
      g()->leds[XY((deltaHue2 + y + 1U) % WIDTH, y)] += color2; // color2
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
