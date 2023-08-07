//+--------------------------------------------------------------------------
//
// File:        PatternSpin.h
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
// History:     Jun-25-202         Davepl      Adapted from own code
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

#ifndef PatternSpin_H
#define PatternSpin_H

class PatternSpin : public LEDStripEffect
{
public:
    PatternSpin() : LEDStripEffect(EFFECT_MATRIX_SPIN, "Spin")
    {
    }

    PatternSpin(const char   * pszFriendlyName) : LEDStripEffect(EFFECT_MATRIX_SPIN, pszFriendlyName)
    {
    }

    PatternSpin(const JsonObjectConst& jsonObject) : LEDStripEffect(jsonObject)
    {
    }

    float degrees = 0;
    float radius = 16;

    float speedStart = 1;
    float velocityStart = 0.6;

    float maxSpeed = 30;

    float speed = speedStart;
    float velocity = velocityStart;

    void Start() override
    {
        speed = speedStart;
        velocity = velocityStart;
        degrees = 0;
    }

    void Draw() override
    {
        g()->DimAll(190);

        CRGB color = g()->ColorFromCurrentPalette(speed * 8);

        // start position
        int x;
        int y;

        // target position
        float targetDegrees = degrees + speed;
        float targetRadians = radians(targetDegrees);
        int targetX = (int) (MATRIX_CENTER_X + radius * cos(targetRadians));
        int targetY = (int) (MATRIX_CENTER_Y - radius * sin(targetRadians));

        float tempDegrees = degrees;

        do{
            float radians = radians(tempDegrees);
            x = (int) (MATRIX_CENTER_X + radius * cos(radians));
            y = (int) (MATRIX_CENTER_Y - radius * sin(radians));

            g()->drawPixel(x, y, color);
            g()->drawPixel(y, x, color);

            tempDegrees += 1;
            if (tempDegrees >= 360)
                tempDegrees = 0;
        } while (x != targetX || y != targetY);

        degrees += speed;

        // add velocity to the particle each pass around the accelerator
        if (degrees >= 360) {
            degrees = 0;
            speed += velocity;
            if (speed <= speedStart) {
                speed = speedStart;
                velocity *= -1;
            }
            else if (speed > maxSpeed){
                speed = maxSpeed - velocity;
                velocity *= -1;
            }
        }
    }
};

#endif
