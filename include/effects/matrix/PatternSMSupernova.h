#pragma once

#include "effectmanager.h"

// Inspired by https://editor.soulmatelights.com/gallery/1923-supernova

class PatternSMSupernova : public LEDStripEffect 
{
    void Init()
    {
        memset(enlargedObjectTime, 0, sizeof(enlargedObjectTime));
        memset(PosX, 0, sizeof(PosX));
        memset(PosY, 0, sizeof(PosY));
        memset(Hue, 0, sizeof(Hue));
        memset(State, 0, sizeof(State));
        memset(SpeedX, 0, sizeof(SpeedX));
        memset(SpeedY, 0, sizeof(SpeedY));
        memset(IsShift, 0, sizeof(IsShift));
    }

public:

    PatternSMSupernova() : LEDStripEffect(EFFECT_MATRIX_SMSUPERNOVA, "Supernova"), hue(0), hue2(0), step(0), deltaValue(0), enlargedObjectNUM(0) 
    {
    }

    PatternSMSupernova(const JsonObjectConst &jsonObject) : LEDStripEffect(jsonObject) 
    {
    }

    virtual size_t DesiredFramesPerSecond() const override 
    {
        return 40;
    }

    void Start() override 
    {
        Init();
        g()->Clear();
        enlargedObjectNUM = 200;
        if (enlargedObjectNUM > MAX_COUNT)
            enlargedObjectNUM = 200U;
        deltaValue = enlargedObjectNUM / (sqrt3(CENTER_X_MAJOR * CENTER_X_MAJOR + CENTER_Y_MAJOR * CENTER_Y_MAJOR) * 4U) + 1U;
        for (unsigned i = 0; i < enlargedObjectNUM; i++)
            IsShift[i] = false;
    }

    void Draw() override {
        step = -1;
        g()->DimAll(200);
        for (unsigned i = 0; i < enlargedObjectNUM; i++) {
            if (!IsShift[i] && step) {
                starfield2Emit(i);
                step -= 1;
            }
            if (IsShift[i]) {
                particlesUpdate2(i);
                CRGB baseRGB = ColorFromPalette(HeatColors_p, Hue[i], 255, LINEARBLEND);
                baseRGB.nscale8(State[i]);
                drawPixelXYF(PosX[i], PosY[i], baseRGB);
            }
        }
    }

private:
    const uint8_t CENTER_X_MINOR = (MATRIX_WIDTH / 2) - ((MATRIX_WIDTH - 1) & 0x01);
    const uint8_t CENTER_Y_MINOR = (MATRIX_HEIGHT / 2) - ((MATRIX_HEIGHT - 1) & 0x01);
    const uint8_t CENTER_X_MAJOR = MATRIX_WIDTH / 2 + (MATRIX_WIDTH % 2);
    const uint8_t CENTER_Y_MAJOR = MATRIX_HEIGHT / 2 + (MATRIX_HEIGHT % 2);

    static constexpr int enlargedOBJECT_MAX_COUNT = (MATRIX_WIDTH * 2);
    static constexpr int MAX_COUNT = 254;

    uint8_t hue, hue2, step, deltaValue, enlargedObjectNUM;
    long enlargedObjectTime[enlargedOBJECT_MAX_COUNT];
    float PosX[MAX_COUNT];
    float PosY[MAX_COUNT];
    uint8_t Hue[MAX_COUNT];
    uint8_t State[MAX_COUNT];
    float SpeedX[MAX_COUNT];
    float SpeedY[MAX_COUNT];
    bool IsShift[MAX_COUNT];

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

    void particlesUpdate2(uint8_t i) 
    {
        PosX[i] += SpeedX[i];
        PosY[i] += SpeedY[i];
        if (State[i] == 0 || PosX[i] < 0 || PosX[i] >= MATRIX_WIDTH || PosY[i] < 0 || PosY[i] >= MATRIX_HEIGHT)
            IsShift[i] = false;
    }

    void starfield2Emit(uint8_t i)
    {
        if (hue++ & 0x01)
            hue2 += 1;
        PosX[i] = MATRIX_WIDTH * 0.5;
        PosY[i] = MATRIX_HEIGHT * 0.5;
        SpeedX[i] = (((float)random8() - 127.) / 512.);
        SpeedY[i] = sqrt(0.0626 - SpeedX[i] * SpeedX[i]);
        if (random8(2U)) {
            SpeedY[i] = -SpeedY[i];
        }
        State[i] = random8(1, 250);
        Hue[i] = hue2;
        IsShift[i] = true;
    }

    static inline uint8_t WU_WEIGHT(uint8_t a, uint8_t b) 
    {
        return (uint8_t)(((a) * (b) + (a) + (b)) >> 8);
    }

    void drawPixelXYF(float x, float y, CRGB color) 
    {
        uint8_t xx = (x - (int)x) * 255, yy = (y - (int)y) * 255, ix = 255 - xx, iy = 255 - yy;
        uint8_t wu[4] = {WU_WEIGHT(ix, iy), WU_WEIGHT(xx, iy), WU_WEIGHT(ix, yy), WU_WEIGHT(xx, yy)};
        for (uint8_t i = 0; i < 4; i++) {
            int16_t xn = x + (i & 1), yn = y + ((i >> 1) & 1);

            if (!g()->isValidPixel(xn, yn))
                continue;

            CRGB clr = g()->leds[XY(xn, MATRIX_HEIGHT - 1 - yn)];
            clr.r = qadd8(clr.r, (color.r * wu[i]) >> 8);
            clr.g = qadd8(clr.g, (color.g * wu[i]) >> 8);
            clr.b = qadd8(clr.b, (color.b * wu[i]) >> 8);
            g()->leds[XY(xn, yn)] = clr;
        }
    }
};
