#pragma once

#include "effectmanager.h"

// Derived from https://wokwi.com/projects/297732081748804105
// High color barber-pole with varying Y-height stripes.
// Quite hypnotic.

class PatternSMTwister : public LEDStripEffect
{
  private:
    void mydrawLine(byte x, byte x1, byte y, CHSV color, bool dot, bool grad, byte numline, byte side, byte sinOff,
                    uint16_t a)
    { // my ugly hori line draw function )))

        byte steps = abs8(x1 - x) + 1;

        for (uint16_t i = 1; i <= steps; i++)
        {
            byte dx = lerp8by8(x1, x, i * 255 / steps);
            uint16_t index = XY(dx, y);
            g()->leds[index] = color;
            if (grad)
                g()->leds[index] %=
                    (sin8(numline * 8 + side * 64 + a + sinOff) + i * 255 / steps) / 2; // for draw gradient line
        }
        if (dot)
        { // add white point at the ends of line
            g()->leds[XY(x, y)] = CRGB::Black;
            g()->leds[XY(x1, y)] = CRGB::Black;
        }
    }

  public:
    PatternSMTwister() : LEDStripEffect(EFFECT_MATRIX_SMTWISTER, "Twister")
    {
    }

    PatternSMTwister(const JsonObjectConst &jsonObject) : LEDStripEffect(jsonObject)
    {
    }

    void Start() override
    {
        g()->Clear();
    }

    void Draw() override
    {
        uint16_t a = millis() / 10;
        LEDS.clear();

        for (uint16_t i = 0; i < MATRIX_HEIGHT; i++)
        {
            byte sinOff = sin8(i * 8 / PI + cos8(a / 2 + i) / 4 + a / 3);

            byte x1 = sin8(sinOff + a) * (MATRIX_WIDTH) / 255;
            byte x2 = sin8(sinOff + a + 64) * (MATRIX_WIDTH) / 255;
            byte x3 = sin8(sinOff + a + 128) * (MATRIX_WIDTH) / 255;
            byte x4 = sin8(sinOff + a + 192) * (MATRIX_WIDTH) / 255;
            x1 = x1 >= MATRIX_WIDTH ? (MATRIX_WIDTH - 1) : x1;
            x2 = x2 >= MATRIX_WIDTH ? (MATRIX_WIDTH - 1) : x2;
            x3 = x3 >= MATRIX_WIDTH ? (MATRIX_WIDTH - 1) : x3;
            x4 = x4 >= MATRIX_WIDTH ? (MATRIX_WIDTH - 1) : x4;

            byte hueColor = sin8(a / 20);
            if (x1 < x2)
                mydrawLine(x1, x2, i, CHSV(hueColor, 255, 255), 1, 1, i, 0, sinOff, a);
            if (x2 < x3)
                mydrawLine(x2, x3, i, CHSV(hueColor + 64, 255, 255), 1, 1, i, 1, sinOff, a);
            if (x3 < x4)
                mydrawLine(x3, x4, i, CHSV(hueColor + 128, 255, 255), 1, 1, i, 2, sinOff, a);
            if (x4 < x1)
                mydrawLine(x4, x1, i, CHSV(hueColor + 192, 255, 255), 1, 1, i, 3, sinOff, a);
        }

        fadeAllChannelsToBlackBy(60);
    }
};
