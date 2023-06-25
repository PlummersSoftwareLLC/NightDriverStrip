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
#include <WString.h>

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
    enum class LimitType : char
    {
        None    = 0,
        Minimum = 1,
        Maximum = 2,
        Both    = Minimum + Maximum
    };

    // Note that if this enum is expanded, ToName() must be also!
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
    LimitType LimitsProvided = LimitType::None;
    double MinimumValue;
    double MaximumValue;


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
        LimitsProvided = LimitType::Both;
    }

    // Constructor that sets both mininum and maximum values
    SettingSpec(const char* name, const char* friendlyName, SettingType type, double min, double max)
      : SettingSpec(name, friendlyName, nullptr, type, min, max)
    {}

    // Constructor that sets either Min or Max. Note that if LimitType::Both is passed for limitType, it will be interpreted as LimitType::Maximum.
    SettingSpec(const char* name, const char* friendlyName, const char* description, SettingType type, LimitType limitType, double limit)
      : SettingSpec(name, friendlyName, description, type)
    {
        if (limitType == LimitType::None)
            return;
        else if (static_cast<char>(limitType) & static_cast<char>(LimitType::Maximum))
        {
            LimitsProvided = LimitType::Maximum;
            MaximumValue = limit;
        }
        else
        {
            LimitsProvided = LimitType::Minimum;
            MinimumValue = limit;
        }
    }

    // Constructor that sets either Min or Max. Note that if LimitType::Both is passed for limitType, it will be interpreted as LimitType::Maximum.
    SettingSpec(const char* name, const char* friendlyName, SettingType type, LimitType limitType, double limit)
      : SettingSpec(name, friendlyName, nullptr, type, limitType, limit)
    {}

    SettingSpec()
    {}

    // This is virtual so we can dynamic_cast SettingSpec to its subclasses

    virtual String TypeName() const
    {
        String names[] = { "Integer", "PositiveBigInteger", "Float", "Boolean", "String", "Palette", "Color" };
        return names[static_cast<int>(Type)];
    }

    virtual bool HasLimit(LimitType type) const
    {
        auto charType = static_cast<char>(type);

        return static_cast<char>(LimitsProvided) & charType == charType;
    }
};
