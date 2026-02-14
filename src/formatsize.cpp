//+--------------------------------------------------------------------------
//
// File:        formatsize.cpp
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
//    String formatting helpers for human-friendly size strings.
//
//---------------------------------------------------------------------------

#include "globals.h"

#include <iomanip>
#include <sstream>

String formatSize(size_t size, size_t threshold)
{
    // If the size is above the threshold, we want a precision of 2 to show more accurate value
    const int precision = size < threshold ? 0 : 2;

    const char* suffixes[] = {"", "K", "M", "G", "T", "P", "E", "Z"};
    size_t suffixIndex = 0;
    double sizeDouble = static_cast<double>(size);

    while (sizeDouble >= threshold && suffixIndex < (sizeof(suffixes) / sizeof(suffixes[0])) - 1)
    {
        sizeDouble /= 1000;
        ++suffixIndex;
    }

    std::ostringstream oss;
    oss << std::fixed << std::setprecision(precision) << sizeDouble << suffixes[suffixIndex];
    std::string result = oss.str();
    return String(result.c_str());
}
