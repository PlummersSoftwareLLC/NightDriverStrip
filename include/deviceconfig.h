//+--------------------------------------------------------------------------
//
// File:        deviceconfig.h
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
//    Declaration of DeviceConfig class and global variable
//
// History:     Apr-18-2023         Rbergen      Created
//
//---------------------------------------------------------------------------

#pragma once

#include <memory>
#include <vector>
#include "jsonserializer.h"

#define DEVICE_CONFIG_FILE "/device.cfg"

class DeviceConfig : public IJSONSerializable
{
    String location;
    bool locationIsZip;
    String countryCode;
    String timeZone;
    String openWeatherApiKey;
    bool use24HourClock;
    bool useCelsius;

/*    
    void WriteToNVS(const String& name, const String& value);
    void WriteToNVS(const String& name, bool value);
*/
    void SaveToJSON();

    template <typename T>
    void SetAndSave(T& target, const T& source)
    {
        target = source;

        SaveToJSON();
    }

    template <typename T>
    void SetIfPresentIn(const JsonObjectConst& jsonObject, T& target, const char *tag)
    {
        if (jsonObject.containsKey(tag)) 
            target = jsonObject[tag].as<T>();
    }

  public:
    DeviceConfig();

    virtual bool SerializeToJSON(JsonObject& jsonObject)
    {
        StaticJsonDocument<1024> jsonDoc;

        jsonDoc[NAME_OF(location)] = location;
        jsonDoc[NAME_OF(locationIsZip)] = locationIsZip;
        jsonDoc[NAME_OF(countryCode)] = countryCode;
        jsonDoc[NAME_OF(openWeatherApiKey)] = openWeatherApiKey;
        jsonDoc[NAME_OF(timeZone)] = timeZone;
        jsonDoc[NAME_OF(use24HourClock)] = use24HourClock;
        jsonDoc[NAME_OF(useCelsius)] = useCelsius;
    
        return jsonObject.set(jsonDoc.as<JsonObjectConst>());
    }

    virtual bool DeserializeFromJSON(const JsonObjectConst& jsonObject)
    {
        return DeserializeFromJSON(jsonObject, false);
    }

    bool DeserializeFromJSON(const JsonObjectConst& jsonObject, bool skipWrite)
    {
        SetIfPresentIn(jsonObject, location, NAME_OF(location));
        SetIfPresentIn(jsonObject, locationIsZip, NAME_OF(locationIsZip));
        SetIfPresentIn(jsonObject, countryCode, NAME_OF(countryCode));
        SetIfPresentIn(jsonObject, openWeatherApiKey, NAME_OF(openWeatherApiKey));
        SetIfPresentIn(jsonObject, use24HourClock, NAME_OF(use24HourClock));
        SetIfPresentIn(jsonObject, useCelsius, NAME_OF(useCelsius));

        const char *tag = NAME_OF(timeZone);
        if (jsonObject.containsKey(tag)) 
            return SetTimeZone(jsonObject[tag], true);
   
        if (!skipWrite)
            SaveToJSON();

        return true;
    }

    void RemovePersisted()
    {
        RemoveJSONFile(DEVICE_CONFIG_FILE);
    }

    const String &GetTimeZone() const
    {
        return timeZone;
    }

    bool SetTimeZone(const String& newTimeZone, bool skipWrite = false);

    bool Use24HourClock() const
    {
        return use24HourClock;
    }

    void Set24HourClock(bool new24HourClock)
    {
        SetAndSave(use24HourClock, new24HourClock);
    }

    const String &GetLocation() const
    {
        return location;
    }

    void SetLocation(const String &newLocation)
    {
        SetAndSave(location, newLocation);
    }

    const String &GetCountryCode() const
    {
        return countryCode;
    }

    void SetCountryCode(const String &newCountryCode)
    {
        SetAndSave(countryCode, newCountryCode);
    }

    bool IsLocationZip() const
    {
        return locationIsZip;
    }

    void SetLocationIsZip(bool newLocationIsZip)
    {
        SetAndSave(locationIsZip, newLocationIsZip);
    }

    const String &GetOpenWeatherAPIKey() const
    {
        return openWeatherApiKey;
    }

    void SetOpenWeatherAPIKey(const String &newOpenWeatherAPIKey)
    {
        SetAndSave(openWeatherApiKey, newOpenWeatherAPIKey);
    }

    bool UseCelsius() const
    {
        return useCelsius;
    }

    void SetUseCelsius(bool newUseCelsius)
    {
        SetAndSave(useCelsius, newUseCelsius);
    }
};

extern DRAM_ATTR std::unique_ptr<DeviceConfig> g_aptrDeviceConfig;
