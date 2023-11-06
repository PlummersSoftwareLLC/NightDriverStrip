#pragma once

#include "effectmanager.h"

// Derived from https://wokwi.com/projects/289218075224441356
// N Glowing balls in orbit around each other around a rotating plane.
// BUGBUG: Harvest possible speed fx from https://pastebin.com/VTAg4QAZ

class PatternSMMetaBalls : public LEDStripEffect
{
  private:
    uint8_t bx[5];
    uint8_t by[5];

    byte dist(uint8_t x1, uint8_t y1, uint8_t x2, uint8_t y2)
    {
        int a = y2 - y1;
        int b = x2 - x1;
        a *= a;
        b *= b;
        //    byte dist = 220 / (sqrt16(a + b) + 1);
        // Avoid a div/0 crash.
        byte dist = 220 / (sqrt16(a + b + 1));
        return dist;
    }

  public:
    PatternSMMetaBalls() : LEDStripEffect(EFFECT_MATRIX_SMMETA_BALLS, "MetaBalls")
    {
    }

    PatternSMMetaBalls(const JsonObjectConst &jsonObject) : LEDStripEffect(jsonObject)
    {
    }

    void Start() override
    {
        g()->Clear();
    }

    void Draw() override
    {
        for (uint8_t a = 0; a < 5; a++)
        {
            bx[a] = beatsin8(15 + a * 2, 0, MATRIX_WIDTH - 1, 0, a * 32);
            by[a] = beatsin8(18 + a * 2, 0, MATRIX_HEIGHT - 1, 0, a * 32);
        }
        for (unsigned i = 0; i < MATRIX_WIDTH - 1; i++)
        {
            for (unsigned j = 0; j < MATRIX_HEIGHT - 1; j++)
            {
                byte sum = dist(i, j, bx[0], by[0]);
                for (uint8_t a = 1; a < 5; a++)
                {
                    sum = qadd8(sum, dist(i, j, bx[a], by[a]));
                }
                // HeatColors2_p peaks with blue instead of white and looks nicer for this effect
                g()->leds[XY(i, j)] = ColorFromPalette(HeatColors2_p, sum + 220, 254, LINEARBLEND);
            }
        }

        g()->blur2d(g()->leds, MATRIX_WIDTH - 1, 0, MATRIX_HEIGHT - 1, 0, 32);
        fadeAllChannelsToBlackBy(10);
    }
};
