#pragma once

#include "effectmanager.h"

// Derived from https://editor.soulmatelights.com/gallery/1090-radialwave
// A three-veined swirl rotates and changes direction, looking like an exhaust.

class PatternSMRadialWave : public LEDStripEffect
{
  private:
    // RadialWave
    // Stepko and Sutaburosu
    // 22/05/22

    bool setupm = 1;
    static constexpr int8_t C_X = MATRIX_WIDTH / 2;
    static constexpr int8_t C_Y = MATRIX_HEIGHT / 2;
    static constexpr uint8_t mapp = 255 / MATRIX_WIDTH;
    // BUGBUG: should probably be allocated into slow RAM.
    struct
    {
        uint8_t angle;
        uint8_t radius;
    } rMap[MATRIX_WIDTH][MATRIX_HEIGHT];

  public:
    PatternSMRadialWave() : LEDStripEffect(EFFECT_MATRIX_SMRADIAL_WAVE, "RadialWave")
    {
    }

    PatternSMRadialWave(const JsonObjectConst &jsonObject) : LEDStripEffect(jsonObject)
    {
    }

    virtual size_t DesiredFramesPerSecond() const           // Desired framerate of the LED drawing
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

    void Draw() override
    {
        static uint32_t t = 0;
        t++;
        for (uint8_t x = 0; x < MATRIX_WIDTH; x++)
        {
            for (uint8_t y = 0; y < MATRIX_HEIGHT; y++)
            {
                byte angle = rMap[x][y].angle;
                byte radius = rMap[x][y].radius;
                g()->leds[XY(x, y)] = CHSV(t + radius, 255, sin8(t * 4 + sin8(t * 4 - radius) + angle * 3));
            }
        }
    }
};
