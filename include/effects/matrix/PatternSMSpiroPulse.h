#pragma once

#include "effectmanager.h"

// Derived from https://editor.soulmatelights.com/gallery/2164-spiro
// A spirograph that evolves the number of spines, becomign a yen-yang.
//
// This is one of relatively few that would look better at a higher refresh.

class PatternSMSpiroPulse : public EffectWithId<idMatrixSMSpiroPulse>
{
  private:

    static constexpr int CenterX = ((MATRIX_WIDTH / 2) - 0.5);
    static constexpr int CenterY = ((MATRIX_HEIGHT / 2) - 0.5);
    bool incenter {false};
    uint8_t AM {1};
    float Angle {0.f};
    bool change = true;

  public:

    PatternSMSpiroPulse() : EffectWithId<idMatrixSMSpiroPulse>("Spiro") {}
    PatternSMSpiroPulse(const JsonObjectConst &jsonObject) : EffectWithId<idMatrixSMSpiroPulse>(jsonObject) {}

    void Start() override
    {
        g()->Clear();
    }

    void Draw() override
    {
        // fadeToBlackBy(leds, NUM_LEDS, 8);
        fadeAllChannelsToBlackBy(8);

        float t = (float)millis() / 500.0f;
        float CalcRad = (sin(t / 2) + 1);
        if (CalcRad <= 0.001)
        {
            if (!incenter)
            {
                AM += ((change * 2) - 1);
                Angle = 6.28318531 / AM;
                if (AM <= 1)
                    change = true;
                else if (AM >= (MATRIX_WIDTH + MATRIX_HEIGHT) / 2)
                    change = false;
            }
            incenter = true;
        }
        else
            incenter = false;
        // Originally /3. Let's go all the way to the edges.
        float radX = CalcRad * CenterY / 2;
        float radY = CalcRad * CenterY / 2;
        for (uint8_t i = 0; i < AM; i++)
        {
            g()->drawPixelXYF_Wu((CenterX + sin(t + (Angle * i)) * radX),
                MATRIX_HEIGHT - 1 - (CenterY + cos(t + (Angle * i)) * radY),
                ColorFromPalette(HeatColors_p, t * 10 + ((256 / AM) * i)));
        }
    }
};
