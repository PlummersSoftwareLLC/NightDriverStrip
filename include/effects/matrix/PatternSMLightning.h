#pragma once

#include "effectmanager.h"

// Adapted from https://editor.soulmatelights.com/gallery/1004-amazing-lightning
//
// Probably best with a diffuser.

class PatternSMLightning : public EffectWithId<PatternSMLightning>
{
  private:
    int call = 0;   // time
    int aux0 = 127; // root position
    int aux1 = 4;   // how many lines

    // all based on perlin noise - nice!
    float RANDOM(const uint8_t i, const int y)
    {
        return (float)inoise8_raw(aux0 * 77 + (i + 1) * 199, y * 33) * 2.0f;
    }

  public:
    PatternSMLightning() : EffectWithId<PatternSMLightning>("Lightning")
    {
    }

    PatternSMLightning(const JsonObjectConst &jsonObject) : EffectWithId<PatternSMLightning>(jsonObject)
    {
    }

    void Start() override
    {
        g()->Clear();
    }

    void Draw() override
    {
        const int matrixWidth = MATRIX_WIDTH;
        const int matrixHeight = MATRIX_HEIGHT;

        g()->Clear();

        call += aux1;
        if (call > 250)
        {
            call = 0;
            if (aux1 > 1)
            {
                aux1 = 1;
            }
            else
            {
                aux1 = 4;
                aux0 = map8(random8(), 64, 192);
                fill_solid(g()->leds, matrixWidth * matrixHeight, CRGB::Black);
            }
        }

        bool condition = (call == 0 || call == 20 || (call == 50 && aux1 == 1));
        if (condition)
        { // draw main line
            int y = matrixHeight;
            float x0 = aux0 - RANDOM(0, y);
            for (; y >= 0; y--)
            {
                auto x = x0 + RANDOM(0, y);
                g()->drawPixelXYF_Wu(x * matrixWidth / 255.0f, y, CRGB::White);
            }
        }
        else if (aux1 != 1)
        { // draw only parts of 'searching' lines
            for (int i = 0; i < aux1; i++)
            {
                int y = matrixHeight;
                float x0 = aux0 - RANDOM(i, y); // substract the first offset so that all
                                                  // lines come from the same place (aux0)
                y = map8(255 - call, 0, matrixHeight);
                auto x = x0 + RANDOM(i, y);
                g()->drawPixelXYF_Wu(x * matrixWidth / 255.0f, y, CRGB::Gray);
            }
        }

        nscale8(g()->leds, matrixWidth * matrixHeight, 250);
    }
};
