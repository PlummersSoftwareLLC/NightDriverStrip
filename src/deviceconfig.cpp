//+--------------------------------------------------------------------------
//
// File:        deviceconfig.cpp
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
//    Implementation of DeviceConfig class methods
//
// History:     Apr-18-2023         Rbergen      Created
//
//---------------------------------------------------------------------------

#include "globals.h"

#include <HTTPClient.h>
#include <algorithm>
#include <array>
#include <driver/gpio.h>
#include <memory>
#include <optional>
#include <UrlEncode.h>

#include "audioservice.h"
#include "deviceconfig.h"
#include "deviceconfig_internal.h"
#include "effectmanager.h"
#include "jsonserializer.h"
#include "systemcontainer.h"

extern const char timezones_start[] asm("_binary_config_timezones_json_start");

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

void DeviceConfig::SaveToJSON() const
{
    g_ptrSystem->GetJSONWriter().FlagWriter(writerIndex);
}

std::array<int8_t, NUM_CHANNELS> DeviceConfig::GetCompiledWS281xPins()
{
    return DeviceConfigInternal::GetCompiledWS281xPins();
}

const char* DeviceConfig::DriverName(OutputDriver driver)
{
    switch (driver)
    {
        case OutputDriver::HUB75:
            return "hub75";

        case OutputDriver::WS281x:
        default:
            return "ws281x";
    }
}

DeviceConfig::WS281xColorOrder DeviceConfig::GetCompiledWS281xColorOrder()
{
    return DeviceConfigInternal::GetCompiledWS281xColorOrder();
}

String DeviceConfig::GetColorOrderName(WS281xColorOrder colorOrder)
{
    switch (colorOrder)
    {
        case WS281xColorOrder::RGB: return "RGB";
        case WS281xColorOrder::RBG: return "RBG";
        case WS281xColorOrder::GRB: return "GRB";
        case WS281xColorOrder::GBR: return "GBR";
        case WS281xColorOrder::BRG: return "BRG";
        case WS281xColorOrder::BGR: return "BGR";
        default:                    return "GRB";
    }
}

bool DeviceConfig::IsHub75Build()
{
    return GetCompiledOutputDriver() == OutputDriver::HUB75;
}

void DeviceConfig::LogRuntimeConfig(const char* reason) const
{
    String activePins;
    for (size_t i = 0; i < runtimeOutputs.channelCount && i < runtimeOutputs.outputPins.size(); ++i)
    {
        if (!activePins.isEmpty())
            activePins += ',';
        activePins += String(runtimeOutputs.outputPins[i]);
    }

    debugI("Runtime config (%s): driver=%s matrix=%ux%u leds=%u serpentine=%d channels=%u colorOrder=%s audioPin=%d",
           reason,
           DriverName(runtimeOutputs.driver),
           runtimeTopology.width,
           runtimeTopology.height,
           static_cast<unsigned>(GetActiveLEDCount()),
           runtimeTopology.serpentine,
            static_cast<unsigned>(runtimeOutputs.channelCount),
           GetColorOrderName(runtimeOutputs.colorOrder).c_str(),
           audioInputPin);
    debugI("Runtime config pins (%s): activeChannels=%s", reason, activePins.c_str());
}

DeviceConfig::DeviceConfig()
{
    runtimeTopology.serpentine = !IsHub75Build();
    runtimeOutputs.driver = GetCompiledOutputDriver();
    runtimeOutputs.channelCount = NUM_CHANNELS;
    runtimeOutputs.outputPins = GetCompiledWS281xPins();
    runtimeOutputs.colorOrder = GetCompiledWS281xColorOrder();

    writerIndex = g_ptrSystem->GetJSONWriter().RegisterWriter(
        [this] { assert(SaveToJSONFile(DEVICE_CONFIG_FILE, *this)); }
    );

    auto jsonDoc = CreateJsonDocument();

    if (LoadJSONFile(DEVICE_CONFIG_FILE, jsonDoc))
    {
        debugI("Loading DeviceConfig from JSON");

        DeserializeFromJSON(jsonDoc.as<JsonObjectConst>(), true);
    }
    else
    {
        debugW("DeviceConfig could not be loaded from JSON, using defaults");

        SetTimeZone(timeZone, true);

        SaveToJSON();
    }

    LogRuntimeConfig("init");
}

bool DeviceConfig::SerializeToJSON(JsonObject& jsonObject)
{
    return SerializeToJSON(jsonObject, true);
}

