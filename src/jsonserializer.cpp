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

#include <FS.h>
#include <SPIFFS.h>

#include "globals.h"
#include "systemcontainer.h"
#include "taskmgr.h"

bool BoolFromText(const String&text)
{
    return text == "true" || strtol(text.c_str(), nullptr, 10);
}

bool LoadJSONFile(const String&fileName, JsonDocument&jsonDoc)
{
    bool jsonReadSuccessful = false;

    File file               = SPIFFS.open(fileName);

    if(file)
    {
        if(file.size() > 0)
        {
            debugI("Attempting to read JSON file %s", fileName.c_str());

            DeserializationError error = deserializeJson(jsonDoc, file);

            if(error == DeserializationError::NoMemory)
                debugW("Out of memory reading JSON from file %s", fileName.c_str());
            else if(error == DeserializationError::Ok)
                jsonReadSuccessful = true;
            else
                debugW("Error with code %d occurred while deserializing JSON from file %s", to_value(error.code()), fileName.c_str());
        }

        file.close();
    }

    return jsonReadSuccessful;
}

bool SaveToJSONFile(const String&fileName, IJSONSerializable&object)
{
    auto jsonDoc    = CreateJsonDocument();
    auto jsonObject = jsonDoc.to<JsonObject>();

    if(!object.SerializeToJSON(jsonObject))
    {
        debugE("Could not serialize object to JSON, skipping write to %s!", fileName.c_str());

        return false;
    }

    SPIFFS.remove(fileName);

    File file = SPIFFS.open(fileName, FILE_WRITE);

    if(!file)
    {
        debugE("Unable to open file %s to write JSON!", fileName.c_str());

        return false;
    }

    size_t bytesWritten = serializeJson(jsonDoc, file);
    debugI("Number of bytes written to JSON file %s: %zu", fileName.c_str(), bytesWritten);

    file.flush();
    file.close();

    if(bytesWritten == 0)
    {
        debugE("Unable to write JSON to file %s!", fileName.c_str());
        SPIFFS.remove(fileName);

        return false;
    }

    return true;
}

bool RemoveJSONFile(const String&fileName)
{
    return SPIFFS.remove(fileName);
}

size_t JSONWriter::RegisterWriter(const std::function<void()>&writer)
{
    // Add the writer with its flag unset
    writers.emplace_back(writer);

    return writers.size() - 1;
}

void JSONWriter::FlagWriter(size_t index)
{
    // Check if we received a valid writer index
    if(index >= writers.size())
        return;

    writers[index].flag.store(true);
    latestFlagMs.store(millis());

    g_ptrSystem->TaskManager().NotifyJSONWriterThread();
}

void JSONWriter::FlushWrites(bool halt)
{
    flushRequested.store(true);
    haltWrites.store(halt);

    g_ptrSystem->TaskManager().NotifyJSONWriterThread();
}

// JSONWriterTaskEntry
//
// Invoke functions that write serialized JSON objects to SPIFFS at request, with some delay
void IRAM_ATTR JSONWriterTaskEntry(void*)
{
    for(;;)
    {
        TickType_t notifyWait = portMAX_DELAY;

        for(;;)
        {
            // Wait until we're woken up by a writer being flagged, or until we've reached the hold point
            ulTaskNotifyTake(pdTRUE, notifyWait);

            if(!g_ptrSystem->HasJSONWriter())
                continue;

            auto&jsonWriter = g_ptrSystem->JSONWriter();

            // If a flush was requested then we execute pending writes now
            if(jsonWriter.flushRequested.load())
            {
                jsonWriter.flushRequested.store(false);
                break;
            }

            // If writes are halted, we don't do anything
            if(jsonWriter.haltWrites.load())
                continue;

            unsigned long holdUntil = jsonWriter.latestFlagMs.load() + JSON_WRITER_DELAY;
            unsigned long now       = millis();
            if(now >= holdUntil)
                break;

            notifyWait = pdMS_TO_TICKS(holdUntil - now);
        }

        for(auto&entry : g_ptrSystem->JSONWriter().writers)
        {
            // Unset flag before we do the actual write. This makes that we don't miss another flag raise if it happens while writing
            if(entry.flag.exchange(false))
                entry.writer();
        }
    }
}

uint32_t toUint32(const CRGB&color)
{
    return (uint32_t) ((color.r << 16) | (color.g << 8) | color.b);
}
