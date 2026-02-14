#pragma once

//+--------------------------------------------------------------------------
//
// File:        array_utils.h
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
//    Array helper utilities.
//

#include "globals.h"

#include <array>
#include <cstddef>

// Because the ESP32 compiler, as of this writing, doesn't have std::to_array, we provide our own (davepl).
// BUGBUG: Once we have compiler support we should use the C++20 versions

template <typename T, std::size_t N>
constexpr std::array<T, N> to_array(const T (&arr)[N])
{
    std::array<T, N> result{};
    for (std::size_t i = 0; i < N; ++i)
    {
        result[i] = arr[i];
    }
    return result;
}
