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

// Validates the consistency of the spec's contents after all fields have been assigned.
// Called by Construct(); can also be called directly if needed.
void SettingSpec::FinishAndValidateInitialization()
{
    // Default to front-end rejection of empty Strings, but only if the caller hasn't already
    // set EmptyAllowed explicitly.
    if (Type == SettingType::String && !EmptyAllowed.has_value())
        EmptyAllowed = false;

    // If min and max value are both set, min must be less or equal than max
    assert(!(MinimumValue.has_value() && MaximumValue.has_value()) || MinimumValue.value() <= MaximumValue.value());

    // For Slider widgets, display scale members must all be set or all be unset
    assert(Widget != WidgetKind::Slider || (DisplayRawMin.has_value() == DisplayRawMax.has_value() &&
           DisplayRawMin.has_value() == DisplayMin.has_value() &&
           DisplayRawMin.has_value() == DisplayMax.has_value()));

    // For Select widgets, validate options source-specific requirements
    if (Widget == WidgetKind::Select)
    {
        // For Inline select options, labels must be empty or match the number of values
        assert(Options != OptionsSource::Inline ||
               OptionLabels.empty() || OptionLabels.size() == OptionValues.size());

        // For SchemaPath select options, OptionsSchemaPath must be set, and
        // any label overrides must be provided as matched pairs (both non-empty, same length)
        assert(Options != OptionsSource::SchemaPath || OptionsSchemaPath != nullptr);
        assert(Options != OptionsSource::SchemaPath ||
               (OptionValues.empty() == OptionLabels.empty() &&
                (OptionValues.empty() || OptionValues.size() == OptionLabels.size())));

        // For ExternalTimeZones select options, OptionsExternalUrl must be set, and
        // any label overrides must be provided as matched pairs (both non-empty, same length)
        assert(Options != OptionsSource::ExternalTimeZones || OptionsExternalUrl != nullptr);
        assert(Options != OptionsSource::ExternalTimeZones ||
               (OptionValues.empty() == OptionLabels.empty() &&
                (OptionValues.empty() || OptionValues.size() == OptionLabels.size())));
    }
}

SettingSpec SettingSpec::Validate(SettingSpec spec)
{
    spec.FinishAndValidateInitialization();
    return spec;
}

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
        default:                                return "Unknown";
    }
}

const char* SettingSpec::WidgetName() const
{
    switch (Widget)
    {
        case WidgetKind::Slider:           return "slider";
        case WidgetKind::Select:           return "select";
        case WidgetKind::IntervalToggle:   return "intervalToggle";
        default:                           return "default";
    }
}

const char* SettingSpec::OptionsSourceName() const
{
    switch (Options)
    {
        case OptionsSource::Inline:             return "inline";
        case OptionsSource::SchemaPath:         return "schemaPath";
        case OptionsSource::IntlCountryCodes:   return "intlCountryCodes";
        case OptionsSource::ExternalTimeZones:  return "externalTimeZones";
        default:                                return "inline";
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
