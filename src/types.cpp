//+--------------------------------------------------------------------------
//
// File:        types.cpp
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
#include "types.h"

#include <cassert>
#include <sys/time.h>

// AppTime
//
// A class that keeps track of the clock, how long the last frame took, calculating FPS, etc.

// NewFrame
//
// Call this at the start of every frame or update, and it'll figure out and keep track of how
// long between frames
void CAppTime::NewFrame()
{
    double current = CurrentTime();
    _deltaTime = current - _lastFrame;

    // Cap the delta time at one full second

    if (_deltaTime > 1.0)
        _deltaTime = 1.0;

    _lastFrame = current;
}

CAppTime::CAppTime()
{
    _lastFrame = CurrentTime();
    _deltaTime = 0;
}

double CAppTime::FrameStartTime() const
{
    return _lastFrame;
}

double CAppTime::CurrentTime()
{
    timeval tv;
    gettimeofday(&tv, nullptr);
    return TimeFromTimeval(tv);
}

double CAppTime::FrameElapsedTime() const
{
    return CurrentTime() - _lastFrame;
}

double CAppTime::TimeFromTimeval(const timeval & tv)
{
    return tv.tv_sec + (tv.tv_usec/(double)MICROS_PER_SECOND);
}

timeval CAppTime::TimevalFromTime(double t)
{
    timeval tv;
    tv.tv_sec = (long)t;
    tv.tv_usec = t - tv.tv_sec;
    return tv;
}

double CAppTime::LastFrameTime() const
{
    return _deltaTime;
}

// Finishes the initialization of the spec, and then validates the consistency of its overall contents.
// Note that it does the latter quite rudely: it uses assert() on things it feels should be in order.
// This function is called by this struct's constructors that initialize values, but this being a struct
// allows itself to be called from the outside as well.
void SettingSpec::FinishAndValidateInitialization()
{
    // Default to front-end rejection of empty Strings
    if (Type == SettingType::String)
        EmptyAllowed = false;
    else
        // Check that both min and max value are set for Slider
        assert(Type != SettingType::Slider || (MinimumValue.has_value() && MaximumValue.has_value()));

    // If min and max value are both set, min must be less or equal than max
    assert(!(MinimumValue.has_value() && MaximumValue.has_value()) || MinimumValue.value() <= MaximumValue.value());
}

SettingSpec::SettingSpec(const char* name, const char* friendlyName, const char* description, SettingType type)
    : Name(name),
    FriendlyName(friendlyName),
    Description(description),
    Type(type)
{
    FinishAndValidateInitialization();
}

SettingSpec::SettingSpec(const char* name, const char* friendlyName, SettingType type) : SettingSpec(name, friendlyName, nullptr, type)
{}

// Constructor that sets both minimum and maximum values
SettingSpec::SettingSpec(const char* name, const char* friendlyName, const char* description, SettingType type, double min, double max)
    : Name(name),
    FriendlyName(friendlyName),
    Description(description),
    Type(type),
    MinimumValue(min),
    MaximumValue(max)
{
    FinishAndValidateInitialization();
}

// Constructor that sets both minimum and maximum values
SettingSpec::SettingSpec(const char* name, const char* friendlyName, SettingType type, double min, double max)
    : SettingSpec(name, friendlyName, nullptr, type, min, max)
{}

String SettingSpec::TypeName() const
{
    switch (Type)
    {
        case SettingType::Integer:              return "Integer";
        case SettingType::PositiveBigInteger:   return "PositiveBigInteger";
        case SettingType::Float:                return "Float";
        case SettingType::Boolean:              return "Boolean";
        case SettingType::String:               return "String";
        case SettingType::Palette:              return "Palette";
        case SettingType::Color:                return "Color";
        case SettingType::Slider:               return "Slider";
        default:                                return "Unknown";
    }
}

// PreferPSRAMAlloc
//
// Will return PSRAM if it's available, regular ram otherwise

// Cache PSRAM availability and prefer it when allocating large buffers.
void * PreferPSRAMAlloc(size_t s)
{
    // Compute PSRAM availability once in a thread-safe way (I believe C++11+ guarantees thread-safe initialization of function-local statics).
    static const int s_psramAvailable = []() noexcept -> int {
        return psramInit() ? 1 : 0;
    }();

    if (s_psramAvailable)
    {
        debugV("PSRAM Array Request for %zu bytes\n", s);
        auto p = ps_malloc(s);
        if (!p)
        {
            debugE("PSRAM Allocation failed for %zu bytes\n", s);
            throw std::bad_alloc();
        }
        return p;
    }
    else
    {
        auto p = malloc(s);
        if (!p)
        {
            debugE("RAM Allocation failed for %zu bytes\n", s);
            throw std::bad_alloc();
        }
        return p;
    }
}
