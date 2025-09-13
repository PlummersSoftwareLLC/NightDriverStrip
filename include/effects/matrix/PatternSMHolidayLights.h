#pragma once

#include "effectmanager.h"

// Derived from
// https://editor.soulmatelights.com/gallery/552-festive-lighting-green-with-toys
// Holiday lights
//@stepko
// Merry Christmas and Happy New Year

class PatternSMHolidayLights : public EffectWithId<PatternSMHolidayLights>
{

  private:

    static constexpr int speed = (200 / (MATRIX_HEIGHT - 4));
    uint8_t hue {0};
    uint8_t effId = 2; // 1 - 3

    const uint8_t maxDim = max(MATRIX_WIDTH, MATRIX_HEIGHT);
    const uint8_t minDim = min(MATRIX_WIDTH, MATRIX_HEIGHT);
    const uint8_t width_adj = (MATRIX_WIDTH < MATRIX_HEIGHT ? (MATRIX_HEIGHT - MATRIX_WIDTH) / 2 : 0);
    const uint8_t height_adj = (MATRIX_HEIGHT < MATRIX_WIDTH ? (MATRIX_WIDTH - MATRIX_HEIGHT) / 2 : 0);
    const bool glitch = abs(MATRIX_WIDTH - MATRIX_HEIGHT) >= minDim / 4;

    uint8_t density = 50;
    uint8_t fadingSpeed = 10;
    uint8_t updateFromRGBWeight = 10;
    const uint8_t scaleToNumLeds = NUM_LEDS / 256;

    void confetti()
    {
        uint16_t idx = random16(NUM_LEDS);
        for (unsigned i = 0; i < scaleToNumLeds; i++)
            if (random8() < density)
                if ((g()->getPixel(idx).r + g()->getPixel(idx).g + g()->getPixel(idx).b) < 10)
                    g()->leds[idx] = random(48, 16777216);
    }

    void addGlitter(uint8_t chanceOfGlitter)
    {
        if (random8() < chanceOfGlitter)
            g()->leds[random16(NUM_LEDS)] = random(0, 16777215);
    }

    void spruce()
    {
        hue++; // Increment the hue value, which likely controls the color of the LEDs.

        // Fade all LED channels to black based on the 'speed' value.
        // The 'map' function scales the 'speed' value from one range to another.
        fadeAllChannelsToBlackBy(::map(speed, 1, 255, 1, 100));

        uint8_t z;
        if (effId == 3)
            z = triwave8(hue); // Set 'z' to a value calculated using 'triwave8' if 'effId' is 3.
        else
            z = beatsin8(1, 1, 255); // Otherwise, set 'z' to a value calculated using 'beatsin8'.

        for (uint8_t i = 0; i < minDim; i++)
        {
            // Calculate 'x' based on various factors.
            unsigned x = beatsin16(i * (::map(speed, 1, 255, 3, 20)), i * 2, (minDim * 4 - 2) - (i * 2 + 2));

            if (effId == 2)
            {
                // Draw a pixel with certain conditions if 'effId' is 2.
                g()->drawPixelXYF_Wu(x / 4 + height_adj, (float)(MATRIX_HEIGHT - 1 - i),
                        random8(10) == 0 ? CHSV(random8(), random8(32, 255), 255)
                                     : CHSV(100, 255, ::map(speed, 1, 255, 128, 100)));
            }
            else
            {
                // Draw a pixel with different color conditions if 'effId' is not 2.
                g()->drawPixelXYF_Wu(x / 4 + height_adj, (float)(MATRIX_HEIGHT - 1 - i), CHSV(hue + i * z, 255, 255));
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

    PatternSMHolidayLights() : EffectWithId<PatternSMHolidayLights>("Tannenbaum") {
    }
    PatternSMHolidayLights(const JsonObjectConst &jsonObject) : EffectWithId<PatternSMHolidayLights>(jsonObject) {
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
