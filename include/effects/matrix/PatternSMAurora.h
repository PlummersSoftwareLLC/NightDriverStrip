#pragma once

#include "effectmanager.h"

// Derived from https://editor.soulmatelights.com/gallery/1244-aurora-borealis
// Could use some fine-tuning on timings. speed doesn't work like the original.

class PatternSMAurora : public EffectWithId<PatternSMAurora>
{
  private:
    unsigned long timer{0};
    uint8_t speed = 7;  // Scale  0-255
    uint8_t scale = 60; // Speed 0-255

    float adjustHeight = fmap(MATRIX_HEIGHT, 8, 32, 28, 12);
    uint16_t adjScale = ::map(MATRIX_WIDTH, 8, 64, 310, 63);

    static constexpr TProgmemRGBPalette16 GreenAuroraColors_p FL_PROGMEM = {
        0x000000, 0x003300, 0x006600, 0x009900, 0x00cc00, 0x00ff00, 0x33ff00, 0x66ff00,
        0x99ff00, 0xccff00, 0xffff00, 0xffcc00, 0xff9900, 0xff6600, 0xff3300, 0xff0000};

    float fmap(const float x, const float in_min, const float in_max, const float out_min, const float out_max)
    {
        return (out_max - out_min) * (x - in_min) / (in_max - in_min) + out_min;
    }

  public:
    PatternSMAurora() : EffectWithId<PatternSMAurora>("Aurora Borealis")
    {
    }

    PatternSMAurora(const JsonObjectConst &jsonObject) : EffectWithId<PatternSMAurora>(jsonObject)
    {
    }

    void Start() override
    {
        g()->Clear();
    }

    void Draw() override
    {
        uint16_t _scale = ::map(scale, 1, 255, 30, adjScale);
        uint8_t _speed = ::map(speed, 1, 255, 128, 16);

        EVERY_N_MILLIS(80)
        for (int x = 0; x < MATRIX_WIDTH; x++)
        {
            for (int y = 0; y < MATRIX_HEIGHT; y++)
            {
                timer++;
                g()->leds[XY(x, y)] = ColorFromPalette(
                    GreenAuroraColors_p, qsub8(inoise8(timer % 2 + x * _scale, y * 16 + timer % 16, timer / _speed),
                                               fabs((float)MATRIX_HEIGHT / 2 - (float)y) * adjustHeight));
            }
        }
    }
};
