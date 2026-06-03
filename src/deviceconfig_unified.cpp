//+--------------------------------------------------------------------------
//
// File:        deviceconfig_unified.cpp
//
// NightDriverStrip - (c) 2018 Plummer's Software LLC.  All Rights Reserved.
//
// This file is part of deviceconfig.cpp; see that file header for additional context.
//
// Split scope: unified settings API mapping and DeviceConfig integration helpers.
//---------------------------------------------------------------------------


#include "globals.h"

#include <array>
#include <optional>

#include "audioservice.h"
#include "deviceconfig.h"
#include "deviceconfig_internal.h"
#include "jsonserializer.h"
#include "systemcontainer.h"

const char* DeviceConfig::OutputDriverName(OutputDriver driver)
{
    return DriverName(driver);
}

const char* DeviceConfig::WS281xColorOrderName(WS281xColorOrder colorOrder)
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

std::optional<DeviceConfig::OutputDriver> DeviceConfig::ParseOutputDriverName(const String& name)
{
    if (name == "hub75")
        return OutputDriver::HUB75;
    if (name == "apa102")
        return OutputDriver::APA102;
    if (name == "ws281x")
        return OutputDriver::WS281x;

    return std::nullopt;
}

std::optional<DeviceConfig::WS281xColorOrder> DeviceConfig::ParseWS281xColorOrderName(const String& name)
{
    if (name == "RGB") return WS281xColorOrder::RGB;
    if (name == "RBG") return WS281xColorOrder::RBG;
    if (name == "GRB") return WS281xColorOrder::GRB;
    if (name == "GBR") return WS281xColorOrder::GBR;
    if (name == "BRG") return WS281xColorOrder::BRG;
    if (name == "BGR") return WS281xColorOrder::BGR;

    return std::nullopt;
}

SuccessResultWithMessage DeviceConfig::SetRuntimeConfig(const RuntimeConfig& config, bool skipWrite)
{
    auto [isValid, validationMessage] = ValidateRuntimeConfig(config);
    if (!isValid)
        return { false, validationMessage };

    const bool changed =
        runtimeTopology.width != config.topology.width
        || runtimeTopology.height != config.topology.height
        || runtimeTopology.serpentine != config.topology.serpentine
        || runtimeOutputs.driver != config.outputs.driver
        || runtimeOutputs.channelCount != config.outputs.channelCount
        || runtimeOutputs.outputPins != config.outputs.outputPins
        || runtimeOutputs.clockPins != config.outputs.clockPins
        || runtimeOutputs.colorOrder != config.outputs.colorOrder;

    runtimeTopology = config.topology;
    runtimeOutputs = config.outputs;

    if (!skipWrite)
        SaveToJSON();

    if (changed && !skipWrite)
        LogRuntimeConfig("runtime config changed");

    return { true, "" };
}

void DeviceConfig::AppendPins(JsonArray target, const std::array<int8_t, NUM_CHANNELS>& pins)
{
    for (auto pin : pins)
        target.add(pin);
}

ResultWithMessage<std::optional<int>> DeviceConfig::ResolveUnifiedAudioInputPin(JsonObjectConst device)
{
    std::optional<int> requestedAudioInputPin;

    if (device[DeviceConfig::AudioInputPinTag].is<int>())
        requestedAudioInputPin = device[DeviceConfig::AudioInputPinTag].as<int>();

    if (device["audio"].is<JsonObjectConst>())
    {
        auto audio = device["audio"].as<JsonObjectConst>();
        if (audio["audioInputPin"].is<int>())
        {
            const int nestedAudioInputPin = audio["audioInputPin"].as<int>();
            if (requestedAudioInputPin.has_value() && requestedAudioInputPin.value() != nestedAudioInputPin)
                return { std::nullopt, "Malformed request" };

            requestedAudioInputPin = nestedAudioInputPin;
        }
    }

    return { requestedAudioInputPin, "" };
}

