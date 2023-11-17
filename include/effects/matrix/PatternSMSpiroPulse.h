#pragma once

#include "effectmanager.h"

// Derived from https://editor.soulmatelights.com/gallery/2164-spiro
// A spirograph that evolves the number of spines, becomign a yen-yang.
//
// This is one of relatively few that would look better at a higher refresh.

class PatternSMSpiroPulse : public LEDStripEffect
{
  private:
    static constexpr int CenterX = ((MATRIX_WIDTH / 2) - 0.5);
    static constexpr int CenterY = ((MATRIX_HEIGHT / 2) - 0.5);
    bool incenter {false};
    byte AM {1};
    float Angle {0.f};
    bool change = true;

    [[nodiscard]] CRGB getPixColorXY(uint8_t x, uint8_t y) const
    {
        return g()->leds[XY(x, MATRIX_HEIGHT - 1 - y)];
    }

    void drawPixelXY(uint8_t x, int8_t y, CRGB color)
    {
        if (g()->isValidPixel(x, MATRIX_HEIGHT - 1 - y) == false)
            return;
        // Mesmerizer flips the Y axis here.
        uint32_t thisPixel = XY(x, MATRIX_HEIGHT - 1 - y);
        g()->leds[thisPixel] = color;
    }

    static inline uint8_t WU_WEIGHT(uint8_t a, uint8_t b)
    {
        return (uint8_t)(((a) * (b) + (a) + (b)) >> 8);
    }

    void drawPixelXYF(float x, float y, CRGB color) //, uint8_t darklevel = 0U)
    {
        //  if (x<0 || y<0) return; //не похоже, чтобы отрицательные значения хоть
        //  как-нибудь учитывались тут // зато с этой строчкой пропадает нижний ряд
        // extract the fractional parts and derive their inverses
        uint8_t xx = (x - (int)x) * 255, yy = (y - (int)y) * 255, ix = 255 - xx, iy = 255 - yy;
// calculate the intensities for each affected pixel
#define WU_WEIGHT(a, b) ((uint8_t)(((a) * (b) + (a) + (b)) >> 8))
        uint8_t wu[4] = {WU_WEIGHT(ix, iy), WU_WEIGHT(xx, iy), WU_WEIGHT(ix, yy), WU_WEIGHT(xx, yy)};
#undef WU_WEIGHT
        // multiply the intensities by the colour, and saturating-add them to the
        // pixels
        for (uint8_t i = 0; i < 4; i++)
        {
            int16_t xn = x + (i & 1), yn = y + ((i >> 1) & 1);
            CRGB clr = getPixColorXY(xn, yn);
            clr.r = qadd8(clr.r, (color.r * wu[i]) >> 8);
            clr.g = qadd8(clr.g, (color.g * wu[i]) >> 8);
            clr.b = qadd8(clr.b, (color.b * wu[i]) >> 8);
            drawPixelXY(xn, yn, clr);
        }
    }

  public:
    PatternSMSpiroPulse() : LEDStripEffect(EFFECT_MATRIX_SMSPIRO_PULSE, "Spiro")
    {
    }

    PatternSMSpiroPulse(const JsonObjectConst &jsonObject) : LEDStripEffect(jsonObject)
    {
    }

    void Start() override
    {
        g()->Clear();
    }

    void Draw() override
    {
        // fadeToBlackBy(leds, NUM_LEDS, 8);
        fadeAllChannelsToBlackBy(8);

        float t = (float)millis() / 500.0f;
        float CalcRad = (sin(t / 2) + 1);
        if (CalcRad <= 0.001)
        {
            if (!incenter)
            {
                AM += ((change * 2) - 1);
                Angle = 6.28318531 / AM;
                if (AM <= 1)
                    change = true;
                else if (AM >= (MATRIX_WIDTH + MATRIX_HEIGHT) / 2)
                    change = false;
            }
            incenter = true;
        }
        else
            incenter = false;
        // Originally /3. Let's go all the way to the edges.
        float radX = CalcRad * CenterY / 2;
        float radY = CalcRad * CenterY / 2;
        for (byte i = 0; i < AM; i++)
        {
            drawPixelXYF((CenterX + sin(t + (Angle * i)) * radX), (CenterY + cos(t + (Angle * i)) * radY),
                         ColorFromPalette(HeatColors_p, t * 10 + ((256 / AM) * i)));
        }
    }
};
