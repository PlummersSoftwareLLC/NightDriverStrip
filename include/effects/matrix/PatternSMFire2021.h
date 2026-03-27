#pragma once

#include "effectmanager.h"

// Derived from https://editor.soulmatelights.com/gallery/388-fire2021

class PatternSMFire2021 : public EffectWithId<PatternSMFire2021>
{
  private:

    uint8_t Speed = 150; 
    uint8_t Scale = 9;   

    uint8_t pcnt;              
    uint8_t deltaValue;        
    uint16_t ff_x {0} , ff_y {0} , ff_z {0} ; 
    uint8_t step; 

    const TProgmemRGBPalette16 *curPalette;

  public:

    PatternSMFire2021() : EffectWithId<PatternSMFire2021>("Fireplace") {}
    PatternSMFire2021(const JsonObjectConst &jsonObject) : EffectWithId<PatternSMFire2021>(jsonObject) {}

    void Start() override
    {
        g()->Clear();
        if (Scale > 100U)
            Scale = 100U; 
        deltaValue = (((Scale - 1U) % 11U + 1U));
        step = ::map(Speed * Speed, 1U, 65025U, (deltaValue - 1U) / 2U + 1U,
                   deltaValue * 18U + 44); 
        deltaValue = 0.7 * deltaValue * deltaValue + 31.3;
        pcnt = ::map(step, 1U, 255U, 20U, 128U);
    }

    void Draw() override
    {
        ff_x += step; // static uint32_t t += speed;
        for (unsigned x = 0; x < MATRIX_WIDTH; x++)
        {
            for (unsigned y = 0; y < MATRIX_HEIGHT; y++)
            {
                int16_t Bri = inoise8(x * deltaValue, (y * deltaValue) - ff_x, ff_z) - (y * (255 / MATRIX_HEIGHT));
                uint8_t Col = Bri; // inoise8(x * deltaValue, (y * deltaValue) - ff_x, ff_z) - (y * (255 / MATRIX_HEIGHT));
                if (Bri < 0)
                    Bri = 0;
                if (Bri != 0)
                    Bri = 256 - (Bri * 0.2);

                // Get the flame color using the black body radiation approximation, but when the palette is paused
                // we make flame in that base color instead of the normal red
                // NightDriver mod - invert Y argument.

                nblend(g()->leds[XY(x, MATRIX_HEIGHT - 1 - y)], GetBlackBodyHeatColor(Col/255.0f, g()->ColorFromCurrentPalette(0, Bri)).fadeToBlackBy(255-Bri), pcnt);
            }
        }

        if (!random8())
            ff_z++;
    }
};
