#pragma once

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

#include "globals.h"

#include <tuple>
#include <vector>

#include "interfaces.h"

#if ENABLE_WIFI
// Make sure we have a secrets.h and that it contains everything we need.

#if !__has_include("secrets.h")
#error Copy include/secrets.example.h to include/secrets.h, edit to taste, and retry. Please see README.md.
#endif

#include "secrets.h"

#if !defined(cszSSID)
#error A definition for cszSSID is missing from secrets.h
#endif

#if !defined(cszPassword)
#error A definition for cszPassword is missing from secrets.h
#endif

#if !defined(cszHostname)
#error A definition for cszHostname is missing from secrets.h
#endif

#if !defined(cszOpenWeatherAPIKey)
#error A definition for cszOpenWeatherAPIKey is missing from secrets.h
#endif

#if !defined(cszLocation)
#error A definition for cszLocation is missing from secrets.h
#endif

#if !defined(bLocationIsZip)
#error A definition for bLocationIsZip is missing from secrets.h
#endif

#if !defined(cszCountryCode)
#error A definition for cszCountryCode is missing from secrets.h
#endif

#if !defined(cszTimeZone)
#error A definition for cszTimeZone is missing from secrets.h
#endif
#else
#define cszHostname ""
#define cszLocation ""
#define bLocationIsZip false
#define cszCountryCode ""
#define cszTimeZone ""
#define cszOpenWeatherAPIKey ""
#define cszSSID ""
#endif // ENABLE_WIFI

// Define this to true to make the DeviceConfig ignore any JSON-persisted config that may be on the device.
// Note that effect settings are not impacted by this setting. Their persisted config is part of the effects
// list JSON, which can be ignored separately (search for EFFECT_SET_VERSION in effects.cpp).
#define IGNORE_SAVED_DEVICE_CONFIG  false

#define DEVICE_CONFIG_FILE          "/device.cfg"
#define NTP_SERVER_DEFAULT          "0.pool.ntp.org"
#ifndef BRIGHTNESS_MIN
    #define BRIGHTNESS_MIN          uint8_t(10)
#endif
#ifndef BRIGHTNESS_MAX
    #define BRIGHTNESS_MAX          uint8_t(255)
#endif
#define POWER_LIMIT_MIN             1000
#define POWER_LIMIT_DEFAULT         4500

// DeviceConfig holds, persists and loads device-wide configuration settings. Effect-specific settings should
// be managed using overrides of the respective methods in LEDStripEffect (mainly FillSettingSpecs(),
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
// 7. If you've added an entry to secrets.example.h to define a default value for your setting then add a
//    test at the top of this file to confirm that the new #define is found. This prevents drift when users
//    have an existing tree and don't know to refresh their modified version of secrets.h.
//
// For the first 5 points, a comment has been added to the respective place in the existing code.
// Generally speaking, one will also want to add logic to the webserver to retrieve and set the setting.

class DeviceConfig : public IJSONSerializable
{
    // Add variables for additional settings to this list
    String  hostname = cszHostname;
    String  location = cszLocation;
    bool    locationIsZip = bLocationIsZip;
    String  countryCode = cszCountryCode;
    String  timeZone = cszTimeZone;
    String  openWeatherApiKey = cszOpenWeatherAPIKey;
    bool    use24HourClock = false;
    bool    useCelsius = false;
    String  ntpServer = NTP_SERVER_DEFAULT;
    bool    rememberCurrentEffect = true;
    int     powerLimit = POWER_LIMIT_DEFAULT;
    bool    showVUMeter = true;
    uint8_t brightness = BRIGHTNESS_MAX;
    CRGB    globalColor = CRGB::Red;
    bool    applyGlobalColors = false;
    CRGB    secondColor = CRGB::Red;

    std::vector<SettingSpec, psram_allocator<SettingSpec>> settingSpecs;
    std::vector<std::reference_wrapper<SettingSpec>> settingSpecReferences;
    size_t writerIndex;

    void SaveToJSON() const;

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
        if (jsonObject[tag].is<T>())
            target = jsonObject[tag].as<T>();
    }

  public:

    using ValidateResponse = std::pair<bool, String>;

    // Add additional setting Tags to this list
    static constexpr const char * HostnameTag = NAME_OF(hostname);
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
    // No need to publish the show VU meter tag unless we're also publishing the setting
    #if SHOW_VU_METER
    static constexpr const char * ShowVUMeterTag = NAME_OF(showVUMeter);
    #endif
    static constexpr const char * ClearGlobalColorTag = "clearGlobalColor";
    static constexpr const char * GlobalColorTag = NAME_OF(globalColor);
    static constexpr const char * ApplyGlobalColorsTag = NAME_OF(applyGlobalColors);
    static constexpr const char * SecondColorTag = NAME_OF(secondColor);

    DeviceConfig();

    bool SerializeToJSON(JsonObject& jsonObject) override;
    bool SerializeToJSON(JsonObject& jsonObject, bool includeSensitive);

    bool DeserializeFromJSON(const JsonObjectConst& jsonObject) override;
    bool DeserializeFromJSON(const JsonObjectConst& jsonObject, bool skipWrite);

    static void RemovePersisted();

    virtual const std::vector<std::reference_wrapper<SettingSpec>>& GetSettingSpecs();

    const String &GetTimeZone() const;
    bool SetTimeZone(const String& newTimeZone, bool skipWrite = false);

    bool Use24HourClock() const { return use24HourClock; }
    void Set24HourClock(bool new24HourClock);

    const String &GetHostname() const { return hostname; }
    void SetHostname(const String &newHostname);

    const String &GetLocation() const { return location; }
    void SetLocation(const String &newLocation);

    const String &GetCountryCode() const { return countryCode; }
    void SetCountryCode(const String &newCountryCode);

    bool IsLocationZip() const { return locationIsZip; }
    void SetLocationIsZip(bool newLocationIsZip);

    const String &GetOpenWeatherAPIKey() const { return openWeatherApiKey; }
    static ValidateResponse ValidateOpenWeatherAPIKey(const String &newOpenWeatherAPIKey);
    void SetOpenWeatherAPIKey(const String &newOpenWeatherAPIKey);

    bool UseCelsius() const { return useCelsius; }
    void SetUseCelsius(bool newUseCelsius);

    const String &GetNTPServer() const { return ntpServer; }
    void SetNTPServer(const String &newNTPServer);

    bool RememberCurrentEffect() const { return rememberCurrentEffect; }
    void SetRememberCurrentEffect(bool newRememberCurrentEffect);

    uint8_t GetBrightness() const { return brightness; }
    static ValidateResponse ValidateBrightness(const String& newBrightness);
    void SetBrightness(int newBrightness);

    bool ShowVUMeter() const { return showVUMeter; }
    void SetShowVUMeter(bool newShowVUMeter);

    int GetPowerLimit() const { return powerLimit; }
    static ValidateResponse ValidatePowerLimit(const String& newPowerLimit);
    void SetPowerLimit(int newPowerLimit);

    const CRGB& GlobalColor() const { return globalColor; }
    void SetApplyGlobalColors();
    void ClearApplyGlobalColors();
    bool ApplyGlobalColors() const { return applyGlobalColors; }
    void SetGlobalColor(const CRGB& newGlobalColor);

    const CRGB& SecondColor() const { return secondColor; }
    void SetSecondColor(const CRGB& newSecondColor);

    void SetColorSettings(const CRGB& globalColor, const CRGB& secondColor);
    void ApplyColorSettings(std::optional<CRGB> globalColor, std::optional<CRGB> secondColor, bool clearGlobalColor, bool applyGlobalColor);
};
