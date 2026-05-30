//+--------------------------------------------------------------------------
//
// File:        webserver_settings.cpp
//
// NightDriverStrip - (c) 2018 Plummer's Software LLC.  All Rights Reserved.
//
// This file is part of webserver.cpp; see that file header for additional context.
//
// Split scope: web endpoints and handlers for settings retrieval and updates.
//---------------------------------------------------------------------------


#include "globals.h"

#if ENABLE_WEBSERVER

#include "webserver.h"

#include <AsyncJson.h>
#include <memory>
#include <mutex>
#include <optional>
#include <utility>

#include "audioservice.h"
#include "deviceconfig.h"
#include "effectmanager.h"
#include "ledstripeffect.h"
#include "systemcontainer.h"

namespace
{
    std::recursive_mutex g_settingSpecsCacheMutex;
}

bool CWebServer::ApplyAudioInputPinChange(int oldPin)
{
    if (!g_ptrSystem || !g_ptrSystem->HasDeviceConfig())
        return true;

    auto& deviceConfig = g_ptrSystem->GetDeviceConfig();
    const int newPin = deviceConfig.GetAudioInputPin();

    if (newPin == oldPin)
        return true;

    if (!deviceConfig.SupportsLiveAudioInputReconfigure())
        return true;

    if (!g_ptrSystem->HasAudioService())
        return true;

    auto& audioService = g_ptrSystem->GetAudioService();
    if (!audioService.Reconfigure(AudioConfig::FromCurrentSettings()))
    {
        debugW("Audio: live reconfigure failed; reverting persisted pin to %d", oldPin);
        deviceConfig.SetAudioInputPin(oldPin);
        return false;
    }
    return true;
}

void CWebServer::SendSettingSpecsResponse(AsyncWebServerRequest * pRequest, const std::vector<std::reference_wrapper<SettingSpec>> & settingSpecs)
{
    String json;
    if (!BuildSettingSpecsJson(json, settingSpecs))
    {
        debugW("Unable to build setting specs JSON response.");
        SendBufferOverflowResponse(pRequest);
        return;
    }

    AddCORSHeaderAndSendResponse(pRequest, pRequest->beginResponse(HttpOk, "application/json", json));
}

