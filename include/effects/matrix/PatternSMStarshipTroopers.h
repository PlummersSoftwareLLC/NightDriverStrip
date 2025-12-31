#pragma once

#include "effectmanager.h"



// Derived from https://editor.soulmatelights.com/gallery/839-starship-troopers
// Fighters fly in circular formation, chasing a common goal but
// never reaching it.

class PatternSMStarshipTroopers : public EffectWithId<PatternSMStarshipTroopers>
{
  private:
    static constexpr int WIDTH = MATRIX_WIDTH;
    static constexpr int HEIGHT = MATRIX_HEIGHT;
// ... (start of private section matches) ...
    static constexpr TProgmemRGBPalette16 HolyLightsColors_p FL_PROGMEM = {
        0xff0000, 0xff4000, 0xff8000, 0xd6c000, 0xffff00, 0xc0ff00, 0x80ff00, 0x40ff00,
        0x00ff00, 0x00c040, 0x008080, 0x0040c0, 0x0000ff, 0x4000c0, 0x800080, 0xc00040};
    static constexpr TProgmemRGBPalette16 NeonColors_p FL_PROGMEM = {
        0x00b1d0, 0x0f93ec, 0x3572ff, 0x4157ff, 0x6162ff, 0x686cff, 0x7473ff, 0x8689e5,
        0x9e9dc6, 0x9694ac, 0x979b9b, 0x888b8c, 0x767680, 0x596160, 0x6c736f, 0x7b7359};
    static constexpr TProgmemRGBPalette16 PotassiumFireColors_p FL_PROGMEM = {
        CRGB::Black, 0x0f001a,           0x1e0034, 0x2d004e, 0x3c0068, CRGB::Indigo, 0x591694,      0x682da6, 0x7643b7,
        0x855ac9,    CRGB::MediumPurple, 0xa95ecd, 0xbe4bbe, 0xd439b0, 0xe926a1,     CRGB::DeepPink}; //* Violet
    static constexpr TProgmemRGBPalette16 WaterfallColors_p FL_PROGMEM = {
        0x000000, 0x060707, 0x101110, 0x151717, 0x1C1D22, 0x242A28, 0x363B3A, 0x313634,
        0x505552, 0x6B6C70, 0x98A4A1, 0xC1C2C1, 0xCACECF, 0xCDDEDD, 0xDEDFE0, 0xB2BAB9}; //* Orange
    const float SPEED_ADJ = (float)NUM_LEDS / 256;
#define chance 2048

    uint8_t speed = 200; // 1-255
    const uint8_t scale = 8;
    const uint8_t DIR_CHARGE = 2; // Chance to change direction 1-5

    CRGBPalette16 pal[5] = {RainbowColors_p, HolyLightsColors_p, NeonColors_p, PotassiumFireColors_p,
                            WaterfallColors_p};

    uint8_t dir = 3;
    uint8_t palette = 0;
    uint8_t count = 0;
    float x1, y1;
    float speedFactor = fmap(speed, 1, 255, 0.25, 1);

    static double fmap(const double x, const double in_min, const double in_max, const double out_min,
                       const double out_max)
    {
        return (out_max - out_min) * (x - in_min) / (in_max - in_min) + out_min;
    }


  public:
    PatternSMStarshipTroopers() : EffectWithId<PatternSMStarshipTroopers>("Starship Troopers")
    {
    }

    PatternSMStarshipTroopers(const JsonObjectConst &jsonObject) : EffectWithId<PatternSMStarshipTroopers>(jsonObject)
    {
    }

    void Start() override
    {
        g()->Clear();
    }

