#pragma once

#include "effectmanager.h"

// Derived from https://editor.soulmatelights.com/gallery/1570-radialfire

class PatternSMRadialFire : public LEDStripEffect
{
  private:

    static auto constexpr C_X = (MATRIX_WIDTH / 2);
    static auto constexpr C_Y = (MATRIX_HEIGHT / 2);
    // BUGBUG: should probably be dynamically allocated into non-DMAable RAM.
    uint8_t XY_angle[MATRIX_WIDTH][MATRIX_HEIGHT];
    uint8_t XY_radius[MATRIX_WIDTH][MATRIX_HEIGHT];

  public:
    PatternSMRadialFire() : LEDStripEffect(idMatrixSMRadialFire, "RadialFire")
    {
    }

    PatternSMRadialFire(const JsonObjectConst &jsonObject) : LEDStripEffect(jsonObject)
    {
    }

    void Start() override
    {
        g()->Clear();
        for (int8_t x = -C_X; x < C_X + (MATRIX_WIDTH % 2); x++)
        {
            for (int8_t y = -C_Y; y < C_Y + (MATRIX_HEIGHT % 2); y++)
            {
                XY_angle[x + C_X][y + C_Y] = 128 * (atan2(y, x) / PI);
                XY_radius[x + C_X][y + C_Y] = hypot(x, y); // thanks Sutaburosu
            }
        }
    }

    void Draw() override
    {
        static uint8_t scaleX = 16;
        static uint8_t scaleY = 1;

        static uint8_t speed = 24;
        static uint32_t t;
        t += speed;
        for (uint8_t x = 0; x < MATRIX_WIDTH; x++)
        {
            for (uint8_t y = 0; y < MATRIX_HEIGHT; y++)
            {
                uint8_t angle = XY_angle[x][y];
                uint8_t radius = XY_radius[x][y];
                int16_t Bri = inoise8(angle * scaleX, (radius * scaleY) - t) - radius * (255 / MATRIX_HEIGHT);
                uint8_t Col = Bri;
                if (Bri < 0)
                    Bri = 0;
                if (Bri != 0)
                    Bri = 256 - (Bri * 0.2);

                // If the palette is paused, we use it to color the fire, otherwise we just use red
                CRGB color = (GetBlackBodyHeatColor(Col/255.0f, g()->IsPalettePaused() ? 
                                    g()->ColorFromCurrentPalette(Col) 
                                  : CRGB::Red).fadeToBlackBy(255-Bri));
                nblend(g()->leds[XY(x, y)], color, speed);
            }
        }
    }
};