bool CWebServer::BuildSettingSpecsJson(String& json, const std::vector<std::reference_wrapper<SettingSpec>> & settingSpecs)
{
    auto jsonDoc = CreateJsonDocument();
    auto jsonArray = jsonDoc.to<JsonArray>();

    for (const auto& specWrapper : settingSpecs)
    {
        const auto& spec = specWrapper.get();
        auto specObject = jsonArray.add<JsonObject>();
        if (specObject.isNull())
        {
            debugW("Setting specs JSON object allocation failed.");
            return false;
        }

        specObject["name"] = spec.Name;
        specObject["friendlyName"] = spec.FriendlyName;
        if (spec.Description)
            specObject["description"] = spec.Description;
        specObject["type"] = to_value(spec.Type);
        specObject["typeName"] = spec.TypeName();
        if (spec.HasValidation)
            specObject["hasValidation"] = true;
        if (spec.MinimumValue.has_value())
            specObject["minimumValue"] = spec.MinimumValue.value();
        if (spec.MaximumValue.has_value())
            specObject["maximumValue"] = spec.MaximumValue.value();
        if (spec.EmptyAllowed.has_value())
            specObject["emptyAllowed"] = spec.EmptyAllowed.value();
        switch (spec.Access)
        {
            case SettingSpec::SettingAccess::ReadOnly:
                specObject["readOnly"] = true;
                break;

            case SettingSpec::SettingAccess::WriteOnly:
                specObject["writeOnly"] = true;
                break;

            default:
                break;
        }

        if (spec.Section)
            specObject["section"] = spec.Section;
        if (spec.Priority.has_value())
            specObject["priority"] = spec.Priority.value();
        if (spec.RequiresReboot)
            specObject["requiresReboot"] = true;
        if (spec.ApiPath)
            specObject["apiPath"] = spec.ApiPath;

        if (spec.Widget != SettingSpec::WidgetKind::Default)
        {
            auto widget = specObject["widget"].to<JsonObject>();
            widget["kind"] = spec.WidgetName();

            if (spec.Widget == SettingSpec::WidgetKind::Slider
                && spec.DisplayRawMin.has_value())
            {
                auto scale = widget["displayScale"].to<JsonObject>();
                scale["rawMin"]     = spec.DisplayRawMin.value();
                scale["rawMax"]     = spec.DisplayRawMax.value();
                scale["displayMin"] = spec.DisplayMin.value();
                scale["displayMax"] = spec.DisplayMax.value();
                if (spec.DisplaySuffix)
                    scale["suffix"] = spec.DisplaySuffix;
            }
            else if (spec.Widget == SettingSpec::WidgetKind::IntervalToggle)
            {
                auto interval = widget["interval"].to<JsonObject>();
                interval["unitDivisor"] = spec.IntervalUnitDivisor;
                if (spec.IntervalUnitLabel)
                    interval["unitLabel"] = spec.IntervalUnitLabel;
                if (spec.IntervalOnLabel)
                    interval["onLabel"] = spec.IntervalOnLabel;
                if (spec.IntervalOffLabel)
                    interval["offLabel"] = spec.IntervalOffLabel;
            }
            else if (spec.Widget == SettingSpec::WidgetKind::Select)
            {
                auto options = widget["options"].to<JsonObject>();
                options["source"] = spec.OptionsSourceName();

                auto addOptionArrays = [&]()
                {
                    auto valuesArr = options["values"].to<JsonArray>();
                    for (auto entry : spec.OptionValues)
                        valuesArr.add(entry ? entry : "");
                    auto labelsArr = options["labels"].to<JsonArray>();
                    for (auto entry : spec.OptionLabels)
                        labelsArr.add(entry ? entry : "");
                };

                if (spec.Options == SettingSpec::OptionsSource::Inline)
                {
                    addOptionArrays();
                }
                else if (spec.OptionsSchemaPath)
                {
                    options["schemaPath"] = spec.OptionsSchemaPath;
                    if (!spec.OptionValues.empty())
                        addOptionArrays();
                }

                if (spec.OptionsExternalUrl)
                    options["url"] = spec.OptionsExternalUrl;
            }
        }
    }

    if (jsonDoc.overflowed())
    {
        debugW("Setting specs JSON document overflowed.");
        return false;
    }

    const size_t measuredLength = measureJson(jsonArray);
    json = String();
    if (!json.reserve(measuredLength))
    {
        debugW("Unable to reserve %zu bytes for setting specs JSON.", measuredLength);
        return false;
    }

    const size_t serializedLength = serializeJson(jsonArray, json);
    if (serializedLength != measuredLength)
    {
        debugW("Setting specs JSON length mismatch: measured=%zu serialized=%zu.", measuredLength, serializedLength);
        return false;
    }

    return true;
}

bool CWebServer::EnsureDeviceSettingSpecsJson()
{
    std::lock_guard guard(g_settingSpecsCacheMutex);

    if (deviceSettingSpecsJson.length() > 0)
        return true;

    String json;
    if (!BuildSettingSpecsJson(json, LoadDeviceSettingSpecs()))
    {
        return false;
    }

    deviceSettingSpecsJson = json;
    debugI("WebServer: cached device setting specs JSON (%zu bytes).", (size_t)deviceSettingSpecsJson.length());
    return true;
}

const std::vector<std::reference_wrapper<SettingSpec>> & CWebServer::LoadDeviceSettingSpecs()
{
    std::lock_guard guard(g_settingSpecsCacheMutex);

    if (deviceSettingSpecs.empty())
    {
        mySettingSpecs.push_back(SettingSpec::Validate(SettingSpec{
            .Name                = "effectInterval",
            .FriendlyName        = "Effect interval",
            .Description         = "The duration in milliseconds that an individual effect runs, before the next effect is activated. "
                                   "Disable rotation to keep the current effect active indefinitely.",
            .Type                = SettingSpec::SettingType::PositiveBigInteger,
            .Section             = "appearance",
            .ApiPath             = "effects.effectInterval",
            .Widget              = SettingSpec::WidgetKind::IntervalToggle,
            .IntervalUnitDivisor = 1000,
            .IntervalOnLabel     = "Rotate effects",
            .IntervalOffLabel    = "Off",
            .IntervalUnitLabel   = "seconds"
        }));

        deviceSettingSpecs.insert(deviceSettingSpecs.end(), mySettingSpecs.begin(), mySettingSpecs.end());

        auto deviceConfigSpecs = g_ptrSystem->GetDeviceConfig().GetSettingSpecs();
        deviceSettingSpecs.insert(deviceSettingSpecs.end(), deviceConfigSpecs.begin(), deviceConfigSpecs.end());
    }

    return deviceSettingSpecs;
}

