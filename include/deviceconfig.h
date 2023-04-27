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
        if (target == source)
            return;

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

    static constexpr const char * LOCATION_TAG = NAME_OF(location);
    static constexpr const char * LOCATION_IS_ZIP_TAG = NAME_OF(locationIsZip);
    static constexpr const char * COUNTRY_CODE_TAG = NAME_OF(countryCode);
    static constexpr const char * OPEN_WEATHER_API_KEY_TAG = NAME_OF(openWeatherApiKey);
    static constexpr const char * TIME_ZONE_TAG = NAME_OF(timeZone);
    static constexpr const char * USE_24_HOUR_CLOCK_TAG = NAME_OF(use24HourClock);
    static constexpr const char * USE_CELSIUS_TAG = NAME_OF(useCelsius);

    DeviceConfig();

    virtual bool SerializeToJSON(JsonObject& jsonObject)
    {
        StaticJsonDocument<1024> jsonDoc;

        jsonDoc[LOCATION_TAG] = location;
        jsonDoc[LOCATION_IS_ZIP_TAG] = locationIsZip;
        jsonDoc[COUNTRY_CODE_TAG] = countryCode;
        jsonDoc[OPEN_WEATHER_API_KEY_TAG] = openWeatherApiKey;
        jsonDoc[TIME_ZONE_TAG] = timeZone;
        jsonDoc[USE_24_HOUR_CLOCK_TAG] = use24HourClock;
        jsonDoc[USE_CELSIUS_TAG] = useCelsius;
    
        return jsonObject.set(jsonDoc.as<JsonObjectConst>());
    }

    virtual bool DeserializeFromJSON(const JsonObjectConst& jsonObject)
    {
        return DeserializeFromJSON(jsonObject, false);
    }

    bool DeserializeFromJSON(const JsonObjectConst& jsonObject, bool skipWrite)
    {
        SetIfPresentIn(jsonObject, location, LOCATION_TAG);
        SetIfPresentIn(jsonObject, locationIsZip, LOCATION_IS_ZIP_TAG);
        SetIfPresentIn(jsonObject, countryCode, COUNTRY_CODE_TAG);
        SetIfPresentIn(jsonObject, openWeatherApiKey, OPEN_WEATHER_API_KEY_TAG);
        SetIfPresentIn(jsonObject, use24HourClock, USE_24_HOUR_CLOCK_TAG);
        SetIfPresentIn(jsonObject, useCelsius, USE_CELSIUS_TAG);

        if (jsonObject.containsKey(TIME_ZONE_TAG)) 
            return SetTimeZone(jsonObject[TIME_ZONE_TAG], true);
   
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
