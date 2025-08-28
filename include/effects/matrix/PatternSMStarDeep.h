#pragma once

#include "effectmanager.h"
#include <algorithm> // for std::swap
#include <cmath>     // for fabsf
#include <memory>    // for std::unique_ptr
#include "types.h"   // for make_unique_psram

// Inspired by https://editor.soulmatelights.com/gallery/1166-stars-beta-ver
// The original has a bunch of Palette management stuff we just didn't
// implement.

class PatternSMStarDeep : public EffectWithId<PatternSMStarDeep>
{
  private:
    // Maximum number of stars to track. Affects memory consumption.
    static constexpr int kMaxStars = 100U;

    // Star data structure
    struct StarData {
        uint8_t color;   // star color index
        uint8_t corners; // number of corners in the star
        int position;    // delay the start of the star relative to the counter
    };

    std::unique_ptr<StarData[]> stars; // Dynamically allocated in PSRAM
    uint8_t nStars;            // number of active stars

    float driftx, drifty;
    float driftAngleX, driftAngleY;
    unsigned int counter { 0 };
    const TProgmemRGBPalette16 *curPalette = &PartyColors_p;

    // Countdown timers for drift movement to avoid expensive modulo operations.
    uint8_t x_drift_countdown;
    uint8_t y_drift_countdown;

    static constexpr int kStarBlender = 128U; // Adjust the brightness or blending of the stars.
    static constexpr int kCenterDriftSpeed = 6U; // Speed of movement of the floating star emergence center.

    // The following constants define a central area on the matrix where
    // the effect's center is allowed to drift.
    static constexpr uint8_t kDriftMarginX = ((MATRIX_WIDTH - 1) / 2) / 2;
    static constexpr uint8_t kDriftMarginY = ((MATRIX_HEIGHT - 1) / 2) / 2;

  public:

    PatternSMStarDeep() : EffectWithId<PatternSMStarDeep>("Star Deep"), stars(make_unique_psram<StarData[]>(kMaxStars)) {}
    PatternSMStarDeep(const JsonObjectConst &jsonObject) : EffectWithId<PatternSMStarDeep>(jsonObject), stars(make_unique_psram<StarData[]>(kMaxStars)) {}

    // Draws a multi-point star.
    // This code can draw outside of the matrix boundaries, but DrawStarLine() is expected to handle clipping.
    void DrawStar(int16_t centerX, int16_t centerY, int16_t outerRadius, int16_t innerRadius, uint8_t numPoints, uint8_t angleOffset,
                  uint8_t colorIndex)
    {
        if (numPoints == 0) return;

        const CRGB starColor = g()->IsPalettePaused() ? g()->ColorFromCurrentPalette(colorIndex) : ColorFromPalette(*curPalette, colorIndex);
        const uint8_t angle_step = 255 / numPoints;

        for (uint8_t i = 0; i < numPoints; i++)
        {
            const uint8_t outer_angle = i * angle_step - angleOffset;
            const int16_t outer_x = centerX + ((outerRadius * (sin8(outer_angle) - 128.0f)) / 128.0f);
            const int16_t outer_y = centerY + ((outerRadius * (cos8(outer_angle) - 128.0f)) / 128.0f);

            const uint8_t inner_angle_1 = outer_angle + angle_step / 2;
            const int16_t inner_x_1 = centerX + ((innerRadius * (sin8(inner_angle_1) - 128.0f)) / 128.0f);
            const int16_t inner_y_1 = centerY + ((innerRadius * (cos8(inner_angle_1) - 128.0f)) / 128.0f);

            const uint8_t inner_angle_2 = outer_angle - angle_step / 2;
            const int16_t inner_x_2 = centerX + ((innerRadius * (sin8(inner_angle_2) - 128.0f)) / 128.0f);
            const int16_t inner_y_2 = centerY + ((innerRadius * (cos8(inner_angle_2) - 128.0f)) / 128.0f);

            DrawStarLine(inner_x_1, inner_y_1, outer_x, outer_y, starColor);
            DrawStarLine(inner_x_2, inner_y_2, outer_x, outer_y, starColor);
        }
    }