void CWebServer::GetSettingSpecs(AsyncWebServerRequest * pRequest)
{
    if (!EnsureDeviceSettingSpecsJson())
    {
        SendBufferOverflowResponse(pRequest);
        return;
    }

    auto response = pRequest->beginResponse(HttpOk, "application/json",
        reinterpret_cast<const uint8_t*>(deviceSettingSpecsJson.c_str()),
        deviceSettingSpecsJson.length());
    AddCORSHeaderAndSendResponse(pRequest, response);
}

void CWebServer::GetSettings(AsyncWebServerRequest * pRequest)
{
    debugV("GetSettings");

    auto response = new AsyncJsonResponse();
    response->addHeader("Server", "NightDriverStrip");
    auto root = response->getRoot();
    JsonObject jsonObject = root.to<JsonObject>();

    g_ptrSystem->GetDeviceConfig().SerializeToJSON(jsonObject, false);
    jsonObject["effectInterval"] = g_ptrSystem->GetEffectManager().GetInterval();

    AddCORSHeaderAndSendResponse(pRequest, response);
}

bool CWebServer::ValidateLegacyDeviceSettings(AsyncWebServerRequest * pRequest, String* errorMessage)
{
    auto& deviceConfig = g_ptrSystem->GetDeviceConfig();

    if (pRequest->hasParam(DeviceConfig::OpenWeatherApiKeyTag, true, false))
    {
        auto [isValid, validationMessage] =
            deviceConfig.ValidateOpenWeatherAPIKey(pRequest->getParam(DeviceConfig::OpenWeatherApiKeyTag, true, false)->value());
        if (!isValid)
        {
            if (errorMessage)
                *errorMessage = validationMessage;
            return false;
        }
    }

    if (pRequest->hasParam(DeviceConfig::PowerLimitTag, true, false))
    {
        auto [isValid, validationMessage] =
            deviceConfig.ValidatePowerLimit(pRequest->getParam(DeviceConfig::PowerLimitTag, true, false)->value().toInt());
        if (!isValid)
        {
            if (errorMessage)
                *errorMessage = validationMessage;
            return false;
        }
    }

    if (pRequest->hasParam(DeviceConfig::BrightnessTag, true, false))
    {
        auto [isValid, validationMessage] =
            deviceConfig.ValidateBrightness(pRequest->getParam(DeviceConfig::BrightnessTag, true, false)->value().toInt());
        if (!isValid)
        {
            if (errorMessage)
                *errorMessage = validationMessage;
            return false;
        }
    }

    if (pRequest->hasParam(DeviceConfig::AudioInputPinTag, true, false))
    {
        auto [isValid, validationMessage] =
            deviceConfig.ValidateAudioInputPin(pRequest->getParam(DeviceConfig::AudioInputPinTag, true, false)->value().toInt());
        if (!isValid)
        {
            if (errorMessage)
                *errorMessage = validationMessage;
            return false;
        }
    }

    if (errorMessage)
        *errorMessage = "";

    return true;
}

