#pragma once

#include "effectmanager.h"

// Derived from
// https://editor.soulmatelights.com/gallery/1895-agressivebouncingballs

class PatternSMGravityBalls : public EffectWithId<PatternSMGravityBalls>
{
  private:
    struct Ball {
        float posx;
        float posy;
        float velx;
        float vely;
        float accel;
    };

    static const int COUNT = 7;
    Ball balls[COUNT];
    uint8_t init = 1;



    float newpos()
    {
        return random(0.1F, 22.0F);
    }

    float newvel()
    {
        return random(0.1F, 1.0F);
    }

  public:
    PatternSMGravityBalls() : EffectWithId<PatternSMGravityBalls>("Gravity Balls")
    {
    }

    PatternSMGravityBalls(const JsonObjectConst &jsonObject) : EffectWithId<PatternSMGravityBalls>(jsonObject)
    {
    }

    void Start() override
    {
        g()->Clear();
        for (int i = 0; i < COUNT; i++)
        {
            balls[i].velx = (beatsin16(random(1, 2) + balls[i].vely, 0, MATRIX_WIDTH, MATRIX_HEIGHT) / MATRIX_HEIGHT) + 0.1F;
            balls[i].vely = (beatsin16(5 + balls[i].vely, 0, MATRIX_WIDTH, MATRIX_HEIGHT) / MATRIX_HEIGHT) + 0.5F;
            balls[i].posx = random(1.01F, float(MATRIX_HEIGHT));
            balls[i].posy = random(1.0F, float(MATRIX_WIDTH));
            balls[i].accel = random(0.1F, 1.0F);
        }
    }

    void Draw() override
    {
        for (int i = 0; i < COUNT; i++)
        {
            if (balls[i].posx < 1 || balls[i].posx > MATRIX_WIDTH - 2)
            {
                balls[i].velx = -balls[i].velx;
                if (balls[i].velx * balls[i].velx > 5)
                {
                    balls[i].velx = newvel();
                }
                if (balls[i].posx < 0 || balls[i].posx > MATRIX_WIDTH)
                {
                    balls[i].posx = newpos();
                }
            }
            if (balls[i].posy < 1 || balls[i].posy > MATRIX_HEIGHT - 2)
            {
                balls[i].vely = -balls[i].vely;
                if (balls[i].vely * balls[i].vely > 10)
                {
                    balls[i].vely = newvel();
                }
                if (balls[i].posy < 0 || balls[i].posy > MATRIX_HEIGHT)
                {
                    balls[i].posy = newpos();
                }
            }
            balls[i].vely += 0.1F;
            balls[i].posx = balls[i].posx - balls[i].velx + balls[i].accel;
            balls[i].posy = balls[i].posy - balls[i].vely - balls[i].accel;
            g()->drawPixelXYF_Wu(balls[i].posx, MATRIX_HEIGHT - balls[i].posy, ColorFromPalette(CloudColors_p, i * 80, 255, LINEARBLEND));
        }

        g()->blur2d(g()->leds, MATRIX_WIDTH, 0, MATRIX_HEIGHT, 0, 15);
        fadeAllChannelsToBlackBy(20);
    }
};
