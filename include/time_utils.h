#pragma once

//+--------------------------------------------------------------------------
//
// File:        time_utils.h
//
// NightDriverStrip - (c) 2026 Plummer's Software LLC.  All Rights Reserved.
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
//    Time-related helpers.
//

#include "globals.h"

#include <cstdint>

constexpr uint32_t kMillisPerSecond = 1000;

inline int FPS(unsigned long duration, uint32_t perSecond = kMillisPerSecond)
{
    if (duration == 0)
        return 999;

    float fpsf = 1.0f / (duration / (float)perSecond);
    int FPS = (int)fpsf;
    if (FPS > 999)
        FPS = 999;
    return FPS;
}
