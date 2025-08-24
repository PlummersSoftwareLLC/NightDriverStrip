#pragma once

#include "effectmanager.h"

// Inspired by https://editor.soulmatelights.com/gallery/1166-stars-beta-ver
// The original has a bunch of Palette management stuff we just didn't
// implement.

class PatternSMStarDeep : public EffectWithId<idMatrixSMStarDeep>
{
  private:

   static constexpr int bballsMaxNUM = 100U; // the maximum number of tracked
                                  // objects (very affects memory consumption)
    uint8_t bballsCOLOR[bballsMaxNUM]; // star color (reusing the Balls effect array)
    uint8_t bballsX[bballsMaxNUM];     // number of corners in the star (reusing the
                                       // Balls effect array
    int bballsPos[bballsMaxNUM];       // delay the start of the star relative to the
                                       // counter (reuse the Balls effect array)
    uint8_t bballsNUM;                 // number of stars (reuse Balls effect variable)

    float driftx, drifty;
    float cangle, sangle;
    unsigned int counter { 0 };
    const TProgmemRGBPalette16 *curPalette = &PartyColors_p;

    const int STAR_BLENDER = 128U; // Unknown what this is.
    const int CENTER_DRIFT_SPEED = 6U; // speed of movement of the floating star emergence center
    const int WIDTH = MATRIX_WIDTH;
    const int HEIGHT = MATRIX_HEIGHT;

    // Matrix size constants are calculated only here and do not change in effects.
    const uint8_t CENTER_X_MINOR = (WIDTH / 2) - ((WIDTH - 1) & 0x01); // The center of the matrix along the X-axis, shifted to the smaller
                                                                       // side if the width is even.
    const uint8_t CENTER_Y_MINOR = (HEIGHT / 2) - ((HEIGHT - 1) & 0x01); // The center of the matrix along the Y-axis, shifted to the smaller
                                                                         // side if the height is even.
    const uint8_t CENTER_X_MAJOR = WIDTH / 2 + (WIDTH % 2); // The center of the matrix along the X-axis, shifted to the larger
                                                            // side if the width is even.
    const uint8_t CENTER_Y_MAJOR = HEIGHT / 2 + (HEIGHT % 2); // The center of the matrix along the Y-axis, shifted to the
                                                              // larger side if the height is even.

    const int spirocenterX = CENTER_X_MINOR;
    const int spirocenterY = CENTER_Y_MINOR;

  public:

    PatternSMStarDeep() : EffectWithId<idMatrixSMStarDeep>("Star Deep") {}
    PatternSMStarDeep(const JsonObjectConst &jsonObject) : EffectWithId<idMatrixSMStarDeep>(jsonObject) {}

    void Start() override
    {
        g()->Clear();

        driftx = random8(4, WIDTH - 4);  // set an initial location for the animation center
        drifty = random8(4, HEIGHT - 4); // set an initial location for the animation center

        cangle = (sin8(random(25, 220)) - 128.0) /
                 128.0; // angle of movement for the center of animation gives a float value between -1 and 1
        sangle = (sin8(random(25, 220)) - 128.0) / 128.0; // angle of movement for the center of animation in the y
                                                          // direction gives a float value between -1 and 1
        // shifty = random (3, 12);//how often the drifter moves будет
        // CENTER_DRIFT_SPEED = 6

        // Now each star has its own number of corners.
        bballsNUM = (WIDTH + 6U) / 2U; //(modes[currentMode].Scale - 1U) / 99.0 *
                                       //(bballsMaxNUM - 1U) + 1U;
        if (bballsNUM > bballsMaxNUM)
            bballsNUM = bballsMaxNUM;
        for (uint8_t num = 0; num < bballsNUM; num++)
        {
            bballsX[num] = random8(3, 9); // pointy = random8(3, 9); // number of corners in the star
            bballsPos[num] = counter + (num << 3) + 1U; // random8(50);//modes[currentMode].Scale;//random8(50,
                                                        // 99); // delay the next star launch
            bballsCOLOR[num] = random8();
        }
    }

    // This code draws into unsafe places, but DrawLine() protects us.
    void Drawstar(int16_t xlocl, int16_t ylocl, int16_t biggy, int16_t little, int16_t points, int16_t dangle,
                  uint8_t koler) // random multipoint star
    {
        auto radius2 = 255 / points;
        for (int i = 0; i < points; i++)
        {
            DrawStarLine(xlocl + ((little * (sin8(i * radius2 + radius2 / 2 - dangle) - 128.0)) / 128),
                         ylocl + ((little * (cos8(i * radius2 + radius2 / 2 - dangle) - 128.0)) / 128),
                         xlocl + ((biggy * (sin8(i * radius2 - dangle) - 128.0)) / 128),
                         ylocl + ((biggy * (cos8(i * radius2 - dangle) - 128.0)) / 128),
                         g()->IsPalettePaused() ? g()->ColorFromCurrentPalette(koler) : ColorFromPalette(*curPalette, koler));
            DrawStarLine(xlocl + ((little * (sin8(i * radius2 - radius2 / 2 - dangle) - 128.0)) / 128),
                         ylocl + ((little * (cos8(i * radius2 - radius2 / 2 - dangle) - 128.0)) / 128),
                         xlocl + ((biggy * (sin8(i * radius2 - dangle) - 128.0)) / 128),
                         ylocl + ((biggy * (cos8(i * radius2 - dangle) - 128.0)) / 128),
                         g()->IsPalettePaused() ? g()->ColorFromCurrentPalette(koler) :ColorFromPalette(*curPalette, koler));
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
                g()->drawPixel(x, HEIGHT - 1 - y, color);
            else
                g()->drawPixel(y, HEIGHT - 1 - x, color);
            err -= (uint8_t)dy;
            if (err < 0)
            {
                y += ystep;
                err += dx;
            }
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
        if (driftx > (WIDTH - spirocenterX / 2U)) // change direction of drift if you get
                                                  // near the right 1/4 of the screen
            cangle = 0 - fabs(cangle);
        if (driftx < spirocenterX / 2U) // change direction of drift if you get near
                                        // the right 1/4 of the screen
            cangle = fabs(cangle);
        if (counter % CENTER_DRIFT_SPEED == 0)
            driftx = driftx + cangle; // move the x center every so often

        if (drifty > (HEIGHT - spirocenterY / 2U)) // if y gets too big, reverse
            sangle = 0 - fabs(sangle);
        if (drifty < spirocenterY / 2U) // if y gets too small reverse
            sangle = fabs(sangle);
        if ((counter + CENTER_DRIFT_SPEED / 2U) % CENTER_DRIFT_SPEED == 0)
            drifty = drifty + sangle; // move the y center every so often

        for (uint8_t num = 0; num < bballsNUM; num++)
        {
            if (counter >= bballsPos[num]) //(counter >= ringdelay)
            {
                float expansionFactor = 1.0f; // Add this member variable to your class
                int starSize = (counter - bballsPos[num]) * expansionFactor;

                if (starSize <= WIDTH + 5U)
                {
                    Drawstar(driftx, drifty, 2 * starSize, starSize, bballsX[num], STAR_BLENDER + bballsCOLOR[num],
                             bballsCOLOR[num] * 2); // What the hell is 85?!
                    bballsCOLOR[num]++;
                }
                else
                {
                    // Number of corners in the star.
                    bballsPos[num] = counter + (bballsNUM << 2) + 1U;
                }
            }
        }
    }
};
