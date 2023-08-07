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

class PatternClock : public LEDStripEffect
{
    // Radius is the lesser of the height and width so that the round clock can fit
    // on rectangular display

    int    radius;

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

        // Draw the clock face, outer ring and inner dot where the hands mount

        radius = std::min(MATRIX_WIDTH, MATRIX_HEIGHT) / 2 - 1;

        g()->Clear();
        g()->DrawSafeCircle(MATRIX_WIDTH/2, MATRIX_HEIGHT/2, 1, GREEN16);

        // Draw the hour ticks around the outside of the clock every 30 degrees

        for (int z = 0; z < 360; z = z + 30)
        {
            // Begin at 0° and stop before 360°
            float angle = z;
            angle = (angle / 57.29577951); // Convert degrees to radians
            int x2 = (MATRIX_CENTER_X + (sin(angle) * radius));
            int y2 = (MATRIX_CENTER_Y - (cos(angle) * radius));
            int x3 = (MATRIX_CENTER_X + (sin(angle) * (radius - 2)));
            int y3 = (MATRIX_CENTER_Y - (cos(angle) * (radius - 2)));
            g()->drawLine(x2, y2, x3, y3, CRGB::Red);
        }

        // Draw the second hand

        float angle = seconds * 6;
        angle = (angle / 57.29577951); // Convert degrees to radians
        int x3 = (MATRIX_CENTER_X + (sin(angle) * (radius)));
        int y3 = (MATRIX_CENTER_Y - (cos(angle) * (radius)));
        g()->drawLine(MATRIX_CENTER_X, MATRIX_CENTER_Y, x3, y3, CRGB::White);

        // Draw the minute hand

        angle = minutes * 6;
        angle = (angle / 57.29577951); // Convert degrees to radians
        x3 = (MATRIX_CENTER_X + (sin(angle) * (radius - 3)));
        y3 = (MATRIX_CENTER_Y - (cos(angle) * (radius - 3)));
        g()->drawLine(MATRIX_CENTER_X, MATRIX_CENTER_Y, x3, y3, CRGB::Yellow);

        // Draw the  hour hand

        angle = hours * 30 + int((minutes / 12) * 6);
        angle = (angle / 57.29577951); // Convert degrees to radians
        x3 = (MATRIX_CENTER_X + (sin(angle) * (radius / 2 )));
        y3 = (MATRIX_CENTER_Y - (cos(angle) * (radius / 2 )));
        g()->drawLine(MATRIX_CENTER_X, MATRIX_CENTER_Y, x3, y3, CRGB::Yellow);

        g()->DrawSafeCircle(MATRIX_WIDTH/2, MATRIX_HEIGHT/2, radius+1, GREEN16);

    }
};

#endif