void DeviceConfig::SerializeUnifiedSettings(JsonObject root) const
{
    auto device = root["device"].to<JsonObject>();
    device["hostname"] = GetHostname();
    device["location"] = GetLocation();
    device["locationIsZip"] = IsLocationZip();
    device["countryCode"] = GetCountryCode();
    device["timeZone"] = GetTimeZone();
    device["use24HourClock"] = Use24HourClock();
    device["useCelsius"] = UseCelsius();
    device["ntpServer"] = GetNTPServer();
    device["rememberCurrentEffect"] = RememberCurrentEffect();
    device["powerLimit"] = GetPowerLimit();
    device["brightness"] = GetBrightness();
    device["globalColor"] = GlobalColor();
    device["secondColor"] = SecondColor();
    device["applyGlobalColors"] = ApplyGlobalColors();

    #if ENABLE_REMOTE
    auto remote = device["remote"].to<JsonObject>();
    remote["enabled"] = true;
    remote["pin"] = IR_REMOTE_PIN;
    #else
    auto remote = device["remote"].to<JsonObject>();
    remote["enabled"] = false;
    remote["pin"] = -1;
    #endif

    auto audio = device["audio"].to<JsonObject>();
    audio["enabled"] =
    #if ENABLE_AUDIO
        true;
    #else
        false;
    #endif
    audio["audioInputPin"] = GetAudioInputPin();
    audio["compiledDefaultPin"] = GetCompiledAudioInputPin();
    audio["mode"] = GetAudioInputModeName();
    audio["liveApply"] = SupportsLiveAudioInputReconfigure();
    audio["requiresReboot"] = !SupportsLiveAudioInputReconfigure();
    audio["supportsPinOverride"] = SupportsConfigurableAudioInputPin();

    auto topology = root["topology"].to<JsonObject>();
    topology["width"] = GetMatrixWidth();
    topology["height"] = GetMatrixHeight();
    topology["serpentine"] = IsMatrixSerpentine();
    topology["ledCount"] = GetActiveLEDCount();
    topology["liveApply"] = SupportsLiveTopology();

    auto outputs = root["outputs"].to<JsonObject>();
    outputs["driver"] = GetRuntimeDriverName();
    outputs["compiledDriver"] = GetCompiledDriverName();
    outputs["liveApply"] = SupportsLiveOutputReconfigure();

    auto ws281x = outputs["ws281x"].to<JsonObject>();
    ws281x["channelCount"] = GetChannelCount();
    ws281x["compiledMaxChannels"] = GetCompiledChannelCount();
    ws281x["colorOrder"] = GetColorOrderName(GetWS281xColorOrder());
    ws281x["compiledColorOrder"] = GetColorOrderName(GetCompiledWS281xColorOrder());
    AppendPins(ws281x["pins"].to<JsonArray>(), GetWS281xPins());

    auto apa102 = outputs["apa102"].to<JsonObject>();
    apa102["channelCount"] = GetChannelCount();
    apa102["compiledMaxChannels"] = GetCompiledChannelCount();
    apa102["colorOrder"] = GetColorOrderName(GetWS281xColorOrder());
    apa102["compiledColorOrder"] = GetColorOrderName(GetCompiledWS281xColorOrder());
    AppendPins(apa102["dataPins"].to<JsonArray>(), GetAPA102DataPins());
    AppendPins(apa102["clockPins"].to<JsonArray>(), GetAPA102ClockPins());
}

