#pragma once

#include "effectmanager.h"

// Derived from https://editor.soulmatelights.com/gallery/1570-radialfire

class PatternSMRadialFire : public EffectWithId<PatternSMRadialFire>
{
  public:

    PatternSMRadialFire() : EffectWithId<PatternSMRadialFire>("RadialFire") {}
    PatternSMRadialFire(const JsonObjectConst &jsonObject) : EffectWithId<PatternSMRadialFire>(jsonObject) {}

    void Start() override
    {
        g()->Clear();
    }

    void Draw() override
    {
        static uint8_t scaleX = 16;
        static uint8_t scaleY = 1;
        static uint8_t speed = 24;
        static uint32_t t;
        t += speed;

        const auto& rMap = LEDMatrixGFX::getPolarMap();

        for (uint8_t x = 0; x < MATRIX_WIDTH; x++)
        {
            for (uint8_t y = 0; y < MATRIX_HEIGHT; y++)
            {
                uint8_t angle = rMap[x][y].angle;
                uint8_t radius = rMap[x][y].unscaled_radius; // Use the unscaled radius
                int16_t Bri = inoise8(angle * scaleX, (radius * scaleY) - t) - radius * (255 / MATRIX_HEIGHT);
                uint8_t Col = Bri;

                if (Bri < 0)
                    Bri = 0;
                if (Bri != 0)
                    Bri = 256 - (Bri * 0.2);

                // If the palette is paused, we use it to color the fire, otherwise we just use red
                // Step 1: Calculate normalized color value

                float normalizedCol = Col / 255.0f;

                // Step 2: Choose base color depending on palette state
                CRGB baseColor;
                if (g()->IsPalettePaused())
                    baseColor = g()->ColorFromCurrentPalette(Col);
                else
                    baseColor = CRGB::Red;

                // Step 3: Get black body heat color
                CRGB heatColor = GetBlackBodyHeatColor(normalizedCol, baseColor);

                // Step 4: Fade color based on brightness
                CRGB color = heatColor.fadeToBlackBy(255 - Bri);
                nblend(g()->leds[XY(x, y)], color, speed);
            }
        }
    }
};
