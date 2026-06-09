//+--------------------------------------------------------------------------
//
// File:        deviceconfig_settings_specs.cpp
//
// NightDriverStrip - (c) 2018 Plummer's Software LLC.  All Rights Reserved.
//
// This file is part of deviceconfig.cpp; see that file header for additional context.
//
// Split scope: DeviceConfig setting specification/schema definitions.
//---------------------------------------------------------------------------


#include "globals.h"

#include "deviceconfig.h"

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
            .Description   = "The maximum power in mW that the LEDs attached to the board are allowed to use.",
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
        settingSpecs.push_back(SettingSpec::Validate(SettingSpec{
            .Name         = RemoteEffectButtonsResetIntervalTag,
            .FriendlyName = "Remote effect buttons reset interval",
            .Description  = "When enabled, remote B+/B- effect changes force the effect interval back to the default rotation speed (60 seconds). "
                            "Disable this to keep your configured interval, including 0 (no timeout), when changing effects with the remote.",
            .Type         = SettingSpec::SettingType::Boolean,
            .Section      = kSectionAppearance,
            .Priority     = 101,
            .ApiPath      = "device.remote.resetEffectInterval"
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
            .Options           = SettingSpec::OptionsSource::SchemaPath,
            .OptionValues      = {"ws281x", "apa102", "hub75"},
            .OptionLabels      = {"WS281x", "APA102", "HUB75"},
            .OptionsSchemaPath = "outputs.allowedDrivers"
        }));
        settingSpecs.push_back(SettingSpec::Validate(SettingSpec{
            .Name              = WS281xChannelCountTag,
            .FriendlyName      = "Strip channel count",
            .Description       = "Number of active strip channels within the compiled maximum.",
            .Type              = SettingSpec::SettingType::PositiveBigInteger,
            .MinimumValue      = 1.0,
            .MaximumValue      = (double)GetCompiledChannelCount(),
            .Section           = kSectionOutput,
            .Priority          = 11,
            .ApiPath           =
            #if USE_APA102
                "outputs.apa102.channelCount",
            #else
                "outputs.ws281x.channelCount",
            #endif
            .Widget            = SettingSpec::WidgetKind::Select,
            .Options           = SettingSpec::OptionsSource::SchemaPath,
            .OptionsSchemaPath =
            #if USE_APA102
                "outputs.apa102.allowedChannelCounts"
            #else
                "outputs.ws281x.allowedChannelCounts"
            #endif
        }));
        settingSpecs.push_back(SettingSpec::Validate(SettingSpec{
            .Name              = WS281xColorOrderTag,
            .FriendlyName      = "Strip color order",
            .Description       = "Byte order used when streaming RGB values to the strip. This applies live on strip builds and is ignored on HUB75 builds.",
            .Type              = SettingSpec::SettingType::String,
            .Section           = kSectionOutput,
            .Priority          = 12,
            .ApiPath           =
            #if USE_APA102
                "outputs.apa102.colorOrder",
            #else
                "outputs.ws281x.colorOrder",
            #endif
            .Widget            = SettingSpec::WidgetKind::Select,
            .Options           = SettingSpec::OptionsSource::SchemaPath,
            .OptionsSchemaPath =
            #if USE_APA102
                "outputs.apa102.allowedColorOrders"
            #else
                "outputs.ws281x.allowedColorOrders"
            #endif
        }));

        constexpr size_t kPinStringsPerChannel =
        #if USE_APA102
            8;
        #else
            4;
        #endif
        const auto compiledChannelCount = GetCompiledChannelCount();
        settingSpecs.reserve(settingSpecs.size() + compiledChannelCount
        #if USE_APA102
            * 2
        #endif
        );
        pinSpecStrings.reserve(compiledChannelCount * kPinStringsPerChannel);
        const auto stableCStr = [](const String& s) { return s.c_str(); };

        for (size_t i = 0; i < compiledChannelCount; ++i)
        {
            const auto& nameStr        = pinSpecStrings.emplace_back(str_sprintf("stripDataPin%zu", i));
            const auto& friendlyStr    = pinSpecStrings.emplace_back(
                #if USE_APA102
                    str_sprintf("APA102 data pin %zu", i + 1)
                #else
                    str_sprintf("WS281x pin %zu", i + 1)
                #endif
            );
            const auto& descriptionStr = pinSpecStrings.emplace_back(
                #if USE_APA102
                    str_sprintf("GPIO assigned to APA102 data for channel %zu.", i + 1)
                #else
                    str_sprintf("GPIO assigned to WS281x channel %zu.", i + 1)
                #endif
            );
            const auto& apiPathStr     = pinSpecStrings.emplace_back(
                #if USE_APA102
                    str_sprintf("outputs.apa102.dataPins[%zu]", i)
                #else
                    str_sprintf("outputs.ws281x.pins[%zu]", i)
                #endif
            );

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

            #if USE_APA102
            const auto& clockNameStr        = pinSpecStrings.emplace_back(str_sprintf("apa102ClockPin%zu", i));
            const auto& clockFriendlyStr    = pinSpecStrings.emplace_back(str_sprintf("APA102 clock pin %zu", i + 1));
            const auto& clockDescriptionStr = pinSpecStrings.emplace_back(str_sprintf("GPIO assigned to APA102 clock for channel %zu.", i + 1));
            const auto& clockApiPathStr     = pinSpecStrings.emplace_back(str_sprintf("outputs.apa102.clockPins[%zu]", i));

            settingSpecs.push_back(SettingSpec::Validate(SettingSpec{
                .Name         = stableCStr(clockNameStr),
                .FriendlyName = stableCStr(clockFriendlyStr),
                .Description  = stableCStr(clockDescriptionStr),
                .Type         = SettingSpec::SettingType::Integer,
                .MinimumValue = -1.0,
                .MaximumValue = 48.0,
                .Section      = kSectionOutput,
                .Priority     = 3 + static_cast<int>(compiledChannelCount) + static_cast<int>(i),
                .ApiPath      = stableCStr(clockApiPathStr)
            }));
            #endif
        }

        settingSpecReferences.insert(settingSpecReferences.end(), settingSpecs.begin(), settingSpecs.end());
    }

    return settingSpecReferences;
}
