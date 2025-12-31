#pragma once

#include "effectmanager.h"
#include "effects/matrix/Boid.h"
#include "effects/matrix/Vector.h"

// Inspired by https://editor.soulmatelights.com/gallery/2254-flocking
// Almost a textbook Boid demo: Separation, Cohesion, Alignment.

class PatternSMFlocking : public EffectWithId<PatternSMFlocking>
{
  private:
    // Just about enough time to spread after a collision on a 64x32
    // Too many and the flock is to large and you spend all the time off
    // screen.
    static constexpr int NUM_PARTICLES = 10;
    Boid boids[NUM_PARTICLES];



  public:
    PatternSMFlocking() : EffectWithId<PatternSMFlocking>("Flocking")
    {
    }

    PatternSMFlocking(const JsonObjectConst &jsonObject) : EffectWithId<PatternSMFlocking>(jsonObject)
    {
    }

    void Start() override
    {
        g()->Clear();
        for (int i = 0; i < NUM_PARTICLES; i++)
        {
            boids[i] = Boid(random(MATRIX_WIDTH-1), random(MATRIX_HEIGHT-1));
            // The default is too fast; we spend all our time bouncing off the walls.
            boids[i].maxspeed = 0.7;
        }
    }

    void Draw() override
    {
        // Run the entire queue. The kink is that update() may move adjacent boids as it'll
        // shuffle them in a flock. Attempting to contrain it onto screen inside this pass
        // is therefore futile. We make another run over the list to do the actual clamp and
        // draw. The update() on boid[N] might push boid[N-1] out of bounds.
        for (auto &boid : boids)
        {
            boid.update(boids, NUM_PARTICLES); // Let update() move the whole flock.
            boid.avoidBorders(); // Keep them 8 pixels from edges.
            boid.bounceOffBorders(.7); // Bounce anything that update() DOES put over the edge.
        }
        for (auto &boid : boids)
        {
            // This drawPixel draws a blended 2x2 block, so we have to constrain even more.
            boid.location.x = std::clamp(boid.location.x, 1.0f, (float) MATRIX_WIDTH - 2);
            boid.location.y = std::clamp(boid.location.y, 1.0f, (float) MATRIX_HEIGHT - 2);
            g()->drawPixelXYF_Wu(boid.location.x, MATRIX_HEIGHT - 1 - boid.location.y,
                         ColorFromPalette(PartyColors_p, boid.hue * 15, 255, LINEARBLEND));
        }
        fadeAllChannelsToBlackBy(25);
    }
};
