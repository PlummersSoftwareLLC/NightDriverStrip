#pragma once

#include <bitset>

#include "effectmanager.h"
#include "effects/strip/musiceffect.h"

// Inspired by
// https://editor.soulmatelights.com/gallery/1685-strobe-and-diffusion Was
// originally drawn for a lamp, but I like it on a panel. The original
// coordinate system had 0,0 in the LL corner. We have 0,0 in UL. BUGBUG: This
// would look better if the snowflakes took longer to decay. I can't find the
// magic for blur2d().

#if ENABLE_AUDIO
class PatternSMStrobeDiffusion : public BeatEffectBase, public LEDStripEffect
#else
class PatternSMStrobeDiffusion : public LEDStripEffect
#endif
{
  private:
    uint8_t hue, hue2; // gradual hue shift or some other cyclic counter
    uint8_t step { 0 }; // some counter of frames or sequences of operations
    std::bitset<MATRIX_WIDTH * MATRIX_HEIGHT> noise3d[MATRIX_WIDTH * MATRIX_HEIGHT]; // Locations of snowflakes.
    uint8_t Speed = 150;                                                             // 1-255 is speed
    uint8_t Scale = 90;                                                              // 1-100 is something parameter
    uint8_t FPSdelay;        // BUGBUG: This is set but never used. :-(
    const int LOW_DELAY = 0; // This is used to set FPSdelay ... which is never used.

#if ENABLE_AUDIO
    const int top_line_offset = 1;
#else
    const int top_line_offset = 0;
#endif

  public:
    PatternSMStrobeDiffusion()
        :
#if ENABLE_AUDIO
          BeatEffectBase(1.50, 0.05),
#endif
          LEDStripEffect(EFFECT_MATRIX_SMSTROBE_DIFFUSION, "Diffusion")
    {
    }

    PatternSMStrobeDiffusion(const JsonObjectConst &jsonObject)
        :
#if ENABLE_AUDIO
          BeatEffectBase(1.50, 0.05),
#endif
          LEDStripEffect(jsonObject)
    {
    }

    void Start() override
    {
        //  	  FPSdelay = 25U; // LOW_DELAY;
        //    hue2 = 1;
        g()->Clear();
    }

    // =========== Christmas Tree ===========
    //             © SlingMaster
    //           EFF_CHRISTMAS_TREE
    //             Christmas Tree
    //---------------------------------------
    void VirtualSnow()
    {
        static int ct;
        ct++;
        // Scroll existing snowflakes down the screen.
        for (uint8_t x = 0U; x < MATRIX_WIDTH; x++)
        {
            // Don't copy the very top (usable) line that we're about to fill with
            // fresh snowflakes.
            for (uint8_t y = MATRIX_HEIGHT - 1; y > top_line_offset; y--)
            {
                assert((x >= 0) && (x < MATRIX_WIDTH));
                assert((y >= 0) && (y < MATRIX_HEIGHT));
                noise3d[x][y] = noise3d[x][y - 1];
                if (noise3d[x][y] > 0)
                {
                    g()->drawPixel(x, y, CHSV(170, 5U, 127 + random8(128)));
                }
            }
        }

        // Scroll the flakes above more often than we add new flakes below.
        //		  if ((ct & 0x07) != 0) return;

        // This is a fragile way to to it, but we fill the top line of
        // the display with fresh snowflakes to be scrolled down later.
        uint8_t posX = random(MATRIX_WIDTH);
        for (uint8_t x = 0U; x < MATRIX_WIDTH; x++)
        {
            // randomly fill in the top row
            noise3d[x][top_line_offset] = (posX == x) && (step % 3 == 0);
        }
    }

    // функция получения цвета пикселя в матрице по его координатам
    [[nodiscard]] CRGB getPixColorXY(uint8_t x, uint8_t y) const
    {
        // Just don't think about what this does to prefetch and prediction...
        return g()->leds[XY(x, y)];
    }

    void Draw() override
    {
        const uint8_t SIZE = 3U;
        const uint8_t DELTA = 1U; // центровка по вертикали
        uint8_t STEP = 2U;

#if ENABLE_AUDIO
        ProcessAudio(); // Without this, HandleBeat has no audio to process.
#endif

        // Most of the things being tested in this mess are constants and
        // many of the things being set are not used. Messy!
        STEP = floor((255 - Speed) / 64) + 1U; // for strob
        if (Scale > 50)
        {
            // diffusion ---
            // The offset is to skip the VU meter.
            g()->blur2d(g()->leds, MATRIX_WIDTH, 0, MATRIX_HEIGHT, top_line_offset, beatsin8(3, 64, 80));
            // g()->blur2d(g()->leds, MATRIX_WIDTH, 0, MATRIX_HEIGHT, top_line_offset,
            // 24);
            FPSdelay = LOW_DELAY;
            STEP = 1U;
            if (Scale < 75)
            {
                // chaos ---
                FPSdelay = 30;
                //      VirtualSnow();
            }
        }
        else
        {
            // strob -------
            if (Scale > 25)
            {
                g()->DimAll(200);
                FPSdelay = 30;
            }
            else
            {
                g()->DimAll(24);
                FPSdelay = 40;
            }
        }

        // Tuned by hand. Too slow and they just flash and show up in new locations.
        EVERY_N_MILLIS(30)
        {
            VirtualSnow();
        }

        // Much of this could be calls to lineDraw() but maybe being able to do
        // both ends while iterating through the loop is a win.
        const uint8_t rows = (MATRIX_HEIGHT + 1) / 3U;
        uint8_t deltaHue = floor(Speed / 64) * 64;
        bool dir = false;
        for (uint8_t y = 0; y < rows; y++)
        {
            if (dir)
            {
                if ((step % STEP) == 0)
                { // small layers
                    g()->drawPixel(MATRIX_WIDTH - 1, y * 3 + DELTA, CHSV(step, 255U, 255U));
                }
                else
                {
                    g()->drawPixel(MATRIX_WIDTH - 1, y * 3 + DELTA, CHSV(170U, 255U, 1U));
                }
            }
            else
            {
                if ((step % STEP) == 0)
                { // big layers
                    g()->drawPixel(0, y * 3 + DELTA, CHSV((step + deltaHue), 255U, 255U));
                }
                else
                {
                    g()->drawPixel(0, y * 3 + DELTA, CHSV(0U, 255U, 0U));
                }
            }

            // Shift layers ------------------
            for (uint8_t x = 1U; x < MATRIX_WIDTH; x++)
            {
                if (dir)
                { // <==
                    g()->drawPixel(x, y * 3 + DELTA, getPixColorXY(x, y * 3 + DELTA));
                }
                else
                { // ==>
                    g()->drawPixel(MATRIX_WIDTH - x, y * 3 + DELTA, getPixColorXY(MATRIX_WIDTH - x, y * 3 + DELTA));
                }
            }
            dir = !dir;
        }

        if (hue2 == 1)
        {
            step++;
            if (step >= 254)
                hue2 = 0;
        }
        else
        {
            step--;
            if (step < 1)
                hue2 = 1;
        }
    }

#if ENABLE_AUDIO
    void HandleBeat(bool bMajor, float elapsed, float span) override
    {
        debugV("HandleBeat");
        // Just light eye candy to show we react.
        hue2 += 128;
    }
#endif
};
