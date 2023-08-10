#pragma once

#include "effectmanager.h"
#include "effects/strip/musiceffect.h"

// Derived from https://editor.soulmatelights.com/gallery/1127-sand
// This looks better on a square display than a landscape one, but it's
// stil cool enough to include, IMO.

#if ENABLE_AUDIO
class PatternSMSand : public BeatEffectBase, public LEDStripEffect
#else
class PatternSMSand : public LEDStripEffect
#endif
{
  private:
    uint8_t Scale = 85; // 1-100 limit Should be a setting. % of screen height

    static constexpr int WIDTH = MATRIX_WIDTH;
    static constexpr int HEIGHT = MATRIX_HEIGHT;

    // matrix size constants are calculated only here and do not change in effects
    static constexpr uint8_t CENTER_X_MINOR =
        (MATRIX_WIDTH / 2) - ((MATRIX_WIDTH - 1) & 0x01); // the center of the matrix according to ICSU, shifted to the
                                                          // smaller side, if the width is even
    static constexpr uint8_t CENTER_Y_MINOR =
        (MATRIX_HEIGHT / 2) -
        ((MATRIX_HEIGHT - 1) & 0x01); // center of the YGREK matrix, shifted down if the height is even
    static constexpr uint8_t CENTER_X_MAJOR =
        MATRIX_WIDTH / 2 + (MATRIX_WIDTH % 2); // the center of the matrix according to IKSU,
                                               // shifted to a larger side, if the width is even
    static constexpr uint8_t CENTER_Y_MAJOR =
        MATRIX_HEIGHT / 2 + (MATRIX_HEIGHT % 2); // center of the YGREK matrix, shifted up if the height is even
#if ENABLE_AUDIO
    static constexpr uint8_t top_reserve = 1; // I'm not sure that every place that needs it has it.
#else
    static constexpr uint8_t top_reserve = 0;
#endif

    uint8_t pcnt{0}; // какой-то счётчик какого-то прогресса

  public:
    PatternSMSand()
        :
#if ENABLE_AUDIO
          BeatEffectBase(1.50, 0.05),
#endif
          LEDStripEffect(EFFECT_MATRIX_SMSAND, "Sand")
    {
    }

    PatternSMSand(const JsonObjectConst &jsonObject)
        :
#if ENABLE_AUDIO
          BeatEffectBase(1.50, 0.05),
#endif
          LEDStripEffect(jsonObject)
    {
    }

    void Start() override
    {
        g()->Clear();
    }

    [[nodiscard]] CRGB getPixColorXY(uint8_t x, uint8_t y) const
    {
        y = MATRIX_HEIGHT - 1 - y;
        assert((g()->isValidPixel(x, y)));
        return g()->leds[XY(x, y)];
    }

    void drawPixelXY(uint8_t x, uint8_t y, CRGB color) const
    {
        y = MATRIX_HEIGHT - 1 - y;
        assert((g()->isValidPixel(x, y)));
        g()->leds[XY(x, y)] = color;
    }

    void Draw() override
    {
#if ENABLE_AUDIO
        ProcessAudio();
#endif

        // if enough has already been poured, bang random grains of sand
        uint8_t temp = map8(random8(), Scale * 2.55, 255U);
        if (pcnt >= map8(temp, 2U, HEIGHT - 3U))
        {
            temp = HEIGHT + 1U - pcnt;
            if (!random8(4U)) // sometimes sand crumbles up to half at once
                if (random8(2U))
                    temp = 2U;
                else
                    temp = 3U;
            for (uint8_t y = 1; y < pcnt; y++)
                for (uint8_t x = 0; x < WIDTH; x++)
                    if (!random8(temp))
                        g()->leds[XY(x, y)] = CRGB::Black;
        }

        pcnt = 0U;
        // In this effect, 0, 0 is our lower left corner. getPX and drawPX handle
        // our coordinate inversion.
        //
        // 1 skips the bottom line, so sand will accumulate.
        // HEIGHT - top_reserve == our top drawable line, respecting VU meter.
        for (uint8_t y = 1; y < HEIGHT - top_reserve; y++) // Skip audio line
            for (uint8_t x = 0; x < WIDTH; x++)
                if (auto me = getPixColorXY(x, y))
                { // checking for every grain of sand
                    if (!getPixColorXY(x, y - 1))
                    {                                   // if it's empty below us, we just fall
                        drawPixelXY(x, y - 1, me);      // draw one pixel down
                        drawPixelXY(x, y, CRGB::Black); //  blank our own
                    }
                    else if (x > 0U && !getPixColorXY(x - 1, y - 1) && x < WIDTH - 1 && !getPixColorXY(x + 1, y - 1))
                    {
                        // if we have a peak, scatter the fallen unit left or right.
                        if (int r = random8(2U))
                            drawPixelXY(x - r * 3, y - 1, me);
                        else
                            drawPixelXY(x + r * 3, y - 1, me);
                        drawPixelXY(x, y, CRGB::Black);
                        pcnt = y - 1;
                    }
                    else if (x > 0U && !getPixColorXY(x - 1, y - 1))
                    { // a slope to the left
                        drawPixelXY(x - 1, y - 1, me);
                        drawPixelXY(x, y, CRGB::Black);
                        pcnt = y - 1;
                    }
                    else if (x < WIDTH - 1 && !getPixColorXY(x + 1, y - 1))
                    { // if below us slopes to the right
                        drawPixelXY(x + 1, y - 1, me);
                        drawPixelXY(x, y, CRGB::Black);
                        pcnt = y - 1;
                    }
                    else // if there is a plateau below us
                        pcnt = y;
                }

        // Emit randomly colored new grains of sand at top.
        // HEIGHT - 1 == top row, occupied by VU.
        // HEIGHT - top_reserve - 1 == our top drawable row
        // If the center has space, randomly fill it. the loop above will fall it.
        if (!getPixColorXY(CENTER_X_MINOR, HEIGHT - top_reserve - 1) &&
            !getPixColorXY(CENTER_X_MAJOR, HEIGHT - top_reserve - 1) && !random8(3))
        {
            temp = random8(2) ? CENTER_X_MINOR : CENTER_X_MAJOR;
            drawPixelXY(temp, HEIGHT - top_reserve - 1, CHSV(random8(), 255U, 255U));
        }
    }

#if ENABLE_AUDIO
    void HandleBeat(bool bMajor, float elapsed, float span) override
    {
        // Force deletion (on the next Draw()) of the bottom row. It's simple,
        // but it's enough to make the pyramid visibly respond to ambient sounds.
        for (uint8_t x = 0; x < WIDTH; x++)
        {
            drawPixelXY(x, 0, CRGB::Black);
        }
    }
#endif
};
