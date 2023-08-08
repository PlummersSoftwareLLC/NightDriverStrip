#pragma once

#include "effectmanager.h"

// Derived from https://editor.soulmatelights.com/gallery/729-google-nexus
// Clearly, this is not a Google product; it's rendition of the boot and life
// screen effect used on the first Google Nexus device, the Nexus S.

class PatternSMGoogleNexus : public LEDStripEffect
{
  private:
#define GOOGLE_NEXUS (MATRIX_WIDTH)

    uint8_t speed = 180; // 0-255
    uint8_t scale = 60;  // Number of drops. (ed.: relatively speaking.... 0-255)
                         // probably should be a function of WIDTH * HEIGHT.

    // These are clearly wrong...Just treat it like a square for now.
    float dotPosX[MATRIX_WIDTH];
    float dotPosY[MATRIX_WIDTH];
    int8_t dotDirect[MATRIX_WIDTH]; // направление точки
    CRGB dotColor[MATRIX_WIDTH];    // цвет точки
    float dotAccel[MATRIX_WIDTH];   // персональное ускорение каждой точки

    double fmap(const double x, const double in_min, const double in_max, const double out_min, const double out_max)
    {
        return (out_max - out_min) * (x - in_min) / (in_max - in_min) + out_min;
    }

    void drawPixelXYF_X(float x, uint16_t y, const CRGB &color)
    {
        if (x < 0 || y < 0 || x > ((float)MATRIX_WIDTH - 1) || y > ((float)MATRIX_HEIGHT - 1))
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
            if (xn < 0 || xn > MATRIX_WIDTH - 1)
                continue; // WHY?
            CRGB clr = g()->leds[XY(xn, y)];
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
            g()->leds[XY(xn, y)] = clr;
        }
    }

    void drawPixelXYF_Y(uint16_t x, float y, const CRGB &color)
    {
        if (x < 0 || y < 0 || x > ((float)MATRIX_WIDTH - 1) || y > ((float)MATRIX_HEIGHT - 1))
            return;

        // extract the fractional parts and derive their inverses
        uint8_t yy = (y - (int)y) * 255, iy = 255 - yy;
        // calculate the intensities for each affected pixel
        uint8_t wu[2] = {iy, yy};
        // multiply the intensities by the colour, and saturating-add them to the
        // pixels
        for (int8_t i = 1; i >= 0; i--)
        {
            int16_t yn = y + (i & 1);
            if (yn < 0 || yn >= MATRIX_HEIGHT - 1)
                continue; // WHY?
            CRGB clr = g()->leds[XY(x, yn)];
            if (yn > 0 && yn < (int)MATRIX_HEIGHT - 1)
            {
                clr.r = qadd8(clr.r, (color.r * wu[i]) >> 8);
                clr.g = qadd8(clr.g, (color.g * wu[i]) >> 8);
                clr.b = qadd8(clr.b, (color.b * wu[i]) >> 8);
            }
            else if (yn == 0 || yn == (int)MATRIX_HEIGHT - 1)
            {
                clr.r = qadd8(clr.r, (color.r * 85) >> 8);
                clr.g = qadd8(clr.g, (color.g * 85) >> 8);
                clr.b = qadd8(clr.b, (color.b * 85) >> 8);
            }
            g()->leds[XY(x, yn)] = clr;
        }
    }

    void resetDot(uint8_t idx)
    {
        randomSeed(micros());
        dotDirect[idx] = random8(0, 4);                                            // set direction
        dotColor[idx] = ColorFromPalette(RainbowColors_p, random(0, 9) * 31, 255); // color
        dotAccel[idx] = (float)random(5, 10) / 70; // make particles slightly different acceleration
        switch (dotDirect[idx])
        {
        case 0:                                      // вверх
            dotPosX[idx] = random8(0, MATRIX_WIDTH); // Scatter drops across the width
            dotPosY[idx] = 0;                        // and height
            break;
        case 1:                                      //  вниз
            dotPosX[idx] = random8(0, MATRIX_WIDTH); // Scatter drops across the width
            dotPosY[idx] = MATRIX_HEIGHT - 1;        // and height
            break;
        case 2:                                       // вправо
            dotPosX[idx] = 0;                         // Scatter drops across the width
            dotPosY[idx] = random8(0, MATRIX_HEIGHT); // and height
            break;
        case 3:                                       // влево
            dotPosX[idx] = MATRIX_WIDTH - 1;          // Scatter drops across the width
            dotPosY[idx] = random8(0, MATRIX_HEIGHT); // and height
            break;
        default:
            break;
        }
    }

  public:
    PatternSMGoogleNexus() : LEDStripEffect(EFFECT_MATRIX_SMGOOGLE_NEXUS, "Google Nexus")
    {
    }

    PatternSMGoogleNexus(const JsonObjectConst &jsonObject) : LEDStripEffect(jsonObject)
    {
    }

    void Start() override
    {
        g()->Clear();
        for (byte i = 0; i < GOOGLE_NEXUS; i++)
        {
            dotDirect[i] = random(0, 4);           // Set direction
            dotPosX[i] = random(0, MATRIX_WIDTH);  // Scatter particles across width
            dotPosY[i] = random(0, MATRIX_HEIGHT); // and height
            dotColor[i] = ColorFromPalette(RainbowColors_p, random8(0, 9) * 31,
                                           255);     // dot color
            dotAccel[i] = (float)random(5, 11) / 70; // make particles each slightly different speed
        }
    }

    void Draw() override
    {
        float speedfactor = fmap(speed, 1, 255, 0.1, .33);
        fadeAllChannelsToBlackBy(map(speed, 1, 255, 11, 33));

        for (byte i = 0; i < map(scale, 1, 255, 4, GOOGLE_NEXUS); i++)
        {
            switch (dotDirect[i])
            {
            case 0: // up
                dotPosY[i] += (speedfactor + dotAccel[i]);
                break;
            case 1: //  down
                dotPosY[i] -= (speedfactor + dotAccel[i]);
                break;
            case 2: // right
                dotPosX[i] += (speedfactor + dotAccel[i]);
                break;
            case 3: // left
                dotPosX[i] -= (speedfactor + dotAccel[i]);
                break;
            default:
                break;
            }

            // // Make it seamless in Y. And move the blob to the beginning of the
            // track
            if (dotPosY[i] < 0)
            {
                dotPosY[i] = (float)MATRIX_HEIGHT - 1.;
                resetDot(i);
            }

            if (dotPosY[i] > (MATRIX_HEIGHT - 1))
            {
                dotPosY[i] = 0;
                resetDot(i);
            }

            // / Make X seamless.
            if (dotPosX[i] < 0)
            {
                dotPosX[i] = (MATRIX_WIDTH - 1);
                resetDot(i);
            }
            if (dotPosX[i] > (MATRIX_WIDTH - 1))
            {
                dotPosX[i] = 0;
                resetDot(i);
            }

            switch (dotDirect[i])
            {
            case 0: // up
            case 1: // down
                drawPixelXYF_Y(dotPosX[i], dotPosY[i], dotColor[i]);
                break;
            case 2: // right
            case 3: // left
                drawPixelXYF_X(dotPosX[i], dotPosY[i], dotColor[i]);
                break;
            default:
                break;
            }
        }
    }
};
