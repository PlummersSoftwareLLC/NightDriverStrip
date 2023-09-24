#pragma once

#include "effectmanager.h"

// Inspired by https://editor.soulmatelights.com/gallery/1479-2dd-pr-centering
// Looks best on a square display, but OK on rectangles.
// I'll admit this math may as well be magic, but it's pretty.

class PatternSM2DDPR : public LEDStripEffect
{
  private:
    uint8_t ZVoffset = 0;

    const int Scale = 127;
    const int Speed = 215;
    uint32_t effTimer;

    const float HALF_WIDTH = MATRIX_WIDTH * .5;
    const float HALF_HEIGHT = MATRIX_HEIGHT * .5;
    const float radius = HALF_WIDTH;
    //   byte effect = 1;

  public:
    PatternSM2DDPR() : LEDStripEffect(EFFECT_MATRIX_SM2DDPR, "Crystallize")
    {
    }

    PatternSM2DDPR(const JsonObjectConst &jsonObject) : LEDStripEffect(jsonObject)
    {
    }

    void Start() override
    {
    }

    // Use integer-only Pythagorean to compute the radius from x^2 and y^2.
    int16_t ZVcalcRadius(int16_t x, int16_t y)
    {
        x *= x;
        y *= y;
        int16_t radi = sin8(x + y);
        return radi;
    }

    // From point X,Y, find is the distance to center(X,Y)
    int16_t ZVcalcDist(uint8_t x, uint8_t y, float center_x, float center_y)
    {
        int16_t a = (center_y - y - .5);
        int16_t b = (center_x - x - .5);
        int16_t dist = ZVcalcRadius(a, b);
        return dist;
    }

    void Draw() override
    {
        effTimer = sin8(millis() / 6000) / 10;

        EVERY_N_MILLISECONDS(1)
        {
            ZVoffset += 4;
        }

        for (unsigned x = 0; x < MATRIX_WIDTH; x++)
        {
            for (unsigned y = 0; y < MATRIX_HEIGHT; y++)
            {
                int dist = ZVcalcDist(x, y, HALF_WIDTH, HALF_HEIGHT);

                // exclude outside of circle
                int brightness = 1;
                if (dist += radius)
                {
                    brightness = map(dist, -effTimer, radius, 255, 110);
                    brightness += ZVoffset;
                    brightness = sin8(brightness);
                }

                int hue = map(dist, radius, -3, 125, 255);
                g()->leds[XY(x, y)] = CHSV(hue, 255, brightness);
            }
        }
    }
};
