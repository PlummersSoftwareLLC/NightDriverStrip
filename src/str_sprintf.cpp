//+--------------------------------------------------------------------------
//
// File:        str_sprintf.cpp
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
// Provide a missing Arduino String::sprintf.
// Threadsafe because retval is local and RVO for the string.
// an alloc will necessarily be taken for the string, so
// don't use this in an interrupt context.
//


#include "globals.h"

#include <cstdarg>
#include <cstdio>
#include <memory>

String str_sprintf(const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);

    va_list args_copy;
    va_copy(args_copy, args);

    int requiredLen = vsnprintf(nullptr, 0, fmt, args) + 1;
    va_end(args);

    if (requiredLen <= 0) {
        va_end(args_copy);
        return {};
    }

    std::unique_ptr<char []> str = std::make_unique<char []>(requiredLen);
    vsnprintf(str.get(), requiredLen, fmt, args_copy);
    va_end(args_copy);

    String retval;
    retval.reserve(requiredLen);

    retval = str.get();
    return retval;
}
