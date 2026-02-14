#pragma once

#include "effectmanager.h"

// Derived from
// https://editor.soulmatelights.com/gallery/2269-aaron-gotwalts-unknown-pleasure
//
// Colored, accelerated, gravity popcorn balls.

class PatternSMColorPopcorn : public EffectWithId<PatternSMColorPopcorn>
{
  private:
    CRGBPalette16 currentPalette = RainbowColors_p;

    uint8_t gravity = 16;
#define NUM_ROCKETS 8

    using Rocket = struct
    {
        int32_t x, y, xd, yd;
    };

    Rocket rockets[NUM_ROCKETS];

    void restart_rocket(uint8_t r)
    {
        // rockets[r].x = random16() >> 4;
        rockets[r].xd = random8() + 32;
        if (rockets[r].x > (MATRIX_WIDTH / 2 * 256))
        {
            // leap towards the centre of the screen
            rockets[r].xd = -rockets[r].xd;
        }
        // controls the leap height
        rockets[r].yd = random8() * 5 + (MATRIX_HEIGHT - 1) * 1;
    }

    void move()
    {
        for (uint8_t r = 0; r < NUM_ROCKETS; r++)
        {
            // add the X & Y velocities to the positions
            rockets[r].x += rockets[r].xd;
            rockets[r].y += rockets[r].yd;

            // bounce off the floor?
            if (rockets[r].y < 0)
            {
                rockets[r].yd = (-rockets[r].yd * 240) >> 8;
                rockets[r].y = rockets[r].yd;
                // settled on the floor?
                if (rockets[r].y <= 200)
                { // if you change gravity, this will probably
                  // need changing too
                    restart_rocket(r);
                }
            }

            // bounce off the sides of the screen?
            if (rockets[r].x < 0 || rockets[r].x > MATRIX_WIDTH * 256)
            {
                rockets[r].xd = (-rockets[r].xd * 248) >> 8;
                // force back onto the screen, otherwise they eventually sneak away
                if (rockets[r].x < 0)
                {
                    rockets[r].x = rockets[r].xd;
                    rockets[r].yd += rockets[r].xd;
                }
                else
                {
                    rockets[r].x = (MATRIX_WIDTH * 256) - rockets[r].xd;
                }
            }

            // gravity
            rockets[r].yd -= gravity;

            // viscosity
            rockets[r].xd = (rockets[r].xd * 224) >> 8;
            rockets[r].yd = (rockets[r].yd * 224) >> 8;
        }
    }

    void paint()
    {
        // fill_solid(g()->leds, N_LEDS, CRGB::Black);
        for (uint8_t r = 0; r < NUM_ROCKETS; r++)
        {
            CRGB rgb = ColorFromPalette(currentPalette, r * (256 / NUM_ROCKETS), 255, LINEARBLEND);

            // make the acme pink, because why not
            if (-1 > rockets[r].yd < 1)
                rgb = CRGB::White;

            float x = rockets[r].x / 256.0f;
            float y = rockets[r].y / 256.0f;
            g()->drawPixelXYF_Wu(x, MATRIX_HEIGHT - 1 - y, rgb);
        }
    }

    bool isSetup = false;

  public:
    PatternSMColorPopcorn() : EffectWithId<PatternSMColorPopcorn>("Color Popcorn")
    {
    }

    PatternSMColorPopcorn(const JsonObjectConst &jsonObject) : EffectWithId<PatternSMColorPopcorn>(jsonObject)
    {
    }

    void Start() override
    {
        g()->Clear();
        for (uint8_t r = 0; r < NUM_ROCKETS; r++)
        {
            rockets[r].x = random8() * MATRIX_WIDTH - 1;
            rockets[r].y = random8() * MATRIX_HEIGHT - 1;
            rockets[r].xd = 0;
            rockets[r].yd = 0;
        }
    }

    void Draw() override
    {
        fadeToBlackBy(g()->leds, NUM_LEDS, 60);

        EVERY_N_MILLISECONDS(16)
        {
            move();
            paint();
        }
    }
};
