//+--------------------------------------------------------------------------
//
// File:        PatternLife.h
//
// NightDriverStrip - (c) 2018 Plummer's Software LLC.  All Rights Reserved.
//
// This file is part of the NightDriver software project.
//
//    NightDriver is free software: you can redistribute it and/or modify
//    it under the terms of the GNU General Public License as published by
//    the Free Software Foundation, either version 3 of the License, or
//    (at your option) any later version.
//
//    NightDriver is distributed in the hope that it will be useful,
//    but WITHOUT ANY WARRANTY; without even the implied warranty of
//    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//    GNU General Public License for more details.
//
//    You should have received a copy of the GNU General Public License
//    along with Nightdriver.  It is normally found in copying.txt
//    If not, see <https://www.gnu.org/licenses/>.
//
//
// Description:
//
//   Effect code ported from Aurora to Mesmerizer's draw routines
//   and added the cycle detection CRC stuff
//
// History:     Jun-25-2022         Davepl      Based on Aurora
//              Jul-08-2022         Davepl      Added loop checks
//
//---------------------------------------------------------------------------

/*
 * Aurora: https://github.com/pixelmatix/aurora
 * Copyright (c) 2014 Jason Coon
 *
 * Portions of this code are adapted from Andrew: http://pastebin.com/f22bfe94d
 * which, in turn, was "Adapted from the Life example on the Processing.org site"
 *
 * Made much more colorful by J.B. Langston:
 *  https://github.com/jblang/aurora/commit/6db5a884e3df5d686445c4f6b669f1668841929b
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this software and associated documentation files (the "Software"), to deal in
 * the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
 * the Software, and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#ifndef PatternLife_H
#define PatternLife_H

#include <bitset>

extern "C"
{
    #include "uzlib/src/uzlib.h"
}

class Cell
{
public:
  uint8_t alive : 1;
  uint8_t prev  : 1;
  uint8_t hue;
  uint8_t brightness;
};

// We check for loops by keeping a number of hashes of previous frames.  A walker that goes up and across
// the screen cycles every 2 times it crosses, so max dimension times 2 is a good place to start

#define CRC_LENGTH (std::max(MATRIX_HEIGHT, MATRIX_WIDTH) * 2)

class PatternLife : public LEDStripEffect
{
private:
    std::unique_ptr<Cell [][MATRIX_HEIGHT]> world;
    std::unique_ptr<uint32_t []> checksums;
    int iChecksum = 0;
    uint32_t bStuckInLoop = 0;
    unsigned int density = 50;
    int cGeneration = 0;
    unsigned long seed;


    virtual bool Init(std::shared_ptr<GFXBase> gfx[NUM_CHANNELS]) override
    {
        LEDStripEffect::Init(gfx);

        // Note: placing the world in PSRAM may slow this effect down, but it's currently running
        //       fast enough (30+ fps) that we can afford to use it

        world.reset(psram_allocator<Cell [MATRIX_HEIGHT]>().allocate(MATRIX_WIDTH)) ;
        checksums.reset(psram_allocator<uint32_t>().allocate(CRC_LENGTH));

        return true;
    }

    virtual bool RequiresDoubleBuffering() const override
    {
        return false;
    }

    // A table of seed vs generation count.  These are seeds that net long runs of at least 3000 generations.
    //
    // Example:  Seed: 92465, Generations: 1626

    static constexpr long bakedInSeeds[] =
    {
        130908,         // 3253
        1576,           // 3125
        275011,         // 3461
        291864,         // 4006
        692598154,      // 3876
        241590764,      // 4808
        701054810,      // 3081
        1824315566,     // 3256
        342432015,      // 3035
        1670458840,     // 3108
        1177135100,     // 3243
        281769225,      // 4354
        1918045960,     // 3601
        1548443429,     // 3305
        1038898468,     // 3538
        1791133398,     // 3235
        1550109533,     // 3823
        1060251497,     // 4336
        555109764,      // 4470
    };


    void randomFillWorld()
    {
        // Some fraction of the time we pick a pre-baked seed that we know lasts for a lot
        // of generations.  Otherwise, we pick a random seed and run with that.

        srand(millis());
        if (random(0, 4) == 0)
        {
            seed = bakedInSeeds[random(ARRAYSIZE(bakedInSeeds))];
            debugI("Prebaked Seed: %lu", seed);
        }
        else
        {
            seed = random();
            debugI("Randomized Seed: %lu", seed);
        }

        srand(seed);
        for (int i = 0; i < MATRIX_WIDTH; i++) {
            for (int j = 0; j < MATRIX_HEIGHT; j++) {
                if ((rand() % 100) < density) {
                    world[i][j].alive = 1;
                    world[i][j].brightness = 128;
                }
                else {
                    world[i][j].alive = 0;
                    world[i][j].brightness = 0;
                }
                world[i][j].prev = world[i][j].alive;
                world[i][j].hue = 0;
            }
        }

        for (int i = 0; i < CRC_LENGTH; i++)
            checksums[i] = 0xFFFFFFF;
    }

    int neighbours(int x, int y) {
        return (world[(x + 1) % MATRIX_WIDTH][y].prev) +
            (world[x][(y + 1) % MATRIX_HEIGHT].prev) +
            (world[(x + MATRIX_WIDTH - 1) % MATRIX_WIDTH][y].prev) +
            (world[x][(y + MATRIX_HEIGHT - 1) % MATRIX_HEIGHT].prev) +
            (world[(x + 1) % MATRIX_WIDTH][(y + 1) % MATRIX_HEIGHT].prev) +
            (world[(x + MATRIX_WIDTH - 1) % MATRIX_WIDTH][(y + 1) % MATRIX_HEIGHT].prev) +
            (world[(x + MATRIX_WIDTH - 1) % MATRIX_WIDTH][(y + MATRIX_HEIGHT - 1) % MATRIX_HEIGHT].prev) +
            (world[(x + 1) % MATRIX_WIDTH][(y + MATRIX_HEIGHT - 1) % MATRIX_HEIGHT].prev);
    }

public:

    PatternLife() : LEDStripEffect(EFFECT_MATRIX_LIFE, "Life")
    {
    }

    PatternLife(const JsonObjectConst& jsonObject) : LEDStripEffect(jsonObject)
    {
    }

    void Reset()
    {
        randomFillWorld();
        for (int i = 0; i < CRC_LENGTH; i++)
            checksums[i] = 0xFFFFFFF;
        cGeneration = 0;
        bStuckInLoop = 0;
    }

    virtual void Draw() override
    {
        if (cGeneration == 0)
            Reset();

        // Display current generation

        for (int i = 0; i < MATRIX_WIDTH; i++) {
            for (int j = 0; j < MATRIX_HEIGHT; j++) {
                if (world[i][j].brightness > 0)
                    g()->leds[g()->xy(i, j)] += g()->ColorFromCurrentPalette(world[i][j].hue * 4, world[i][j].brightness);
                else
                    g()->leds[g()->xy(i, j)] = CRGB::Black;
            }
        }

        // We maintain a scrolling window of the last N crcs and if the current crc makes it all
        // the way down to the bottom half we assume we're stuck in a loop and restart.
        // We have to first extract the alive bits alone because we don't want the hue and brightness
        // data to mess with the CRC.

        bool alive[MATRIX_WIDTH][MATRIX_HEIGHT];
        for (int i = 0; i < MATRIX_WIDTH; i++)
            for (int j = 0; j < MATRIX_HEIGHT; j++)
                alive[i][j] = world[i][j].alive;

        auto crc = uzlib_crc32(alive, sizeof(alive), 0xffffffff);
        for (int i = 0; i < CRC_LENGTH - 1; i++)
            checksums[i] = checksums[i+1];
        checksums[CRC_LENGTH - 1] = crc;


        // Look for any occurrences of the current CRC in the first half of the window, which would mean
        // a loop has occurred.  If

        if (bStuckInLoop)
        {
            const int flashTime = 250;
            const int resetTime = 1500;

            auto elapsed = millis() - bStuckInLoop;
            if (elapsed < flashTime)
            {
                auto whiteColor = CRGB(0x60, 0x00, 0x00);
                g()->fillRectangle(0, 0, MATRIX_WIDTH, MATRIX_HEIGHT, whiteColor);
            }
            g()->DimAll(255 - 255*elapsed/resetTime);

            for (int x = 0; x < MATRIX_WIDTH; x++)
                for (int y = 0; y < MATRIX_HEIGHT; y++)
                        world[x][y].brightness *= 0.9;
            if (elapsed > resetTime)
                Reset();
        }
        else
        {
            for (int i = CRC_LENGTH - 2; i >= 0; i--)
            {
                if (checksums[i] == crc)
                {
                    bStuckInLoop = millis();
                    debugW("Seed: %10lu, Generations: %5d, %s", seed, cGeneration, cGeneration > 3000 ? "Y" : "N");
                }
                if (checksums[i] == 0xFFFFFFF)
                    break;
            }
        }

        // Birth and death cycle
        for (int x = 0; x < MATRIX_WIDTH; x++) {
            for (int y = 0; y < MATRIX_HEIGHT; y++) {
                // Default is for cell to stay the same
                if (world[x][y].brightness > 0 && world[x][y].prev == 0)
                  world[x][y].brightness *= 0.75;

                int count = neighbours(x, y);
                if (count == 3 && world[x][y].prev == 0) {
                    // A new cell is born
                    world[x][y].alive = 1;
                    world[x][y].hue += 1;
                    world[x][y].brightness = 255;
                } else if ((count < 2 || count > 3) && world[x][y].prev == 1) {
                    // Cell dies
                    world[x][y].alive = 0;
                    world[x][y].brightness = 0;
                }
            }
        }

        // Copy next generation into place

        int cCount = 0;

        for (int x = 0; x < MATRIX_WIDTH; x++) {
            for (int y = 0; y < MATRIX_HEIGHT; y++) {
                if (world[x][y].prev != world[x][y].alive)
                    cCount++;
                world[x][y].prev = world[x][y].alive;
            }
        }

        cGeneration++;
    }
};

#endif
