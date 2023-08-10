#pragma once

#include "effectmanager.h"

// Yo Dawg! Circles inside your circles, but XORing the patterns.
// Needs more clever color.
// Inspired by https://editor.soulmatelights.com/gallery/1521-xor-circles

class PatternSMXorCircles : public LEDStripEffect
{
  private:
    /*
        double log2(double num){
          double number=log(num)/log(2);
          return (number);
        }
    */
#define log2(num) log(num) / log(2)
    static constexpr uint8_t scale_x = log2(64 / MATRIX_WIDTH);
    static constexpr uint8_t scale_y = log2(64 / MATRIX_HEIGHT);

  public:
    PatternSMXorCircles() : LEDStripEffect(EFFECT_MATRIX_SMXOR_CIRCLES, "Xor Circles")
    {
    }

    PatternSMXorCircles(const JsonObjectConst &jsonObject) : LEDStripEffect(jsonObject)
    {
    }

    void Start() override
    {
        g()->Clear();
    }

    void Draw() override
    {
        uint8_t x1sh = beatsin8(5, 0, MATRIX_WIDTH);
        uint8_t y1sh = beatsin8(6, 0, MATRIX_HEIGHT);
        uint8_t x2sh = beatsin8(7, 0, MATRIX_WIDTH);
        uint8_t y2sh = beatsin8(4, 0, MATRIX_HEIGHT);
        for (uint8_t y = 0; y < MATRIX_HEIGHT; y++)
        {
            for (uint8_t x = 0; x < MATRIX_WIDTH; x++)
            {
                int8_t cx = x - x1sh;
                int8_t cy = y - y1sh;
                uint8_t a = sqrt16(((cx * cx) + (cy * cy))) << scale_x;
                cx = x - x2sh;
                cy = y - y2sh;
                uint8_t v = sqrt16(((cx * cx) + (cy * cy))) << scale_y;
                g()->leds[XY(x, y)] = (((a ^ v) >> 4) & 1) * 255;
            }
        }
    }
};

#undef log2
