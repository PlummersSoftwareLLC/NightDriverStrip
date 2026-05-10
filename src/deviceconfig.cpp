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

#include "deviceconfig.h"
#include "effectmanager.h"
#include "jsonserializer.h"
#include "systemcontainer.h"

extern const char timezones_start[] asm("_binary_config_timezones_json_start");

namespace
{
    constexpr const char* kRecompileNeededMessage = "recompile needed";

    // It's annoying to have to map from the compile-time COLOR_ORDER macro to the
    // runtime enum, but it is what it is.  It's better than tying to a FastLED type.

    constexpr DeviceConfig::WS281xColorOrder ToRuntimeColorOrder(EOrder order)
    {
        switch (order)
        {
            case EOrder::RGB: return DeviceConfig::WS281xColorOrder::RGB;
            case EOrder::RBG: return DeviceConfig::WS281xColorOrder::RBG;
            case EOrder::GRB: return DeviceConfig::WS281xColorOrder::GRB;
            case EOrder::GBR: return DeviceConfig::WS281xColorOrder::GBR;
            case EOrder::BRG: return DeviceConfig::WS281xColorOrder::BRG;
            case EOrder::BGR: return DeviceConfig::WS281xColorOrder::BGR;
            default:          return DeviceConfig::WS281xColorOrder::GRB;
        }
    }

    #if USE_WS281X
    constexpr auto kCompiledWS281xColorOrder = ToRuntimeColorOrder(COLOR_ORDER);
    #else
    constexpr auto kCompiledWS281xColorOrder = DeviceConfig::WS281xColorOrder::GRB;
    #endif

    constexpr std::array<int8_t, NUM_CHANNELS> kCompiledWS281xPins = {
        #if NUM_CHANNELS >= 1
        LED_PIN0,
        #endif
        #if NUM_CHANNELS >= 2
        LED_PIN1,
        #endif
        #if NUM_CHANNELS >= 3
        LED_PIN2,
        #endif
        #if NUM_CHANNELS >= 4
        LED_PIN3,
        #endif
        #if NUM_CHANNELS >= 5
        LED_PIN4,
        #endif
        #if NUM_CHANNELS >= 6
        LED_PIN5,
        #endif
        #if NUM_CHANNELS >= 7
        LED_PIN6,
        #endif
        #if NUM_CHANNELS >= 8
        LED_PIN7,
        #endif
    };
}

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
    return kCompiledWS281xPins;
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
    return kCompiledWS281xColorOrder;
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

    String runtimeConfigError;
    if (!SetRuntimeConfig(updated, true, &runtimeConfigError))
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

