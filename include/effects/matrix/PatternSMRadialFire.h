#pragma once

#include "effectmanager.h"

// Derived from https://editor.soulmatelights.com/gallery/1570-radialfire

class PatternSMRadialFire : public LEDStripEffect
{
public:
    static constexpr EffectId kId = idMatrixSMRadialFire;
    EffectId effectId() const override { return kId; }

    PatternSMRadialFire() : LEDStripEffect(idMatrixSMRadialFire, "RadialFire") {}
    PatternSMRadialFire(const JsonObjectConst &jsonObject) : LEDStripEffect(jsonObject) {}

  private:
    static auto constexpr C_X = (MATRIX_WIDTH / 2);
    static auto constexpr C_Y = (MATRIX_HEIGHT / 2);
    
    std::unique_ptr<uint8_t[]> XY_angle_buf;
    std::unique_ptr<uint8_t[]> XY_radius_buf;

    bool Init(std::vector<std::shared_ptr<GFXBase>>& gfx) override
    {
        XY_angle_buf = make_unique_psram<uint8_t[]>(MATRIX_WIDTH * MATRIX_HEIGHT);
        XY_radius_buf = make_unique_psram<uint8_t[]>(MATRIX_WIDTH * MATRIX_HEIGHT);
        for (int8_t x = -C_X; x < C_X + (MATRIX_WIDTH % 2); x++) {
            for (int8_t y = -C_Y; y < C_Y + (MATRIX_HEIGHT % 2); y++) {
                int idx = (x + C_X) + MATRIX_WIDTH * (y + C_Y);
                XY_angle_buf[idx] = 128 * (atan2(y, x) / PI);
                XY_radius_buf[idx] = hypot(x, y); // thanks Sutaburosu
            }
        }
        return LEDStripEffect::Init(gfx);
    }

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
        for (uint8_t x = 0; x < MATRIX_WIDTH; x++) {
            for (uint8_t y = 0; y < MATRIX_HEIGHT; y++) {
                int idx = x + MATRIX_WIDTH * y;
                uint8_t angle = XY_angle_buf[idx];
                uint8_t radius = XY_radius_buf[idx];
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
