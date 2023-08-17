#pragma once

#include "effectmanager.h"

// Derived from https://editor.soulmatelights.com/gallery/2110-twist
// Honestly, it looks better on their emulator...probably something
// to improve here.
//
// A fine color-waving matrix of oscillating grids.

class PatternSMTwist : public LEDStripEffect
{
  private:
    const int BRIGHTNESS = 255;
    byte hue = 0;

    void patt1(uint8_t i, uint8_t j, uint8_t color1, uint8_t color2)
    {
        //  leds[XY(i, j)] = CHSV(0, 255, 0);
        g()->leds[XY(i + 1, j)] = CHSV(color1, 255, BRIGHTNESS);
        g()->leds[XY(i + 1, j + 1)] = CHSV(color1, 255, BRIGHTNESS);
        g()->leds[XY(i, j + 1)] = CHSV(color2, 255, BRIGHTNESS);
    }

    void patt2(uint8_t i, uint8_t j, uint8_t color1, uint8_t color2)
    {
        //  leds[XY(i, j)] = CHSV(0, 255, 0);
        g()->leds[XY(i + 1, j)] = CHSV(color1, 255, BRIGHTNESS);
        g()->leds[XY(i + 1, j + 1)] = CHSV(color2, 255, BRIGHTNESS);
        g()->leds[XY(i, j + 1)] = CHSV(color2, 255, BRIGHTNESS);
    }

  public:
    PatternSMTwist() : LEDStripEffect(EFFECT_MATRIX_SMTWIST, "Twist")
    {
    }

    PatternSMTwist(const JsonObjectConst &jsonObject) : LEDStripEffect(jsonObject)
    {
    }

    void Start() override
    {
        g()->Clear();
    }

    void Draw() override
    {
        EVERY_N_MILLISECONDS(50)
        {
            hue++;
        }

        for (uint32_t i = 0; i < MATRIX_WIDTH; i += 4)
        {
            for (uint32_t j = 0; j < MATRIX_HEIGHT; j += 4)
            {
                patt1(i, j, 64 + j + hue, i + hue);
                patt1(i + 2, j + 2, 64 + j + hue, i + hue);
                patt2(i, j + 2, 64 + j + hue, i + hue);
                patt2(i + 2, j, 64 + j + hue, i + hue);
            }
        }
        // Without an aggressive fade, they all run together into squares.
        fadeAllChannelsToBlackBy(200);
    }
};
