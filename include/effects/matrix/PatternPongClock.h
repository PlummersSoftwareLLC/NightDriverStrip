//+--------------------------------------------------------------------------
//
// File:        PatternPongClock.h
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
//   Effect code ported from Aurora to Mesmerizer's draw routines.  I
//   removed the hard-coded pixel counts, made the AI a little more
//   flexible, variable ball velocity, etc.
//
// History:     Jul-08-2022         Davepl      Based on Aurora
//
//---------------------------------------------------------------------------

/*
 *
 * RGB Pong Clock - Andrew Holmes @pongclock
 * https://github.com/pkourany/RGBPongClock
 *
 * Inspired by, and shamelessly derived from
 *     Nick's LED Projects
 * https://123led.wordpress.com/about/
 *
 * Modified for SmartMatrix by Jason Coon
 *
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

#ifndef PatternPongClock_H
#define PatternPongClock_H

#include "deviceconfig.h"

#define BAT1_X 2 // Pong left bat x pos (this is where the ball collision occurs, the bat is drawn 1 behind these coords)
#define BAT2_X (MATRIX_WIDTH - 4)
#define BAT_HEIGHT (MATRIX_HEIGHT / 4)
#define SPEEDUP 1.15
#define MAXSPEED 4.0f

class PatternPongClock : public LEDStripEffect
{
  private:
    float ballpos_x, ballpos_y;
    uint8_t erase_x = 10; // holds ball old pos so we can erase it, set to blank area of screen initially.
    uint8_t erase_y = 10;
    float ballvel_x, ballvel_y;
    int bat1_y = 5; // bat starting y positions
    int bat2_y = 5;
    int bat1_target_y = 5; // bat targets for bats to move to
    int bat2_target_y = 5;
    uint8_t bat1_update = 1; // flags - set to update bat position
    uint8_t bat2_update = 1;
    uint8_t bat1miss, bat2miss; // flags set on the minute or hour that trigger the bats to miss the ball, thus upping the score to match the time.
    uint8_t restart = 1;        // game restart flag - set to 1 initially to set up 1st game

    uint8_t mins;
    uint8_t hours;

  public:

    PatternPongClock() : LEDStripEffect(EFFECT_MATRIX_PONG_CLOCK, "PongClock")
    {
    }

    PatternPongClock(const JsonObjectConst& jsonObject) : LEDStripEffect(jsonObject)
    {
    }

    virtual size_t DesiredFramesPerSecond() const override
    {
        return 35;
    }

    virtual bool RequiresDoubleBuffering() const override
    {
        return false;
    }

    virtual void Start() override
    {
        time_t ttime = time(0);
        tm *local_time = localtime(&ttime);

        bool ampm = !g_ptrDeviceConfig->Use24HourClock();

        // update score / time
        mins = local_time->tm_min;
        hours = local_time->tm_hour;
        if (hours > 12 && ampm)
            hours -= 12;
        else if (hours == 0 && ampm)
            hours = 12;
    }

    virtual void Draw() override
    {
        auto g = g_ptrEffectManager->g();

        time_t ttime = time(0);
        tm *local_time = localtime(&ttime);

        g->Clear();

        // draw pitch centre line
        for (uint16_t i = 0; i < MATRIX_WIDTH / 2; i += 2)
            g->setPixel(MATRIX_WIDTH / 2, i, 0x6666);

        // draw hh:mm separator colon that blinks once per second

        if (local_time->tm_sec % 2 == 0)
        {
            g->setPixel(MATRIX_WIDTH / 2, 4, RED16);
            g->setPixel(MATRIX_WIDTH / 2, 6, RED16);
        }

        LEDMatrixGFX::backgroundLayer.setFont(gohufont11b);
        char buffer[3];

        auto clockColor = rgb24(255,255,255);
        sprintf(buffer, "%2d", hours);
        LEDMatrixGFX::backgroundLayer.drawString(MATRIX_WIDTH / 2 - 12, 0, clockColor, buffer);

        sprintf(buffer, "%02d", mins);
        LEDMatrixGFX::backgroundLayer.drawString(MATRIX_WIDTH / 2 + 2, 0, clockColor, buffer);

        // if restart flag is 1, set up a new game
        if (restart)
        {
            // set ball start pos
            ballpos_x = MATRIX_WIDTH / 2;
            ballpos_y = random(4, MATRIX_HEIGHT-4);
            ballvel_x = 0;

            // pick random ball direction
            if (random(0, 2) > 0)
            {
                ballvel_x = 1;
            }
            else
            {
                ballvel_x = -1;
            }

            if (random(0, 2) > 0)
            {
                ballvel_y = 0.5;
            }
            else
            {
                ballvel_y = -0.5;
            }
            // draw bats in initial positions
            bat1miss = 0;
            bat2miss = 0;
            // reset game restart flag
            restart = 0;
        }

        // if coming up to the minute: secs = 59 and mins < 59, flag bat 2 (right side) to miss the return so we inc the minutes score
        if (local_time->tm_sec == 59 && local_time->tm_min < 59)
        {
            bat1miss = 1;
        }
        // if coming up to the hour: secs = 59  and mins = 59, flag bat 1 (left side) to miss the return, so we inc the hours score.
        if (local_time->tm_sec == 59 && local_time->tm_min == 59)
        {
            bat2miss = 1;
        }

        // AI - we run 2 sets of 'AI' for each bat to work out where to go to hit the ball back
        // very basic AI...
        //  For each bat, First just tell the bat to move to the height of the ball when we get to a random location.
        // for bat1

        if (ballpos_x == random(MATRIX_WIDTH / 2 + 2, MATRIX_WIDTH))
        {
            bat1_target_y = ballpos_y;
        }
        // for bat2
        if (ballpos_x == random(4, MATRIX_WIDTH / 2))
        {
            bat2_target_y = ballpos_y;
        }

        // Define the "Thinking zone" around center.  Player will set their targets based on what they see here.

        constexpr float LOOKAHEAD = 1.0;
        constexpr float leftEdge  = MATRIX_WIDTH / 2 - MAXSPEED * LOOKAHEAD;
        constexpr float rightEdge = MATRIX_WIDTH / 2 + MAXSPEED * LOOKAHEAD;

        // If ball going leftwards towards BAT1,

        if (ballvel_x < 0 && ballpos_x > leftEdge  && ballpos_x < rightEdge)
        {

            uint8_t end_ball_y = pong_get_ball_endpoint(ballpos_x, ballpos_y, ballvel_x, ballvel_y);

            // if the miss flag is set,  then the bat needs to miss the ball when it gets to end_ball_y
            if (bat1miss == 1)
            {
                bat1miss = 0;
                if (end_ball_y > MATRIX_HEIGHT / 2)
                {
                    bat1_target_y = random(0, 3);
                }
                else
                {
                    bat1_target_y = 8 + random(0, 3);
                }
            }
            // if the miss flag isn't set,  set bat target to ball end point with some randomness, so it's not always hitting top of bat
            else
            {
                bat1_target_y = end_ball_y - random(0, BAT_HEIGHT);
                if (bat1_target_y < 0)
                    bat1_target_y = 0;
                if (bat1_target_y > MATRIX_HEIGHT - BAT_HEIGHT)
                    bat1_target_y = MATRIX_HEIGHT - BAT_HEIGHT;
            }
        }

        // right bat AI
        // if positive velocity then predict for right bat - first just match ball height
        // when the ball is closer to the right bat, run the ball maths to find out where it will land

        if (ballvel_x > 0 && ballpos_x > leftEdge && ballpos_x < rightEdge)
        {
            uint8_t end_ball_y = pong_get_ball_endpoint(ballpos_x, ballpos_y, ballvel_x, ballvel_y);

            // if flag set to miss, move bat out way of ball
            if (bat2miss == 1)
            {
                bat2miss = 0;
                // if ball end point above 8 then move bat down, else move it up- so either way it misses
                if (end_ball_y > MATRIX_HEIGHT / 2)
                {
                    bat2_target_y = random(0, 3);
                }
                else
                {
                    bat2_target_y =  MATRIX_HEIGHT / 2 + random(0, 3);
                }
            }
            else
            {
                // set bat target to ball end point with some randomness
                bat2_target_y = end_ball_y - random(0, BAT_HEIGHT);
                if (bat2_target_y < 0)
                    bat2_target_y = 0;
                if (bat2_target_y > MATRIX_HEIGHT - BAT_HEIGHT)
                    bat2_target_y = MATRIX_HEIGHT - BAT_HEIGHT;
            }
        }

        // move bat 1 towards target
        // if bat y greater than target y move down until hit 0 (don't go any further or bat will move off screen)
        if (bat1_y > bat1_target_y && bat1_y > 0)
        {
            bat1_y--;
            bat1_update = 1;
        }

        // if bat y less than target y move up until hit 10 (as bat is 6)
        if (bat1_y < bat1_target_y && bat1_y < MATRIX_HEIGHT - BAT_HEIGHT)
        {
            bat1_y++;
            bat1_update = 1;
        }

        // draw bat 1
        //       if (bat1_update) {
        LEDMatrixGFX::backgroundLayer.fillRectangle(BAT1_X - 1, bat1_y, BAT1_X, bat1_y + BAT_HEIGHT, rgb24(255,255,255));
        //      }

        // move bat 2 towards target (don't go any further or bat will move off screen)
        // if bat y greater than target y move down until hit 0
        if (bat2_y > bat2_target_y && bat2_y > 0)
        {
            bat2_y--;
            bat2_update = 1;
        }

        // if bat y less than target y move up until hit max of 10 (as bat is 6)
        if (bat2_y < bat2_target_y && bat2_y < MATRIX_HEIGHT - 6)
        {
            bat2_y++;
            bat2_update = 1;
        }

        LEDMatrixGFX::backgroundLayer.fillRectangle(BAT2_X + 1, bat2_y, BAT2_X + 2, bat2_y +BAT_HEIGHT, rgb24(255,255,255));

        // update the ball position using the velocity
        ballpos_x = ballpos_x + ballvel_x;
        ballpos_y = ballpos_y + ballvel_y;

        // check ball collision with top and bottom of screen and reverse the y velocity if either is hit
        if (ballpos_y <= 0)
        {
            ballpos_y *= -1;
            ballvel_y *= -1;
        }

        if (ballpos_y >= MATRIX_HEIGHT)
        {
            ballpos_y = 2 * MATRIX_HEIGHT - ballpos_y;
            ballvel_y *= -1;
        }

        // check for ball collision with bat1. check ballx is same as batx
        // and also check if bally lies within width of bat i.e. baty to baty + 6. We can use the exp if(a < b && b < c)
        if ((int)ballpos_x <= BAT1_X + 1 && (bat1_y <= (int)ballpos_y && (int)ballpos_y <= bat1_y + BAT_HEIGHT - 1))
        {

            ballpos_x = BAT1_X + 1;

            // random if bat flicks ball to return it - and therefor changes ball velocity
            if (!random(0, 3))
            { // not true = no flick - just straight rebound and no change to ball y vel
                ballvel_x = ballvel_x * -SPEEDUP;
                ballvel_x = std::max(ballvel_x, -MAXSPEED);
                ballvel_x = std::min(ballvel_x,  MAXSPEED);
            }
            else
            {
                bat1_update = 1;
                uint8_t flick; // 0 = up, 1 = down.

                if (bat1_y > 1 || bat1_y < MATRIX_HEIGHT / 2)
                {
                    flick = random(0, 2); // pick a random dir to flick - up or down
                }

                // if bat 1 or 2 away from top only flick down
                if (bat1_y <= 1)
                {
                    flick = 0; // move bat down 1 or 2 pixels
                }
                // if bat 1 or 2 away from bottom only flick up
                if (bat1_y >= MATRIX_HEIGHT / 2)
                {
                    flick = 1; // move bat up 1 or 2 pixels
                }

                switch (flick)
                {
                // flick up
                case 0:
                    bat1_target_y = bat1_target_y + random(1, 3);
                    ballvel_x = ballvel_x * -1;
                    if (ballvel_y < 2)
                    {
                        ballvel_y = ballvel_y + 0.5;
                    }
                    break;

                    // flick down
                case 1:
                    bat1_target_y = bat1_target_y - random(1, 3);
                    ballvel_x = ballvel_x * -1;
                    if (ballvel_y > 0.5)
                    {
                        ballvel_y = ballvel_y - 0.5;
                    }
                    break;
                }
            }
        }

        // check for ball collision with bat2. check ballx is same as batx
        // and also check if bally lies within width of bat i.e. baty to baty + 6. We can use the exp if(a < b && b < c)
        if ((int)ballpos_x >= BAT2_X && (bat2_y <= (int)ballpos_y && (int)ballpos_y <= bat2_y + BAT_HEIGHT - 1))
        {

            ballpos_x = BAT2_X;

            // random if bat flicks ball to return it - and therefor changes ball velocity
            if (!random(0, 3))
            {
                ballvel_x = ballvel_x * -SPEEDUP; // not true = no flick - just straight rebound and no change to ball y vel
                ballvel_x = std::max(ballvel_x, -MAXSPEED);
                ballvel_x = std::min(ballvel_x,  MAXSPEED);
            }
            else
            {
                bat1_update = 1;
                uint8_t flick; // 0 = up, 1 = down.

                if (bat2_y > 1 || bat2_y < MATRIX_HEIGHT / 2)
                    flick = random(0, 2); // pick a random dir to flick - up or down

                // if bat 1 or 2 away from top only flick down

                if (bat2_y <= 1)
                    flick = 0; // move bat up 1 or 2 pixels

                // if bat 1 or 2 away from bottom only flick up

                if (bat2_y >= MATRIX_HEIGHT / 2)
                    flick = 1; // move bat down 1 or 2 pixels

                switch (flick)
                {
                // flick up
                case 0:
                    bat2_target_y = bat2_target_y + random(1, 3);
                    ballvel_x = ballvel_x * -1;
                    if (ballvel_y < 2)
                        ballvel_y = ballvel_y + random(1.0) + 0.5;
                    break;

                    // flick down
                case 1:
                    bat2_target_y = bat2_target_y - random(1, 3);
                    ballvel_x = ballvel_x * -1;
                    if (ballvel_y > 0.5)
                        ballvel_y = ballvel_y - random(1.0) - 0.5;
                    break;
                }
            }
        }

        // plot the ball on the screen
        uint8_t plot_x = (int)(ballpos_x + 0.5f);
        uint8_t plot_y = (int)(ballpos_y + 0.5f);

        g->setPixel(plot_x, plot_y, WHITE16);

        // check if a bat missed the ball. if it did, reset the game.
        if (ballpos_x < 0 || ballpos_x > MATRIX_WIDTH)
        {
            restart = 1;

            // update score / time
            bool ampm = !g_ptrDeviceConfig->Use24HourClock();

            mins = local_time->tm_min;
            hours = local_time->tm_hour;
            if (hours > 12 && ampm)
                hours -= 12;
            else if (hours == 0 && ampm)
                hours = 12;
        }
    }

    float pong_get_ball_endpoint(float xpos, float ypos, float xspeed, float yspeed)
    {
        // In the following, the fabs() mirrors it over the bottom wall.  The fmod wraps it when it exceeds twice
        // the top wall.  If the ball ends up in the top half of the double height section, we reflect it back
        //
        // auto deltaX = (xspeed > 0) ? (BAT2_X - xpos) : -(xpos - BAT1_X);        // How far from ball to opponent bat
        // auto slope = yspeed / xspeed;                                           // Rise over run, ie: deltaY per X
        // float newY = fmod(fabs(ypos + deltaX * slope), (2 * MATRIX_HEIGHT));    // New Y, but wrappped every 2*height
        //
        // if (newY > MATRIX_HEIGHT)                                               // If in top half, reflect to bottom
        //    newY = 2 * MATRIX_HEIGHT - newY;
        // return newY;
        auto deltaX = (xspeed > 0) ? (BAT2_X - xpos) : -(xpos - BAT1_X);        // How far from ball to opponent bat
        auto slope = yspeed / xspeed;                                           // Rise over run, ie: deltaY per X
        float newY = fmod(fabs(ypos + deltaX * slope), (2 * MATRIX_HEIGHT));    // New Y, but wrappped every 2*height
        if (newY > MATRIX_HEIGHT)                                               // If in top half, reflect to bottom
            newY = 2 * MATRIX_HEIGHT - newY;
        return newY;
    }
};

#endif