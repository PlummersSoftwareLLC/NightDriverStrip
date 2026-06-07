//+--------------------------------------------------------------------------
//
// File:        jsonserializer.cpp
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
//   Implementations for some of the functions declared in jsonserializer.h
//
// History:     Apr-18-2023         Rbergen     Created
//---------------------------------------------------------------------------

#include "globals.h"

#include <esp_heap_caps.h>
#include <FS.h>
#include <functional>
#include <memory>
#include <SPIFFS.h>

#include "jsonserializer.h"
#include "systemcontainer.h"
#include "taskmgr.h"

#if USE_HUB75
#include "hub75gfx.h"
#endif

namespace
{
    std::mutex l_jsonFilesystemWriteMutex;
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

    JsonDocument CreateJsonDocument()
    {
        static auto jsonPsramAllocator = JsonPsramAllocator();

        return JsonDocument(&jsonPsramAllocator);
    }

#else

    JsonDocument CreateJsonDocument()
    {
        return JsonDocument();
    }

#endif

namespace
{
#if USE_PSRAM
    struct JsonInternalAllocator : ArduinoJson::Allocator
    {
        void* allocate(size_t size) override
        {
            return heap_caps_malloc(size, MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
        }

        void deallocate(void* pointer) override
        {
            heap_caps_free(pointer);
        }

        void* reallocate(void* ptr, size_t new_size) override
        {
            return heap_caps_realloc(ptr, new_size, MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
        }
    };

    JsonDocument CreateInternalJsonDocument()
    {
        static auto jsonInternalAllocator = JsonInternalAllocator();

        return JsonDocument(&jsonInternalAllocator);
    }
#else
    JsonDocument CreateInternalJsonDocument()
    {
        return JsonDocument();
    }
#endif
}

// SetIfNotOverflowed
//
// This function attempts to set the content of a JsonObject from a JsonDocument, provided the document
// has not overflowed. It takes three parameters:
// - A reference to a JsonDocument (jsonDoc).
// - A reference to a JsonObject (jsonObject).
// - An optional C-string (location) that indicates the location in the code where the function is called.
//
// If the JsonDocument has overflowed, the function logs an error message (including the location if provided)
// and returns false. If the document has not overflowed, the function sets the JsonObject's content to the
// document's content and returns true.

bool SetIfNotOverflowed(JsonDocument& jsonDoc, JsonObject& jsonObject, const char* location)
{
    if (jsonDoc.overflowed())
    {
        if (location)
            debugE("JSON document overflowed at: %s", location);
        else
            debugE("JSON document overflowed");

        return false;
    }

    return jsonObject.set(jsonDoc.as<JsonObjectConst>());
}

namespace ArduinoJson
{
    bool Converter<CRGB>::toJson(const CRGB& color, JsonVariant dst)
    {
        return dst.set(toUint32(color));
    }

    CRGB Converter<CRGB>::fromJson(JsonVariantConst src)
    {
        return CRGB(src.as<uint32_t>());
    }

    bool Converter<CRGB>::checkJson(JsonVariantConst src)
    {
        return src.is<uint32_t>();
    }

    bool Converter<CRGBPalette16>::toJson(const CRGBPalette16& palette, JsonVariant dst)
    {
        auto doc = CreateJsonDocument();

        JsonArray colors = doc.to<JsonArray>();

        for (auto& color: palette.entries)
            colors.add(color);

        return dst.set(doc);
    }

    CRGBPalette16 Converter<CRGBPalette16>::fromJson(JsonVariantConst src)
    {
        CRGB colors[16];
        int colorIndex = 0;

        JsonArrayConst componentsArray = src.as<JsonArrayConst>();
        for (JsonVariantConst value : componentsArray)
            colors[colorIndex++] = value.as<CRGB>();

        return CRGBPalette16(colors);
    }

    bool Converter<CRGBPalette16>::checkJson(JsonVariantConst src)
    {
        return src.is<JsonArrayConst>() && src.as<JsonArrayConst>().size() == 16;
    }
}

struct JSONWriter::WriterEntry
{
    std::atomic_bool flag = false;
    std::function<void()> writer;

