//+--------------------------------------------------------------------------
//
// File:        webserver.cpp
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
//   Implementations for some of the web server methods declared in webserver.h
//
// History:     Apr-18-2023         Rbergen     Created
//              Apr-28-2023         Rbergen     Reduce code duplication
//---------------------------------------------------------------------------

#include "globals.h"
#include "webserver.h"

// Maps settings for which a validator is available to the invocation thereof
const std::map<String, CWebServer::ValueValidator> CWebServer::settingValidators
{
    { DeviceConfig::OpenWeatherApiKeyTag, [](const String& value) { return g_ptrDeviceConfig->ValidateOpenWeatherAPIKey(value); } }
};

std::vector<SettingSpec> CWebServer::deviceSettingSpecs{};

// Member function template specialzations

// Push param that represents a bool. Values considered true are text "true" and any whole number not equal to 0
template<>
bool CWebServer::PushPostParamIfPresent<bool>(AsyncWebServerRequest * pRequest, const String &paramName, ValueSetter<bool> setter)
{
    return PushPostParamIfPresent<bool>(pRequest, paramName, setter, [](AsyncWebParameter * param) constexpr
    {
        const String& value = param->value();
        return value == "true" || strtol(value.c_str(), NULL, 10);
    });
}

// Push param that represents a size_t
template<>
bool CWebServer::PushPostParamIfPresent<size_t>(AsyncWebServerRequest * pRequest, const String &paramName, ValueSetter<size_t> setter)
{
    return PushPostParamIfPresent<size_t>(pRequest, paramName, setter, [](AsyncWebParameter * param) constexpr
    {
        return strtoul(param->value().c_str(), NULL, 10);
    });
}

// Add CORS header to and send JSON response
template<>
void CWebServer::AddCORSHeaderAndSendResponse<AsyncJsonResponse>(AsyncWebServerRequest * pRequest, AsyncJsonResponse * pResponse)
{
    pResponse->setLength();
    AddCORSHeaderAndSendResponse<AsyncWebServerResponse>(pRequest, pResponse);
}

// Member function implementations

bool CWebServer::IsPostParamTrue(AsyncWebServerRequest * pRequest, const String & paramName)
{
    bool returnValue = false;

    PushPostParamIfPresent<bool>(pRequest, paramName, [&returnValue](auto value) { returnValue = value; return true; });

    return returnValue;
}

long CWebServer::GetEffectIndexFromParam(AsyncWebServerRequest * pRequest, bool post)
{
    if (!pRequest->hasParam("effectIndex", post, false))
        return -1;

    return strtol(pRequest->getParam("effectIndex", post, false)->value().c_str(), NULL, 10);
}

void CWebServer::GetEffectListText(AsyncWebServerRequest * pRequest)
{
    static size_t jsonBufferSize = JSON_BUFFER_BASE_SIZE;
    bool bufferOverflow;
    debugV("GetEffectListText");

    do
    {
        bufferOverflow = false;
        auto response = std::make_unique<AsyncJsonResponse>(false, jsonBufferSize);
        auto j = response->getRoot();

        j["currentEffect"]         = g_ptrEffectManager->GetCurrentEffectIndex();
        j["millisecondsRemaining"] = g_ptrEffectManager->GetTimeRemainingForCurrentEffect();
        j["effectInterval"]        = g_ptrEffectManager->GetInterval();
        j["enabledCount"]          = g_ptrEffectManager->EnabledCount();

        auto effectsList = g_ptrEffectManager->EffectsList();

        for (int i = 0; i < g_ptrEffectManager->EffectCount(); i++)
        {
            StaticJsonDocument<256> effectDoc;

            auto effect = effectsList[i];
            effectDoc["name"]    = effect->FriendlyName();
            effectDoc["enabled"] = effect->IsEnabled();
            effectDoc["hasSettings"] = effect->HasSettings();

            if (!j["Effects"].add(effectDoc))
            {
                bufferOverflow = true;
                jsonBufferSize += JSON_BUFFER_INCREMENT;
                debugV("JSON reponse buffer overflow! Increased buffer to %zu bytes", jsonBufferSize);
                break;
            }
        }

        if (!bufferOverflow)
            AddCORSHeaderAndSendResponse(pRequest, response.release());

    } while (bufferOverflow);
}