void DeviceConfig::SerializeUnifiedSettingsSchema(JsonObject root) const
{
    auto topology = root["topology"].to<JsonObject>();
    topology["compiledMaxWidth"] =
        GetCompiledOutputDriver() == OutputDriver::HUB75
            ? GetCompiledMatrixWidth()
            : GetCompiledLEDCount();
    topology["compiledMaxHeight"] =
        GetCompiledOutputDriver() == OutputDriver::HUB75
            ? GetCompiledMatrixHeight()
            : GetCompiledLEDCount();
    topology["compiledNominalWidth"] = GetCompiledMatrixWidth();
    topology["compiledNominalHeight"] = GetCompiledMatrixHeight();
    topology["compiledMaxLEDs"] = GetCompiledLEDCount();
    topology["liveApply"] = SupportsLiveTopology();
    topology["rejectMessage"] = DeviceConfigInternal::RecompileNeededMessage();

    auto outputs = root["outputs"].to<JsonObject>();
    outputs["compiledDriver"] = GetCompiledDriverName();
    outputs["liveApply"] = SupportsLiveOutputReconfigure();
    outputs["rejectMessage"] = DeviceConfigInternal::RecompileNeededMessage();

    auto drivers = outputs["allowedDrivers"].to<JsonArray>();
    drivers.add(GetCompiledDriverName());

    auto ws281x = outputs["ws281x"].to<JsonObject>();
    const auto compiledChannels = GetCompiledChannelCount();
    ws281x["compiledMaxChannels"] = compiledChannels;
    ws281x["compiledMaxLEDs"] = GetCompiledLEDCount();
    ws281x["compiledColorOrder"] = GetColorOrderName(GetCompiledWS281xColorOrder());
    auto allowedChannelCounts = ws281x["allowedChannelCounts"].to<JsonArray>();
    for (size_t channel = 1; channel <= compiledChannels; ++channel)
        allowedChannelCounts.add(channel);

    auto allowedColorOrders = ws281x["allowedColorOrders"].to<JsonArray>();
    allowedColorOrders.add("RGB");
    allowedColorOrders.add("RBG");
    allowedColorOrders.add("GRB");
    allowedColorOrders.add("GBR");
    allowedColorOrders.add("BRG");
    allowedColorOrders.add("BGR");
    AppendPins(ws281x["compiledPins"].to<JsonArray>(), GetCompiledWS281xPins());

    auto apa102 = outputs["apa102"].to<JsonObject>();
    apa102["compiledMaxChannels"] = compiledChannels;
    apa102["compiledMaxLEDs"] = GetCompiledLEDCount();
    apa102["compiledColorOrder"] = GetColorOrderName(GetCompiledWS281xColorOrder());
    auto apa102AllowedChannelCounts = apa102["allowedChannelCounts"].to<JsonArray>();
    for (size_t channel = 1; channel <= compiledChannels; ++channel)
        apa102AllowedChannelCounts.add(channel);

    auto apa102AllowedColorOrders = apa102["allowedColorOrders"].to<JsonArray>();
    apa102AllowedColorOrders.add("RGB");
    apa102AllowedColorOrders.add("RBG");
    apa102AllowedColorOrders.add("GRB");
    apa102AllowedColorOrders.add("GBR");
    apa102AllowedColorOrders.add("BRG");
    apa102AllowedColorOrders.add("BGR");
    AppendPins(apa102["compiledDataPins"].to<JsonArray>(), GetCompiledWS281xPins());
    AppendPins(apa102["compiledClockPins"].to<JsonArray>(), GetCompiledAPA102ClockPins());

    auto device = root["device"].to<JsonObject>();
    auto remote = device["remote"].to<JsonObject>();
    remote["enabled"] =
    #if ENABLE_REMOTE
        true;
    #else
        false;
    #endif
    remote["pin"] = IR_REMOTE_PIN;

    auto audio = device["audio"].to<JsonObject>();
    audio["enabled"] =
    #if ENABLE_AUDIO
        true;
    #else
        false;
    #endif
    audio["compiledDefaultPin"] = GetCompiledAudioInputPin();
    audio["mode"] = GetAudioInputModeName();
    audio["liveApply"] = SupportsLiveAudioInputReconfigure();
    audio["requiresReboot"] = !SupportsLiveAudioInputReconfigure();
    audio["supportsPinOverride"] = SupportsConfigurableAudioInputPin();
    audio["rejectMessage"] = DeviceConfigInternal::RecompileNeededMessage();

    struct SectionInfo
    {
        const char* id;
        const char* title;
        const char* description;
    };

    static constexpr SectionInfo kSections[] =
    {
        { "topology",   "Topology",          "Active matrix dimensions and layout." },
        { "output",     "Output",            "LED driver, channel count, color order, and per-channel pin assignments." },
        { "appearance", "Appearance",        "Brightness, colors, effect rotation, and visual preferences." },
        { "audio",      "Audio",             "Microphone input pin and audio capture configuration." },
        { "location",   "Location",          "Where the device is for weather and timezone defaults." },
        { "clock",      "Clock & Weather",   "Time display, NTP, and weather API options." },
        { "system",     "System",            "Identification, power limits, and other system-wide options." },
    };

    auto sections = root["sections"].to<JsonArray>();
    for (const auto& info : kSections)
    {
        auto entry = sections.add<JsonObject>();
        entry["id"] = info.id;
        entry["title"] = info.title;
        entry["description"] = info.description;
    }
}

