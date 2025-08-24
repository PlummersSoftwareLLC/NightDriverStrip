#pragma once

#include "effectmanager.h"
#include "effects/matrix/Boid.h"
#include "effects/matrix/Vector.h"

// Derived from https://editor.soulmatelights.com/gallery/2132-flowfields
// This makes a very cool green vine that grows up the display.

class PatternSMFlowFields : public EffectWithId<idMatrixSMFlowFields>
{
  private:

    const int WIDTH = MATRIX_WIDTH;
    const int HEIGHT = MATRIX_HEIGHT;
    const int COLS = MATRIX_WIDTH;
    const int ROWS = MATRIX_HEIGHT;
    static const int NUM_PARTICLES = 40;
    std::array<Boid, NUM_PARTICLES> boids;

    uint16_t x;
    uint16_t y;
    uint16_t z;

    uint16_t speed = 10;
    uint16_t scale = 30;

  public:

    PatternSMFlowFields() : EffectWithId<idMatrixSMFlowFields>("Liquidflow") {}
    PatternSMFlowFields(const JsonObjectConst &jsonObject) : EffectWithId<idMatrixSMFlowFields>(jsonObject) {}

    void Start() override
    {
        g()->Clear();

        x = random16();
        y = random16();
        z = random16();

        for (auto &boid : boids)
        {
            boid = Boid(random(COLS), 0);
        }
    }

    void Draw() override
    {
        for (auto &boid : boids)
        {
            int ioffset = scale * boid.location.x;
            int joffset = scale * boid.location.y;

            uint8_t angle = inoise8(x + ioffset, y + joffset, z);

            boid.velocity.x = (float)sin8(angle) * 0.0078125 - 1.0;
            boid.velocity.y = -((float)cos8(angle) * 0.0078125 - 1.0);
            boid.update();

            // NightDriver Bugfix: The original had COLS and ROWS swapped here,
            // clamping to the wrong axis.
            if (boid.location.x < 0 || boid.location.x >= COLS || boid.location.y < 0 || boid.location.y >= ROWS)
            {
                boid.location.x = random(COLS);
                boid.location.y = 0;
            }
            g()->drawPixelXYF_Wu(boid.location.x, boid.location.y, g()->ColorFromCurrentPalette(boid.hue, 255, LINEARBLEND));
        }
        fadeAllChannelsToBlackBy(15);

        x += speed;
        y += speed;
        z += speed;
    }
};
