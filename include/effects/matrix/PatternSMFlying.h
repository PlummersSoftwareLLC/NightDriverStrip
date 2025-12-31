#pragma once

#include "effectmanager.h"

// Derived from https://www.soulmatelights.com/gallery/434-f-lying

class PatternSMFlying : public EffectWithId<PatternSMFlying>
{
  private:
  public:
    PatternSMFlying() : EffectWithId<PatternSMFlying>("Flying")
    {
    }

    PatternSMFlying(const PatternSMFlying &) = default;
    PatternSMFlying(PatternSMFlying &&) = default;
    PatternSMFlying &operator=(const PatternSMFlying &) = default;
    PatternSMFlying &operator=(PatternSMFlying &&) = default;
    PatternSMFlying(const JsonObjectConst &jsonObject) : EffectWithId<PatternSMFlying>(jsonObject)
    {
    }

    void Start() override
    {
        g()->Clear();
    }

    void Draw() override
    {

        static uint8_t hue = 0;
        EVERY_N_MILLISECONDS(30)
        {
            hue++;
        }                                // 30 - speed of hue change
        static constexpr int speed = -5; // global speed of dots move

        uint16_t x1 = beatsin16(18 + speed, 0, (MATRIX_WIDTH - 1));
        uint16_t x2 = beatsin16(23 + speed, 0, (MATRIX_WIDTH - 1));
        uint16_t x3 = beatsin16(27 + speed, 0, (MATRIX_WIDTH - 1));
        uint16_t x4 = beatsin16(31 + speed, 0, (MATRIX_WIDTH - 1));
        uint16_t x5 = beatsin16(33 + speed, 0, (MATRIX_WIDTH - 1));

        uint16_t y1 = beatsin16(20 + speed, 0, (MATRIX_HEIGHT - 1));
        uint16_t y2 = beatsin16(26 + speed, 0, (MATRIX_HEIGHT - 1));
        uint16_t y3 = beatsin16(15 + speed, 0, (MATRIX_HEIGHT - 1));
        uint16_t y4 = beatsin16(27 + speed, 0, (MATRIX_HEIGHT - 1));
        uint16_t y5 = beatsin16(30 + speed, 0, (MATRIX_HEIGHT - 1));

        CRGB color = CHSV(hue, 255, 255);

        fadeAllChannelsToBlackBy(80);

        g()->drawLine(x1, y1, x2, y2, color);
        g()->drawLine(x2, y2, x3, y3, color);
        g()->drawLine(x3, y3, x1, y1, color);
        g()->drawLine(x4, y4, x1, y1, color);
        g()->drawLine(x4, y4, x2, y2, color);
        g()->drawLine(x4, y4, x3, y3, color);

        g()->blur2d(g()->leds, MATRIX_WIDTH - 1, 0, MATRIX_HEIGHT - 1, 0, 8);
    }
};
