#pragma once

#include "effectmanager.h"

// Inspired by https://editor.soulmatelights.com/gallery/2105-pattern-trick
// Blocky math looking like a QR code oscillating above a rainbow background.

class PatternSMPatternTrick : public LEDStripEffect
{
  private:
  public:
    PatternSMPatternTrick() : LEDStripEffect(EFFECT_MATRIX_SMPATTERN_TRICK, "PatternTrick")
    {
    }

    PatternSMPatternTrick(const JsonObjectConst &jsonObject) : LEDStripEffect(jsonObject)
    {
    }

    void Start() override
    {
        g()->Clear();
    }

    void Draw() override
    {
        int a = millis() / 10;

        for (unsigned x = 0; x < MATRIX_WIDTH; x++)
        {
            for (unsigned y = 0; y < MATRIX_HEIGHT; y++)
            {
                if (!(((x + sin8(a / 2) / 24) % MATRIX_WIDTH * 6 ^ (y + cos8(a / 3) / 8) % MATRIX_HEIGHT * 6) % 5))
                    g()->leds[XY(x, y)].setHue(sin8(x * 4 + cos8(y * 2 + a / 4) + a / 3));
                else
                    g()->leds[XY(x, y)] = 0;
            }
        }
    }
};