SuccessResultWithMessage DeviceConfig::ParseAndValidateUnifiedSettings(JsonObjectConst root, UnifiedSettingsRequest& out) const
{
    out = UnifiedSettingsRequest{};
    out.requestedRuntimeConfig = GetRuntimeConfig();

    if (root["topology"].is<JsonObjectConst>())
    {
        auto topology = root["topology"].as<JsonObjectConst>();
        if (topology["width"].is<uint16_t>())
        {
            out.requestedRuntimeConfig.topology.width = topology["width"].as<uint16_t>();
            out.runtimeConfigTouched = true;
        }
        if (topology["height"].is<uint16_t>())
        {
            out.requestedRuntimeConfig.topology.height = topology["height"].as<uint16_t>();
            out.runtimeConfigTouched = true;
        }
        if (topology["serpentine"].is<bool>())
        {
            out.requestedRuntimeConfig.topology.serpentine = topology["serpentine"].as<bool>();
            out.runtimeConfigTouched = true;
        }
    }

    if (root["outputs"].is<JsonObjectConst>())
    {
        auto outputs = root["outputs"].as<JsonObjectConst>();
        if (outputs["driver"].is<String>())
        {
            const auto driver = outputs["driver"].as<String>();
            auto parsedDriver = ParseOutputDriverName(driver);
            if (!parsedDriver.has_value())
                return { false, "invalid output driver" };

            out.requestedRuntimeConfig.outputs.driver = parsedDriver.value();
            out.runtimeConfigTouched = true;
        }

        if (outputs["ws281x"].is<JsonObjectConst>())
        {
            auto ws281x = outputs["ws281x"].as<JsonObjectConst>();
            if (ws281x["channelCount"].is<size_t>())
            {
                out.requestedRuntimeConfig.outputs.channelCount = ws281x["channelCount"].as<size_t>();
                out.runtimeConfigTouched = true;
            }
            if (ws281x["colorOrder"].is<String>())
            {
                auto colorOrder = ParseWS281xColorOrderName(ws281x["colorOrder"].as<String>());
                if (!colorOrder.has_value())
                    return { false, "invalid WS281x color order" };

                out.requestedRuntimeConfig.outputs.colorOrder = colorOrder.value();
                out.runtimeConfigTouched = true;
            }
            if (ws281x["pins"].is<JsonArrayConst>())
            {
                auto pins = ws281x["pins"].as<JsonArrayConst>();
                for (size_t i = 0; i < out.requestedRuntimeConfig.outputs.outputPins.size() && i < pins.size(); ++i)
                {
                    if (pins[i].is<int>())
                    {
                        out.requestedRuntimeConfig.outputs.outputPins[i] = pins[i].as<int>();
                        out.runtimeConfigTouched = true;
                    }
                }
            }
        }

        if (outputs["apa102"].is<JsonObjectConst>())
        {
            auto apa102 = outputs["apa102"].as<JsonObjectConst>();
            if (apa102["channelCount"].is<size_t>())
            {
                out.requestedRuntimeConfig.outputs.channelCount = apa102["channelCount"].as<size_t>();
                out.runtimeConfigTouched = true;
            }
            if (apa102["colorOrder"].is<String>())
            {
                auto colorOrder = ParseWS281xColorOrderName(apa102["colorOrder"].as<String>());
                if (!colorOrder.has_value())
                    return { false, "invalid APA102 color order" };

                out.requestedRuntimeConfig.outputs.colorOrder = colorOrder.value();
                out.runtimeConfigTouched = true;
            }
            if (apa102["dataPins"].is<JsonArrayConst>())
            {
                auto pins = apa102["dataPins"].as<JsonArrayConst>();
                for (size_t i = 0; i < out.requestedRuntimeConfig.outputs.outputPins.size() && i < pins.size(); ++i)
                {
                    if (pins[i].is<int>())
                    {
                        out.requestedRuntimeConfig.outputs.outputPins[i] = pins[i].as<int>();
                        out.runtimeConfigTouched = true;
                    }
                }
            }
            if (apa102["clockPins"].is<JsonArrayConst>())
            {
                auto pins = apa102["clockPins"].as<JsonArrayConst>();
                for (size_t i = 0; i < out.requestedRuntimeConfig.outputs.clockPins.size() && i < pins.size(); ++i)
                {
                    if (pins[i].is<int>())
                    {
                        out.requestedRuntimeConfig.outputs.clockPins[i] = pins[i].as<int>();
                        out.runtimeConfigTouched = true;
                    }
                }
            }
        }
    }

    auto [runtimeConfigValid, runtimeConfigMessage] = ValidateRuntimeConfig(out.requestedRuntimeConfig);
    if (!runtimeConfigValid)
        return { false, runtimeConfigMessage };

    if (root["device"].is<JsonObjectConst>())
    {
        auto device = root["device"].as<JsonObjectConst>();

        FieldAccess::AssignIfPresent(device, HostnameTag, out.hostname);
        FieldAccess::AssignIfPresent(device, LocationTag, out.location);
        FieldAccess::AssignIfPresent(device, LocationIsZipTag, out.locationIsZip);
        FieldAccess::AssignIfPresent(device, CountryCodeTag, out.countryCode);
        FieldAccess::AssignIfPresent(device, TimeZoneTag, out.timeZone);
        FieldAccess::AssignIfPresent(device, Use24HourClockTag, out.use24HourClock);
        FieldAccess::AssignIfPresent(device, UseCelsiusTag, out.useCelsius);
        FieldAccess::AssignIfPresent(device, NTPServerTag, out.ntpServer);
        FieldAccess::AssignIfPresent(device, RememberCurrentEffectTag, out.rememberCurrentEffect);

        if (device[OpenWeatherApiKeyTag].is<String>())
        {
            const auto requestedKey = device[OpenWeatherApiKeyTag].as<String>();
            auto [isValid, validationMessage] = ValidateOpenWeatherAPIKey(requestedKey);
            if (!isValid)
                return { false, validationMessage };

            out.openWeatherApiKey = requestedKey;
        }

        if (device[PowerLimitTag].is<int>())
        {
            const int requestedPowerLimit = device[PowerLimitTag].as<int>();
            auto [isValid, validationMessage] = ValidatePowerLimit(requestedPowerLimit);
            if (!isValid)
                return { false, validationMessage };

            out.powerLimit = requestedPowerLimit;
        }

        if (device[BrightnessTag].is<int>())
        {
            const int requestedBrightness = device[BrightnessTag].as<int>();
            auto [isValid, validationMessage] = ValidateBrightness(requestedBrightness);
            if (!isValid)
                return { false, validationMessage };

            out.brightness = requestedBrightness;
        }

        const bool hasTopLevelAudioInputPin = device[DeviceConfig::AudioInputPinTag].is<int>();
        const bool hasNestedAudioInputPin =
            device["audio"].is<JsonObjectConst>()
            && device["audio"].as<JsonObjectConst>()["audioInputPin"].is<int>();

        if (hasTopLevelAudioInputPin || hasNestedAudioInputPin)
        {
            auto [requestedAudioInputPin, audioPinResolveMessage] = ResolveUnifiedAudioInputPin(device);
            if (!requestedAudioInputPin.has_value())
                return { false, audioPinResolveMessage };

            auto [isValid, validationMessage] = ValidateAudioInputPin(requestedAudioInputPin.value());
            if (!isValid)
                return { false, validationMessage };

            out.audioInputPin = requestedAudioInputPin.value();
        }

        FieldAccess::AssignIfPresent(device, GlobalColorTag, out.globalColor);
        FieldAccess::AssignIfPresent(device, SecondColorTag, out.secondColor);
        out.clearGlobalColor = device[ClearGlobalColorTag].is<bool>() && device[ClearGlobalColorTag].as<bool>();
        out.applyGlobalColors = device[ApplyGlobalColorsTag].is<bool>() && device[ApplyGlobalColorsTag].as<bool>();
    }

    return { true, "" };
}

