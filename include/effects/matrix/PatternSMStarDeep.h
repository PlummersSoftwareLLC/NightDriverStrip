#pragma once

#include "effectmanager.h"

// Inspired by https://editor.soulmatelights.com/gallery/1166-stars-beta-ver
// The original has a bunch of Palette management stuff we just didn't
// implement.

class PatternSMStarDeep : public LEDStripEffect
{
  private:
// Why are these named "bballs"? Probably reused effect innards.
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

    const int STAR_BLENDER = 128U; // хз что это
    const int CENTER_DRIFT_SPEED = 6U; // speed of movement of the floating star emergence center
    const int WIDTH = MATRIX_WIDTH;
    const int HEIGHT = MATRIX_HEIGHT;

    // константы размера матрицы вычисляется только здесь и не меняется в эффектах
    const uint8_t CENTER_X_MINOR = (WIDTH / 2) - ((WIDTH - 1) & 0x01); // центр матрицы по ИКСУ, сдвинутый в меньшую
                                                                       // сторону, если ширина чётная
    const uint8_t CENTER_Y_MINOR = (HEIGHT / 2) - ((HEIGHT - 1) & 0x01); // центр матрицы по ИГРЕКУ, сдвинутый в меньшую
                                                                         // сторону, если высота чётная
    const uint8_t CENTER_X_MAJOR = WIDTH / 2 + (WIDTH % 2); // центр матрицы по ИКСУ, сдвинутый в большую
                                                            // сторону, если ширина чётная
    const uint8_t CENTER_Y_MAJOR = HEIGHT / 2 + (HEIGHT % 2); // центр матрицы по ИГРЕКУ, сдвинутый в
                                                              // большую сторону, если высота чётная

    const int spirocenterX = CENTER_X_MINOR;
    const int spirocenterY = CENTER_Y_MINOR;

  public:
    PatternSMStarDeep() : LEDStripEffect(EFFECT_MATRIX_SMSTARDEEP, "Star Deep")
    {
    }

    PatternSMStarDeep(const JsonObjectConst &jsonObject) : LEDStripEffect(jsonObject)
    {
    }

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

