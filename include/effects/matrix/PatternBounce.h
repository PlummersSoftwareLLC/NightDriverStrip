//+--------------------------------------------------------------------------
//
// File:        PatternSpiro.h
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
//
// History:     Jul-08-2022         Davepl      Based on Aurora
//
//---------------------------------------------------------------------------

/*
 * Aurora: https://github.com/pixelmatix/aurora
 * Copyright (c) 2014 Jason Coon
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

#ifndef PatternBounce_H
#define PatternBounce_H

#include "Vector.h"
#include "Boid.h"

// Description: This file defines the PatternBounce class, a subclass of LEDStripEffect. 
//              The class creates a bouncing effect on an LED matrix, where multiple points 
//              (referred to as 'boids') bounce off the bottom edge of the matrix.
//
//              The PatternBounce class uses the PVector class for representing the gravity 
//              affecting the boids. Each boid's velocity and position are calculated based 
//              on this gravity, and their interaction with the bottom edge of the LED matrix.
//
//              The Start() method initializes the boids with a vertical velocity and positions 
//              them along the top edge of the matrix. Each boid is assigned a unique color from 
//              the current color palette.
//
//              The Draw() method updates the position of each boid based on its velocity and 
//              gravity. When a boid hits the bottom edge of the matrix, its velocity is inverted, 
//              creating a bouncing effect. The method also handles dimming and blurring of the 
//              LEDs to enhance the visual effect.

class PatternBounce : public LEDStripEffect
{
private:
    static const int count = MATRIX_WIDTH;
    PVector gravity = PVector(0, 0.0125);

public:
    PatternBounce() : LEDStripEffect(EFFECT_MATRIX_BOUNCE, "Bounce")
    {
    }

    PatternBounce(const JsonObjectConst& jsonObject) : LEDStripEffect(jsonObject)
    {
    }

    bool RequiresDoubleBuffering() const override
    {
        return true;
    }

    void Start() override
    {
        unsigned int colorWidth = 256 / count;
        for (int i = 0; i < count; i++)
        {
            Boid boid = Boid(i, 0);
            boid.velocity.x = 0;
            boid.velocity.y = i * -0.01;
            boid.colorIndex = colorWidth * i;
            boid.maxforce = 10;
            boid.maxspeed = 10;
            g()->_boids[i] = boid;
        }
    }

    void Draw() override
    {
        // dim all pixels on the display

        // Blue columns only, and skip the first row of each column if the VU meter is being shown so we don't blend it onto ourselves
        g()->blurColumns(g()->leds, MATRIX_WIDTH, MATRIX_HEIGHT, g_ptrSystem->EffectManager().IsVUVisible() ? 1 : 0, 200);
        g()->DimAll(250);

        auto totalVelocity = 0.0;
        for (int i = 0; i < count; i++)
        {
            Boid boid = g()->_boids[i];
            boid.applyForce(gravity);
            boid.update();
            totalVelocity += abs(boid.velocity.y);

            if (g()->isValidPixel(boid.location.x, boid.location.y))
                g()->setPixel(boid.location.x, boid.location.y, g()->ColorFromCurrentPalette(boid.colorIndex));

            if (boid.location.y >= MATRIX_HEIGHT - 1)
            {
                boid.location.y = MATRIX_HEIGHT - 1;
                boid.velocity.y *= -1.0;
            }

            g()->_boids[i] = boid;
        }
        
        // If the combined velocity of all the boids is less than 1.0, restart the animation.
        // The value of 1.0 is arbitrary, but it seems to work well.

        if (totalVelocity < 1.0)
            Start();
    }
};

#endif
