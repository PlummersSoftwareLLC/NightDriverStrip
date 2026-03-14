#pragma once

//+--------------------------------------------------------------------------
//
// File:        types.h
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
//    Types of a somewhat general use
//
// History:     May-23-2023         Rbergen      Created
//
//---------------------------------------------------------------------------

#include "globals.h"
#include "interfaces.h"

#include <cstddef>
#include <cstdint>
#include <memory>
#include <optional>
#include <sys/time.h>
#include <type_traits>

#ifndef MICROS_PER_SECOND
    #define MICROS_PER_SECOND 1000000
#endif

// str_snprintf
//
// va-args style printf that returns the formatted string as a result

// Let compiler warn if our arguments don't match.
String str_sprintf(const char *fmt, ...) __attribute__((format(printf, 1, 2)));

// AppTime
//
// A class that keeps track of the clock, how long the last frame took, calculating FPS, etc.

class CAppTime
{
  protected:

    double _lastFrame;
    double _deltaTime;

  public:

    // NewFrame
    //
    // Call this at the start of every frame or update, and it'll figure out and keep track of how
    // long between frames
    void NewFrame();

    CAppTime();

    double FrameStartTime() const;

    static double CurrentTime();

    double FrameElapsedTime() const;

    static double TimeFromTimeval(const timeval & tv);

    static timeval TimevalFromTime(double t);

    double LastFrameTime() const;
};

struct EmbeddedFile
{
    // Embedded file size in bytes
    const size_t length;
    // Contents as bytes
    const uint8_t *const contents;

    EmbeddedFile(const uint8_t start[], const uint8_t end[]) :
        length(end - start),
        contents(start)
    {}
};
