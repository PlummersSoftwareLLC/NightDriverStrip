#pragma once

#include "effectmanager.h"
#include "effects/matrix/Boid.h"
#include "effects/matrix/Vector.h"

// Derived from https://editor.soulmatelights.com/gallery/2140-boidexplosion
// These are awesome explosion effects, but they're largely lost on
// the 64x32 displays. Included to show more Boid use.

class PatternSMBoidExplosion : public EffectWithId<PatternSMBoidExplosion>
{
  private:
    static constexpr int COLS = MATRIX_WIDTH;
    static constexpr int ROWS = MATRIX_HEIGHT;

    // set this to the number of particles. the variable describes what it's
    // supposed to be. it works with 50, but it's a little slow. on an esp32 it
    // looks pretty nice at that number 15 is a safe number
    static constexpr int NUM_PARTICLES = 35;
    static const int count = NUM_PARTICLES;

    Boid boids[NUM_PARTICLES]; // this makes the boids

    CRGBPalette16 currentPalette;
    TBlendType currentBlending;

    void ChangePalettePeriodically()
    {
        uint8_t secondHand = (millis() / 1000) % 60;
        static uint8_t lastSecond = 99;

        if (lastSecond != secondHand)
        {
            lastSecond = secondHand;
            if (secondHand == 0)
            {
                currentPalette = HeatColors_p;
                currentBlending = NOBLEND;
            }
            if (secondHand == 10)
            {
                currentPalette = HeatColors_p;
                currentBlending = LINEARBLEND;
            }
            if (secondHand == 15)
            {
                currentPalette = LavaColors_p;
                currentBlending = NOBLEND;
            }
            if (secondHand == 20)
            {
                currentPalette = LavaColors_p;
                currentBlending = LINEARBLEND;
            }
            if (secondHand == 25)
            {
                currentPalette = ForestColors_p;
                currentBlending = NOBLEND;
            }
            if (secondHand == 30)
            {
                currentPalette = ForestColors_p;
                currentBlending = LINEARBLEND;
            }
            if (secondHand == 35)
            {
                currentPalette = OceanColors_p;
                currentBlending = NOBLEND;
            }
            if (secondHand == 40)
            {
                currentPalette = OceanColors_p;
                currentBlending = LINEARBLEND;
            }
            if (secondHand == 45)
            {
                currentPalette = CloudColors_p;
                currentBlending = NOBLEND;
            }
            if (secondHand == 50)
            {
                currentPalette = CloudColors_p;
                currentBlending = LINEARBLEND;
            }
        }
    }

    class Attractor
    {
      public:
        float mass;       // Mass, tied to size
        float G;          // Gravitational Constant
        PVector location; // Location
        float coldiv;
        Attractor()
        {
            location = PVector(ROWS / (float)2.0,
                               COLS / (float)coldiv); // PVector(ROWS /
                                                      // (float)random((float)1.1,(float)4.0), COLS
                                                      // / (float)random((float)1.1,(float)4.0));
            mass = 11.0F;                             // random(5.5,8);
            G = 11.0F;                                //(float)random((float)1.5F,(float)2.9F);//random(.5,1.1);
            coldiv = 1.5F;
        }
        void UpdateLocation()
        {
            location = PVector(ROWS / (float)2.0, COLS / (float)coldiv);
            coldiv -= 0.01;
            G += 0.03;
            mass += 0.03;
            if (coldiv < 1.5)
                coldiv = 8.00;
            if (G > 12.00F)
                G = 1.01F;
            if (mass > 12.00F)
                mass = 1.01F;
        }
        PVector attract(Boid m)
        {
            PVector force = location - m.location; // Calculate direction of force
            float d = force.mag();                 // Distance between objects
            d = constrain(d, 7.0,
                          10.0);                            // Limiting the distance to eliminate "extreme"
                                                            // results for very close or very far objects
            force.normalize();                              // Normalize vector (distance doesn't matter here, we
                                                            // just want this vector for direction)
            float strength = (G * mass * m.mass) / (d * d); // Calculate gravitional force magnitude
            force *= strength;                              // Get force vector --> magnitude * direction
            return force;
        }
    };

  public:
    PatternSMBoidExplosion() : EffectWithId<PatternSMBoidExplosion>("Boid Explosion")
    {
    }

    PatternSMBoidExplosion(const JsonObjectConst &jsonObject) : EffectWithId<PatternSMBoidExplosion>(jsonObject)
    {
    }

    Attractor attractor;
    Attractor attractor2;

    void Start() override
    {
        g()->Clear();

        int direction = random(0, 2);
        if (direction == 0)
            direction = -1;
        for (int i = 0; i < NUM_PARTICLES; i++)
        {
            Boid boid = Boid(random(1, ROWS - 1), random(1, COLS - 1));
            boid.velocity.x = ((float)random(40, 50)) / 100.0;
            boid.velocity.x *= direction;
            boid.velocity.y = 0;
            boid.colorIndex = i * 32;
            boids[i] = boid;
            boid.hue = i * 10;
        }
        attractor.location = PVector(ROWS / 2, COLS / 1.1);
        attractor2.location = PVector(ROWS / 1.5, COLS / 4);
    }

    void Draw() override
    {
        ChangePalettePeriodically();
        for (int i = 0; i < NUM_PARTICLES; i++)
        {
            Boid boid = boids[i];
            PVector force = attractor.attract(boid);
            boid.applyForce(force);
            boid.update();
            // Was 50. Once per 30Hz frame looks better.
            EVERY_N_MILLISECONDS(16)
            {
                attractor.UpdateLocation();
            }
            g()->drawPixelXYF_Wu(boid.location.x, MATRIX_HEIGHT - 1 - boid.location.y,
                         ColorFromPalette(currentPalette, boid.hue, 255, currentBlending));
            boids[i] = boid;
            fadeToBlackBy(g()->leds, NUM_LEDS, 1);
        }
    }
};
