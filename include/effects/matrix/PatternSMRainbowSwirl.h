#pragma once

#include "effectmanager.h"

// Derived from https://editor.soulmatelights.com/gallery/1245-rainbow-swirl
// Super simple code, but quite pleasant.

class PatternSMRainbowSwirl : public LEDStripEffect
{
  private:
  public:
    PatternSMRainbowSwirl() : LEDStripEffect(EFFECT_MATRIX_SMRAINBOW_SWIRL, "Rainbow Swirl")
    {
    }

    PatternSMRainbowSwirl(const JsonObjectConst &jsonObject) : LEDStripEffect(jsonObject)
    {
    }

    void Start() override
    {
        g()->Clear();
    }

    void Draw() override
    {
        for (int x = 0; x < MATRIX_WIDTH; x++)
        {
            for (int y = 0; y < MATRIX_HEIGHT; y++)
            {
                // XY tells us the index of a given X/Y coordinate
                int index = XY(x, y);
                int hue = x * 10 + y * 10;
                hue += sin8(millis() / 50 + y * 5 + x * 7);
                g()->leds[index] = CHSV(hue, 255, 255);
            }
        }
    }
};
