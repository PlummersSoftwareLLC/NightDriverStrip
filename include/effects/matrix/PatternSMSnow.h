#pragma once

#include "effectmanager.h"
#include <cmath> // sinf

// Falling snowflakes for a 2D LED matrix (e.g. Mesmerizer 64x32).
//
// Each snowflake falls from the top (y=0) to the bottom (y=MATRIX_HEIGHT-1)
// with a slight sinusoidal horizontal drift.  Small flakes are a single
// pixel; large flakes (25 % of spawns) draw a 5-pixel "+" cross for a
// chunkier look.  DimAll() each frame provides a soft fading trail.

class PatternSMSnow : public EffectWithId<PatternSMSnow>
{
  private:

    static constexpr int   kMaxFlakes    = 60;
    static constexpr int   kAccumFrames  = 10; // frames a flake rests at the bottom before resetting

    struct Snowflake
    {
        float   x;           // current X position (fractional for smooth horizontal drift)
        float   y;           // current Y position (fractional for smooth fall)
        float   speedY;      // fall rate in pixels per frame  (0.15 – 0.45)
        float   driftPhase;  // current phase for sinusoidal horizontal sway (radians)
        float   driftSpeed;  // how quickly driftPhase advances each frame
        uint8_t brightness;  // 160 – 255
        bool    large;       // true → 5-pixel cross; false → single pixel
    };

    Snowflake _flakes[kMaxFlakes];
    int       _accumCounters[kMaxFlakes]; // counts frames spent resting at the bottom

    void resetFlake(int i)
    {
        // Stagger Y start positions so flakes don't all enter at once
        _flakes[i].x          = (float)random(0, MATRIX_WIDTH);
        _flakes[i].y          = -(float)random(0, MATRIX_HEIGHT);
        _flakes[i].speedY     = 0.15f + (random(0, 100) / 333.0f); // 0.15 – 0.45
        _flakes[i].driftPhase = (float)random(0, 628) / 100.0f;    // 0 – 2π
        _flakes[i].driftSpeed = 0.02f + (float)random(0, 100) / 2000.0f;
        _flakes[i].brightness = (uint8_t)random(160, 255);
        _flakes[i].large      = (random(0, 4) == 0);               // ~25 % large
        _accumCounters[i]     = 0;
    }

    void drawFlake(const Snowflake& f)
    {
        int16_t ix = (int16_t)f.x;
        int16_t iy = (int16_t)f.y;

        if (!g()->isValidPixel(ix, iy))
            return;

        // Pale ice-blue: hue≈148, low saturation, variable brightness
        CRGB color = CHSV(148, 60, f.brightness);

        g()->leds[XY(ix, iy)] += color;

        if (f.large)
        {
            // 5-pixel "+" cross – isValidPixel guards against out-of-bounds
            if (g()->isValidPixel((uint)(ix - 1), (uint)iy)) g()->leds[XY(ix - 1, iy)] += color;
            if (g()->isValidPixel((uint)(ix + 1), (uint)iy)) g()->leds[XY(ix + 1, iy)] += color;
            if (g()->isValidPixel((uint)ix, (uint)(iy - 1))) g()->leds[XY(ix, iy - 1)] += color;
            if (g()->isValidPixel((uint)ix, (uint)(iy + 1))) g()->leds[XY(ix, iy + 1)] += color;
        }
    }

  public:

    PatternSMSnow() : EffectWithId<PatternSMSnow>("Snow") {}
    PatternSMSnow(const JsonObjectConst& jsonObject) : EffectWithId<PatternSMSnow>(jsonObject) {}

    void Start() override
    {
        g()->Clear();
        for (int i = 0; i < kMaxFlakes; i++)
            resetFlake(i);
    }

    void Draw() override
    {
        // Dim previous frame so flakes leave a short fading trail
        g()->DimAll(210);

        for (int i = 0; i < kMaxFlakes; i++)
        {
            Snowflake& f = _flakes[i];

            // Skip flakes that haven't entered the top of the matrix yet
            if (f.y < 0.0f)
            {
                f.y += f.speedY;
                continue;
            }

            // Update horizontal drift
            f.driftPhase += f.driftSpeed;
            f.x          += sinf(f.driftPhase) * 0.3f;

            // Wrap X so flakes never disappear off the sides
            if (f.x < 0.0f)              f.x += (float)MATRIX_WIDTH;
            if (f.x >= (float)MATRIX_WIDTH) f.x -= (float)MATRIX_WIDTH;

            // Advance downward
            f.y += f.speedY;

            if (f.y >= (float)MATRIX_HEIGHT)
            {
                // Rest at the bottom row for kAccumFrames, then recycle
                f.y = (float)(MATRIX_HEIGHT - 1);
                _accumCounters[i]++;
                if (_accumCounters[i] >= kAccumFrames)
                    resetFlake(i);
            }

            drawFlake(f);
        }
    }
};
