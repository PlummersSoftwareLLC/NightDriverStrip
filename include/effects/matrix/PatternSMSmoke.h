#pragma once

#include "effectmanager.h"

// Derived from https://editor.soulmatelights.com/gallery/1116-smoke

class PatternSMSmoke : public LEDStripEffect
{
    virtual size_t DesiredFramesPerSecond() const override
    {
        return 20;
    }

  private:
    uint8_t Scale = 50; // 1-100. SettingA

    static constexpr int WIDTH = MATRIX_WIDTH;
    static constexpr int HEIGHT = MATRIX_HEIGHT;
    // matrix size constants are calculated only here and do not change in effects
    const uint8_t CENTER_X_MINOR =
        (MATRIX_WIDTH / 2) - ((MATRIX_WIDTH - 1) & 0x01); // the center of the matrix according to ICSU, shifted to the
                                                          // smaller side, if the width is even
    const uint8_t CENTER_Y_MINOR =
        (MATRIX_HEIGHT / 2) -
        ((MATRIX_HEIGHT - 1) & 0x01); // center of the YGREK matrix, shifted down if the height is even
    const uint8_t CENTER_X_MAJOR =
        MATRIX_WIDTH / 2 + (MATRIX_WIDTH % 2); // the center of the matrix according to IKSU,
                                               // shifted to a larger side, if the width is even
    const uint8_t CENTER_Y_MAJOR =
        MATRIX_HEIGHT / 2 + (MATRIX_HEIGHT % 2); // center of the YGREK matrix, shifted up if the height is even

    static constexpr int NUM_LAYERSMAX = 3;
    uint8_t hue, hue2; // постепенный сдвиг оттенка или какой-нибудь другой
                       // цикличный счётчик
    uint8_t deltaHue, deltaHue2; // ещё пара таких же, когда нужно много
    uint8_t noise3d[NUM_LAYERSMAX][WIDTH][HEIGHT]; // двухслойная маска или хранилище свойств в размер
                                                   // всей матрицы
    uint32_t noise32_x[NUM_LAYERSMAX];
    uint32_t noise32_y[NUM_LAYERSMAX];
    uint32_t noise32_z[NUM_LAYERSMAX];
    uint32_t scale32_x[NUM_LAYERSMAX];
    uint32_t scale32_y[NUM_LAYERSMAX];
    uint8_t noisesmooth;
    int8_t zD;
    int8_t zF;

    void FillNoise(uint8_t layer)
    {
        for (uint8_t i = 0; i < WIDTH; i++)
        {
            int32_t ioffset = scale32_x[layer] * (i - CENTER_X_MINOR);
            for (uint8_t j = 0; j < HEIGHT; j++)
            {
                int32_t joffset = scale32_y[layer] * (j - CENTER_Y_MINOR);
                int8_t data = inoise16(noise32_x[layer] + ioffset, noise32_y[layer] + joffset, noise32_z[layer]) >> 8;
                int8_t olddata = noise3d[layer][i][j];
                int8_t newdata = scale8(olddata, noisesmooth) + scale8(data, 255 - noisesmooth);
                data = newdata;
                noise3d[layer][i][j] = data;
            }
        }
    }

    void MoveFractionalNoiseX(int8_t amplitude = 1, float shift = 0)
    {
        for (uint8_t y = 0; y < HEIGHT; y++)
        {
            int16_t amount = ((int16_t)noise3d[0][0][y] - 128) * 2 * amplitude + shift * 256;
            int8_t delta = abs(amount) >> 8;
            int8_t fraction = abs(amount) & 255;
            for (uint8_t x = 0; x < WIDTH; x++)
            {
                if (amount < 0)
                {
                    zD = x - delta;
                    zF = zD - 1;
                }
                else
                {
                    zD = x + delta;
                    zF = zD + 1;
                }
                CRGB PixelA = CRGB::Black;
                if ((zD >= 0) && (zD < WIDTH))
                    PixelA = g()->leds[XY(zD, y)];
                CRGB PixelB = CRGB::Black;
                if ((zF >= 0) && (zF < WIDTH))
                    PixelB = g()->leds[XY(zF, y)];
                g()->leds[XY(x, y)] =
                    (PixelA.nscale8(ease8InOutApprox(255 - fraction))) +
                    (PixelB.nscale8(ease8InOutApprox(fraction))); // lerp8by8(PixelA, PixelB, fraction );
            }
        }
        // memcpy(g()->leds, ledsbuff, sizeof(CRGB) * NUM_LEDS);
    }

