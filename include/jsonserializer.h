//+--------------------------------------------------------------------------
//
// File:        jsonserializer.h
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
//    Declares classes for JSON (de)serialization of selected 
//    properties of selected classes
//
// History:     Mar-29-2023         Rbergen      Created for NightDriverStrip
//              
//---------------------------------------------------------------------------

#pragma once

#include <ArduinoJson.h>
#include "jsonbase.h"
#include "FastLED.h"

struct IJSONSerializable
{
    virtual bool SerializeToJSON(JsonObject& jsonObject) = 0;
};

template <class E>
constexpr auto to_value(E e) noexcept
{
	return static_cast<std::underlying_type_t<E>>(e);
}

namespace JSONSerializer 
{
    bool SerializeToJSON(JsonObject& jsonObject, CRGBPalette16 palette) 
    {        
        StaticJsonDocument<512> doc;
 
        JsonArray colors = doc.createNestedArray("cs");

        for (auto& color: palette.entries)
            colors.add(ToUInt32(color));
    
        return jsonObject.set(doc.as<JsonObjectConst>());
    }

    CRGBPalette16 DeserializeCRGBPalette16FromJSON(const JsonObjectConst&  jsonObject) 
    {
        CRGB colors[16];

        if (jsonObject.containsKey("cs"))
        {
            int colorIndex = 0;

            JsonArray componentsArray = jsonObject["cs"].as<JsonArray>();
            for (JsonVariant value : componentsArray) 
            {
                colors[colorIndex] = CRGB(value.as<uint32_t>());
            
                if (++colorIndex > 15)
                    break;
            }
        }

        return CRGBPalette16(colors); 
    }

    uint32_t ToUInt32(CRGB color) 
    {
        return (color.r << 16) | (color.g << 8) | color.b;
    }
}