#pragma once

#include "effectmanager.h"

// Derived from https://editor.soulmatelights.com/gallery/729-google-nexus
// Clearly, this is not a Google product; it's rendition of the boot and life
// screen effect used on the first Google Nexus device, the Nexus S.

class PatternSMGoogleNexus : public EffectWithId<PatternSMGoogleNexus>
{
  private:
#define GOOGLE_NEXUS (MATRIX_WIDTH)

    uint8_t speed = 180; // 0-255
    uint8_t scale = 60;  // Number of drops. (ed.: relatively speaking.... 0-255)
                         // probably should be a function of WIDTH * HEIGHT.

    // These are clearly wrong...Just treat it like a square for now.
    struct Dot {
        float posX;
        float posY;
        int8_t direct;
        CRGB color;
        float accel;
    };
    Dot dots[MATRIX_WIDTH];

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
        if (!g()->isValidPixel(x, y))
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

    void resetDot(int idx)
    {
        randomSeed(micros());
        dots[idx].direct = random8(0, 4);                                            // set direction
        dots[idx].color = ColorFromPalette(RainbowColors_p, random(0, 9) * 31, 255); // color
        dots[idx].accel = (float)random(5, 10) / 70; // make particles slightly different acceleration
        switch (dots[idx].direct)
        {
        case 0:                                      // вверх
            dots[idx].posX = random8(0, MATRIX_WIDTH); // Scatter drops across the width
            dots[idx].posY = 0;                        // and height
            break;
        case 1:                                      //  вниз
            dots[idx].posX = random8(0, MATRIX_WIDTH); // Scatter drops across the width
            dots[idx].posY = MATRIX_HEIGHT - 1;        // and height
            break;
        case 2:                                       // вправо
            dots[idx].posX = 0;                         // Scatter drops across the width
            dots[idx].posY = random8(0, MATRIX_HEIGHT); // and height
            break;
        case 3:                                       // влево
            dots[idx].posX = MATRIX_WIDTH - 1;          // Scatter drops across the width
            dots[idx].posY = random8(0, MATRIX_HEIGHT); // and height
            break;
        default:
            break;
        }
    }

  public:
    PatternSMGoogleNexus() : EffectWithId<PatternSMGoogleNexus>("Google Nexus")
    {
    }

    PatternSMGoogleNexus(const JsonObjectConst &jsonObject) : EffectWithId<PatternSMGoogleNexus>(jsonObject)
    {
    }

    void Start() override
    {
        g()->Clear();
        for (int i = 0; i < GOOGLE_NEXUS; i++)
        {
            dots[i].direct = random(0, 4);           // Set direction
            dots[i].posX = random(0, MATRIX_WIDTH);  // Scatter particles across width
            dots[i].posY = random(0, MATRIX_HEIGHT); // and height
            dots[i].color = ColorFromPalette(RainbowColors_p, random8(0, 9) * 31,
                                           255);     // dot color
            dots[i].accel = (float)random(5, 11) / 70; // make particles each slightly different speed
        }
    }

    void Draw() override
    {
        float speedfactor = fmap(speed, 1, 255, 0.1, .33);
        fadeAllChannelsToBlackBy(::map(speed, 1, 255, 11, 33));

        for (int i = 0; i < ::map(scale, 1, 255, 4, GOOGLE_NEXUS); i++)
        {
            switch (dots[i].direct)
            {
            case 0: // up
                dots[i].posY += (speedfactor + dots[i].accel);
                break;
            case 1: //  down
                dots[i].posY -= (speedfactor + dots[i].accel);
                break;
            case 2: // right
                dots[i].posX += (speedfactor + dots[i].accel);
                break;
            case 3: // left
                dots[i].posX -= (speedfactor + dots[i].accel);
                break;
            default:
                break;
            }

            // // Make it seamless in Y. And move the blob to the beginning of the
            // track
            if (dots[i].posY < 0)
            {
                dots[i].posY = (float)MATRIX_HEIGHT - 1.;
                resetDot(i);
            }

            if (dots[i].posY > (MATRIX_HEIGHT - 1))
            {
                dots[i].posY = 0;
                resetDot(i);
            }

            // / Make X seamless.
            if (dots[i].posX < 0)
            {
                dots[i].posX = (MATRIX_WIDTH - 1);
                resetDot(i);
            }
            if (dots[i].posX > (MATRIX_WIDTH - 1))
            {
                dots[i].posX = 0;
                resetDot(i);
            }

            switch (dots[i].direct)
            {
            case 0: // up
            case 1: // down
                drawPixelXYF_Y(dots[i].posX, dots[i].posY, dots[i].color);
                break;
            case 2: // right
            case 3: // left
                drawPixelXYF_X(dots[i].posX, dots[i].posY, dots[i].color);
                break;
            default:
                break;
            }
        }
    }
};
