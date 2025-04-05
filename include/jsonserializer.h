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

#include <atomic>
#include <utility>
#include <ArduinoJson.h>

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

#if USE_PSRAM

    struct JsonPsramAllocator : ArduinoJson::Allocator
    {
        void* allocate(size_t size) override 
        {
            return ps_malloc(size);
        }

        void deallocate(void* pointer) override 
        {
            free(pointer);
        }

        void* reallocate(void* ptr, size_t new_size) override {
            return ps_realloc(ptr, new_size);
        }
    };

    inline JsonDocument CreateJsonDocument()
    {
        static auto jsonPsramAllocator = JsonPsramAllocator();

        return JsonDocument(&jsonPsramAllocator);
    }

#else

    inline JsonDocument CreateJsonDocument()
    {
        return JsonDocument();
    }

#endif

uint32_t toUint32(const CRGB& color);

namespace ArduinoJson
{
    template <>
    struct Converter<CRGB>
    {
        static bool toJson(const CRGB& color, JsonVariant dst)
        {
            return dst.set(toUint32(color));
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

    inline bool canConvertFromJson(JsonVariantConst src, const CRGB&)
    {
        return Converter<CRGB>::checkJson(src);
    }
    
    template <>
    struct Converter<CRGBPalette16>
    {
        static bool toJson(const CRGBPalette16& palette, JsonVariant dst)
        {
            auto doc = CreateJsonDocument();

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

    inline bool canConvertFromJson(JsonVariantConst src, const CRGBPalette16&)
    {
        return Converter<CRGBPalette16>::checkJson(src);
    }
}

bool BoolFromText(const String& text);
bool LoadJSONFile(const String & fileName, JsonDocument& jsonDoc);
bool SaveToJSONFile(const String & fileName, IJSONSerializable& object);
bool RemoveJSONFile(const String & fileName);

#define JSON_WRITER_DELAY 3000

class JSONWriter
{
    // We allow the main JSON Writer task entry point function to access private members
    friend void IRAM_ATTR JSONWriterTaskEntry(void *);

  private:

    // Writer function and flag combo
    struct WriterEntry
    {
        std::atomic_bool flag = false;
        std::function<void()> writer;

        explicit WriterEntry(std::function<void()> writer) :
            writer(std::move(writer))
        {}

        WriterEntry(WriterEntry&& entry)  noexcept : WriterEntry(entry.writer)
        {}
    };

    std::vector<WriterEntry, psram_allocator<WriterEntry>> writers;
    std::atomic_ulong        latestFlagMs;
    std::atomic_bool         flushRequested;
    std::atomic_bool         haltWrites;

  public:

    // Add a writer to the collection. Returns the index of the added writer, for use with FlagWriter()
    size_t RegisterWriter(const std::function<void()>& writer);

    // Flag a writer for invocation and wake up the task that calls them
    void FlagWriter(size_t index);

    // Flush pending writes now
    void FlushWrites(bool halt = false);
};

