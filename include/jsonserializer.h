#pragma once

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

#include "globals.h"

#include <ArduinoJson.h>
#include <atomic>
#include <memory>
#include <mutex>
#include <vector>

#include "itaskservice.h"

template <class E>
constexpr auto to_value(E e) noexcept
{
	return static_cast<std::underlying_type_t<E>>(e);
}

JsonDocument CreateJsonDocument();

bool SetIfNotOverflowed(JsonDocument& jsonDoc, JsonObject& jsonObject, const char* location = nullptr);

uint32_t toUint32(const CRGB& color);

namespace ArduinoJson
{
    template <>
    struct Converter<CRGB>
    {
        static bool toJson(const CRGB& color, JsonVariant dst);
        static CRGB fromJson(JsonVariantConst src);
        static bool checkJson(JsonVariantConst src);
    };

    inline bool canConvertFromJson(JsonVariantConst src, const CRGB&)
    {
        return Converter<CRGB>::checkJson(src);
    }

    template <>
    struct Converter<CRGBPalette16>
    {
        static bool toJson(const CRGBPalette16& palette, JsonVariant dst);
        static CRGBPalette16 fromJson(JsonVariantConst src);
        static bool checkJson(JsonVariantConst src);
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

// JSONWriter
//
// Background SPIFFS writer. Implementers register a write callback once and
// call FlagWriter / FlushWrites when something has been mutated; the writer
// task batches flagged writes through JSON_WRITER_DELAY of quiet time so
// rapid mutations don't thrash the flash. Inherits ITaskService so launch
// and shutdown discipline are shared with the other task-owning services.

class JSONWriter : public ITaskService
{
  private:

    struct WriterEntry;

    std::vector<std::shared_ptr<WriterEntry>> writers;
    mutable std::mutex       writersMutex;
    std::atomic_ulong        latestFlagMs{0};
    std::atomic_bool         flushRequested{false};
    std::atomic_bool         haltWrites{false};

    // Wakes the writer task from its long ulTaskNotifyTake. Used both by
    // FlagWriter/FlushWrites (the public API) and by Stop() (via
    // OnBeforeWaitForStop) to break the task out of its blocking wait.
    
    void NotifyTask();

  public:
    JSONWriter();
    ~JSONWriter() override;

    // IService::Name
    const char* Name() const override { return "JSONWriter"; }

    // Add a writer to the collection. Returns the index of the added writer, for use with FlagWriter()
    size_t RegisterWriter(const std::function<void()>& writer);

    // Flag a writer for invocation and wake up the task that calls them
    void FlagWriter(size_t index);

    // Flush pending writes now
    void FlushWrites(bool halt = false);

  protected:
    // ITaskService hooks
    TaskConfig GetTaskConfig() const override;
    void Run() override;
    void OnBeforeWaitForStop() override;
};