bool DeviceConfig::SerializeToJSON(JsonObject& jsonObject, bool includeSensitive)
{
    auto jsonDoc = CreateJsonDocument();

    // Add serialization logic for additional settings to this code
    jsonDoc[HostnameTag] = hostname;
    jsonDoc[LocationTag] = location;
    jsonDoc[LocationIsZipTag] = locationIsZip;
    jsonDoc[CountryCodeTag] = countryCode;
    jsonDoc[TimeZoneTag] = timeZone;
    jsonDoc[Use24HourClockTag] = use24HourClock;
    jsonDoc[UseCelsiusTag] = useCelsius;
    jsonDoc[NTPServerTag] = ntpServer;
    jsonDoc[RememberCurrentEffectTag] = rememberCurrentEffect;
    jsonDoc[PowerLimitTag] = powerLimit;
    // Only serialize showVUMeter if the VU meter is enabled in the build
    #if SHOW_VU_METER
    jsonDoc[ShowVUMeterTag] = showVUMeter;
    #endif
    jsonDoc[BrightnessTag] = brightness;
    jsonDoc[GlobalColorTag] = globalColor;
    jsonDoc[ApplyGlobalColorsTag] = applyGlobalColors;
    jsonDoc[SecondColorTag] = secondColor;
    jsonDoc[AudioInputPinTag] = audioInputPin;
    jsonDoc[MatrixWidthTag] = runtimeTopology.width;
    jsonDoc[MatrixHeightTag] = runtimeTopology.height;
    jsonDoc[MatrixSerpentineTag] = runtimeTopology.serpentine;
    jsonDoc[OutputDriverTag] = DriverName(runtimeOutputs.driver);
    jsonDoc[WS281xChannelCountTag] = runtimeOutputs.channelCount;
    jsonDoc[WS281xColorOrderTag] = GetColorOrderName(runtimeOutputs.colorOrder);

    auto ws281xPins = jsonDoc[WS281xPinsTag].to<JsonArray>();
    for (auto pin : runtimeOutputs.outputPins)
        ws281xPins.add(pin);

    if (includeSensitive)
        jsonDoc[OpenWeatherApiKeyTag] = openWeatherApiKey;

    return SetIfNotOverflowed(jsonDoc, jsonObject, __PRETTY_FUNCTION__);
}

bool DeviceConfig::DeserializeFromJSON(const JsonObjectConst& jsonObject)
{
    return DeserializeFromJSON(jsonObject, false);
}

bool DeviceConfig::DeserializeFromJSON(const JsonObjectConst& jsonObject, bool skipWrite)
{
    // If we're told to ignore saved config, we shouldn't touch anything
    if (IGNORE_SAVED_DEVICE_CONFIG)
        return true;

    // Add deserialization logic for additional settings to this code
    SetIfPresentIn(jsonObject, hostname, HostnameTag);
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
    // Persisted config predates the newer brightness guardrails in some installs, so treat an invalid
    // saved brightness as "unset" and fall back to the normal 100% default instead of booting dark.
    if (brightness < BRIGHTNESS_MIN || brightness > BRIGHTNESS_MAX)
        brightness = BRIGHTNESS_MAX;
    // Only deserialize showVUMeter if the VU meter is enabled in the build
    #if SHOW_VU_METER
    SetIfPresentIn(jsonObject, showVUMeter, ShowVUMeterTag);
    #endif
    SetIfPresentIn(jsonObject, globalColor, GlobalColorTag);
    SetIfPresentIn(jsonObject, applyGlobalColors, ApplyGlobalColorsTag);
    SetIfPresentIn(jsonObject, secondColor, SecondColorTag);
    if (jsonObject[AudioInputPinTag].is<int>())
    {
        const int persistedAudioInputPin = jsonObject[AudioInputPinTag].as<int>();
        auto [pinValid, _] = ValidateAudioInputPin(persistedAudioInputPin);
        audioInputPin = pinValid ? persistedAudioInputPin : GetCompiledAudioInputPin();
    }

    RuntimeConfig updated = GetRuntimeConfig();

    SetIfPresentIn(jsonObject, updated.topology.width, MatrixWidthTag);
    SetIfPresentIn(jsonObject, updated.topology.height, MatrixHeightTag);
    SetIfPresentIn(jsonObject, updated.topology.serpentine, MatrixSerpentineTag);

    if (jsonObject[OutputDriverTag].is<String>())
    {
        const auto driverName = jsonObject[OutputDriverTag].as<String>();
        if (driverName == DriverName(OutputDriver::HUB75))
            updated.outputs.driver = OutputDriver::HUB75;
        else if (driverName == DriverName(OutputDriver::WS281x))
            updated.outputs.driver = OutputDriver::WS281x;
    }

    if (jsonObject[WS281xChannelCountTag].is<size_t>())
        updated.outputs.channelCount = jsonObject[WS281xChannelCountTag].as<size_t>();

    if (jsonObject[WS281xColorOrderTag].is<String>())
    {
        const auto colorOrderName = jsonObject[WS281xColorOrderTag].as<String>();
        if (colorOrderName == "RGB") updated.outputs.colorOrder = WS281xColorOrder::RGB;
        else if (colorOrderName == "RBG") updated.outputs.colorOrder = WS281xColorOrder::RBG;
        else if (colorOrderName == "GRB") updated.outputs.colorOrder = WS281xColorOrder::GRB;
        else if (colorOrderName == "GBR") updated.outputs.colorOrder = WS281xColorOrder::GBR;
        else if (colorOrderName == "BRG") updated.outputs.colorOrder = WS281xColorOrder::BRG;
        else if (colorOrderName == "BGR") updated.outputs.colorOrder = WS281xColorOrder::BGR;
    }

    if (jsonObject[WS281xPinsTag].is<JsonArrayConst>())
    {
        auto pinArray = jsonObject[WS281xPinsTag].as<JsonArrayConst>();
        for (size_t i = 0; i < updated.outputs.outputPins.size() && i < pinArray.size(); ++i)
        {
            if (pinArray[i].is<int>())
                updated.outputs.outputPins[i] = pinArray[i].as<int>();
        }
    }

    auto [runtimeConfigValid, runtimeConfigError] = SetRuntimeConfig(updated, true);
    if (!runtimeConfigValid)
        debugW("Ignoring invalid persisted runtime config: %s", runtimeConfigError.c_str());

    if (ntpServer.isEmpty())
        ntpServer = NTP_SERVER_DEFAULT;

    if (jsonObject[TimeZoneTag].is<String>())
        return SetTimeZone(jsonObject[TimeZoneTag], true);

    if (!skipWrite)
        SaveToJSON();

    return true;
}

