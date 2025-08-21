#pragma once

#include "effectmanager.h"

// Inspired by https://editor.soulmatelights.com/gallery/2272-hypnosis
// Spiraling swirls of rotating colors.

class PatternSMHypnosis : public LEDStripEffect
{
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
    }

    uint16_t t = 0;

    void Draw() override
    {
        t += 4;
        const auto& rMap = LEDMatrixGFX::getPolarMap(); // Get the map on demand

        for (uint x = 0; x < MATRIX_WIDTH; x++)
            for (uint y = 0; y < MATRIX_HEIGHT; y++)
                g()->drawPixelXY_Set(x, y, ColorFromPalette(g()->IsPalettePaused()
                                      ? g()->GetCurrentPalette()
                                      : RainbowStripeColors_p, t / 2 + rMap[x][y].scaled_radius + rMap[x][y].angle, sin8(rMap[x][y].angle + (rMap[x][y].scaled_radius * 2) - t)));
    }
};