const std::vector<std::reference_wrapper<SettingSpec>>& DeviceConfig::GetSettingSpecs()
{
    if (settingSpecs.empty())
    {
        // Section keys used below correspond to entries in the section catalog
        // emitted by FillUnifiedSettingsSchemaJson(). Keep them in sync.
        constexpr const char* kSectionSystem     = "system";
        constexpr const char* kSectionLocation   = "location";
        constexpr const char* kSectionClock      = "clock";
        constexpr const char* kSectionAudio      = "audio";
        constexpr const char* kSectionAppearance = "appearance";
        constexpr const char* kSectionTopology   = "topology";
        constexpr const char* kSectionOutput     = "output";

        // ---- system section ------------------------------------------------
        settingSpecs.push_back(SettingSpec::Validate(SettingSpec{
            .Name           = HostnameTag,
            .FriendlyName   = "Hostname",
            .Description    = "The hostname of the device. A reboot is required after changing this.",
            .Type           = SettingSpec::SettingType::String,
            .EmptyAllowed   = true,
            .Section        = kSectionSystem,
            .RequiresReboot = true,
            .ApiPath        = "device.hostname"
        }));
        settingSpecs.push_back(SettingSpec::Validate(SettingSpec{
            .Name          = PowerLimitTag,
            .FriendlyName  = "Power limit",
            .Description   = "The maximum power in mW that the matrix attached to the board is allowed to use. As the previous sentence implies, this "
                             "setting only applies if a matrix is used.",
            .Type          = SettingSpec::SettingType::Integer,
            .HasValidation = true,
            .MinimumValue  = (double)POWER_LIMIT_MIN,
            .Section       = kSectionSystem,
            .ApiPath       = "device.powerLimit"
        }));

        // ---- location section ----------------------------------------------
        settingSpecs.push_back(SettingSpec::Validate(SettingSpec{
            .Name         = LocationTag,
            .FriendlyName = "Location",
            .Description  = "The location (city or postal code) where the device is located.",
            .Type         = SettingSpec::SettingType::String,
            .Section      = kSectionLocation,
            .ApiPath      = "device.location"
        }));
        settingSpecs.push_back(SettingSpec::Validate(SettingSpec{
            .Name         = LocationIsZipTag,
            .FriendlyName = "Location is postal code",
            .Description  = "Indicates if the value for the \"Location\" setting is a postal code (yes if checked) or not.",
            .Type         = SettingSpec::SettingType::Boolean,
            .Section      = kSectionLocation,
            .ApiPath      = "device.locationIsZip"
        }));
        settingSpecs.push_back(SettingSpec::Validate(SettingSpec{
            .Name         = CountryCodeTag,
            .FriendlyName = "Country code",
            .Description  = "The <a href=\"https://en.wikipedia.org/wiki/ISO_3166-1_alpha-2\">ISO 3166-1 alpha-2</a> country "
                            "code for the country that the device is located in.",
            .Type         = SettingSpec::SettingType::String,
            .Section      = kSectionLocation,
            .ApiPath      = "device.countryCode",
            .Widget       = SettingSpec::WidgetKind::Select,
            .Options      = SettingSpec::OptionsSource::IntlCountryCodes
        }));
        settingSpecs.push_back(SettingSpec::Validate(SettingSpec{
            .Name               = TimeZoneTag,
            .FriendlyName       = "Time zone",
            .Description        = "The timezone the device resides in, in <a href=\"https://en.wikipedia.org/wiki/Tz_database\">tz database</a> format. "
                                  "The list of available timezone identifiers can be found in the <a href=\"/timezones.json\">timezones.json</a> file.",
            .Type               = SettingSpec::SettingType::String,
            .Section            = kSectionLocation,
            .ApiPath            = "device.timeZone",
            .Widget             = SettingSpec::WidgetKind::Select,
            .Options            = SettingSpec::OptionsSource::ExternalTimeZones,
            .OptionsExternalUrl = "/timezones.json"
        }));

        // ---- clock section -------------------------------------------------
        settingSpecs.push_back(SettingSpec::Validate(SettingSpec{
            .Name         = Use24HourClockTag,
            .FriendlyName = "Use 24 hour clock",
            .Description  = "Indicates if time should be shown in 24-hour format (yes if checked) or 12-hour AM/PM format.",
            .Type         = SettingSpec::SettingType::Boolean,
            .Section      = kSectionClock,
            .ApiPath      = "device.use24HourClock"
        }));
        settingSpecs.push_back(SettingSpec::Validate(SettingSpec{
            .Name         = UseCelsiusTag,
            .FriendlyName = "Use degrees Celsius",
            .Description  = "Indicates if temperatures should be shown in degrees Celsius (yes if checked) or degrees Fahrenheit.",
            .Type         = SettingSpec::SettingType::Boolean,
            .Section      = kSectionClock,
            .ApiPath      = "device.useCelsius"
        }));
        settingSpecs.push_back(SettingSpec::Validate(SettingSpec{
            .Name         = NTPServerTag,
            .FriendlyName = "NTP server address",
            .Description  = "The hostname or IP address of the NTP server to be used for time synchronization.",
            .Type         = SettingSpec::SettingType::String,
            .Section      = kSectionClock,
            .ApiPath      = "device.ntpServer"
        }));
        settingSpecs.push_back(SettingSpec::Validate(SettingSpec{
            .Name          = OpenWeatherApiKeyTag,
            .FriendlyName  = "Open Weather API key",
            .Description   = "The API key for the <a href=\"https://openweathermap.org/api\">Weather API provided by Open Weather Map</a>.",
            .Type          = SettingSpec::SettingType::String,
            .HasValidation = true,
            .Access        = SettingSpec::SettingAccess::WriteOnly,
            .Section       = kSectionClock,
            .ApiPath       = "device.openWeatherApiKey"
        }));

        // ---- audio section -------------------------------------------------
        settingSpecs.push_back(SettingSpec::Validate(SettingSpec{
            .Name           = AudioInputPinTag,
            .FriendlyName   = "Audio input pin",
            .Description    = "External microphone input pin. This is boot-applied today because the audio task still owns the active DMA/I2S handles once sampling starts.",
            .Type           = SettingSpec::SettingType::Integer,
            .HasValidation  = true,
            .MinimumValue   = -1.0,
            .MaximumValue   = 48.0,
            .Section        = kSectionAudio,
            .RequiresReboot = !SupportsLiveAudioInputReconfigure(),
            .ApiPath        = "device.audioInputPin"
        }));

        // ---- appearance section --------------------------------------------
        settingSpecs.push_back(SettingSpec::Validate(SettingSpec{
            .Name          = BrightnessTag,
            .FriendlyName  = "Brightness",
            .Description   = "Overall brightness the connected LEDs or matrix should be run at.",
            .Type          = SettingSpec::SettingType::Integer,
            .HasValidation = true,
            .Section       = kSectionAppearance,
            .ApiPath       = "device.brightness",
            .Widget        = SettingSpec::WidgetKind::Slider,
            // Display as 5..100 percent over the raw 13..255 range. The endpoints
            // mirror what the legacy UI did: clamp at 5% on the low side, 100% at
            // raw max, with linear remapping in between. The UI does the math.
            .DisplayRawMin = (double)BRIGHTNESS_MIN,
            .DisplayRawMax = (double)BRIGHTNESS_MAX,
            .DisplayMin    = 5.0,
            .DisplayMax    = 100.0,
            .DisplaySuffix = "%"
        }));
        settingSpecs.push_back(SettingSpec::Validate(SettingSpec{
            .Name         = GlobalColorTag,
            .FriendlyName = "Global color",
            .Description  = "Main color that is applied to all those effects that support using it.",
            .Type         = SettingSpec::SettingType::Color,
            .Section      = kSectionAppearance,
            .ApiPath      = "device.globalColor"
        }));
        settingSpecs.push_back(SettingSpec::Validate(SettingSpec{
            .Name         = SecondColorTag,
            .FriendlyName = "Second color",
            .Description  = "Second color that is used to create a global palette in combination with the current global color. That palette is used "
                            "by some effects. Defaults to the <em>previous</em> global color if not explicitly set.",
            .Type         = SettingSpec::SettingType::Color,
            .Section      = kSectionAppearance,
            .ApiPath      = "device.secondColor"
        }));
        settingSpecs.push_back(SettingSpec::Validate(SettingSpec{
            .Name         = ApplyGlobalColorsTag,
            .FriendlyName = "(Re)apply global color",
            .Description  = "You can use this to \"reselect\" and apply the current global color, to force the composition of the derived "
                            "global palette. This checkbox is ignored if the \"Clear global color\" checkbox is selected.",
            .Type         = SettingSpec::SettingType::Boolean,
            .Access       = SettingSpec::SettingAccess::WriteOnly,
            .Section      = kSectionAppearance,
            .ApiPath      = "device.applyGlobalColors"
        }));
        settingSpecs.push_back(SettingSpec::Validate(SettingSpec{
            .Name         = ClearGlobalColorTag,
            .FriendlyName = "Clear global color",
            .Description  = "Stop applying the global color/derived palette. This takes precedence over the \"(Re)apply global color\" checkbox.",
            .Type         = SettingSpec::SettingType::Boolean,
            .Access       = SettingSpec::SettingAccess::WriteOnly,
            .Section      = kSectionAppearance,
            .ApiPath      = "device.clearGlobalColor"
        }));

        #if SHOW_VU_METER
        settingSpecs.push_back(SettingSpec::Validate(SettingSpec{
            .Name         = ShowVUMeterTag,
            .FriendlyName = "Show VU meter",
            .Description  = "Used to show (checked) or hide the VU meter at the top of the matrix.",
            .Type         = SettingSpec::SettingType::Boolean,
            .Section      = kSectionAppearance
        }));
        #endif

        settingSpecs.push_back(SettingSpec::Validate(SettingSpec{
            .Name         = RememberCurrentEffectTag,
            .FriendlyName = "Remember current effect",
            .Description  = "Indicates if the current effect index should be saved after an effect transition, so the device resumes "
                            "from the same effect when restarted. Enabling this will lead to more wear on the flash chip of your device.",
            .Type         = SettingSpec::SettingType::Boolean,
            .Section      = kSectionAppearance,
            .ApiPath      = "device.rememberCurrentEffect"
        }));

        // ---- topology section ----------------------------------------------
        settingSpecs.push_back(SettingSpec::Validate(SettingSpec{
            .Name         = MatrixWidthTag,
            .FriendlyName = "Matrix width",
            .Description  = "Active matrix width. WS281x builds validate this by total LED capacity, so width * height must stay within the compiled LED budget.",
            .Type         = SettingSpec::SettingType::PositiveBigInteger,
            .MinimumValue = 1.0,
            .MaximumValue = (double)GetCompiledLEDCount(),
            .Section      = kSectionTopology,
            .Priority     = 0,
            .ApiPath      = "topology.width"
        }));
        settingSpecs.push_back(SettingSpec::Validate(SettingSpec{
            .Name         = MatrixHeightTag,
            .FriendlyName = "Matrix height",
            .Description  = "Active matrix height. WS281x builds validate this by total LED capacity, so width * height must stay within the compiled LED budget.",
            .Type         = SettingSpec::SettingType::PositiveBigInteger,
            .MinimumValue = 1.0,
            .MaximumValue = (double)GetCompiledLEDCount(),
            .Section      = kSectionTopology,
            .Priority     = 1,
            .ApiPath      = "topology.height"
        }));
        settingSpecs.push_back(SettingSpec::Validate(SettingSpec{
            .Name         = MatrixSerpentineTag,
            .FriendlyName = "Serpentine layout",
            .Description  = "Controls the logical XY mapping for strip-based matrices. HUB75 ignores this because its panel mapping is build-defined.",
            .Type         = SettingSpec::SettingType::Boolean,
            .Section      = kSectionTopology,
            .Priority     = 2,
            .ApiPath      = "topology.serpentine"
        }));

        // ---- output section -------------------------------------------------
        settingSpecs.push_back(SettingSpec::Validate(SettingSpec{
            .Name              = OutputDriverTag,
            .FriendlyName      = "Output driver",
            .Description       = "Runtime-selected driver. If this differs from the firmware's compiled driver, the API reports recompile required.",
            .Type              = SettingSpec::SettingType::String,
            .Section           = kSectionOutput,
            .Priority          = 10,
            .ApiPath           = "outputs.driver",
            .Widget            = SettingSpec::WidgetKind::Select,
            // Friendly labels for the raw driver identifiers the schema exposes.
            .Options           = SettingSpec::OptionsSource::SchemaPath,
            .OptionValues      = {"ws281x", "hub75"},
            .OptionLabels      = {"WS281x", "HUB75"},
            .OptionsSchemaPath = "outputs.allowedDrivers"
        }));
        settingSpecs.push_back(SettingSpec::Validate(SettingSpec{
            .Name              = WS281xChannelCountTag,
            .FriendlyName      = "WS281x channel count",
            .Description       = "Number of active strip channels within the compiled maximum. Live updates are limited to WS281x builds.",
            .Type              = SettingSpec::SettingType::PositiveBigInteger,
            .MinimumValue      = 1.0,
            .MaximumValue      = (double)GetCompiledChannelCount(),
            .Section           = kSectionOutput,
            .Priority          = 11,
            .ApiPath           = "outputs.ws281x.channelCount",
            .Widget            = SettingSpec::WidgetKind::Select,
            // Concrete list at outputs.ws281x.allowedChannelCounts in the
            // schema, so the UI doesn't have to derive a range itself.
            .Options           = SettingSpec::OptionsSource::SchemaPath,
            .OptionsSchemaPath = "outputs.ws281x.allowedChannelCounts"
        }));
        settingSpecs.push_back(SettingSpec::Validate(SettingSpec{
            .Name              = WS281xColorOrderTag,
            .FriendlyName      = "WS281x color order",
            .Description       = "Byte order used when streaming RGB values to WS281x LEDs. This applies live on strip builds and is ignored on HUB75 builds.",
            .Type              = SettingSpec::SettingType::String,
            .Section           = kSectionOutput,
            .Priority          = 12,
            .ApiPath           = "outputs.ws281x.colorOrder",
            .Widget            = SettingSpec::WidgetKind::Select,
            .Options           = SettingSpec::OptionsSource::SchemaPath,
            .OptionsSchemaPath = "outputs.ws281x.allowedColorOrders"
        }));

        // ---- per-channel pin specs ------------------------------------------
        // ws281xPin{N} specs are synthesized here because all the relevant
        // details (channel count, naming convention, apiPath format, priority
        // range) are DeviceConfig internals.
        // pinSpecStrings holds the String backing; settingSpecs owns the specs.
        // Both are sized up front so no reallocation can invalidate pointers.
        constexpr size_t kPinStringsPerChannel = 4; // name, friendly, description, apiPath
        const auto compiledChannelCount = GetCompiledChannelCount();
        settingSpecs.reserve(settingSpecs.size() + compiledChannelCount);
        pinSpecStrings.reserve(compiledChannelCount * kPinStringsPerChannel);
        const auto stableCStr = [](const String& s) { return s.c_str(); };

        for (size_t i = 0; i < compiledChannelCount; ++i)
        {
            const auto& nameStr        = pinSpecStrings.emplace_back(str_sprintf("ws281xPin%zu", i));
            const auto& friendlyStr    = pinSpecStrings.emplace_back(str_sprintf("WS281x pin %zu", i + 1));
            const auto& descriptionStr = pinSpecStrings.emplace_back(str_sprintf("GPIO assigned to WS281x channel %zu.", i + 1));
            const auto& apiPathStr     = pinSpecStrings.emplace_back(str_sprintf("outputs.ws281x.pins[%zu]", i));

            settingSpecs.push_back(SettingSpec::Validate(SettingSpec{
                .Name         = stableCStr(nameStr),
                .FriendlyName = stableCStr(friendlyStr),
                .Description  = stableCStr(descriptionStr),
                .Type         = SettingSpec::SettingType::Integer,
                .MinimumValue = -1.0,
                .MaximumValue = 48.0,
                .Section      = kSectionOutput,
                .Priority     = 3 + static_cast<int>(i),
                .ApiPath      = stableCStr(apiPathStr)
            }));
        }

        settingSpecReferences.insert(settingSpecReferences.end(), settingSpecs.begin(), settingSpecs.end());
    }

    return settingSpecReferences;
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
        return { false, kRecompileNeededMessage };

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

        std::unique_ptr<char[]> value = make_unique_psram<char[]>(length + 1);
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

void DeviceConfig::SetColorSettings(const CRGB& newGlobalColor, const CRGB& newSecondColor)
{
    globalColor = newGlobalColor;
    secondColor = newSecondColor;
    applyGlobalColors = true;

    SaveToJSON();
}

// This function contains the logic for dealing with the various color-related settings we have.
// The logic effectively mimics the behavior of pressing a color button on the IR remote control when (only) the
// global color is set or (re)applied, but also allows the secondary global palette color to be specified directly.
// The code in this function figures out how to prioritize and combine the values of (optional) settings; the actual
// logic for applying the correct color(s) and palette is contained in a number of EffectManager member functions.
void DeviceConfig::ApplyColorSettings(std::optional<CRGB> newGlobalColor, std::optional<CRGB> newSecondColor, bool clearGlobalColor, bool forceApplyGlobalColor)
{
    // If we're asked to clear the global color, we'll remember any colors we were passed, but won't do anything with them
    if (clearGlobalColor)
    {
        if (newGlobalColor.has_value())
            globalColor = newGlobalColor.value();
        if (newSecondColor.has_value())
            secondColor = newSecondColor.value();

        g_ptrSystem->GetEffectManager().ClearRemoteColor();

        applyGlobalColors = false;

        SaveToJSON();

        return;
    }

    CRGB finalGlobalColor = newGlobalColor.has_value() ? newGlobalColor.value() : globalColor;
    forceApplyGlobalColor = forceApplyGlobalColor || newGlobalColor.has_value();

    // If we were given a second color, set it and the global one if necessary. Then have EffectManager do its thing...
    if (newSecondColor.has_value())
    {
        if (forceApplyGlobalColor)
        {
            applyGlobalColors = true;
            globalColor = finalGlobalColor;
        }

        secondColor = newSecondColor.value();

        g_ptrSystem->GetEffectManager().ApplyGlobalPaletteColors();

        SaveToJSON();
    }
    else if (forceApplyGlobalColor)
    {
        // ...otherwise, apply the "set global color" logic if we were asked to do so
        g_ptrSystem->GetEffectManager().ApplyGlobalColor(finalGlobalColor);
    }
}

DeviceConfig::ValidateResponse DeviceConfig::ValidateTopology(uint16_t width, uint16_t height, bool serpentine) const
{
    if (width == 0 || height == 0)
        return { false, "matrix dimensions must be greater than zero" };

    // The strip path sizes its live buffers from total LED capacity, not the original compile-time aspect ratio.
    // That keeps reshaping flexible while still refusing requests that would outgrow the compiled backing store.
    if (static_cast<size_t>(width) * height > GetCompiledLEDCount())
        return { false, kRecompileNeededMessage };

    if (IsHub75Build())
    {
        if (width != GetCompiledMatrixWidth() || height != GetCompiledMatrixHeight())
            return { false, kRecompileNeededMessage };

        if (serpentine != GetCompiledMatrixSerpentine())
            return { false, kRecompileNeededMessage };
    }

    return { true, "" };
}

DeviceConfig::ValidateResponse DeviceConfig::ValidateOutputDriver(OutputDriver driver) const
{
    if (driver != GetCompiledOutputDriver())
        return { false, kRecompileNeededMessage };

    return { true, "" };
}

DeviceConfig::ValidateResponse DeviceConfig::ValidateWS281xSettings(size_t channelCount, const std::array<int8_t, NUM_CHANNELS>& pins, WS281xColorOrder colorOrder) const
{
    if (channelCount == 0)
        return { false, "channel count must be greater than zero" };

    if (channelCount > GetCompiledChannelCount())
        return { false, kRecompileNeededMessage };

    if (IsHub75Build())
    {
        if (channelCount != GetCompiledChannelCount())
            return { false, kRecompileNeededMessage };

        if (pins != GetCompiledWS281xPins())
            return { false, kRecompileNeededMessage };

        if (colorOrder != GetCompiledWS281xColorOrder())
            return { false, kRecompileNeededMessage };
    }

    for (size_t i = 0; i < channelCount; ++i)
    {
        if (pins[i] < 0)
            return { false, "active channels require valid GPIO pins" };

        if (!GPIO_IS_VALID_OUTPUT_GPIO(static_cast<gpio_num_t>(pins[i])))
            return { false, "WS281x channel pins must be valid output GPIOs" };

        for (size_t j = i + 1; j < channelCount; ++j)
        {
            if (pins[i] == pins[j])
                return { false, "WS281x channel pins must be unique" };
        }
    }

    return { true, "" };
}

DeviceConfig::ValidateResponse DeviceConfig::ValidateRuntimeConfig(const RuntimeConfig& config) const
{
    auto [driverValid, driverMessage] = ValidateOutputDriver(config.outputs.driver);
    if (!driverValid)
        return { false, driverMessage };

    auto [topologyValid, topologyMessage] = ValidateTopology(config.topology.width, config.topology.height, config.topology.serpentine);
    if (!topologyValid)
        return { false, topologyMessage };

    auto [ws281xValid, ws281xMessage] = ValidateWS281xSettings(config.outputs.channelCount, config.outputs.outputPins, config.outputs.colorOrder);
    if (!ws281xValid)
        return { false, ws281xMessage };

    return { true, "" };
}

bool DeviceConfig::SetRuntimeConfig(const RuntimeConfig& config, bool skipWrite, String* errorMessage)
{
    auto [isValid, validationMessage] = ValidateRuntimeConfig(config);
    if (!isValid)
    {
        if (errorMessage)
            *errorMessage = validationMessage;
        return false;
    }

    const bool changed =
        runtimeTopology.width != config.topology.width
        || runtimeTopology.height != config.topology.height
        || runtimeTopology.serpentine != config.topology.serpentine
        || runtimeOutputs.driver != config.outputs.driver
        || runtimeOutputs.channelCount != config.outputs.channelCount
        || runtimeOutputs.outputPins != config.outputs.outputPins
        || runtimeOutputs.colorOrder != config.outputs.colorOrder;

    runtimeTopology = config.topology;
    runtimeOutputs = config.outputs;

    if (!skipWrite)
        SaveToJSON();

    if (changed && !skipWrite)
        LogRuntimeConfig("runtime config changed");

    if (errorMessage)
        *errorMessage = "";

    return true;
}
