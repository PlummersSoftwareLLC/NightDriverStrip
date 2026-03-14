#pragma once

//+--------------------------------------------------------------------------
//
// File:        range_utils.h
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
//    Range helper utilities.
//

#include "globals.h"

#include <iterator>
#include <type_traits>

// Provide a single-parameter std::accumulate overload for ranges/containers
// This allows: auto total = accumulate(container);

template <typename Range>
inline auto accumulate(const Range& r)
    -> std::remove_cv_t<std::remove_reference_t<decltype(*std::begin(r))>>
{
    using T = std::remove_cv_t<std::remove_reference_t<decltype(*std::begin(r))>>;
    T total{};
    for (const auto& v : r)
        total += v;
    return total;
}