void DeviceConfig::RemovePersisted()
{
    RemoveJSONFile(DEVICE_CONFIG_FILE);
}

const String& DeviceConfig::GetTimeZone() const
{
    return timeZone;
}

void DeviceConfig::Set24HourClock(bool new24HourClock)
{
    SetAndSave(use24HourClock, new24HourClock);
}

void DeviceConfig::SetHostname(const String &newHostname)
{
    SetAndSave(hostname, newHostname);
}

void DeviceConfig::SetLocation(const String &newLocation)
{
    SetAndSave(location, newLocation);
}

void DeviceConfig::SetCountryCode(const String &newCountryCode)
{
    SetAndSave(countryCode, newCountryCode);
}

void DeviceConfig::SetLocationIsZip(bool newLocationIsZip)
{
    SetAndSave(locationIsZip, newLocationIsZip);
}

void DeviceConfig::SetOpenWeatherAPIKey(const String &newOpenWeatherAPIKey)
{
    SetAndSave(openWeatherApiKey, newOpenWeatherAPIKey);
}

void DeviceConfig::SetUseCelsius(bool newUseCelsius)
{
    SetAndSave(useCelsius, newUseCelsius);
}

void DeviceConfig::SetNTPServer(const String &newNTPServer)
{
    SetAndSave(ntpServer, newNTPServer);
}

void DeviceConfig::SetRememberCurrentEffect(bool newRememberCurrentEffect)
{
    SetAndSave(rememberCurrentEffect, newRememberCurrentEffect);
}

DeviceConfig::ValidateResponse DeviceConfig::ValidateBrightness(int newBrightness)
{
    if (newBrightness < BRIGHTNESS_MIN)
        return { false, String("brightness is below minimum value of ") + BRIGHTNESS_MIN };

    if (newBrightness > BRIGHTNESS_MAX)
        return { false, String("brightness is above maximum value of ") + BRIGHTNESS_MAX };

    return { true, "" };
}

DeviceConfig::ValidateResponse DeviceConfig::ValidateBrightness(const String& newBrightness)
{
    return ValidateBrightness(newBrightness.toInt());
}

void DeviceConfig::SetBrightness(int newBrightness)
{
    SetAndSave(brightness, uint8_t(std::clamp<int>(newBrightness, BRIGHTNESS_MIN, BRIGHTNESS_MAX)));
}

void DeviceConfig::SetShowVUMeter(bool newShowVUMeter)
{
    // We only actually persist if the VU meter is enabled in the build
    #if SHOW_VU_METER
    SetAndSave(showVUMeter, newShowVUMeter);
    #else
    showVUMeter = newShowVUMeter;
    #endif
}

DeviceConfig::ValidateResponse DeviceConfig::ValidatePowerLimit(int newPowerLimit)
{
    if (newPowerLimit < POWER_LIMIT_MIN)
        return { false, String("powerLimit is below minimum value of ") + POWER_LIMIT_MIN };

    return { true, "" };
}