    // Custom line drawing function.
    // This implementation is kept for its specific visual characteristics ("funky wrapping corners"),
    // which differ from the standard g()->DrawLine().
    void DrawStarLine(int x1, int y1, int x2, int y2, CRGB color)
    {
        int x, y;
        int dx, dy;
        int err;
        int ystep;

        bool isSteep = abs(y2 - y1) > abs(x2 - x1);
        if (isSteep)
        {
            std::swap(x1, y1);
            std::swap(x2, y2);
        }
        if (x1 > x2)
        {
            std::swap(x1, x2);
            std::swap(y1, y2);
        }

        dx = x2 - x1;
        dy = abs(y2 - y1);
        err = dx / 2;
        ystep = (y1 < y2) ? 1 : -1;
        y = y1;

        for (x = x1; x <= x2; x++)
        {
            if (isSteep)
                g()->drawPixel(y, MATRIX_HEIGHT - 1 - x, color);
            else
                g()->drawPixel(x, MATRIX_HEIGHT - 1 - y, color);
            err -= dy;
            if (err < 0)
            {
                y += ystep;
                err += dx;
            }
        }
    }

    void Start() override
    {
        g()->Clear();

        // Set an initial location and movement vector for the animation center.
        driftx = random8(4, MATRIX_WIDTH - 4);
        drifty = random8(4, MATRIX_HEIGHT - 4);

        driftAngleX = (sin8(random(25, 220)) - 128.0f) / 128.0f;
        driftAngleY = (sin8(random(25, 220)) - 128.0f) / 128.0f;

        // Initialize drift counters
        x_drift_countdown = kCenterDriftSpeed;
        y_drift_countdown = kCenterDriftSpeed / 2;

        nStars = (MATRIX_WIDTH + 6U) / 2U;
        if (nStars > kMaxStars)
            nStars = kMaxStars;

        for (uint8_t num = 0; num < nStars; num++)
        {
            stars[num].corners = random8(3, 9);
            stars[num].position = counter + (num << 3) + 1U;
            stars[num].color = random8();
        }
    }

    void Draw() override
    {
        g()->DimAll(175U);
        counter++;

        // Update drift center position, bouncing off the edges of the defined drift area.
        if (driftx > (MATRIX_WIDTH - kDriftMarginX)) {
            driftAngleX = -fabsf(driftAngleX); // Force movement left
        } else if (driftx < kDriftMarginX) {
            driftAngleX = fabsf(driftAngleX); // Force movement right
        }
        if (--x_drift_countdown == 0) {
            driftx += driftAngleX;
            x_drift_countdown = kCenterDriftSpeed;
        }

        if (drifty > (MATRIX_HEIGHT - kDriftMarginY)) {
            driftAngleY = -fabsf(driftAngleY); // Force movement up
        } else if (drifty < kDriftMarginY) {
            driftAngleY = fabsf(driftAngleY); // Force movement down
        }
        if (--y_drift_countdown == 0) {
            drifty += driftAngleY;
            y_drift_countdown = kCenterDriftSpeed;
        }

        for (uint8_t num = 0; num < nStars; num++)
        {
            if (counter >= stars[num].position)
            {
                int starSize = counter - stars[num].position;

                if (starSize <= MATRIX_WIDTH + 5U)
                {
                    DrawStar(driftx, drifty, 2 * starSize, starSize, stars[num].corners,
                             kStarBlender + stars[num].color, stars[num].color * 2);
                    stars[num].color++;
                }
                else
                {
                    // Reset the star when it's grown large enough
                    stars[num].position = counter + (nStars << 2) + 1U;
                }
            }
        }
    }
};
