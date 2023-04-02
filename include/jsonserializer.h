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

#define JSON_SERIALIZION_OK 0
#define JSON_SERIALIZION_OUT_OF_MEMORY 1

struct IJSONSerializable
{
    virtual int SerializeToJSON(JsonObject& jsonObject) = 0;
};

template<typename T>
struct IJSONDeserializable : IJSONSerializable 
{
    virtual void DeserializeMembersFromJSON(const JsonObject& jsonObject) {}

    static virtual T DeserializeFromJSON(const JsonObject& jsonObject) = 0;
};

namespace JSONSerializer 
{
    DynamicJsonDocument* SerializeToJSON(IJSONSerializable& serializableObject) 
    {
        static size_t jsonBufferSize = JSON_BUFFER_BASE_SIZE;
        DynamicJsonDocument* jsonDocument = nullptr;

        // Loop is in place to deal with the possible scenario that we run out of JSON buffer space
        while(true)
        {
            jsonDocument = new DynamicJsonDocument(jsonBufferSize);
            JsonObject jsonObject = jsonDocument->to<JsonObject>();

            if (serializableObject.SerializeToJSON(jsonObject) == JSON_SERIALIZION_OUT_OF_MEMORY)
            {
                jsonBufferSize += JSON_BUFFER_INCREMENT;
                delete jsonDocument;
            }
            else
                break;
        }

        return jsonDocument;
    }
}