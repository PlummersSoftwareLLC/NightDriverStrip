//+--------------------------------------------------------------------------
//
// File:        NTPTimeClient.h
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
//    Sets the system clock from the specified NTP Server
//
// History:     Jul-12-2018         Davepl      Created for BigBlueLCD
//              Oct-09-2018         Davepl      Copied to LEDWifi project
//---------------------------------------------------------------------------

#pragma once

#include <sys/cdefs.h>
#include <sys/time.h>
#include <time.h>
#include <WiFi.h>
#include <WiFiUdp.h>
#include <mutex>

// NTPTimeClient
//
// Basically, I took some really ancient NTP code that I had on hand that I knew
// worked and wrapped it in a class.  As expected, it works, but it could likely
// benefit from cleanup or even wholesale replacement.

class NTPTimeClient
{
    static bool        _bClockSet;
    static std::mutex  _clockMutex;

  public:

    NTPTimeClient()
    {
    }

    static inline bool HasClockBeenSet()
    {
        return _bClockSet;
    }

    static bool UpdateClockFromWeb(WiFiUDP * pUDP);

    static void ShowUptime();
};