void CWebServer::GetStatistics(AsyncWebServerRequest * pRequest)
{
    debugV("GetStatistics");

    auto response = new AsyncJsonResponse(false, JSON_BUFFER_BASE_SIZE);
    auto j = response->getRoot();

    j["LED_FPS"]               = g_FPS;
    j["SERIAL_FPS"]            = g_Analyzer._serialFPS;
    j["AUDIO_FPS"]             = g_Analyzer._AudioFPS;

    j["HEAP_SIZE"]             = _staticStats.HeapSize;
    j["HEAP_FREE"]             = ESP.getFreeHeap();
    j["HEAP_MIN"]              = ESP.getMinFreeHeap();

    j["DMA_SIZE"]              = _staticStats.DmaHeapSize;
    j["DMA_FREE"]              = heap_caps_get_free_size(MALLOC_CAP_DMA);
    j["DMA_MIN"]               = heap_caps_get_largest_free_block(MALLOC_CAP_DMA);

    j["PSRAM_SIZE"]            = _staticStats.PsramSize;
    j["PSRAM_FREE"]            = ESP.getFreePsram();
    j["PSRAM_MIN"]             = ESP.getMinFreePsram();

    j["CHIP_MODEL"]            = _staticStats.ChipModel;
    j["CHIP_CORES"]            = _staticStats.ChipCores;
    j["CHIP_SPEED"]            = _staticStats.CpuFreqMHz;
    j["PROG_SIZE"]             = _staticStats.SketchSize;

    j["CODE_SIZE"]             = _staticStats.SketchSize;
    j["CODE_FREE"]             = _staticStats.FreeSketchSpace;
    j["FLASH_SIZE"]            = _staticStats.FlashChipSize;

    j["CPU_USED"]              = g_TaskManager.GetCPUUsagePercent();
    j["CPU_USED_CORE0"]        = g_TaskManager.GetCPUUsagePercent(0);
    j["CPU_USED_CORE1"]        = g_TaskManager.GetCPUUsagePercent(1);

    AddCORSHeaderAndSendResponse(pRequest, response);
}

void CWebServer::SetCurrentEffectIndex(AsyncWebServerRequest * pRequest)
{
    debugV("SetCurrentEffectIndex");
    PushPostParamIfPresent<size_t>(pRequest, "currentEffectIndex", SET_VALUE(g_ptrEffectManager->SetCurrentEffectIndex(value)));
    AddCORSHeaderAndSendOKResponse(pRequest);
}

void CWebServer::EnableEffect(AsyncWebServerRequest * pRequest)
{
    debugV("EnableEffect");
    PushPostParamIfPresent<size_t>(pRequest, "effectIndex", SET_VALUE(g_ptrEffectManager->EnableEffect(value)));
    AddCORSHeaderAndSendOKResponse(pRequest);
}

void CWebServer::DisableEffect(AsyncWebServerRequest * pRequest)
{
    debugV("DisableEffect");
    PushPostParamIfPresent<size_t>(pRequest, "effectIndex", SET_VALUE(g_ptrEffectManager->DisableEffect(value)));
    AddCORSHeaderAndSendOKResponse(pRequest);
}

void CWebServer::NextEffect(AsyncWebServerRequest * pRequest)
{
    debugV("NextEffect");
    g_ptrEffectManager->NextEffect();
    AddCORSHeaderAndSendOKResponse(pRequest);
}

void CWebServer::PreviousEffect(AsyncWebServerRequest * pRequest)
{
    debugV("PreviousEffect");
    g_ptrEffectManager->PreviousEffect();
    AddCORSHeaderAndSendOKResponse(pRequest);
}

void CWebServer::SendSettingSpecsResponse(AsyncWebServerRequest * pRequest, const std::vector<SettingSpec> & settingSpecs)
{
    static size_t jsonBufferSize = JSON_BUFFER_BASE_SIZE;
    bool bufferOverflow;

    do
    {
        bufferOverflow = false;
        auto response = std::make_unique<AsyncJsonResponse>(false, jsonBufferSize);
        auto jsonArray = response->getRoot().to<JsonArray>();

        for (auto& spec : settingSpecs)
        {
            auto specObject = jsonArray.createNestedObject();

            StaticJsonDocument<384> jsonDoc;

            jsonDoc["name"] = spec.Name;
            jsonDoc["friendlyName"] = spec.FriendlyName;
            jsonDoc["description"] = spec.Description;
            jsonDoc["type"] = to_value(spec.Type);
            jsonDoc["typeName"] = spec.ToName(spec.Type);

            if (!specObject.set(jsonDoc.as<JsonObjectConst>()))
            {
                bufferOverflow = true;
                jsonBufferSize += JSON_BUFFER_INCREMENT;
                debugV("JSON reponse buffer overflow! Increased buffer to %zu bytes", jsonBufferSize);
                break;
            }
        }

        if (!bufferOverflow)
            AddCORSHeaderAndSendResponse(pRequest, response.release());

    } while (bufferOverflow);
}