bool CWebServer::SetSettingsIfPresent(AsyncWebServerRequest * pRequest, String* errorMessage)
{
    auto& deviceConfig = g_ptrSystem->GetDeviceConfig();
    auto& effectManager = g_ptrSystem->GetEffectManager();

    if (!ValidateLegacyDeviceSettings(pRequest, errorMessage))
        return false;

    auto runtimeConfig = deviceConfig.GetRuntimeConfig();
    bool runtimeConfigChanged = false;

    runtimeConfigChanged = PushPostParamIfPresent<size_t>(pRequest, DeviceConfig::MatrixWidthTag, SET_VALUE(runtimeConfig.topology.width = value)) || runtimeConfigChanged;
    runtimeConfigChanged = PushPostParamIfPresent<size_t>(pRequest, DeviceConfig::MatrixHeightTag, SET_VALUE(runtimeConfig.topology.height = value)) || runtimeConfigChanged;
    runtimeConfigChanged = PushPostParamIfPresent<bool>(pRequest, DeviceConfig::MatrixSerpentineTag, SET_VALUE(runtimeConfig.topology.serpentine = value)) || runtimeConfigChanged;
    runtimeConfigChanged = PushPostParamIfPresent<size_t>(pRequest, DeviceConfig::WS281xChannelCountTag, SET_VALUE(runtimeConfig.outputs.channelCount = value)) || runtimeConfigChanged;
    runtimeConfigChanged = PushPostParamIfPresent<String>(pRequest, DeviceConfig::OutputDriverTag, SET_VALUE(runtimeConfig.outputs.driver = value == "hub75" ? DeviceConfig::OutputDriver::HUB75 : DeviceConfig::OutputDriver::WS281x)) || runtimeConfigChanged;
    runtimeConfigChanged = PushPostParamIfPresent<String>(pRequest, DeviceConfig::WS281xColorOrderTag, SET_VALUE(
        if (value == "RGB") runtimeConfig.outputs.colorOrder = DeviceConfig::WS281xColorOrder::RGB;
        else if (value == "RBG") runtimeConfig.outputs.colorOrder = DeviceConfig::WS281xColorOrder::RBG;
        else if (value == "GRB") runtimeConfig.outputs.colorOrder = DeviceConfig::WS281xColorOrder::GRB;
        else if (value == "GBR") runtimeConfig.outputs.colorOrder = DeviceConfig::WS281xColorOrder::GBR;
        else if (value == "BRG") runtimeConfig.outputs.colorOrder = DeviceConfig::WS281xColorOrder::BRG;
        else if (value == "BGR") runtimeConfig.outputs.colorOrder = DeviceConfig::WS281xColorOrder::BGR;
    )) || runtimeConfigChanged;

    if (runtimeConfigChanged)
    {
        auto [isValid, validationMessage] = deviceConfig.ValidateRuntimeConfig(runtimeConfig);
        if (!isValid)
        {
            if (errorMessage)
                *errorMessage = validationMessage;
            return false;
        }
    }

    if (runtimeConfigChanged)
    {
        auto [runtimeConfigApplied, runtimeErrorMessage] = g_ptrSystem->ApplyRuntimeConfigurationTransaction(runtimeConfig);
        if (!runtimeConfigApplied)
        {
            if (errorMessage)
                *errorMessage = runtimeErrorMessage;
            return false;
        }
    }

    PushPostParamIfPresent<size_t>(pRequest,"effectInterval", SET_VALUE(effectManager.SetInterval(value)));
    PushPostParamIfPresent<String>(pRequest, DeviceConfig::HostnameTag, SET_VALUE(deviceConfig.SetHostname(value)));
    PushPostParamIfPresent<String>(pRequest, DeviceConfig::LocationTag, SET_VALUE(deviceConfig.SetLocation(value)));
    PushPostParamIfPresent<bool>(pRequest, DeviceConfig::LocationIsZipTag, SET_VALUE(deviceConfig.SetLocationIsZip(value)));
    PushPostParamIfPresent<String>(pRequest, DeviceConfig::CountryCodeTag, SET_VALUE(deviceConfig.SetCountryCode(value)));
    PushPostParamIfPresent<String>(pRequest, DeviceConfig::OpenWeatherApiKeyTag, SET_VALUE(deviceConfig.SetOpenWeatherAPIKey(value)));
    PushPostParamIfPresent<String>(pRequest, DeviceConfig::TimeZoneTag, SET_VALUE(deviceConfig.SetTimeZone(value)));
    PushPostParamIfPresent<bool>(pRequest, DeviceConfig::Use24HourClockTag, SET_VALUE(deviceConfig.Set24HourClock(value)));
    PushPostParamIfPresent<bool>(pRequest, DeviceConfig::UseCelsiusTag, SET_VALUE(deviceConfig.SetUseCelsius(value)));
    PushPostParamIfPresent<String>(pRequest, DeviceConfig::NTPServerTag, SET_VALUE(deviceConfig.SetNTPServer(value)));
    PushPostParamIfPresent<bool>(pRequest, DeviceConfig::RememberCurrentEffectTag, SET_VALUE(deviceConfig.SetRememberCurrentEffect(value)));
    PushPostParamIfPresent<int>(pRequest, DeviceConfig::PowerLimitTag, SET_VALUE(deviceConfig.SetPowerLimit(value)));
    PushPostParamIfPresent<int>(pRequest, DeviceConfig::BrightnessTag, SET_VALUE(deviceConfig.SetBrightness(value)));

    {
        const int oldPin = deviceConfig.GetAudioInputPin();
        const bool audioInputPinChanged =
            PushPostParamIfPresent<int>(pRequest, DeviceConfig::AudioInputPinTag, SET_VALUE(deviceConfig.SetAudioInputPin(value)));
        if (audioInputPinChanged && !ApplyAudioInputPinChange(oldPin))
        {
            if (errorMessage)
                *errorMessage = "Audio input pin live apply failed";
            return false;
        }
    }

    #if SHOW_VU_METER
    PushPostParamIfPresent<bool>(pRequest, DeviceConfig::ShowVUMeterTag, SET_VALUE(effectManager.ShowVU(value)));
    #endif

    std::optional<CRGB> globalColor = {};
    std::optional<CRGB> secondColor = {};

    PushPostParamIfPresent<CRGB>(pRequest, DeviceConfig::GlobalColorTag, SET_VALUE(globalColor = value));
    PushPostParamIfPresent<CRGB>(pRequest, DeviceConfig::SecondColorTag, SET_VALUE(secondColor = value));

    deviceConfig.ApplyColorSettings(globalColor, secondColor,
                                    IsPostParamTrue(pRequest, DeviceConfig::ClearGlobalColorTag),
                                    IsPostParamTrue(pRequest, DeviceConfig::ApplyGlobalColorsTag));

    if (errorMessage)
        *errorMessage = "";

    return true;
}

