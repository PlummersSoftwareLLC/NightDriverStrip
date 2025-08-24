#pragma once

#include "effectmanager.h"

// Inspired by https://editor.soulmatelights.com/gallery/1166-stars-beta-ver
// The original has a bunch of Palette management stuff we just didn't
// implement.

class PatternSMStarDeep : public EffectWithId<idMatrixSMStarDeep>
{
  private:

   static constexpr int kMaxStars = 100U; // the maximum number of tracked
                                  // objects (very affects memory consumption)

    // Define a struct to hold star data
    struct StarData {
        uint8_t color;   // star color
        uint8_t corners; // number of corners in the star
        int position;    // delay the start of the star relative to the counter
    };

    StarData stars[kMaxStars];
    uint8_t nStars;            // number of stars

    float driftx, drifty;
    float driftAngleX, driftAngleY;
    unsigned int counter { 0 };
    const TProgmemRGBPalette16 *curPalette = &PartyColors_p;

    static constexpr int STAR_BLENDER = 128U; // Adjust the brightness or blending of the stars.
    static constexpr int CENTER_DRIFT_SPEED = 6U; // speed of movement of the floating star emergence center

    // Matrix size constants are calculated only here and do not change in effects.
    static constexpr uint8_t CENTER_X_MINOR = (MATRIX_WIDTH / 2) - ((MATRIX_WIDTH - 1) & 0x01); // The center of the matrix along the X-axis, shifted to the smaller
                                                                       // side if the width is even.
    static constexpr uint8_t CENTER_Y_MINOR = (MATRIX_HEIGHT / 2) - ((MATRIX_HEIGHT - 1) & 0x01); // The center of the matrix along the Y-axis, shifted to the smaller
                                                                         // side if the height is even.
    static constexpr uint8_t CENTER_X_MAJOR = MATRIX_WIDTH / 2 + (MATRIX_WIDTH % 2); // The center of the matrix along the X-axis, shifted to the larger
                                                            // side if the width is even.
    static constexpr uint8_t CENTER_Y_MAJOR = MATRIX_HEIGHT / 2 + (MATRIX_HEIGHT % 2); // The center of the matrix along the Y-axis, shifted to the
                                                              // larger side if the height is even.

  public:

    PatternSMStarDeep() : EffectWithId<idMatrixSMStarDeep>("Star Deep") {}
    PatternSMStarDeep(const JsonObjectConst &jsonObject) : EffectWithId<idMatrixSMStarDeep>(jsonObject) {}

