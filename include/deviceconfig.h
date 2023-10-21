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
#include <tuple>
#include <algorithm>
#include "jsonserializer.h"
#include "secrets.h"

#define DEVICE_CONFIG_FILE "/device.cfg"
#define NTP_SERVER_DEFAULT "0.pool.ntp.org"
#define BRIGHTNESS_MIN uint8_t(10)
#define BRIGHTNESS_MAX uint8_t(255)
#define POWER_LIMIT_MIN 2000
#define POWER_LIMIT_DEFAULT 4500

// DeviceConfig holds, persists and loads device-wide configuration settings. Effect-specific settings should
// be managed using overrides of the respective methods in LEDStripEffect (HasSettings(), GetSettingSpecs(),
// SerializeSettingsToJSON() and SetSetting()).
//
// Adding a setting to the list of known/saved settings requires the following:
// 1. Adding the setting variable to the list at the top of the class definition
// 2. Adding a corresponding Tag to the list of static constexpr const char * strings further below
// 3. Adding a corresponding SettingSpec in the GetSettingSpecs() function
// 4. Adding logic to set a default in case the JSON load isn't possible in the DeviceConfig() constructor
//    (in deviceconfig.cpp)
// 5. Adding (de)serialization logic for the setting to the SerializeToJSON()/DeserializeFromJSON() methods
// 6. Adding a Get/Set method for the setting (and, where applicable, their implementation in deviceconfig.cpp)
//
// For the first 5 points, a comment has been added to the respective place in the existing code.
// Generally speaking, one will also want to add logic to the webserver to retrieve and set the setting.

class DeviceConfig : public IJSONSerializable
{
    // Add variables for additional settings to this list
    String  location = cszLocation;
    bool    locationIsZip = false;
    String  countryCode = cszCountryCode;
    String  timeZone = cszTimeZone;
    String  openWeatherApiKey = cszOpenWeatherAPIKey;
    bool    use24HourClock = false;
    bool    useCelsius = false;
    String  ntpServer = NTP_SERVER_DEFAULT;
    bool    rememberCurrentEffect = true;
    int     powerLimit = POWER_LIMIT_DEFAULT;
    uint8_t brightness = BRIGHTNESS_MAX;

    std::vector<SettingSpec, psram_allocator<SettingSpec>> settingSpecs;
    std::vector<std::reference_wrapper<SettingSpec>> settingSpecReferences;
    size_t writerIndex;

    static constexpr int _jsonSize = 1024;

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

    using ValidateResponse = std::pair<bool, String>;

    // Add additional setting Tags to this list
    static constexpr const char * LocationTag = NAME_OF(location);
    static constexpr const char * LocationIsZipTag = NAME_OF(locationIsZip);
    static constexpr const char * CountryCodeTag = NAME_OF(countryCode);
    static constexpr const char * OpenWeatherApiKeyTag = NAME_OF(openWeatherApiKey);
    static constexpr const char * TimeZoneTag = NAME_OF(timeZone);
    static constexpr const char * Use24HourClockTag = NAME_OF(use24HourClock);
    static constexpr const char * UseCelsiusTag = NAME_OF(useCelsius);
    static constexpr const char * NTPServerTag = NAME_OF(ntpServer);
    static constexpr const char * RememberCurrentEffectTag = NAME_OF(rememberCurrentEffect);
    static constexpr const char * PowerLimitTag = NAME_OF(powerLimit);
    static constexpr const char * BrightnessTag = NAME_OF(brightness);

    DeviceConfig();

    bool SerializeToJSON(JsonObject& jsonObject) override
    {
        return SerializeToJSON(jsonObject, true);
    }