void CWebServer::SetSettings(AsyncWebServerRequest * pRequest)
{
    debugV("SetSettings");

    String errorMessage;
    if (!SetSettingsIfPresent(pRequest, &errorMessage))
    {
        AddCORSHeaderAndSendBadRequest(pRequest, errorMessage);
        return;
    }

    GetSettings(pRequest);
}

void CWebServer::ValidateAndSetSetting(AsyncWebServerRequest * pRequest)
{
    String paramName;

    for (auto& settingSpecWrapper : LoadDeviceSettingSpecs())
    {
        auto& settingSpec = settingSpecWrapper.get();

        if (settingSpec.ApiPath)
            continue;

        if (pRequest->hasParam(settingSpec.Name, true))
        {
            if (paramName.isEmpty())
                paramName = settingSpec.Name;
            else
            {
                AddCORSHeaderAndSendBadRequest(pRequest, "Malformed request");
                return;
            }
        }
    }

    if (paramName.isEmpty())
    {
        AddCORSHeaderAndSendOKResponse(pRequest);
        return;
    }

    auto validator = settingValidators.find(paramName);
    if (validator != settingValidators.end())
    {
        const String &paramValue = pRequest->getParam(paramName, true)->value();
        bool isValid;
        String validationMessage;

        std::tie(isValid, validationMessage) = validator->second(paramValue);

        if (!isValid)
        {
            AddCORSHeaderAndSendBadRequest(pRequest, validationMessage);
            return;
        }
    }

    String errorMessage;
    if (!SetSettingsIfPresent(pRequest, &errorMessage))
    {
        AddCORSHeaderAndSendBadRequest(pRequest, errorMessage);
        return;
    }
    AddCORSHeaderAndSendOKResponse(pRequest);
}

#endif  // ENABLE_WEBSERVER
