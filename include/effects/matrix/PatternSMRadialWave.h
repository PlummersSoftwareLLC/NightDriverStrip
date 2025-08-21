#pragma once

#include "effectmanager.h"

// Derived from https://editor.soulmatelights.com/gallery/1090-radialwave
// A three-veined swirl rotates and changes direction, looking like an exhaust.

class PatternSMRadialWave : public EffectWithId<idMatrixSMRadialWave>
{
  private:
    // RadialWave
    // Stepko and Sutaburosu
    // 22/05/22

  public:
  
    PatternSMRadialWave() : EffectWithId<idMatrixSMRadialWave>("RadialWave") {}
    PatternSMRadialWave(const JsonObjectConst &jsonObject) : EffectWithId<idMatrixSMRadialWave>(jsonObject) {}

    virtual size_t DesiredFramesPerSecond() const           // Desired framerate of the LED drawing
    {
        return 45;
    }

    void Start() override
    {
        g()->Clear();
    }

    void Draw() override
    {
        static uint32_t t = 0;
        t++;
        const auto& rMap = LEDMatrixGFX::getPolarMap();

        for (uint8_t x = 0; x < MATRIX_WIDTH; x++)
        {
            for (uint8_t y = 0; y < MATRIX_HEIGHT; y++)
            {
                uint8_t angle = rMap[x][y].angle;
                uint8_t radius = rMap[x][y].scaled_radius;
                g()->leds[XY(x, y)] = CHSV(t + radius, 255, sin8(t * 4 + sin8(t * 4 - radius) + angle * 3));
            }
        }
    }
};