    explicit WriterEntry(std::function<void()> writer) :
        writer(std::move(writer))
    {}
};

JSONWriter::JSONWriter() {}
JSONWriter::~JSONWriter() { Stop(); }

// ITaskService hooks
//
// Start/Stop/IsRunning are inherited final from ITaskService; this class
// only supplies the task config, the loop body, and the wake-on-stop hook.

ITaskService::TaskConfig JSONWriter::GetTaskConfig() const
{
    return TaskConfig {
        "JSON Writer Loop",
        JSON_STACK_SIZE,
        JSONWRITER_PRIORITY,
        JSONWRITER_CORE,
        1000   // Stop timeout: task wakes from ulTaskNotifyTake on the wake nudge.
    };
}

void JSONWriter::OnBeforeWaitForStop()
{
    // Flush whatever is pending so a config change isn't lost on shutdown,
    // and wake the task out of its long ulTaskNotifyTake so it sees the
    // shutdown flag promptly.
    flushRequested.store(true);
    WakeTask();
}

void JSONWriter::NotifyTask()
{
    WakeTask();
}

namespace
{
    template <typename T>
    bool AssignIfNameMatches(const String& selectedName, const String& fieldName, T& target, const T& value)
    {
        if (selectedName != fieldName)
            return false;

        target = value;
        return true;
    }
}

bool FieldAccess::AssignIfSelected(const String& selectedName, const String& fieldName, int& target, const String& value)
{
    return AssignIfNameMatches(selectedName, fieldName, target, static_cast<int>(value.toInt()));
}

bool FieldAccess::AssignIfSelected(const String& selectedName, const String& fieldName, size_t& target, const String& value)
{
    return AssignIfNameMatches(selectedName, fieldName, target, static_cast<size_t>(strtoul(value.c_str(), nullptr, 10)));
}

bool FieldAccess::AssignIfSelected(const String& selectedName, const String& fieldName, float& target, const String& value)
{
    return AssignIfNameMatches(selectedName, fieldName, target, value.toFloat());
}

bool FieldAccess::AssignIfSelected(const String& selectedName, const String& fieldName, bool& target, const String& value)
{
    return AssignIfNameMatches(selectedName, fieldName, target, BoolFromText(value));
}

bool FieldAccess::AssignIfSelected(const String& selectedName, const String& fieldName, String& target, const String& value)
{
    return AssignIfNameMatches(selectedName, fieldName, target, value);
}

bool FieldAccess::AssignIfSelected(const String& selectedName, const String& fieldName, CRGBPalette16& target, const String& value)
{
    if (selectedName != fieldName)
        return false;

    auto src = CreateJsonDocument();
    deserializeJson(src, value);
    CRGB colors[16];
    int colorIndex = 0;

    const auto& componentsArray = src.as<JsonArrayConst>();
    for (const auto& v : componentsArray)
        colors[colorIndex++] = v.as<CRGB>();

    target = CRGBPalette16(colors);
    return true;
}

bool FieldAccess::AssignIfSelected(const String& selectedName, const String& fieldName, CRGB& target, const String& value)
{
    return AssignIfNameMatches(selectedName, fieldName, target, CRGB(strtoul(value.c_str(), nullptr, 10)));
}

bool BoolFromText(const String& text)
{
    return text == "true" || strtol(text.c_str(), nullptr, 10);
}

bool LoadJSONFile(const String & fileName, JsonDocument& jsonDoc)
{
    bool jsonReadSuccessful = false;

    File file = SPIFFS.open(fileName);

    if (file)
    {
        if (file.size() > 0)
        {
            debugI("Attempting to read JSON file %s", fileName.c_str());

            DeserializationError error = deserializeJson(jsonDoc, file);

            if (error == DeserializationError::NoMemory)
            {
                debugW("Out of memory reading JSON from file %s", fileName.c_str());
            }
            else if (error == DeserializationError::Ok)
            {
                jsonReadSuccessful = true;
            }
            else
            {
                debugW("Error with code %d occurred while deserializing JSON from file %s", to_value(error.code()), fileName.c_str());
            }
        }

        file.close();
    }

    return jsonReadSuccessful;
}

bool SaveToJSONFile(const String & fileName, IJSONSerializable& object)
{
    auto jsonDoc = CreateInternalJsonDocument();
    auto jsonObject = jsonDoc.to<JsonObject>();

    if (!object.SerializeToJSON(jsonObject))
    {
        debugE("Could not serialize object to JSON, skipping write to %s!", fileName.c_str());
        return false;
    }

    if (jsonDoc.overflowed())
    {
        debugE("JSON document overflowed while preparing %s, skipping write!", fileName.c_str());
        return false;
    }

    WaitForRenderSwapBeforeFilesystemWrite();
    std::lock_guard filesystemGuard(JSONFilesystemWriteMutex());

    SPIFFS.remove(fileName);

    File file = SPIFFS.open(fileName, FILE_WRITE);

    if (!file)
    {
        debugE("Unable to open file %s to write JSON!", fileName.c_str());
        return false;
    }

    size_t bytesWritten = serializeJson(jsonDoc, file);

    file.flush();
    file.close();

    debugI("Number of bytes written to JSON file %s: %zu", fileName.c_str(), (size_t)bytesWritten);

    if (bytesWritten == 0)
    {
        debugE("Unable to write JSON to file %s!", fileName.c_str());
        SPIFFS.remove(fileName);
        return false;
    }

    return true;
}

bool RemoveJSONFile(const String & fileName)
{
    WaitForRenderSwapBeforeFilesystemWrite();
    std::lock_guard filesystemGuard(JSONFilesystemWriteMutex());
    return SPIFFS.remove(fileName);
}

std::mutex& JSONFilesystemWriteMutex()
{
    return l_jsonFilesystemWriteMutex;
}

void WaitForRenderSwapBeforeFilesystemWrite()
{
    // SPIFFS remove/write/flush can take long enough to cause visible frame
    // stalls if the JSON writer holds g_render_mutex. Wait for an active HUB75
    // buffer swap to settle, but do not block the next render frame on file I/O.
    #if USE_HUB75
        HUB75GFX::WaitForMatrixSwap();
    #endif
}

size_t JSONWriter::RegisterWriter(const std::function<void()>& writer)
{
    // Add a writer to the collection. Returns the index of the added writer, for use with FlagWriter()

    // Add the writer with its flag unset
    std::lock_guard guard(writersMutex);
    writers.push_back(make_shared_internal<WriterEntry>(writer));
    return writers.size() - 1;
}

void JSONWriter::FlagWriter(size_t index)
{
    // Flag a writer for invocation and wake up the task that calls them

    // Check if we received a valid writer index
    std::shared_ptr<WriterEntry> entry;
    {
        std::lock_guard guard(writersMutex);
        if (index >= writers.size())
            return;
        entry = writers[index];
    }

    entry->flag.store(true);
    latestFlagMs.store(millis());

    NotifyTask();
}

void JSONWriter::FlushWrites(bool halt)
{
    // Flush pending writes now

    flushRequested.store(true);
    haltWrites.store(halt);

    NotifyTask();
}

// JSONWriter::Run
//
// Invoke functions that write serialized JSON objects to SPIFFS at request,
// with some delay. Wakes from ulTaskNotifyTake on either a flag/flush/stop
// signal or the JSON_WRITER_DELAY hold-point timeout, then runs one pass
// over the registered writers and goes back to sleep. Exits cleanly when
// ShouldShutdown() is true so a config change isn't dropped between the
// flush request and the actual disk write.
void JSONWriter::Run()
{
    for(;;)
    {
        TickType_t notifyWait = portMAX_DELAY;

        for (;;)
        {
            // Wait until we're woken up by a writer being flagged, or until we've reached the hold point
            ulTaskNotifyTake(pdTRUE, notifyWait);

            // Stop requested: drop straight into a final flush pass below.
            if (ShouldShutdown())
                break;

            // If a flush was requested then we execute pending writes now
            if (flushRequested.load())
            {
                flushRequested.store(false);
                break;
            }

            // If writes are halted, we don't do anything
            if (haltWrites.load())
                continue;

            unsigned long holdUntil = latestFlagMs.load() + JSON_WRITER_DELAY;
            unsigned long now = millis();
            if (now >= holdUntil)
                break;

            notifyWait = pdMS_TO_TICKS(holdUntil - now);
        }

        std::vector<std::shared_ptr<JSONWriter::WriterEntry>> writersSnapshot;
        {
            std::lock_guard guard(writersMutex);
            writersSnapshot = writers;
        }

        for (auto &entryPtr : writersSnapshot)
        {
            auto& entry = *entryPtr;
            // Unset flag before we do the actual write. This makes that we don't miss another flag raise if it happens while reading
            if (entry.flag.exchange(false))
                entry.writer();
        }

        // Honor the shutdown request after the flush pass so any pending writes
        // are persisted before the task exits. ITaskService::TaskEntryThunk
        // handles the actual NotifyTaskExited / vTaskDelete on return.
        if (ShouldShutdown())
            return;
    }
}

uint32_t toUint32(const CRGB& color)
{
    return (uint32_t)((color.r << 16) | (color.g << 8) | color.b);
}
