//+--------------------------------------------------------------------------
//
// File:        types.h
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
//    Types of a somewhat general use
//
// History:     May-23-2023         Rbergen      Created
//
//---------------------------------------------------------------------------

#pragma once

#include <cstddef>
#include <cstdint>
#include <sys/time.h>
#include <optional>
#include <WString.h>

#ifndef MICROS_PER_SECOND
    #define MICROS_PER_SECOND 1000000
#endif

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
    // Call this at the start of every frame or udpate, and it'll figure out and keep track of how
    // long between frames

    void NewFrame()
    {
        timeval tv;
        gettimeofday(&tv, nullptr);
        double current = CurrentTime();
        _deltaTime = current - _lastFrame;

        // Cap the delta time at one full second

        if (_deltaTime > 1.0)
            _deltaTime = 1.0;

        _lastFrame = current;
    }

    CAppTime() : _lastFrame(CurrentTime())
    {
        NewFrame();
    }

    double FrameStartTime() const
    {
        return _lastFrame;
    }

    static double CurrentTime()
    {
        timeval tv;
        gettimeofday(&tv, nullptr);
        return (double)tv.tv_sec + (tv.tv_usec/(double)MICROS_PER_SECOND);
    }

    double FrameElapsedTime() const
    {
        return FrameStartTime() - CurrentTime();
    }

    static double TimeFromTimeval(const timeval & tv)
    {
        return tv.tv_sec + (tv.tv_usec/(double)MICROS_PER_SECOND);
    }

    static timeval TimevalFromTime(double t)
    {
        timeval tv;
        tv.tv_sec = (long)t;
        tv.tv_usec = t - tv.tv_sec;
        return tv;
    }

    double LastFrameTime() const
    {
        return _deltaTime;
    }
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

struct SettingSpec
{
    // Note that if this enum is expanded, TypeName() must be also!
    enum class SettingType : int
    {
        Integer,
        PositiveBigInteger,
        Float,
        Boolean,
        String,
        Palette,
        Color
    };

    const char* Name;
    const char* FriendlyName;
    const char* Description;
    SettingType Type;
    bool HasValidation = false;
    std::optional<double> MinimumValue = {};
    std::optional<double> MaximumValue = {};


    SettingSpec(const char* name, const char* friendlyName, const char* description, SettingType type)
      : Name(name),
        FriendlyName(friendlyName),
        Description(description),
        Type(type)
    {}

    SettingSpec(const char* name, const char* friendlyName, SettingType type) : SettingSpec(name, friendlyName, nullptr, type)
    {}

    // Constructor that sets both mininum and maximum values
    SettingSpec(const char* name, const char* friendlyName, const char* description, SettingType type, double min, double max)
      : SettingSpec(name, friendlyName, description, type)
    {
        MinimumValue = min;
        MaximumValue = max;
    }

    // Constructor that sets both mininum and maximum values
    SettingSpec(const char* name, const char* friendlyName, SettingType type, double min, double max)
      : SettingSpec(name, friendlyName, nullptr, type, min, max)
    {}

    SettingSpec()
    {}

    virtual String TypeName() const
    {
        String names[] = { "Integer", "PositiveBigInteger", "Float", "Boolean", "String", "Palette", "Color" };
        return names[static_cast<int>(Type)];
    }
};
