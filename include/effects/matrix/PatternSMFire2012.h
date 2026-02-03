#pragma once

#include "effectmanager.h"

// Derived from https://editor.soulmatelights.com/gallery/1138-fire2012

class PatternSMFire2012 : public EffectWithId<PatternSMFire2012>
{
  private:
    uint8_t Scale = 63; // 0-100. < 50= FirePalette + scale > 50, choose scale //
                        // Should be a Setting.

    // TProgmemRGBPalette16 *curPalette = &PartyColors_p;
    // добавлено изменение текущей палитры (используется во многих эффектах ниже
    // для бегунка Масштаб)
    const TProgmemRGBPalette16 *palette_arr[9] = {&PartyColors_p,  &OceanColors_p,     &LavaColors_p,
                                                  &HeatColors_p,   &OceanColors_p, &CloudColors_p,
                                                  &ForestColors_p, &RainbowColors_p,   &RainbowStripeColors_p};
    const TProgmemRGBPalette16 *curPalette = palette_arr[0];

    static inline int wrapX(int x)
    {
        return (x + MATRIX_WIDTH) % MATRIX_WIDTH;
    }
    static inline int wrapY(int y)
    {
        return (y + MATRIX_HEIGHT) % MATRIX_HEIGHT;
    }

  public:
    PatternSMFire2012() : EffectWithId<PatternSMFire2012>("Fire 2012")
    {
    }

    PatternSMFire2012(const JsonObjectConst &jsonObject) : EffectWithId<PatternSMFire2012>(jsonObject)
    {
    }

    void Start() override
    {
        g()->Clear();

        if (Scale > 100)
            Scale = 100; // чтобы не было проблем при прошивке без очистки памяти
        if (Scale > 50)
            curPalette = palette_arr[(uint8_t)((Scale - 50) / 50.0F *
                                                ((sizeof(palette_arr) / sizeof(TProgmemRGBPalette16 *)) - 0.01F))];
        else
            curPalette = palette_arr[(uint8_t)(Scale / 50.0F *
                                               ((sizeof(palette_arr) / sizeof(TProgmemRGBPalette16 *)) - 0.01F))];
    }

    void Draw() override
    {
#if MATRIX_HEIGHT / 6 > 6
#define FIRE_BASE 6
#else
#define FIRE_BASE MATRIX_HEIGHT / 6 + 1
#endif
        // COOLING: How much does the air cool as it rises?
        // Less cooling = taller flames.  More cooling = shorter flames.
        uint8_t cooling = 70;
        // SPARKING: What chance (out of 255) is there that a new spark will be lit?
        // Higher chance = more roaring fire.  Lower chance = more flickery fire.
        uint8_t sparking = 130;
        // SMOOTHING; How much blending should be done between frames
        // Lower = more blending and smoother flames. Higher = less blending and
        // flickery flames
        const uint8_t fireSmoothing = 80;
        // Add entropy to random number generator; we use a lot of it.
        random16_add_entropy(random(256));

        auto& noise3d = g()->GetNoise().noise;

        // Loop for each column individually
        for (int x = 0; x < MATRIX_WIDTH; x++)
        {
            // Step 1.  Cool down every cell a little
            for (int i = 0; i < MATRIX_HEIGHT; i++)
            {
                noise3d[x][i] = qsub8(noise3d[x][i], random(0, ((cooling * 10) / MATRIX_HEIGHT) + 2));
            }

            // Step 2.  Heat from each cell drifts 'up' and diffuses a little
            for (int k = MATRIX_HEIGHT - 1; k > 0; k--)
            { // fixed by SottNick
                noise3d[x][k] = (noise3d[x][k - 1] + noise3d[x][k - 1] + noise3d[x][wrapY(k - 2)]) / 3; // fixed by SottNick
            }

            // Step 3.  Randomly ignite new 'sparks' of heat near the bottom
            if (random8() < sparking)
            {
                uint8_t j = random8(FIRE_BASE);
                noise3d[x][j] = qadd8(noise3d[x][j], random(160, 255));
            }

            // Step 4.  Map from heat cells to LED colors
            // Blend new data with previous frame. Average data between neighbouring
            // pixels Nightdriver/Mesmerizer change: invert Y axis.
            for (int y = 0; y < MATRIX_HEIGHT; y++)
                nblend(g()->leds[XY(x, MATRIX_HEIGHT - 1 - y)],
                       ColorFromPalette(*curPalette, ((noise3d[x][y] * 0.7) + (noise3d[wrapX(x + 1)][y] * 0.3))),
                       fireSmoothing);
        }
    }
};
