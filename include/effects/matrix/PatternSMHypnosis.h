#pragma once

#include "effectmanager.h"

// Inspired by https://editor.soulmatelights.com/gallery/2272-hypnosis
// Spiraling swirls of rotating colors.

class PatternSMHypnosis : public LEDStripEffect
{
  private:
    const uint8_t C_X = MATRIX_WIDTH / 2;
    const uint8_t C_Y = MATRIX_HEIGHT / 2;
    const uint8_t mapp = 255 / MATRIX_WIDTH;
    struct
    {
        uint8_t angle;
        uint8_t radius;
    } rMap[MATRIX_WIDTH][MATRIX_HEIGHT];

  public:
    PatternSMHypnosis() : LEDStripEffect(EFFECT_MATRIX_SMHYPNOSIS, "Hypnosis")
    {
    }

    PatternSMHypnosis(const JsonObjectConst &jsonObject) : LEDStripEffect(jsonObject)
    {
    }

    size_t DesiredFramesPerSecond() const override
    {
        return 45;
    }

    void Start() override
    {
        g()->Clear();
        for (int8_t x = -C_X; x < C_X + (MATRIX_WIDTH % 2); x++)
        {
            for (int8_t y = -C_Y; y < C_Y + (MATRIX_HEIGHT % 2); y++)
            {
                rMap[x + C_X][y + C_Y].angle = 128 * (atan2(y, x) / PI);
                rMap[x + C_X][y + C_Y].radius = hypot(x, y) * mapp; // thanks
                                                                    // Sutaburosu
            }
        }
    }

    uint16_t t = 0;

    void Draw() override
    {
        t += 4;
        for (uint x = 0; x < MATRIX_WIDTH; x++)
            for (uint y = 0; y < MATRIX_HEIGHT; y++)
                g()->leds[XY(x, y)] = ColorFromPalette(g()->IsPalettePaused()
                                      ? g()->GetCurrentPalette()
                                      : RainbowStripeColors_p, t / 2 + rMap[x][y].radius + rMap[x][y].angle, sin8(rMap[x][y].angle + (rMap[x][y].radius * 2) - t));
    }
};
