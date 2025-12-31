#pragma once

#include "effectmanager.h"



// Inspired by https://editor.soulmatelights.com/gallery/2107-sun-radiation
// or https://editor.soulmatelights.com/gallery/599-sun-radiation
// They're the same code, so probably the lower number.

class PatternSMSunRadiation : public EffectWithId<PatternSMSunRadiation>
{
// ... (private matches below) ...
  private:
    // sun radiation
    // fastled 16x16 matrix demo (work for any size too)
    // Yaroslaw Turbin 19.01.2020
    // https://vk.com/ldirko
    // https://www.reddit.com/user/ldirko/

    // https://wokwi.com/arduino/projects/288134661735973389
    CRGB chsvLut[256];
    uint8_t bump[(MATRIX_WIDTH + 2) * (MATRIX_HEIGHT + 2)];

    void generateCHSVlut()
    {
        for (int j = 0; j < 256; j++)
            chsvLut[j] = HeatColor(j / 1.4); // 256 pallette color
    }

    void generatebump()
    {
        int t = millis() / 4;
        int index = 0;
        for (int j = 0; j < (MATRIX_HEIGHT + 2); j++)
        {
            for (int i = 0; i < (MATRIX_WIDTH + 2); i++)
            {
                uint8_t col = (inoise8_raw(i * 25, j * 25, t)) / 2;
                bump[index++] = col;
            }
        }
    }

    void Bumpmap()
    {
        int yindex = MATRIX_WIDTH + 3;
        int8_t vly = -(MATRIX_HEIGHT / 2 + 1);
        for (unsigned y = 0; y < MATRIX_HEIGHT; y++)
        {
            ++vly;
            int8_t vlx = -(MATRIX_WIDTH / 2 + 1);
            for (unsigned x = 0; x < MATRIX_WIDTH; x++)
            {
                ++vlx;
                int8_t nx = bump[x + yindex + 1] - bump[x + yindex - 1];
                int8_t ny = bump[x + yindex + (MATRIX_WIDTH + 2)] - bump[x + yindex - (MATRIX_WIDTH + 2)];
                // Mesmerizer: changed scales to 4 from 7 to take up more of our larger
                // screen w/o repeats.
                const int8_t scaleX = 4;
                const int8_t scaleY = 4;
                uint8_t difx = abs8(vlx * scaleX - nx);
                uint8_t dify = abs8(vly * scaleY - ny);
                int temp = difx * difx + dify * dify;
                int col = 255 - temp / 8; // 8 its a size of effect

                if (col < 0)
                    col = 0;
                g()->leds[XY(x, y)] = chsvLut[col]; // thx satubarosu ))
            }
            yindex += (MATRIX_WIDTH + 2);
        }
    }

  public:
    PatternSMSunRadiation() : EffectWithId<PatternSMSunRadiation>("Sun Radiation")
    {
    }

    PatternSMSunRadiation(const JsonObjectConst &jsonObject) : EffectWithId<PatternSMSunRadiation>(jsonObject)
    {
    }

    void Start() override
    {
        g()->Clear();
        generateCHSVlut();
    }

    void Draw() override
    {
        generatebump();
        Bumpmap();
    }
};
