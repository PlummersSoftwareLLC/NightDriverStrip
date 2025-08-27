#pragma once

#include "systemcontainer.h"

// Inspired by https://editor.soulmatelights.com/gallery/1923-supernova


class PatternSMSupernova : public EffectWithId<PatternSMSupernova>
{
public:

    PatternSMSupernova() : EffectWithId<PatternSMSupernova>("Supernova"), hue(0), hue2(0), step(0) {}
    PatternSMSupernova(const JsonObjectConst &jsonDebrisItem) : EffectWithId<PatternSMSupernova>(jsonDebrisItem) {}

    virtual size_t DesiredFramesPerSecond() const override
    {
        return 60;
    }

    void Start() override
    {
        std::for_each(_debris_items.begin(), _debris_items.end(),
		              [](DebrisItem& debris_item) { debris_item.Clear(); });
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
                g()->drawPixelXYF_Wu(debris_item._position_x, debris_item._position_y, baseRGB);
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
};