    bool SerializeToJSON(JsonObject& jsonObject, bool includeSensitive)
    {
        AllocatedJsonDocument jsonDoc(_jsonSize);

        // Add serialization logic for additionl settings to this code
        jsonDoc[LocationTag] = location;
        jsonDoc[LocationIsZipTag] = locationIsZip;
        jsonDoc[CountryCodeTag] = countryCode;
        jsonDoc[TimeZoneTag] = timeZone;
        jsonDoc[Use24HourClockTag] = use24HourClock;
        jsonDoc[UseCelsiusTag] = useCelsius;
        jsonDoc[NTPServerTag] = ntpServer;
        jsonDoc[RememberCurrentEffectTag] = rememberCurrentEffect;
        jsonDoc[PowerLimitTag] = powerLimit;
        jsonDoc[BrightnessTag] = brightness;

        if (includeSensitive)
            jsonDoc[OpenWeatherApiKeyTag] = openWeatherApiKey;

        assert(!jsonDoc.overflowed());

        return jsonObject.set(jsonDoc.as<JsonObjectConst>());
    }

    bool DeserializeFromJSON(const JsonObjectConst& jsonObject) override
    {
        return DeserializeFromJSON(jsonObject, false);
    }

    bool DeserializeFromJSON(const JsonObjectConst& jsonObject, bool skipWrite)
    {
        // Add deserialization logic for additional settings to this code
        SetIfPresentIn(jsonObject, location, LocationTag);
        SetIfPresentIn(jsonObject, locationIsZip, LocationIsZipTag);
        SetIfPresentIn(jsonObject, countryCode, CountryCodeTag);
        SetIfPresentIn(jsonObject, openWeatherApiKey, OpenWeatherApiKeyTag);
        SetIfPresentIn(jsonObject, use24HourClock, Use24HourClockTag);
        SetIfPresentIn(jsonObject, useCelsius, UseCelsiusTag);
        SetIfPresentIn(jsonObject, ntpServer, NTPServerTag);
        SetIfPresentIn(jsonObject, rememberCurrentEffect, RememberCurrentEffectTag);
        SetIfPresentIn(jsonObject, powerLimit, PowerLimitTag);
        SetIfPresentIn(jsonObject, brightness, BrightnessTag);

        if (ntpServer.isEmpty())
            ntpServer = NTP_SERVER_DEFAULT;

        if (jsonObject.containsKey(TimeZoneTag))
            return SetTimeZone(jsonObject[TimeZoneTag], true);

        if (!skipWrite)
            SaveToJSON();

        return true;
    }

    void RemovePersisted()
    {
        RemoveJSONFile(DEVICE_CONFIG_FILE);
    }