        // pointy = 7; теперь количество углов у каждой звезды своё
        bballsNUM = (WIDTH + 6U) / 2U; //(modes[currentMode].Scale - 1U) / 99.0 *
                                       //(bballsMaxNUM - 1U) + 1U;
        if (bballsNUM > bballsMaxNUM)
            bballsNUM = bballsMaxNUM;
        for (uint8_t num = 0; num < bballsNUM; num++)
        {
            bballsX[num] = random8(3, 9); // pointy = random8(3, 9); // количество углов в звезде
            bballsPos[num] = counter + (num << 3) + 1U; // random8(50);//modes[currentMode].Scale;//random8(50,
                                                        // 99); // задержка следующего пуска звезды
            bballsCOLOR[num] = random8();
        }
    }

    // Don't use the system g()->DrawLine because this code draws into unsafe
    // places, which clobbers memory and brings down the app. Use our clamped
    // versions instead.

    // функция отрисовки точки по координатам X Y
    void drawPixelXY(uint8_t x, uint8_t y, CRGB color)
    {
        if (!g()->isValidPixel(x, HEIGHT - 1 - y))
            return;
        // Mesmerizer flips the Y axis here.
        uint32_t thisPixel = XY(x, HEIGHT - 1 - y);
        g()->leds[thisPixel] = color;
    }

    // Дополнительная функция построения линий
    void DrawLine(int x1, int y1, int x2, int y2, CRGB color)
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
                drawPixelXY(x, y, color);
            else
                drawPixelXY(y, x, color);
            err -= (uint8_t)dy;
            if (err < 0)
            {
                y += ystep;
                err += dx;
            }
        }
    }

    void Drawstar(int16_t xlocl, int16_t ylocl, int16_t biggy, int16_t little, int16_t points, int16_t dangle,
                  uint8_t koler) // random multipoint star
    {
        auto radius2 = 255 / points;
        for (int i = 0; i < points; i++)
        {
            // две строчки выше - рисуют звезду просто по оттенку, а две строчки ниже
            // - берут цвет из текущей палитры
            DrawLine(xlocl + ((little * (sin8(i * radius2 + radius2 / 2 - dangle) - 128.0)) / 128),
                     ylocl + ((little * (cos8(i * radius2 + radius2 / 2 - dangle) - 128.0)) / 128),
                     xlocl + ((biggy * (sin8(i * radius2 - dangle) - 128.0)) / 128),
                     ylocl + ((biggy * (cos8(i * radius2 - dangle) - 128.0)) / 128),
                     g()->IsPalettePaused() ? g()->ColorFromCurrentPalette(koler) : ColorFromPalette(*curPalette, koler));
            DrawLine(xlocl + ((little * (sin8(i * radius2 - radius2 / 2 - dangle) - 128.0)) / 128),
                     ylocl + ((little * (cos8(i * radius2 - radius2 / 2 - dangle) - 128.0)) / 128),
                     xlocl + ((biggy * (sin8(i * radius2 - dangle) - 128.0)) / 128),
                     ylocl + ((biggy * (cos8(i * radius2 - dangle) - 128.0)) / 128),
                     g()->IsPalettePaused() ? g()->ColorFromCurrentPalette(koler) :ColorFromPalette(*curPalette, koler));
        }
    }

    void Draw() override
    {
        g()->DimAll(175U);

        // hue++;//increment the color basis был общий оттенок на весь эффект.
        // теперь у каждой звезды свой h = hue;  //set h to the color basis
        counter++;

        // This block of 6 'if's could probably be replaced by a std::clamp...if I
        // could read this junk. This keeps the absolute unit of our drift (xy) and
        // the angle of our drift (sin, cos) in check.
        if (driftx > (WIDTH - spirocenterX / 2U)) // change directin of drift if you get
                                                  // near the right 1/4 of the screen
            cangle = 0 - fabs(cangle);
        if (driftx < spirocenterX / 2U) // change directin of drift if you get near
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

        // The follow comment is encrypted. I think it says "don't let the center
        // drift off the screen". I'm not sure why this is necessary, but I'm
        // leaving it in (davepl).

        // по идее, не нужно равнять диапазоны плавающего центра. за них и так вылет
        // невозможен driftx = constrain(driftx, spirocenterX - spirocenterX / 3,
        // spirocenterX + spirocenterX / 3);//constrain the center, probably never
        // gets evoked any more but was useful at one time to keep the graphics on
        // the screen.... drifty = constrain(drifty, spirocenterY - spirocenterY /
        // 3, spirocenterY + spirocenterY / 3);

        for (uint8_t num = 0; num < bballsNUM; num++)
        {
            if (counter >= bballsPos[num]) //(counter >= ringdelay)
            {
                float expansionFactor = 1.0f; // Add this member variable to your class
                int starSize = (counter - bballsPos[num]) * expansionFactor;

                if (starSize <= WIDTH + 5U)
                { //(counter - ringdelay <= WIDTH + 5) {
                    // drawstar(driftx  , drifty, 2 * (counter - ringdelay), (counter -
                    // ringdelay), pointy, blender + h, h * 2 + 85);
                    Drawstar(driftx, drifty, 2 * starSize, starSize, bballsX[num], STAR_BLENDER + bballsCOLOR[num],
                             bballsCOLOR[num] * 2); //, h * 2 + 85);// что, бл, за 85?!
                    bballsCOLOR[num]++;
                }
                else
                {
                    // bballsX[num] = random8(3, 9);//pointy = random8(3, 9); //
                    // количество углов в звезде
                    bballsPos[num] =
                        counter + (bballsNUM << 2) + 1U; // random8(50, 99);//modes[currentMode].Scale;//random8(50,
                                                         // 99); // задержка следующего пуска звезды  // Means: "Set
                                                         // the next star launch delay"
                }
            }
        }
    }
};