const std::vector<SettingSpec> & CWebServer::LoadDeviceSettingSpecs()
{
    if (deviceSettingSpecs.size() == 0)
    {
        auto deviceConfigSpecs = g_ptrDeviceConfig->GetSettingSpecs();
        deviceSettingSpecs.emplace_back(
            "effectInterval",
            "Effect interval",
            "The duration in milliseconds that an individual effect runs, before the next effect is activated.",
            SettingSpec::SettingType::PositiveBigInteger
        );
        deviceSettingSpecs.insert(deviceSettingSpecs.end(), deviceConfigSpecs.begin(), deviceConfigSpecs.end());
    }

    return deviceSettingSpecs;
}

void CWebServer::GetSettingSpecs(AsyncWebServerRequest * pRequest)
{
    SendSettingSpecsResponse(pRequest, LoadDeviceSettingSpecs());
}

// Responds with current config, excluding any sensitive values
void CWebServer::GetSettings(AsyncWebServerRequest * pRequest)
{
    debugV("GetSettings");

    auto response = new AsyncJsonResponse(false, JSON_BUFFER_BASE_SIZE);
    response->addHeader("Server","NightDriverStrip");
    auto root = response->getRoot();
    JsonObject jsonObject = root.to<JsonObject>();

    // We get the serialized JSON for the device config, without any sensitive values
    g_ptrDeviceConfig->SerializeToJSON(jsonObject, false);
    jsonObject["effectInterval"] = g_ptrEffectManager->GetInterval();

    AddCORSHeaderAndSendResponse(pRequest, response);
}

// Support function that silently sets whatever settings are included in the request passed.
//   Composing a response is left to the invoker!
void CWebServer::SetSettingsIfPresent(AsyncWebServerRequest * pRequest)
{
    PushPostParamIfPresent<size_t>(pRequest,"effectInterval", SET_VALUE(g_ptrEffectManager->SetInterval(value)));
    PushPostParamIfPresent<String>(pRequest, DeviceConfig::LocationTag, SET_VALUE(g_ptrDeviceConfig->SetLocation(value)));
    PushPostParamIfPresent<bool>(pRequest, DeviceConfig::LocationIsZipTag, SET_VALUE(g_ptrDeviceConfig->SetLocationIsZip(value)));
    PushPostParamIfPresent<String>(pRequest, DeviceConfig::CountryCodeTag, SET_VALUE(g_ptrDeviceConfig->SetCountryCode(value)));
    PushPostParamIfPresent<String>(pRequest, DeviceConfig::OpenWeatherApiKeyTag, SET_VALUE(g_ptrDeviceConfig->SetOpenWeatherAPIKey(value)));
    PushPostParamIfPresent<String>(pRequest, DeviceConfig::TimeZoneTag, SET_VALUE(g_ptrDeviceConfig->SetTimeZone(value)));
    PushPostParamIfPresent<bool>(pRequest, DeviceConfig::Use24HourClockTag, SET_VALUE(g_ptrDeviceConfig->Set24HourClock(value)));
    PushPostParamIfPresent<bool>(pRequest, DeviceConfig::UseCelsiusTag, SET_VALUE(g_ptrDeviceConfig->SetUseCelsius(value)));
    PushPostParamIfPresent<String>(pRequest, DeviceConfig::NTPServerTag, SET_VALUE(g_ptrDeviceConfig->SetNTPServer(value)));
}

// Set settings and return resulting config
void CWebServer::SetSettings(AsyncWebServerRequest * pRequest)
{
    debugV("SetSettings");

    SetSettingsIfPresent(pRequest);

    // We return the current config in response
    GetSettings(pRequest);
}

bool CWebServer::CheckAndGetSettingsEffect(AsyncWebServerRequest * pRequest, std::shared_ptr<LEDStripEffect> & effect, bool post)
{
    auto effectsList = g_ptrEffectManager->EffectsList();
    auto effectIndex = GetEffectIndexFromParam(pRequest, post);

    if (effectIndex < 0 || effectIndex >= effectsList.size() || !effectsList[effectIndex]->HasSettings())
    {
        AddCORSHeaderAndSendOKResponse(pRequest);

        return false;
    }

    effect = effectsList[effectIndex];

    return true;
}