    virtual const std::vector<std::reference_wrapper<SettingSpec>>& GetSettingSpecs()
    {
        if (settingSpecs.size() == 0)
        {
            // Add SettingSpec for additional settings to this list
            settingSpecs.emplace_back(
                NAME_OF(location),
                "Location",
                "The location (city or postal code) where the device is located.",
                SettingSpec::SettingType::String
            );
            settingSpecs.emplace_back(
                NAME_OF(locationIsZip),
                "Location is postal code",
                "A boolean indicating if the value in the 'location' setting is a postal code ('true'/1) or not ('false'/0).",
                SettingSpec::SettingType::Boolean
            );
            settingSpecs.emplace_back(
                NAME_OF(countryCode),
                "Country code",
                "The <a href=\"https://en.wikipedia.org/wiki/ISO_3166-1_alpha-2\">ISO 3166-1 alpha-2</a> country "
                "code for the country that the device is located in.",
                SettingSpec::SettingType::String
            );

            auto weatherKeySpec = settingSpecs.emplace_back(
                NAME_OF(openWeatherApiKey),
                "Open Weather API key",
                "The API key for the <a href=\"https://openweathermap.org/api\">Weather API provided by Open Weather Map</a>.",
                SettingSpec::SettingType::String
            );
            weatherKeySpec.HasValidation = true;
            weatherKeySpec.Access = SettingSpec::SettingAccess::WriteOnly;

            settingSpecs.emplace_back(
                NAME_OF(timeZone),
                "Time zone",
                "The timezone the device resides in, in <a href=\"https://en.wikipedia.org/wiki/Tz_database\">tz database</a> format. "
                "The list of available timezone identifiers can be found in the <a href=\"/timezones.json\">timezones.json</a> file.",
                SettingSpec::SettingType::String
            );
            settingSpecs.emplace_back(
                NAME_OF(use24HourClock),
                "Use 24 hour clock",
                "A boolean that indicates if time should be shown in 24-hour format ('true'/1) or 12-hour AM/PM format ('false'/0).",
                SettingSpec::SettingType::Boolean
            );
            settingSpecs.emplace_back(
                NAME_OF(useCelsius),
                "Use degrees Celsius",
                "A boolean that indicates if temperatures should be shown in degrees Celsius ('true'/1) or degrees Fahrenheit ('false'/0).",
                SettingSpec::SettingType::Boolean
            );
            settingSpecs.emplace_back(
                NAME_OF(ntpServer),
                "NTP server address",
                "The hostname or IP address of the NTP server to be used for time synchronization.",
                SettingSpec::SettingType::String
            );
            settingSpecs.emplace_back(
                NAME_OF(rememberCurrentEffect),
                "Remember current effect",
                "A boolean that indicates if the current effect index should be saved after an effect transition, so the device resumes "
                "from the same effect when restarted. Enabling this will lead to more wear on the flash chip of your device.",
                SettingSpec::SettingType::Boolean
            );
            settingSpecs.emplace_back(
                NAME_OF(brightness),
                "Brightness",
                "Overall brightness the connected LEDs or matrix should be run at.",
                SettingSpec::SettingType::Integer,
                BRIGHTNESS_MIN,
                BRIGHTNESS_MAX
            ).HasValidation = true;

            auto& powerLimitSpec = settingSpecs.emplace_back(
                NAME_OF(powerLimit),
                "Power limit",
                "The maximum power in mW that the matrix attached to the board is allowed to use. As the previous sentence implies, this "
                "setting only applies if a matrix is used.",
                SettingSpec::SettingType::Integer
            );
            powerLimitSpec.MinimumValue = POWER_LIMIT_MIN;
            powerLimitSpec.HasValidation = true;

            settingSpecReferences.insert(settingSpecReferences.end(), settingSpecs.begin(), settingSpecs.end());
        }

        return settingSpecReferences;
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

    ValidateResponse ValidateOpenWeatherAPIKey(const String &newOpenWeatherAPIKey);

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

    const String &GetNTPServer() const
    {
        return ntpServer;
    }

    void SetNTPServer(const String &newNTPServer)
    {
        SetAndSave(ntpServer, newNTPServer);
    }

    bool RememberCurrentEffect() const
    {
        return rememberCurrentEffect;
    }

    void SetRememberCurrentEffect(bool newRememberCurrentEffect)
    {
        SetAndSave(rememberCurrentEffect, newRememberCurrentEffect);
    }

    uint8_t GetBrightness() const
    {
        return brightness;
    }

    ValidateResponse ValidateBrightness(const String& newBrightness)
    {
        auto newNumericBrightness = newBrightness.toInt();

        if (newNumericBrightness < BRIGHTNESS_MIN)
            return { false, String("brightness is below minimum value of ") + BRIGHTNESS_MIN };

        if (newNumericBrightness > BRIGHTNESS_MAX)
            return { false, String("brightness is above maximum value of ") + BRIGHTNESS_MAX };

        return { true, "" };
    }

    void SetBrightness(int newBrightness)
    {
        SetAndSave(brightness, uint8_t(std::clamp<int>(newBrightness, BRIGHTNESS_MIN, BRIGHTNESS_MAX)));
    }

    int GetPowerLimit() const
    {
        return powerLimit;
    }

    ValidateResponse ValidatePowerLimit(const String& newPowerLimit)
    {
        if (newPowerLimit.toInt() < POWER_LIMIT_MIN)
            return { false, String("powerLimit is below minimum value of ") + POWER_LIMIT_MIN };

        return { true, "" };
    }

    void SetPowerLimit(int newPowerLimit)
    {
        if (newPowerLimit >= POWER_LIMIT_MIN)
            SetAndSave(powerLimit, newPowerLimit);
    }
};