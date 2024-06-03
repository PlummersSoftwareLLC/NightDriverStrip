#pragma once

#include "systemcontainer.h"

// Inspired by https://editor.soulmatelights.com/gallery/1923-supernova


class PatternSMSupernova : public LEDStripEffect
{
public:

    PatternSMSupernova() : LEDStripEffect(EFFECT_MATRIX_SMSUPERNOVA, "Supernova"), hue(0), hue2(0), step(0)
    {
    }

    PatternSMSupernova(const JsonObjectConst &jsonDebrisItem) : LEDStripEffect(jsonDebrisItem)
    {
    }

    virtual size_t DesiredFramesPerSecond() const override
    {
        return 60;
    }

    void Start() override
    {
        std::for_each(_debris_items.begin(), _debris_items.end(), [](DebrisItem& debris_item) { debris_item.Clear(); });
        g()->Clear();
    }

    void Draw() override 
    {
        step = -1;
        g()->DimAll(200);

        for (auto& debris_item : _debris_items) {
            if (!debris_item._is_shift && step) {
                StarfieldEmit(debris_item);
                step -= 1;
            }

            if (debris_item._is_shift && ParticlesUpdate(debris_item)) {
                CRGB baseRGB = ColorFromPalette(g()->IsPalettePaused() ? g()->GetCurrentPalette() : HeatColors_p, debris_item._hue, 255, LINEARBLEND);
                baseRGB.nscale8(debris_item._state);
                drawPixelXYF(debris_item._position_x, debris_item._position_y, baseRGB);
            }
        }
    }

private:
    // A more cache-friendly version of the 8 independent arrays that were
    // used. This allows better locality per member, especially since we
    // access them sequentially.
    // Nothing special here; just a default-zero initialized struct.
    class DebrisItem
    {
      public:
        DebrisItem()
        {
        }

        void Clear()
        {
            _position_x = 0.0f;
            _position_y = 0.0f;
            _state = 0.0f;
            _speed_x = 0.0f;
            _speed_y = 0.0f;
            _hue = 0;
            _is_shift = false;
        }

        float _position_x;
        float _position_y;
        float _speed_x;
        float _speed_y;
        uint8_t _hue;
        uint8_t _state;
        bool _is_shift;
    };

    uint8_t hue, hue2, step;

    static constexpr int DEBRIS_ITEM_COUNT = 200;
    std::array<DebrisItem, DEBRIS_ITEM_COUNT> _debris_items;

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

    bool inline ParticlesUpdate(DebrisItem& debris_item)
    {
        // Intentionally do a narrowing conversion here.
        const int x = debris_item._position_x += debris_item._speed_x;
        const int y = debris_item._position_y += debris_item._speed_y;

        if (debris_item._state == 0
            || x < 0 || x >= MATRIX_WIDTH
            || y < 0 || y >= MATRIX_HEIGHT)
        {
            debris_item._is_shift = false;
        }

        return debris_item._is_shift;
    }

    void inline StarfieldEmit(DebrisItem& debris_item)
    {
        if (hue++ & 0x01)
            hue2 += 1;
        debris_item._position_x = MATRIX_WIDTH * 0.5;
        debris_item._position_y = MATRIX_HEIGHT * 0.5;

        debris_item._speed_x = (((float)random8() - 127.) / 512.);
        debris_item._speed_y = sqrtf(0.0626f - debris_item._speed_x * debris_item._speed_x);
        if (random8(2U))
            debris_item._speed_y = -debris_item._speed_y;

        debris_item._state = random8(1, 250);
        debris_item._hue = hue2;
        debris_item._is_shift = true;
    }

    static inline uint8_t WU_WEIGHT(uint8_t a, uint8_t b)
    {
        return (uint8_t)(((a) * (b) + (a) + (b)) >> 8);
    }

    void drawPixelXYF(float x, float y, CRGB color)
    {
        const uint8_t xx = (x - (int)x) * 255, yy = (y - (int)y) * 255, ix = 255 - xx, iy = 255 - yy;
        const uint8_t wu[4] = {WU_WEIGHT(ix, iy), WU_WEIGHT(xx, iy), WU_WEIGHT(ix, yy), WU_WEIGHT(xx, yy)};
        for (uint8_t i = 0; i < 4; i++) 
        {
            const int xn = x + (i & 1);
            const int yn = y + ((i >> 1) & 1);

            // Make sure we're on the panel and leave the VU meter pixels alone, if we're showing it
            if (!g()->isValidPixel(xn, yn) || (g_ptrSystem->EffectManager().IsVUVisible() && yn == (MATRIX_HEIGHT - 1)))
                continue;

            CRGB clr = g()->leds[XY(xn, MATRIX_HEIGHT - 1 - yn)];
            clr.r = qadd8(clr.r, (color.r * wu[i]) >> 8);
            clr.g = qadd8(clr.g, (color.g * wu[i]) >> 8);
            clr.b = qadd8(clr.b, (color.b * wu[i]) >> 8);
            g()->leds[XY(xn, yn)] = clr;
        }
    }
};
