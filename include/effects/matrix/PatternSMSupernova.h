#pragma once

#include "effectmanager.h"

// Inspired by https://editor.soulmatelights.com/gallery/1923-supernova

// A more cache-friendly version of the 8 independent arrays that were
// used. This allows better locality per member, especially since we
// access them sequentially.
// Nothing special here; just default-zero initialized struct.
class Object {
  public:
    void Objects() {
        PosX = 0.0f;
        PosY = 0.0f;
        State = 0.0f;
        SpeedX = 0.0f;
        SpeedY = 0.0f;
        Hue = 0;
        IsShift = false;
    }

    float PosX;
    float PosY;
    float SpeedX;
    float SpeedY;
    uint8_t Hue;
    uint8_t State;
    bool IsShift;
};

class PatternSMSupernova : public LEDStripEffect
{
public:

    PatternSMSupernova() : LEDStripEffect(EFFECT_MATRIX_SMSUPERNOVA, "Supernova"), hue(0), hue2(0), step(0), deltaValue(0), enlargedObjectNUM(0)
    {
    }

    PatternSMSupernova(const JsonObjectConst &jsonObject) : LEDStripEffect(jsonObject)
    {
    }

    virtual size_t DesiredFramesPerSecond() const override
    {
        return 120;
    }

    void Start() override
    {
        g()->Clear();
        enlargedObjectNUM = 200;
        if (enlargedObjectNUM > MAX_COUNT)
            enlargedObjectNUM = 200U;
        deltaValue = enlargedObjectNUM /
            (sqrt3(CENTER_X_MAJOR * CENTER_X_MAJOR +
                   CENTER_Y_MAJOR * CENTER_Y_MAJOR) * 4U) + 1U;
    }

    void Draw() override {
        step = -1;
        g()->DimAll(200);

        for (auto& object : Objects) {
            if (!object.IsShift && step) {
                starfield2Emit(object);
                step -= 1;
            }

            if (object.IsShift) {
                particlesUpdate2(object);
                // pU2() may have updated the object beyond bounds and
                // cleared IsShift. Retest it and ignore if so.
                // Alternate design could use pU2()'s return value for this.
                if (object.IsShift == false) continue;;

                CRGB baseRGB = ColorFromPalette(HeatColors_p, object.Hue,
                                                255, LINEARBLEND);
                baseRGB.nscale8(object.State);
                drawPixelXYF(object.PosX, object.PosY, baseRGB);
            }
        }
    }

private:
    const uint8_t CENTER_X_MINOR = (MATRIX_WIDTH / 2) - ((MATRIX_WIDTH - 1) & 0x01);
    const uint8_t CENTER_Y_MINOR = (MATRIX_HEIGHT / 2) - ((MATRIX_HEIGHT - 1) & 0x01);
    const uint8_t CENTER_X_MAJOR = MATRIX_WIDTH / 2 + (MATRIX_WIDTH % 2);
    const uint8_t CENTER_Y_MAJOR = MATRIX_HEIGHT / 2 + (MATRIX_HEIGHT % 2);
    uint8_t hue, hue2, step, deltaValue, enlargedObjectNUM;

    static constexpr int MAX_COUNT = 254;
    std::array<Object, MAX_COUNT> Objects;

    // Fast Babylonian Approximate square root.
    // It's called one time in the constructor, so no need for
    // crazy optimization here.
    float sqrt3(const float x)
    {
        union
        {
            int i;
            float x;
        } u;
        u.x = x;
        u.i = (1 << 29) + (u.i >> 1) - (1 << 22);
        return u.x;
    }

    void inline particlesUpdate2(Object& object)
    {
        // Intentionally do a narrowing conversion here.
        const int x = object.PosX += object.SpeedX;
        const int y = object.PosY += object.SpeedY;

        if (object.State == 0 || x < 0 || x >= MATRIX_WIDTH ||
                                 y < 0 || y >= MATRIX_HEIGHT)
            object.IsShift = false;
    }

    void inline starfield2Emit(Object& object)
    {
        if (hue++ & 0x01)
            hue2 += 1;
        object.PosX = MATRIX_WIDTH * 0.5;
        object.PosY = MATRIX_HEIGHT * 0.5;
        object.SpeedX = (((float)random8() - 127.) / 512.);
        object.SpeedY = sqrt(0.0626 - object.SpeedX * object.SpeedX);
        if (random8(2U)) {
            object.SpeedY = -object.SpeedY;
        }
        object.State = random8(1, 250);
        object.Hue = hue2;
        object.IsShift = true;
    }

    static inline uint8_t WU_WEIGHT(uint8_t a, uint8_t b)
    {
        return (uint8_t)(((a) * (b) + (a) + (b)) >> 8);
    }

    void drawPixelXYF(float x, float y, CRGB color)
    {
        const uint8_t xx = (x - (int)x) * 255, yy = (y - (int)y) * 255, ix = 255 - xx, iy = 255 - yy;
        const uint8_t wu[4] = {WU_WEIGHT(ix, iy), WU_WEIGHT(xx, iy), WU_WEIGHT(ix, yy), WU_WEIGHT(xx, yy)};
        for (uint8_t i = 0; i < 4; i++) {
            // Until now, x and y are totally in bound, but we're about
            // to make our coords into 4-pixel wide splotches.
            // If we knew x and y would be ^2, we could do cute bit math,
            // but we just clamp instead.
            const int xn = std::clamp((int)x + (i & 1), 0, MATRIX_WIDTH - 1);
            const int yn = std::clamp((int)y + ((i >> 1) & 1), 0, MATRIX_HEIGHT - 1);

            CRGB clr = g()->leds[XY(xn, MATRIX_HEIGHT - yn)];
            clr.r = qadd8(clr.r, (color.r * wu[i]) >> 8);
            clr.g = qadd8(clr.g, (color.g * wu[i]) >> 8);
            clr.b = qadd8(clr.b, (color.b * wu[i]) >> 8);
            g()->leds[XY(xn, yn)] = clr;
        }
    }
};