SuccessResultWithMessage DeviceConfig::ApplyUnifiedDeviceSettings(const UnifiedSettingsRequest& request)
{
    FieldAccess::ApplyIfPresent(request.hostname, *this, &DeviceConfig::SetHostname);
    FieldAccess::ApplyIfPresent(request.location, *this, &DeviceConfig::SetLocation);
    FieldAccess::ApplyIfPresent(request.locationIsZip, *this, &DeviceConfig::SetLocationIsZip);
    FieldAccess::ApplyIfPresent(request.countryCode, *this, &DeviceConfig::SetCountryCode);
    FieldAccess::ApplyIfPresent(request.openWeatherApiKey, *this, &DeviceConfig::SetOpenWeatherAPIKey);
    FieldAccess::ApplyIfPresent(request.timeZone, [this](const String& value) { SetTimeZone(value); });
    FieldAccess::ApplyIfPresent(request.use24HourClock, *this, &DeviceConfig::Set24HourClock);
    FieldAccess::ApplyIfPresent(request.useCelsius, *this, &DeviceConfig::SetUseCelsius);
    FieldAccess::ApplyIfPresent(request.ntpServer, *this, &DeviceConfig::SetNTPServer);
    FieldAccess::ApplyIfPresent(request.rememberCurrentEffect, *this, &DeviceConfig::SetRememberCurrentEffect);
    FieldAccess::ApplyIfPresent(request.powerLimit, *this, &DeviceConfig::SetPowerLimit);
    FieldAccess::ApplyIfPresent(request.brightness, *this, &DeviceConfig::SetBrightness);

    if (request.audioInputPin.has_value())
    {
        const int oldPin = GetAudioInputPin();
        SetAudioInputPin(request.audioInputPin.value());

        const int newPin = GetAudioInputPin();
        if (newPin != oldPin && SupportsLiveAudioInputReconfigure() && g_ptrSystem && g_ptrSystem->HasAudioService())
        {
            auto& audioService = g_ptrSystem->GetAudioService();
            if (!audioService.Reconfigure(AudioConfig::FromCurrentSettings()))
            {
                SetAudioInputPin(oldPin);
                return { false, "Audio input pin live apply failed" };
            }
        }
    }

    ApplyColorSettings(request.globalColor,
                       request.secondColor,
                       request.clearGlobalColor,
                       request.applyGlobalColors);

    return { true, "" };
}