    void MoveFractionalNoiseY(int8_t amplitude = 1, float shift = 0)
    {
        for (uint8_t x = 0; x < WIDTH; x++)
        {
            int16_t amount = ((int16_t)noise3d[0][x][0] - 128) * 2 * amplitude + shift * 256;
            int8_t delta = abs(amount) >> 8;
            int8_t fraction = abs(amount) & 255;
            for (uint8_t y = 0; y < HEIGHT; y++)
            {
                if (amount < 0)
                {
                    zD = y - delta;
                    zF = zD - 1;
                }
                else
                {
                    zD = y + delta;
                    zF = zD + 1;
                }
                CRGB PixelA = CRGB::Black;
                if ((zD >= 0) && (zD < HEIGHT))
                    PixelA = g()->leds[XY(x, zD)];
                CRGB PixelB = CRGB::Black;
                if ((zF >= 0) && (zF < HEIGHT))
                    PixelB = g()->leds[XY(x, zF)];
                g()->leds[XY(x, y)] =
                    (PixelA.nscale8(ease8InOutApprox(255 - fraction))) + (PixelB.nscale8(ease8InOutApprox(fraction)));
            }
        }
        // memcpy(g()->leds, ledsbuff, sizeof(CRGB) * NUM_LEDS);
    }

  public:
    PatternSMSmoke() : LEDStripEffect(EFFECT_MATRIX_SMSMOKE, "Smoke")
    {
    }

    PatternSMSmoke(const JsonObjectConst &jsonObject) : LEDStripEffect(jsonObject)
    {
    }

    void Start() override
    {
        g()->Clear();
    }

    void Draw() override
    {
        deltaHue++;
        CRGB color; //, color2;

        if (hue2 == Scale)
        {
            hue2 = 0U;
            hue = random8();
        }
        if (deltaHue & 0x01) //((deltaHue >> 2U) == 0U) // какой-то умножитель охота, подключить к задержке смены цвета,
                             //но хз какой...
            hue2++;

        hsv2rgb_spectrum(CHSV(hue, 255U, 127U), color);

        // deltaHue2--;
        if (random8(WIDTH) != 0U) // встречная спираль движется не всегда синхронно основной
            deltaHue2--;

        for (uint8_t y = 0; y < HEIGHT; y++)
        {
            g()->leds[XY((deltaHue + y + 1U) % WIDTH, HEIGHT - 1U - y)] += color;
            g()->leds[XY((deltaHue + y) % WIDTH, HEIGHT - 1U - y)] += color; // color2
            g()->leds[XY((deltaHue2 + y) % WIDTH, y)] += color;
            g()->leds[XY((deltaHue2 + y + 1U) % WIDTH, y)] += color; // color2
        }

        // Noise

        // Movement speed in the noise array
        // uint32_t mult = 500U * ((Scale - 1U) % 10U);
        noise32_x[0] += 1000; // 1000;
        noise32_y[0] += 1000; // 1000;
        noise32_z[0] += 1000; // 1000;

        // Scale determines the size of the noise elements.
        // хрен знает что
        // mult = 1000U * ((Speed - 1U) % 10U);
        scale32_x[0] = 4000;
        scale32_y[0] = 4000;

        FillNoise(0);
        MoveFractionalNoiseX(3); // 4
        MoveFractionalNoiseY(3); // 4
        g()->BlurFrame(20);      // без размытия как-то пиксельно, наверное...
    }
};
