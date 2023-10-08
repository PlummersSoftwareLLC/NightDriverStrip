//+--------------------------------------------------------------------------
//
// File:        values.h
//
// NightDriverStrip - (c) 2023 Plummer's Software LLC.  All Rights Reserved.
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
//    Struct with values of a somewhat general use
//
// History:     Aug-08-2023         Rbergen      Added header
//
//---------------------------------------------------------------------------

#pragma once

#include <esp_attr.h>
#include "globals.h"
#include "types.h"

// Struct with global values that are not persisted as settings - those reside in DeviceConfig
struct Values
{
    CAppTime AppTime;                                                       // Keeps track of frame times
    volatile double FreeDrawTime = 0.0;
    float Brite;
    uint32_t Watts;
    uint32_t FPS = 0;                                                       // Our global framerate
    bool UpdateStarted = false;                                             // Has an OTA update started?
    uint8_t Fader = 255;
#if USE_HUB75
    int MatrixPowerMilliwatts = 0;                                         // Matrix power draw in mw
    uint8_t MatrixScaledBrightness = 255;                                  // 0-255 scaled brightness to stay in limit
#endif
};

extern DRAM_ATTR Values g_Values;