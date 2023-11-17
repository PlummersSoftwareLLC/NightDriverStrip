#pragma once

#include "effectmanager.h"

// Inspired by https://editor.soulmatelights.com/gallery/1620-rainbow-tunel
// Like Hypnosis, a swirling radial rainbow, but entering a black hole.

class PatternSMRainbowTunnel : public LEDStripEffect
{
  private:
    // RadialRainbow
    // Stepko and Sutaburosu
    // 23/12/21

    static constexpr uint8_t C_X = MATRIX_WIDTH / 2;
    static constexpr uint8_t C_Y = MATRIX_HEIGHT / 2;
    static constexpr uint8_t mapp = 255 / MATRIX_WIDTH;

    struct
    {
        uint8_t angle;
        uint8_t radius;
    } rMap[MATRIX_WIDTH][MATRIX_HEIGHT];

  public:
    PatternSMRainbowTunnel() : LEDStripEffect(EFFECT_MATRIX_SMRAINBOW_TUNNEL, "Colorspin")
    {
    }

    PatternSMRainbowTunnel(const JsonObjectConst &jsonObject) : LEDStripEffect(jsonObject)
    {
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

    void Draw() override
    {
        static constexpr byte scaleX = 4;
        static constexpr byte scaleY = 4;
        static constexpr byte speed = 2;
        
        static uint16_t t;

        t += speed;
        for (uint8_t x = 0; x < MATRIX_WIDTH; x++)
        {
            for (uint8_t y = 0; y < MATRIX_HEIGHT; y++)
            {
                byte angle = rMap[x][y].angle;
                byte radius = rMap[x][y].radius;
                g()->leds[XY(x, y)] =
                    CHSV((angle * scaleX) - t + (radius * scaleY), 255, constrain(radius * 3, 0, 255));
            }
        }
    }
};