    // This code draws into unsafe places, but DrawLine() protects us.
    void Drawstar(int16_t xlocl, int16_t ylocl, int16_t biggy, int16_t little, int16_t points, int16_t dangle,
                  uint8_t color) // random multipoint star
    {
        auto radius2 = 255 / points;
        for (int i = 0; i < points; i++)
        {
            DrawStarLine(xlocl + ((little * (sin8(i * radius2 + radius2 / 2 - dangle) - 128.0)) / 128),
                         ylocl + ((little * (cos8(i * radius2 + radius2 / 2 - dangle) - 128.0)) / 128),
                         xlocl + ((biggy * (sin8(i * radius2 - dangle) - 128.0)) / 128),
                         ylocl + ((biggy * (cos8(i * radius2 - dangle) - 128.0)) / 128),
                         g()->IsPalettePaused() ? g()->ColorFromCurrentPalette(color) : ColorFromPalette(*curPalette, color));
            DrawStarLine(xlocl + ((little * (sin8(i * radius2 - radius2 / 2 - dangle) - 128.0)) / 128),
                         ylocl + ((little * (cos8(i * radius2 - radius2 / 2 - dangle) - 128.0)) / 128),
                         xlocl + ((biggy * (sin8(i * radius2 - dangle) - 128.0)) / 128),
                         ylocl + ((biggy * (cos8(i * radius2 - dangle) - 128.0)) / 128),
                         g()->IsPalettePaused() ? g()->ColorFromCurrentPalette(color) :ColorFromPalette(*curPalette, color));
        }
    }
    // This code looks like g()->DrawLine(), but isn't. It does funky
    // things to swap x and y internally to let the corners wrap. Replacing
    // it is visually ugly and teaching the system version about this
    // isn't a net win. For this reason, it has a unique name.
    void DrawStarLine(int x1, int y1, int x2, int y2, CRGB color)
    {
        int tmp;
        int x, y;
        int dx, dy;
        int err;
        int ystep;

        uint8_t swapxy = 0;

        if (x1 > x2)
            dx = x1 - x2;
        else
            dx = x2 - x1;
        if (y1 > y2)
            dy = y1 - y2;
        else
            dy = y2 - y1;

        if (dy > dx)
        {
            swapxy = 1;
            tmp = dx;
            dx = dy;
            dy = tmp;
            tmp = x1;
            x1 = y1;
            y1 = tmp;
            tmp = x2;
            x2 = y2;
            y2 = tmp;
        }
        if (x1 > x2)
        {
            tmp = x1;
            x1 = x2;
            x2 = tmp;
            tmp = y1;
            y1 = y2;
            y2 = tmp;
        }
        err = dx >> 1;
        if (y2 > y1)
            ystep = 1;
        else
            ystep = -1;
        y = y1;

        for (x = x1; x <= x2; x++)
        {
            if (swapxy == 0)
                g()->drawPixel(x, MATRIX_HEIGHT - 1 - y, color);
            else
                g()->drawPixel(y, MATRIX_HEIGHT - 1 - x, color);
            err -= (uint8_t)dy;
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

        driftx = random8(4, MATRIX_WIDTH - 4);  // set an initial location for the animation center
        drifty = random8(4, MATRIX_HEIGHT - 4); // set an initial location for the animation center

        driftAngleX = (sin8(random(25, 220)) - 128.0) /
                 128.0; // angle of movement for the center of animation gives a float value between -1 and 1
        driftAngleY = (sin8(random(25, 220)) - 128.0) / 128.0; // angle of movement for the center of animation in the y
                                                          // direction gives a float value between -1 and 1

        // Each star has its own number of corners.
        nStars = (MATRIX_WIDTH + 6U) / 2U;

        if (nStars > kMaxStars)
            nStars = kMaxStars;

        for (uint8_t num = 0; num < nStars; num++)
        {
            stars[num].corners = random8(3, 9); // number of corners in the star
            stars[num].position = counter + (num << 3) + 1U; // delay the next star launch
            stars[num].color = random8();
        }
    }

    void Draw() override
    {
        g()->DimAll(175U);

        // Now each star has its own hue.
        counter++;

        // This block of 6 'if's could probably be replaced by a std::clamp...if I
        // could read this junk. This keeps the absolute unit of our drift (xy) and
        // the angle of our drift (sin, cos) in check.
        if (driftx > (MATRIX_WIDTH - CENTER_X_MINOR / 2U)) {
            driftAngleX = -fabs(driftAngleX); // Force movement left
        } else if (driftx < CENTER_X_MINOR / 2U) {
            driftAngleX = fabs(driftAngleX); // Force movement right
        }
        if (counter % CENTER_DRIFT_SPEED == 0) {
            driftx = driftx + driftAngleX; // Move the x center every so often
        }

        if (drifty > (MATRIX_HEIGHT - CENTER_Y_MINOR / 2U)) {
            driftAngleY = -fabs(driftAngleY); // Force movement up
        } else if (drifty < CENTER_Y_MINOR / 2U) {
            driftAngleY = fabs(driftAngleY); // Force movement down
        }
        if ((counter + CENTER_DRIFT_SPEED / 2U) % CENTER_DRIFT_SPEED == 0) {
            drifty = drifty + driftAngleY; // Move the y center every so often
        }

        for (uint8_t num = 0; num < nStars; num++)
        {
            if (counter >= stars[num].position) //(counter >= ringdelay)
            {
                float expansionFactor = 1.0f; // Add this member variable to your class
                int starSize = (counter - stars[num].position) * expansionFactor;

                if (starSize <= MATRIX_WIDTH + 5U)
                {
                    Drawstar(driftx, drifty, 2 * starSize, starSize, stars[num].corners,
                             STAR_BLENDER + stars[num].color, stars[num].color * 2);
                    stars[num].color++;
                }
                else
                {
                    // Number of corners in the star.
                    stars[num].position = counter + (nStars << 2) + 1U;
                }
            }
        }
    }
};