    void Draw() override
    {
        randomSeed(millis());
        //   fadeToBlackBy(leds, NUM_LEDS, map(speed, 1, 255, 5, 10));
        switch (dir)
        {
        case 0: // Up
            for (int x = 0; x < WIDTH; x++)
            {
                if (x > WIDTH / 2 and random(chance) < DIR_CHARGE)
                {
                    count++;
                    break;
                }
                for (float y = 0; y < HEIGHT; y += speedFactor)
                {
                    g()->leds[XY((int)x, MATRIX_HEIGHT - 1 - (int)y)] = (((int)y == HEIGHT - 1) ? CRGB::Black : g()->leds[XY((int)x, MATRIX_HEIGHT - 1 - ((int)y + 1))]);
                }
            }
            break;
        case 1: // Up - Right
            for (float x = 0; x < WIDTH; x += speedFactor)
            {
                if ((int)x > WIDTH / 2 and random(chance) < DIR_CHARGE)
                {
                    count++;
                    break;
                }
                for (int y = 0; y < HEIGHT; y++)
                {
                    g()->leds[XY((int)x, MATRIX_HEIGHT - 1 - y)] = ((y == HEIGHT - 1 or (int) x == WIDTH - 1) ? CRGB::Black : g()->leds[XY((int)x + 1, MATRIX_HEIGHT - 1 - (y + 1))]);
                }
            }
            break;
        case 2: // Right
            for (float x = 0; x < WIDTH; x += speedFactor)
            {
                if ((int)x > WIDTH / 2 and random(chance) < DIR_CHARGE)
                {
                    count++;
                    break;
                }
                for (int y = HEIGHT - 1; y > 0; y--)
                {
                    g()->leds[XY((int)x, MATRIX_HEIGHT - 1 - y)] = (((int)x == WIDTH - 1) ? CRGB::Black : g()->leds[XY((int)x + 1, MATRIX_HEIGHT - 1 - y)]);
                }
            }
            break;
        case 3: // Down - Right
            for (float x = 0; x < WIDTH; x += speedFactor)
            {
                if ((int)x > WIDTH / 2 and random(chance) < DIR_CHARGE)
                {
                    count++;
                    break;
                }
                for (int y = HEIGHT - 1; y > 0; y--)
                {
                    g()->leds[XY((int)x, MATRIX_HEIGHT - 1 - y)] = (((int)x == WIDTH - 1 or y == 0) ? CRGB::Black : g()->leds[XY((int)x + 1, MATRIX_HEIGHT - 1 - (y - 1))]);
                }
            }
            break;
        case 4: // Down
            for (int x = 0; x < WIDTH; x++)
            {
                if (x < WIDTH / 2 and random(chance) < DIR_CHARGE)
                {
                    count++;
                    break;
                }
                for (float y = HEIGHT - 1; y > 0; y -= speedFactor)
                {
                    g()->leds[XY(x, MATRIX_HEIGHT - 1 - (int)y)] = (((int)y == 0) ? CRGB::Black : g()->leds[XY(x, MATRIX_HEIGHT - 1 - ((int)y - 1))]);
                }
            }
            break;
        case 5: // Down - Left
            for (float x = WIDTH - 1; x > 0; x -= speedFactor)
            {
                if (!(uint8_t)x < WIDTH / 2 and random(chance) < DIR_CHARGE)
                {
                    count++;
                    break;
                }
                for (uint8_t y = HEIGHT - 1; y > 0; y--)
                {
                    g()->leds[XY((int)x, MATRIX_HEIGHT - 1 - y)] = ((y == 0 or (int) x == 0) ? CRGB::Black : g()->leds[XY((int)x - 1, MATRIX_HEIGHT - 1 - (y - 1))]);
                }
            }
            break;
        case 6: // Left
            for (float x = WIDTH - 1; x > 0; x -= speedFactor)
            {
                if ((uint8_t)x < WIDTH / 2 and random(chance) < DIR_CHARGE)
                {
                    count++;
                    break;
                }
                for (uint8_t y = HEIGHT - 1; y > 0; y--)
                {
                    g()->leds[XY((int)x, MATRIX_HEIGHT - 1 - y)] = ((int)x == 0 ? CRGB::Black : g()->leds[XY((int)x - 1, MATRIX_HEIGHT - 1 - y)]);
                }
            }
            break;
        case 7: // Up - Left
            for (float x = WIDTH - 1; x > 0; x -= speedFactor)
            {
                if ((uint8_t)x < WIDTH / 2 and random(chance) < DIR_CHARGE)
                {
                    count++;
                    break;
                }
                for (uint8_t y = HEIGHT - 1; y > 0; y--)
                {
                    g()->leds[XY((int)x, MATRIX_HEIGHT - 1 - y)] = ((y == HEIGHT - 1 or (int) x == 0) ? CRGB::Black : g()->leds[XY((int)x - 1, MATRIX_HEIGHT - 1 - (y + 1))]);
                }
            }
            break;
        }

        for (uint8_t i = 0; i < scale; i++)
        {
            x1 = (float)beatsin88(3840 * speedFactor + i * 256, 0, (WIDTH - 1) * 4, 0, scale * i * 256) / 4;
            y1 = (float)beatsin88(3072 * speedFactor + i * 256, 0, (HEIGHT - 1) * 4, 0, 0) / 4;
            if ((x1 >= 0 and x1 <= WIDTH - 1) and (y1 >= 0 and y1 <= HEIGHT - 1))
                g()->drawPixelXYF_Wu(x1, MATRIX_HEIGHT - 1 - y1,
                          ColorFromPalette(pal[palette], beatsin88(256 * 12. * speedFactor + i * 256, 0, 255), 255));
        }

        // blur2d(leds, MATRIX_WIDTH, MATRIX_HEIGHT, 64);
        g()->blur2d(g()->leds, MATRIX_WIDTH, 0, MATRIX_HEIGHT, 0, 64);

        dir = count % 8;
        palette = count % 5;

        //  delay(16); // ~60FPS
    }
};