void CWebServer::GetEffectSettingSpecs(AsyncWebServerRequest * pRequest)
{
    std::shared_ptr<LEDStripEffect> effect;

    if (!CheckAndGetSettingsEffect(pRequest, effect))
        return;

    auto settingSpecs = effect->GetSettingSpecs();

    SendSettingSpecsResponse(pRequest, settingSpecs);
}

void CWebServer::SendEffectSettingsResponse(AsyncWebServerRequest * pRequest, std::shared_ptr<LEDStripEffect> & effect)
{
    static size_t jsonBufferSize = JSON_BUFFER_BASE_SIZE;

    do
    {
        auto response = std::make_unique<AsyncJsonResponse>(false, jsonBufferSize);
        auto jsonObject = response->getRoot().to<JsonObject>();

        if (effect->SerializeSettingsToJSON(jsonObject))
        {
            AddCORSHeaderAndSendResponse(pRequest, response.release());
            return;
        }

        jsonBufferSize += JSON_BUFFER_INCREMENT;
        debugV("JSON reponse buffer overflow! Increased buffer to %zu bytes", jsonBufferSize);
    } while (true);
}

void CWebServer::GetEffectSettings(AsyncWebServerRequest * pRequest)
{
    debugV("GetEffectSettings");

    std::shared_ptr<LEDStripEffect> effect;

    if (!CheckAndGetSettingsEffect(pRequest, effect))
        return;

    SendEffectSettingsResponse(pRequest, effect);
}

void CWebServer::SetEffectSettings(AsyncWebServerRequest * pRequest)
{
    debugV("SetEffectSettings");

    std::shared_ptr<LEDStripEffect> effect;

    if (!CheckAndGetSettingsEffect(pRequest, effect, true))
        return;

    bool settingChanged = false;

    for (auto& settingSpec : effect->GetSettingSpecs())
    {
        const String& settingName = settingSpec.Name;
        settingChanged = PushPostParamIfPresent<String>(pRequest, settingName, [&](auto value) { return effect->SetSetting(settingName, value); })
            || settingChanged;
    }

    if (settingChanged)
        SaveEffectManagerConfig();

    SendEffectSettingsResponse(pRequest, effect);
}

// Validate and set one setting. If no validator is available in settingValidators for the setting, validation is skipped.
//   Requests containing more than one known setting are malformed and rejected.
void CWebServer::ValidateAndSetSetting(AsyncWebServerRequest * pRequest)
{
    String paramName;

    for (auto settingSpec : LoadDeviceSettingSpecs())
    {
        if (pRequest->hasParam(settingSpec.Name, true))
        {
            if (paramName.isEmpty())
                paramName = settingSpec.Name;
            else
            // We found multiple known settings in the request, which we don't allow
            {
                String responseText = "{\"message\": \"Malformed request\"}";
                auto pResponse = pRequest->beginResponse(HTTP_CODE_BAD_REQUEST, "text/json", responseText);
                AddCORSHeaderAndSendResponse(pRequest, pResponse);
                return;
            }
        }
    }

    // No known setting in the request, so we can stop processing and go on with our business
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
            String responseText = "{\"message\": \"" + validationMessage + "\"}";
            auto pResponse = pRequest->beginResponse(HTTP_CODE_BAD_REQUEST, "text/json", responseText);
            AddCORSHeaderAndSendResponse(pRequest, pResponse);
            return;
        }
    }

    // Process the setting as per usual
    SetSettingsIfPresent(pRequest);
    AddCORSHeaderAndSendOKResponse(pRequest);
}

// Reset effect config, device config and/or the board itself
void CWebServer::Reset(AsyncWebServerRequest * pRequest)
{
    if (IsPostParamTrue(pRequest, "deviceConfig"))
    {
        debugI("Removing DeviceConfig");
        g_ptrDeviceConfig->RemovePersisted();
    }

    if (IsPostParamTrue(pRequest, "effectsConfig"))
    {
        debugI("Removing EffectManager config");
        RemoveEffectManagerConfig();
    }

    bool boardResetRequested = IsPostParamTrue(pRequest, "board");

    AddCORSHeaderAndSendOKResponse(pRequest);

    if (boardResetRequested)
    {
        debugW("Resetting device at API request!");
        delay(1000);    // Give the response a second to be sent
        throw new std::runtime_error("Resetting device at API request");
    }
}