DeviceConfig::ValidateResponse DeviceConfig::ValidatePowerLimit(const String& newPowerLimit)
{
    return ValidatePowerLimit(newPowerLimit.toInt());
}

void DeviceConfig::SetPowerLimit(int newPowerLimit)
{
    if (newPowerLimit >= POWER_LIMIT_MIN)
        SetAndSave(powerLimit, newPowerLimit);
}

void DeviceConfig::SetApplyGlobalColors()
{
    SetAndSave(applyGlobalColors, true);
}

void DeviceConfig::ClearApplyGlobalColors()
{
    SetAndSave(applyGlobalColors, false);
}

void DeviceConfig::SetGlobalColor(const CRGB& newGlobalColor)
{
    SetAndSave(globalColor, newGlobalColor);
}

void DeviceConfig::SetSecondColor(const CRGB& newSecondColor)
{
    SetAndSave(secondColor, newSecondColor);
}

DeviceConfig::ValidateResponse DeviceConfig::ValidateAudioInputPin(int pin) const
{
    if (pin < -1)
        return { false, "audio input pin must be -1 or a valid GPIO" };

    if (pin == GetCompiledAudioInputPin())
        return { true, "" };

    // The settings API now separates "compiled default" from "active value". External I2S mics can
    // move their DIN pin at boot, but the M5 onboard mic path and the current ADC path are still fixed.
    if (!SupportsConfigurableAudioInputPin())
        return { false, DeviceConfigInternal::RecompileNeededMessage() };

    if (pin == -1)
        return { true, "" };

    if (!GPIO_IS_VALID_GPIO(static_cast<gpio_num_t>(pin)))
        return { false, "audio input pin must be a valid GPIO" };

    return { true, "" };
}

void DeviceConfig::SetAudioInputPin(int newAudioInputPin)
{
    auto [isValid, _] = ValidateAudioInputPin(newAudioInputPin);
    if (!isValid)
        return;

    if (audioInputPin == newAudioInputPin)
        return;

    SetAndSave(audioInputPin, static_cast<int8_t>(newAudioInputPin));
    LogRuntimeConfig("audio input pin changed");
}

// This setter separates "apply the timezone to the running process" from "persist a user edit".
// Startup/config-load needs to set TZ immediately so localtime() is correct, but it must not
// immediately rewrite device.cfg just because we re-applied the already-persisted value.
// The timezone JSON file used by this logic is generated using tools/gen-tz-json.py
bool DeviceConfig::SetTimeZone(const String& newTimeZone, bool skipWrite)
{
    String quotedTZ = "\n\"" + newTimeZone + '"';

    const char *start = strstr(timezones_start, quotedTZ.c_str());

    // If we can't find the new timezone as a timezone name, assume it's a literal value
    if (start == nullptr)
        setenv("TZ", newTimeZone.c_str(), 1);
    // We received a timezone name, so we extract and use its timezone value
    else
    {
        start += quotedTZ.length();
        start = strchr(start, '"');
        if (start == nullptr)      // Can't actually happen unless timezone file is malformed
            return false;

        start++;
        const char *end = strchr(start, '"');
        if (end == nullptr)        // Can't actually happen unless timezone file is malformed
            return false;

        size_t length = end - start;

        std::unique_ptr<char[]> value = std::make_unique<char[]>(length + 1);
        strncpy(value.get(), start, length);
        value[length] = 0;

        setenv("TZ", value.get(), 1);
    }

    tzset();

    timeZone = newTimeZone;
    if (!skipWrite)
        SaveToJSON();

    return true;
}

#if ENABLE_WIFI
DeviceConfig::ValidateResponse DeviceConfig::ValidateOpenWeatherAPIKey(const String &newOpenWeatherAPIKey)
{
    HTTPClient http;

    String url = "http://api.openweathermap.org/data/2.5/weather?lat=0&lon=0&appid=" + urlEncode(newOpenWeatherAPIKey);

    http.begin(url);

    switch (http.GET())
    {
        case HTTP_CODE_OK:
        {
            http.end();
            return { true, "" };
        }

        case HTTP_CODE_UNAUTHORIZED:
        {
            auto jsonDoc = CreateJsonDocument();
            deserializeJson(jsonDoc, http.getString());

            String message = "";
            if (jsonDoc["message"].is<String>())
                message = jsonDoc["message"].as<String>();

            http.end();
            return { false, message };
        }

        // Anything else
        default:
        {
            http.end();
            return { false, "Unable to validate" };
        }
    }
}
#endif  // ENABLE_WIFI

