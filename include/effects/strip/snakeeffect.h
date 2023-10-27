//+--------------------------------------------------------------------------
//
// File:        SnakeEffect.h
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
//    The game of Snake as an LEDStrip effect.
//
// History:     Dec-22-2022         MatthewvanBuuren      Created
//
//---------------------------------------------------------------------------

#pragma once

#include "effects.h"
#include "globals.h"

class SnakeEffect : public LEDStripEffect
{
    void construct()
    {
        lastLEDIndex = LEDCount - 1;
        Reset();
    }

  protected:
    int     LEDCount;             // Number of LEDs total
    int     SnakeSpeed;           // Max duration between iterations.

    int     snakeSize;            // Length of snake.
    int     snakeHead;            // Pointer to head of snake.
    int     lastEaten;            // Increase the snake-speed as since eaten grows.
    int     direction;            // forward|reverse
    int     apple;                // Spawn location of Apple

    int     winMode;              // End of Game Indicator

    int     lastLEDIndex;         // One-time init for cleaner code.

    static const int dSnakeSize = 3;    // Default start snake size.
    static const int dSnakeSpeed = 25;  // Default snake speed.

    static const int dForward = 1;      // ENUM for direction forward.
    static const int dBackward = -1;

  public:

    SnakeEffect(const char * strName, int ledCount = NUM_LEDS, int snakeSpeed = dSnakeSpeed)
        : LEDStripEffect(EFFECT_STRIP_SNAKE, strName),
          LEDCount(ledCount),
          SnakeSpeed(snakeSpeed)
    {
        construct();
    }

    SnakeEffect(const JsonObjectConst& jsonObject)
        : LEDStripEffect(jsonObject),
          LEDCount(jsonObject[PTY_LEDCOUNT]),
          SnakeSpeed(jsonObject[PTY_SPEED])
    {
        construct();
    }

    bool SerializeToJSON(JsonObject& jsonObject) override
    {
        AllocatedJsonDocument jsonDoc(LEDStripEffect::_jsonSize + 64);

        JsonObject root = jsonDoc.to<JsonObject>();
        LEDStripEffect::SerializeToJSON(root);

        jsonDoc[PTY_LEDCOUNT] = LEDCount;
        jsonDoc[PTY_SPEED] = SnakeSpeed;

        assert(!jsonDoc.overflowed());

        return jsonObject.set(jsonDoc.as<JsonObjectConst>());
    }

    virtual ~SnakeEffect()
    {
    }

    virtual void Reset()
    {
        snakeSize = dSnakeSize;
        snakeHead = snakeSize;
        lastEaten = 0;
        direction = dForward;
        apple = getNextApple();
        winMode = 0;
    }

    void Draw() override
    {
        setAllOnAllChannels(0,0,0); // Clear

        // Game Loop
        EVERY_N_MILLISECONDS(std::max(SnakeSpeed - (lastEaten * 2), 5)) // 5ms faster cap.
        {
            // Win Reset
            if(snakeSize >= LEDCount)
            {
                if(winMode < 90)
                {
                    winMode++;
                }
                else
                {
                    Reset();
                }
            }

            // Reverse direction if on bound.
            if (direction == dForward && snakeHead == lastLEDIndex)
            {
                direction = dBackward;
            }
            else if (direction == dBackward && snakeHead == 0)
            {
                direction = dForward;
            }

            // Move Head && Check for Food
            snakeHead = snakeHead + direction;
            if (snakeHead == apple)
            {
                lastEaten = 0;
                apple = getNextApple();
                snakeSize++;
            }

            lastEaten++;
        }

        // Finally draw
        for (int i = 0; i < LEDCount; i++)
        {
            // Win Reset
            if(winMode != 0 && winMode % 2 == 0)
            {
                setPixelOnAllChannels(i, CRGB::Green);
            }
            else if(winMode != 0 && winMode % 2 == 1)
            {
                setPixelOnAllChannels(i, CRGB::Black);
            }
            else
            {
                // BUGBUG: Optional Palette scheme.
                if (i == apple)
                {
                    setPixelOnAllChannels(i, CRGB::Yellow);
                }
                else if (i == snakeHead)
                {
                    setPixelOnAllChannels(i, CRGB(0x9CB071));
                }
                else if (direction == dForward && (
                    ((snakeHead-snakeSize) < 0 && (i < snakeHead || i <= 0 - (snakeHead-snakeSize))) || // Wrap-around
                    ((snakeHead-snakeSize) >= 0 && i <= snakeHead && i >= (snakeHead-snakeSize))
                ))
                {
                    setPixelOnAllChannels(i, CRGB::Green);
                }
                else if (direction == dBackward && ( // Cleaner to keep seperate
                    ((snakeHead+snakeSize) >= lastLEDIndex && (i > snakeHead || i > lastLEDIndex + (lastLEDIndex - (snakeHead+snakeSize)))) || // Wrap-around
                    ((snakeHead+snakeSize) < lastLEDIndex && i >= snakeHead && i <= (snakeHead+snakeSize))
                ))
                {
                    setPixelOnAllChannels(i, CRGB::Green);
                }
                else
                {
                    setPixelOnAllChannels(i, CRGB::Black);
                }
            }


        }
    }

    int ifWrapAroundReturnTailIndex()
    {
        if (direction == dForward) {
            if ((snakeHead-snakeSize) < 0)
            {
                return 0 - (snakeHead-snakeSize);
            }
        } else {
            if ((snakeHead+snakeSize) > lastLEDIndex)
            {
                return (lastLEDIndex) + ((lastLEDIndex) - (snakeHead+snakeSize));
            }
        }

        return -1; // Not Wrap-Around
    }

    int getNextApple()
    {
        int wrapIndex = ifWrapAroundReturnTailIndex();
        if(wrapIndex != -1)
        {   // Only 1 range in non-wrap-around.
            if (direction == dForward)
            {
                return random_range(std::max(snakeHead, wrapIndex), lastLEDIndex);
            }
            else
            {
                return random_range(0, std::min(snakeHead, wrapIndex));
            }
        }
        else
        {
            if (direction == dForward)
            {
                return findRandomInRanges(0, snakeHead-snakeSize, snakeHead, lastLEDIndex);
            }
            else
            {
                return findRandomInRanges(0, snakeHead, snakeHead+snakeSize, lastLEDIndex);
            }

        }
    }

    int findRandomInRanges(int r1s, int r1e, int r2s, int r2e) // Easily extended for more ranges.
    {
        // Assume ranges are in bounds.
        // Assume r1s < r1e && r2s < r2e

        int r1Diff = (r1e - r1s);
        int random = random_range(0, r1Diff + (r2e - r2s));

        if (random <= r1Diff)
        {
            return r1s + random;
        }
        else
        {
            return r2s + (random - r1Diff);
        }
    }
};
