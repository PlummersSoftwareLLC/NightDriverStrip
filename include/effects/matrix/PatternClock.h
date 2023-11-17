//+--------------------------------------------------------------------------
//
// File:        PatternClock.h
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
// Description:
//
//   Effect code to draw an analog clock
//
// History:     Aug-01-2022         Davepl      For David S.
//
//---------------------------------------------------------------------------

#ifndef PatternClock_H
#define PatternClock_H

// Description:
//
// This file defines the PatternClock class, a subclass of LEDStripEffect.
// The class is designed to render a clock effect on an LED matrix. It 
// includes functionality to display time with hour, minute, and second 
// hands, along with tick marks for each hour. The clock's appearance and 
// behavior are customizable through various methods.

class PatternClock : public LEDStripEffect
{
    // Radius is the lesser of the height and width so that the round clock can fit
    // on rectangular display

    float    radius;

  public:

    PatternClock() : LEDStripEffect(EFFECT_MATRIX_CLOCK, "Clock")
    {
    }

    PatternClock(const JsonObjectConst& jsonObject) : LEDStripEffect(jsonObject)
    {
    }

    bool RequiresDoubleBuffering() const override
    {
        return false;
    }

    virtual size_t DesiredFramesPerSecond() const override
    {
        return 60;
    }

    void Draw() override
    {
        // Get the hours, minutes, and seconds of hte current time

        time_t currentTime;
        struct tm *localTime;
        time( &currentTime );
        localTime = localtime( &currentTime );
        auto hours   = localTime->tm_hour;
        auto minutes = localTime->tm_min;
        auto seconds = localTime->tm_sec;

        timeval tv;
        gettimeofday(&tv, nullptr);
        auto sixtieths = tv.tv_usec * 60 / 1000000;

        // Draw the clock face, outer ring and inner dot where the hands mount

        radius = std::min(MATRIX_WIDTH, MATRIX_HEIGHT) / 2 - 0.5;

        g()->Clear();
        g()->DrawSafeCircle(MATRIX_WIDTH/2, MATRIX_HEIGHT/2, 1, CRGB::Blue);

        // Draw the hour ticks around the outside of the clock every 30 degrees

        for (int z = 0; z < 360; z = z + 30)
        {
            // Begin at 0° and stop before 360°
            float angle = z;
            angle = (angle / 57.29577951); // Convert degrees to radians
            int x2 = (MATRIX_CENTER_X + round((sin(angle) * (radius - 4))));         // Extra 0.5 helps rounding land more evenly
            int y2 = (MATRIX_CENTER_Y - round(cos(angle) * (radius - 4)));
            int x3 = (MATRIX_CENTER_X + round((sin(angle) * (radius - 1))));
            int y3 = (MATRIX_CENTER_Y - round(cos(angle) * (radius - 1)));
            g()->drawLine(x2, y2, x3, y3, CRGB::Red);
        }

        g()->DrawSafeCircle(MATRIX_WIDTH/2, MATRIX_HEIGHT/2, radius, CRGB::Blue);
        g()->DrawSafeCircle(MATRIX_WIDTH/2, MATRIX_HEIGHT/2, radius+1, CRGB::Green);

        // Draw the second hand

        float angle = seconds * 6;
        angle = (angle / 57.29577951); // Convert degrees to radians
        int x3 = (MATRIX_CENTER_X + round(sin(angle) * (radius - 2)));
        int y3 = (MATRIX_CENTER_Y - round(cos(angle) * (radius - 2)));
        g()->drawLine(MATRIX_CENTER_X, MATRIX_CENTER_Y, x3, y3, CRGB::White);

        // Draw the minute hand

        angle = minutes * 6;
        angle = (angle / 57.29577951); // Convert degrees to radians
        x3 = (MATRIX_CENTER_X + round(sin(angle) * (radius - 3)));
        y3 = (MATRIX_CENTER_Y - round(cos(angle) * (radius - 3)));
        g()->drawLine(MATRIX_CENTER_X, MATRIX_CENTER_Y, x3, y3, CRGB::Yellow);

        // Draw the  hour hand

        angle = hours * 30 + int((minutes / 12) * 6);
        angle = (angle / 57.29577951); // Convert degrees to radians
        x3 = (MATRIX_CENTER_X + round(sin(angle) * (radius / 2 )));
        y3 = (MATRIX_CENTER_Y - round(cos(angle) * (radius / 2 )));
        g()->drawLine(MATRIX_CENTER_X, MATRIX_CENTER_Y, x3, y3, CRGB::Yellow);

        // Draw the sixtieths pixel

        angle = sixtieths * 6;
        angle = (angle / 57.29577951); // Convert degrees to radians
        int x2 = (MATRIX_CENTER_X + round((sin(angle) * (radius - 1))));         // Extra 0.5 helps rounding land more evenly
        int y2 = (MATRIX_CENTER_Y - round(cos(angle) * (radius - 1)));
        x3 = (MATRIX_CENTER_X + round((sin(angle) * (radius))));
        y3 = (MATRIX_CENTER_Y - round(cos(angle) * (radius)));
        g()->drawLine(x2, y2, x3, y3, CRGB::White);

    }
};

#endif
