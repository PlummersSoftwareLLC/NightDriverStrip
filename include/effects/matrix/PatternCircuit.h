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
//   and
//
// History:     Jun-25-2022         Davepl      Based on Aurora
//
//---------------------------------------------------------------------------

/*
 * Aurora: https://github.com/pixelmatix/aurora
 * Copyright (c) 2014 Jason Coon
 *
 * Portions of this code are adapted from LedEffects Snake by Robert Atkins: https://bitbucket.org/ratkins/ledeffects/src/26ed3c51912af6fac5f1304629c7b4ab7ac8ca4b/Snake.cpp?at=default
 * Copyright (c) 2013 Robert Atkins
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

#ifndef PatternCircuit_H
#define PatternCircuit_H

#include "globals.h"
#include "ledstripeffect.h"
#include "gfxbase.h"

class PatternCircuit : public LEDStripEffect
{
private:
    static const byte SNAKE_LENGTH = 64;

    CRGB colors[SNAKE_LENGTH];
    uint8_t initialHue;

    enum Direction
    {
        UP,
        DOWN,
        LEFT,
        RIGHT
    };

    struct Pixel
    {
        uint8_t x;
        uint8_t y;
    };

    struct Path
    {
        Pixel pixels[SNAKE_LENGTH];

        Direction direction;

        void newDirection()
        {
            switch (direction)
            {
            case UP:
            case DOWN:
                direction = random(0, 2) == 1 ? RIGHT : LEFT;
                break;

            case LEFT:
            case RIGHT:
                direction = random(0, 2) == 1 ? DOWN : UP;

            default:
                break;
            }
        }

        void shuffleDown()
        {
            for (byte i = SNAKE_LENGTH - 1; i > 0; i--)
            {
                pixels[i] = pixels[i - 1];
            }
        }

        void reset()
        {
            direction = UP;
            for (int i = 0; i < SNAKE_LENGTH; i++)
            {
                pixels[i].x = 0;
                pixels[i].y = 0;
            }
        }

        void move()
        {
            switch (direction)
            {
            case UP:
                pixels[0].y = (pixels[0].y + 1) % MATRIX_HEIGHT;
                break;
            case LEFT:
                pixels[0].x = (pixels[0].x + 1) % MATRIX_WIDTH;
                break;
            case DOWN:
                pixels[0].y = pixels[0].y == 0 ? MATRIX_HEIGHT - 1 : pixels[0].y - 1;
                break;
            case RIGHT:
                pixels[0].x = pixels[0].x == 0 ? MATRIX_WIDTH - 1 : pixels[0].x - 1;
                break;
            }
        }

        void draw(GFXBase * graphics, CRGB colors[SNAKE_LENGTH])
        {
            for (byte i = 0; i < SNAKE_LENGTH; i++)
            {
                graphics->leds[graphics->xy(pixels[i].x, pixels[i].y)] = colors[i] %= (255 - i * (255 / SNAKE_LENGTH / 4));
            }
            byte m = random(20, 100);
            graphics->leds[graphics->xy(pixels[SNAKE_LENGTH - 1].x, pixels[SNAKE_LENGTH - 1].y)] = CRGB(0, m, 0); // End tail with random dark green
            graphics->leds[graphics->xy(pixels[0].x, pixels[0].y)] = CRGB::White;                                 // Head end bright white dot
        }
    };

    static const int snakeCount = 20;

public:
    Path *snakes; // BUGBUG marked static so as not to be on the stack, as it's too big

    PatternCircuit() : LEDStripEffect("Circuit")
    {
        snakes = new Path[snakeCount];
    }

    unsigned long msStart;

    void start()
    {
        auto graphics = (GFXBase *)_GFX[0].get();

        for (int i = 0; i < snakeCount; i++)
            snakes[i].reset();
        msStart = millis();
        graphics->Clear();
    }

    virtual void Draw()
    {
        auto graphics = (GFXBase *)_GFX[0].get();

//        graphics->drawLine(0, 0, 0, MATRIX_HEIGHT-1, CRGB::Red);                                // Left
//        graphics->drawLine(MATRIX_WIDTH-1, 0, MATRIX_WIDTH-1, MATRIX_HEIGHT-1, CRGB::Blue);     // Right
//        return;

        // Reset after 20 seconds
        if (millis() - msStart > 20000)
            start();

        for (int i = 0; i < MATRIX_WIDTH * MATRIX_HEIGHT / 10; i++)
        {
            graphics->leds[graphics->xy(random(0, MATRIX_WIDTH), random(0, MATRIX_HEIGHT))].fadeToBlackBy(32);
        }

        // fill_palette(colors, SNAKE_LENGTH, initialHue++, 5, graphics->currentPalette, 255, LINEARBLEND);
        fill_palette(colors, SNAKE_LENGTH, 0, 4, ForestColors_p, 255, LINEARBLEND);
        for (int i = 0; i < snakeCount; i++)
        {
            Path *path = &snakes[i];

            path->shuffleDown();

            if (random(10) > 7)
            {
                path->newDirection();
            }

            path->move();
            path->draw(graphics, colors);
        }
    }
};

#endif
