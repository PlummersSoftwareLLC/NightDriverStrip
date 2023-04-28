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
    virtual bool DeserializeFromJSON(const JsonObjectConst& jsonObject) { return false; }
};

template <class E>
constexpr auto to_value(E e) noexcept
{
	return static_cast<std::underlying_type_t<E>>(e);
}

namespace ArduinoJson
{
    template <>
    struct Converter<CRGB>
    {
        static bool toJson(const CRGB& color, JsonVariant dst)
        {
            return dst.set((uint32_t)((color.r << 16) | (color.g << 8) | color.b));
        }

        static CRGB fromJson(JsonVariantConst src)
        {
            return CRGB(src.as<uint32_t>());
        }

        static bool checkJson(JsonVariantConst src)
        {
            return src.is<uint32_t>();
        }
    };

    template <>
    struct Converter<CRGBPalette16>
    {
        static bool toJson(const CRGBPalette16& palette, JsonVariant dst)
        {
            StaticJsonDocument<384> doc;

            JsonArray colors = doc.to<JsonArray>();

            for (auto& color: palette.entries)
                colors.add(color);

            return dst.set(doc);
        }

        static CRGBPalette16 fromJson(JsonVariantConst src)
        {
            CRGB colors[16];
            int colorIndex = 0;

            JsonArrayConst componentsArray = src.as<JsonArrayConst>();
            for (JsonVariantConst value : componentsArray)
                colors[colorIndex++] = value.as<CRGB>();

            return CRGBPalette16(colors);
        }

        static bool checkJson(JsonVariantConst src)
        {
            return src.is<JsonArrayConst>() && src.as<JsonArrayConst>().size() == 16;
        }
    };
}

bool LoadJSONFile(const char *fileName, size_t& bufferSize, std::unique_ptr<DynamicJsonDocument>& pJsonDoc);
bool SaveToJSONFile(const char *fileName, size_t& bufferSize, IJSONSerializable& object);
bool RemoveJSONFile(const char *fileName);

