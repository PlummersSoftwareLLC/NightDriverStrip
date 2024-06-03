#pragma once

#include "effectmanager.h"

// Derived from
// https://editor.soulmatelights.com/gallery/552-festive-lighting-green-with-toys
// Holiday lights
//@stepko
// Merry Christmas and Happy New Year

class PatternSMHolidayLights : public LEDStripEffect
{
  private:

    static constexpr int speed = (200 / (MATRIX_HEIGHT - 4));
    byte hue {0};
    byte effId = 2; // 1 - 3

    const byte maxDim = max(MATRIX_WIDTH, MATRIX_HEIGHT);
    const byte minDim = min(MATRIX_WIDTH, MATRIX_HEIGHT);
    const byte width_adj = (MATRIX_WIDTH < MATRIX_HEIGHT ? (MATRIX_HEIGHT - MATRIX_WIDTH) / 2 : 0);
    const byte height_adj = (MATRIX_HEIGHT < MATRIX_WIDTH ? (MATRIX_WIDTH - MATRIX_HEIGHT) / 2 : 0);
    const bool glitch = abs(MATRIX_WIDTH - MATRIX_HEIGHT) >= minDim / 4;

    byte density = 50;
    byte fadingSpeed = 10;
    byte updateFromRGBWeight = 10;
    const byte scaleToNumLeds = NUM_LEDS / 256;

    CRGB RGBweight(uint16_t idx)
    {
        return (g()->leds[idx].r + g()->leds[idx].g + g()->leds[idx].b);
    }

    void confetti()
    {
        uint16_t idx = random16(NUM_LEDS);
        for (unsigned i = 0; i < scaleToNumLeds; i++)
            if (random8() < density)
                if (RGBweight(idx) < 10)
                    g()->leds[idx] = random(48, 16777216);
    }

    void drawPixelXYF_X(float x, uint16_t y, const CRGB &color)
    {
	if (!g()->isValidPixel((int)x, y))
            return;

        // extract the fractional parts and derive their inverses
        uint8_t xx = (x - (int)x) * 255, ix = 255 - xx;
        // calculate the intensities for each affected pixel
        uint8_t wu[2] = {ix, xx};
        // multiply the intensities by the colour, and saturating-add them to the
        // pixels
        for (int8_t i = 1; i >= 0; i--)
        {
            int16_t xn = x + (i & 1);
            CRGB clr = g()->leds[XY(xn, MATRIX_HEIGHT - 1 - y)];
            if (xn > 0 && xn < (int)MATRIX_WIDTH - 1)
            {
                clr.r = qadd8(clr.r, (color.r * wu[i]) >> 8);
                clr.g = qadd8(clr.g, (color.g * wu[i]) >> 8);
                clr.b = qadd8(clr.b, (color.b * wu[i]) >> 8);
            }
            else if (xn == 0 || xn == (int)MATRIX_WIDTH - 1)
            {
                clr.r = qadd8(clr.r, (color.r * 85) >> 8);
                clr.g = qadd8(clr.g, (color.g * 85) >> 8);
                clr.b = qadd8(clr.b, (color.b * 85) >> 8);
            }
            g()->leds[XY(xn, MATRIX_HEIGHT - 1 - y)] = clr;
        }
    }

    void addGlitter(uint8_t chanceOfGlitter)
    {
        if (random8() < chanceOfGlitter)
            g()->leds[random16(NUM_LEDS)] = random(0, 16777215);
    }

void spruce()
{
    hue++;  // Increment the hue value, which likely controls the color of the LEDs.

    // Fade all LED channels to black based on the 'speed' value.
    // The 'map' function scales the 'speed' value from one range to another.
    fadeAllChannelsToBlackBy(map(speed, 1, 255, 1, 100));

    uint8_t z;
    if (effId == 3)
        z = triwave8(hue);  // Set 'z' to a value calculated using 'triwave8' if 'effId' is 3.
    else
        z = beatsin8(1, 1, 255);  // Otherwise, set 'z' to a value calculated using 'beatsin8'.

    for (uint8_t i = 0; i < minDim; i++)
    {
        // Calculate 'x' based on various factors.
        unsigned x = beatsin16(i * (map(speed, 1, 255, 3, 20)), i * 2, (minDim * 4 - 2) - (i * 2 + 2));

        if (effId == 2)
        {
            // Draw a pixel with certain conditions if 'effId' is 2.
            drawPixelXYF_X(x / 4 + height_adj, i,
                           random8(10) == 0 ? CHSV(random8(), random8(32, 255), 255)
                                            : CHSV(100, 255, map(speed, 1, 255, 128, 100)));
        }
        else
        {
            // Draw a pixel with different color conditions if 'effId' is not 2.
            drawPixelXYF_X(x / 4 + height_adj, i, CHSV(hue + i * z, 255, 255));
        }
    }

    // Set a specific LED color based on some conditions and time.
    if (!(MATRIX_WIDTH & 0x01))
    {
        g()->leds[XY(MATRIX_WIDTH / 2 - ((millis() >> 9) & 0x01 ? 1 : 0), minDim - 1 - ((millis() >> 8) & 0x01 ? 1 : 0))] =
            CHSV(0, 255, 255);
    }
    else
    {
        g()->leds[XY(MATRIX_WIDTH / 2, minDim - 1)] = CHSV(0, (millis() >> 9) & 0x01 ? 0 : 255, 255);
    }

    // If 'glitch' is true, call the 'confetti' function.
    if (glitch)
        confetti();
}


  public:
    PatternSMHolidayLights() : LEDStripEffect(EFFECT_MATRIX_SMHOLIDAY_LIGHTS, "Tannenbaum")
    {
    }

    PatternSMHolidayLights(const JsonObjectConst &jsonObject) : LEDStripEffect(jsonObject)
    {
    }

    void Start() override
    {
        g()->Clear();
    }

    void Draw() override
    {
        spruce();
    }
};
