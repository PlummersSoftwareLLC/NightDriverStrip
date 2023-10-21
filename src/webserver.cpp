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
#include "systemcontainer.h"

// Static member initializers

// Maps settings for which a validator is available to the invocation thereof
const std::map<String, CWebServer::ValueValidator> CWebServer::settingValidators
{
    { DeviceConfig::OpenWeatherApiKeyTag, [](const String& value) { return g_ptrSystem->DeviceConfig().ValidateOpenWeatherAPIKey(value); } },
    { DeviceConfig::PowerLimitTag, [](const String& value) { return g_ptrSystem->DeviceConfig().ValidatePowerLimit(value); } },
    { DeviceConfig::BrightnessTag, [](const String& value) { return g_ptrSystem->DeviceConfig().ValidateBrightness(value); } }
};

std::vector<SettingSpec, psram_allocator<SettingSpec>> CWebServer::mySettingSpecs = {};
std::vector<std::reference_wrapper<SettingSpec>> CWebServer::deviceSettingSpecs{};

// Member function template specializations

// Push param that represents a bool. Values considered true are text "true" and any whole number not equal to 0
template<>
bool CWebServer::PushPostParamIfPresent<bool>(AsyncWebServerRequest * pRequest, const String &paramName, ValueSetter<bool> setter)
{
    return PushPostParamIfPresent<bool>(pRequest, paramName, setter, [](AsyncWebParameter * param) constexpr
    {
        return BoolFromText(param->value());
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

// Push param that represents an int
template<>
bool CWebServer::PushPostParamIfPresent<int>(AsyncWebServerRequest * pRequest, const String &paramName, ValueSetter<int> setter)
{
    return PushPostParamIfPresent<int>(pRequest, paramName, setter, [](AsyncWebParameter * param) constexpr
    {
        return std::stoi(param->value().c_str());
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

// begin - register page load handlers and start serving pages
void CWebServer::begin()
{
    extern const uint8_t html_start[] asm("_binary_site_dist_index_html_gz_start");
    extern const uint8_t html_end[] asm("_binary_site_dist_index_html_gz_end");
    extern const uint8_t js_start[] asm("_binary_site_dist_index_js_gz_start");
    extern const uint8_t js_end[] asm("_binary_site_dist_index_js_gz_end");
    extern const uint8_t ico_start[] asm("_binary_site_dist_favicon_ico_gz_start");
    extern const uint8_t ico_end[] asm("_binary_site_dist_favicon_ico_gz_end");
    extern const uint8_t timezones_start[] asm("_binary_config_timezones_json_start");
    extern const uint8_t timezones_end[] asm("_binary_config_timezones_json_end");

    _staticStats.HeapSize = ESP.getHeapSize();
    _staticStats.DmaHeapSize = heap_caps_get_total_size(MALLOC_CAP_DMA);
    _staticStats.PsramSize = ESP.getPsramSize();
    _staticStats.ChipModel = ESP.getChipModel();
    _staticStats.ChipCores = ESP.getChipCores();
    _staticStats.CpuFreqMHz = ESP.getCpuFreqMHz();
    _staticStats.SketchSize = ESP.getSketchSize();
    _staticStats.FreeSketchSpace = ESP.getFreeSketchSpace();
    _staticStats.FlashChipSize = ESP.getFlashChipSize();

    debugI("Connecting Web Endpoints");

    _server.on("/effects",               HTTP_GET,  [](AsyncWebServerRequest * pRequest)        { GetEffectListText(pRequest); });
    _server.on("/getEffectList",         HTTP_GET,  [](AsyncWebServerRequest * pRequest)        { GetEffectListText(pRequest); });
    _server.on("/statistics",            HTTP_GET,  [this](AsyncWebServerRequest * pRequest)    { this->GetStatistics(pRequest); });
    _server.on("/getStatistics",         HTTP_GET,  [this](AsyncWebServerRequest * pRequest)    { this->GetStatistics(pRequest); });
    _server.on("/nextEffect",            HTTP_POST, [](AsyncWebServerRequest * pRequest)        { NextEffect(pRequest); });
    _server.on("/previousEffect",        HTTP_POST, [](AsyncWebServerRequest * pRequest)        { PreviousEffect(pRequest); });

    _server.on("/currentEffect",         HTTP_POST, [](AsyncWebServerRequest * pRequest)        { SetCurrentEffectIndex(pRequest); });
    _server.on("/setCurrentEffectIndex", HTTP_POST, [](AsyncWebServerRequest * pRequest)        { SetCurrentEffectIndex(pRequest); });
    _server.on("/enableEffect",          HTTP_POST, [](AsyncWebServerRequest * pRequest)        { EnableEffect(pRequest); });
    _server.on("/disableEffect",         HTTP_POST, [](AsyncWebServerRequest * pRequest)        { DisableEffect(pRequest); });
    _server.on("/moveEffect",            HTTP_POST, [](AsyncWebServerRequest * pRequest)        { MoveEffect(pRequest); });
    _server.on("/copyEffect",            HTTP_POST, [](AsyncWebServerRequest * pRequest)        { CopyEffect(pRequest); });
    _server.on("/deleteEffect",          HTTP_POST, [](AsyncWebServerRequest * pRequest)        { DeleteEffect(pRequest); });

    _server.on("/settings/effect/specs", HTTP_GET,  [](AsyncWebServerRequest * pRequest)        { GetEffectSettingSpecs(pRequest); });
    _server.on("/settings/effect",       HTTP_GET,  [](AsyncWebServerRequest * pRequest)        { GetEffectSettings(pRequest); });
    _server.on("/settings/effect",       HTTP_POST, [](AsyncWebServerRequest * pRequest)        { SetEffectSettings(pRequest); });
    _server.on("/settings/validated",    HTTP_POST, [](AsyncWebServerRequest * pRequest)        { ValidateAndSetSetting(pRequest); });
    _server.on("/settings/specs",        HTTP_GET,  [](AsyncWebServerRequest * pRequest)        { GetSettingSpecs(pRequest); });
    _server.on("/settings",              HTTP_GET,  [](AsyncWebServerRequest * pRequest)        { GetSettings(pRequest); });
    _server.on("/settings",              HTTP_POST, [](AsyncWebServerRequest * pRequest)        { SetSettings(pRequest); });
    _server.on("/effectsConfig",         HTTP_GET,  [](AsyncWebServerRequest * pRequest)        { pRequest->send(SPIFFS, EFFECTS_CONFIG_FILE, "text/json"); });

    _server.on("/reset",                 HTTP_POST, [](AsyncWebServerRequest * pRequest)        { Reset(pRequest); });

    #if ENABLE_IMPROV_LOGGING
        _server.on(IMPROV_LOG_FILE,      HTTP_GET,  [](AsyncWebServerRequest * pRequest)        { pRequest->send(SPIFFS, IMPROV_LOG_FILE, "text/plain"); });
    #endif

    EmbeddedWebFile html_file(html_start, html_end, "text/html", "gzip");
    EmbeddedWebFile js_file(js_start, js_end, "application/javascript", "gzip");
    EmbeddedWebFile ico_file(ico_start, ico_end, "image/vnd.microsoft.icon", "gzip");
    EmbeddedWebFile timezones_file(timezones_start, timezones_end - 1, "text/json", ""); // end - 1 because of zero-termination

    debugI("Embedded html file size: %d", html_file.length);
    debugI("Embedded jsx file size: %d", js_file.length);
    debugI("Embedded ico file size: %d", ico_file.length);
    debugI("Embedded timezones file size: %d", timezones_file.length);

    ServeEmbeddedFile("/", html_file);
    ServeEmbeddedFile("/index.html", html_file);
    ServeEmbeddedFile("/index.js", js_file);
    ServeEmbeddedFile("/favicon.ico", ico_file);
    ServeEmbeddedFile("/timezones.json", timezones_file);

    _server.onNotFound([](AsyncWebServerRequest *request)
    {
        if (request->method() == HTTP_OPTIONS) {
            request->send(HTTP_CODE_OK);                                     // Apparently needed for CORS: https://github.com/me-no-dev/ESPAsyncWebServer
        } else {
                debugW("Failed GET for %s\n", request->url().c_str() );
            request->send(HTTP_CODE_NOT_FOUND);
        }
    });

    _server.begin();

    debugI("HTTP server started");
}

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
        auto& j = response->getRoot();
        auto& effectManager = g_ptrSystem->EffectManager();

        j["currentEffect"]         = effectManager.GetCurrentEffectIndex();
        j["millisecondsRemaining"] = effectManager.GetTimeRemainingForCurrentEffect();
        j["eternalInterval"]       = effectManager.IsIntervalEternal();
        j["effectInterval"]        = effectManager.GetEffectiveInterval();

        for (auto effect : effectManager.EffectsList())
        {
            StaticJsonDocument<256> effectDoc;

            effectDoc["name"]    = effect->FriendlyName();
            effectDoc["enabled"] = effect->IsEnabled();
            effectDoc["core"]    = effect->IsCoreEffect();

            if (!j["Effects"].add(effectDoc))
            {
                bufferOverflow = true;
                jsonBufferSize += JSON_BUFFER_INCREMENT;
                debugV("JSON response buffer overflow! Increased buffer to %zu bytes", jsonBufferSize);
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
    auto& j = response->getRoot();

    j["LED_FPS"]               = g_Values.FPS;
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

    auto& taskManager = g_ptrSystem->TaskManager();

    j["CPU_USED"]              = taskManager.GetCPUUsagePercent();
    j["CPU_USED_CORE0"]        = taskManager.GetCPUUsagePercent(0);
    j["CPU_USED_CORE1"]        = taskManager.GetCPUUsagePercent(1);

    AddCORSHeaderAndSendResponse(pRequest, response);
}

void CWebServer::SetCurrentEffectIndex(AsyncWebServerRequest * pRequest)
{
    debugV("SetCurrentEffectIndex");
    PushPostParamIfPresent<size_t>(pRequest, "currentEffectIndex", SET_VALUE(g_ptrSystem->EffectManager().SetCurrentEffectIndex(value)));
    AddCORSHeaderAndSendOKResponse(pRequest);
}

void CWebServer::EnableEffect(AsyncWebServerRequest * pRequest)
{
    debugV("EnableEffect");
    PushPostParamIfPresent<size_t>(pRequest, "effectIndex", SET_VALUE(g_ptrSystem->EffectManager().EnableEffect(value)));
    AddCORSHeaderAndSendOKResponse(pRequest);
}

void CWebServer::DisableEffect(AsyncWebServerRequest * pRequest)
{
    debugV("DisableEffect");
    PushPostParamIfPresent<size_t>(pRequest, "effectIndex", SET_VALUE(g_ptrSystem->EffectManager().DisableEffect(value)));
    AddCORSHeaderAndSendOKResponse(pRequest);
}

void CWebServer::MoveEffect(AsyncWebServerRequest * pRequest)
{
    debugV("MoveEffect");

    auto fromIndex = GetEffectIndexFromParam(pRequest, true);
    if (fromIndex == -1)
    {
        AddCORSHeaderAndSendOKResponse(pRequest);
        return;
    }

    PushPostParamIfPresent<size_t>(pRequest, "newIndex", SET_VALUE(g_ptrSystem->EffectManager().MoveEffect(fromIndex, value)));
    AddCORSHeaderAndSendOKResponse(pRequest);
}

void CWebServer::CopyEffect(AsyncWebServerRequest * pRequest)
{
    debugV("CopyEffect");

    auto index = GetEffectIndexFromParam(pRequest, true);
    if (index == -1)
    {
        AddCORSHeaderAndSendOKResponse(pRequest);
        return;
    }

    auto effect = g_ptrSystem->EffectManager().CopyEffect(index);
    if (!effect)
    {
        AddCORSHeaderAndSendOKResponse(pRequest);
        return;
    }

    ApplyEffectSettings(pRequest, effect);

    if (g_ptrSystem->EffectManager().AppendEffect(effect))
        SendEffectSettingsResponse(pRequest, effect);
    else
        AddCORSHeaderAndSendOKResponse(pRequest);
}

void CWebServer::DeleteEffect(AsyncWebServerRequest * pRequest)
{
    debugV("DeleteEffect");

    auto index = GetEffectIndexFromParam(pRequest, true);
    if (index == -1)
    {
        AddCORSHeaderAndSendOKResponse(pRequest);
        return;
    }

    if (index < g_ptrSystem->EffectManager().EffectCount() && g_ptrSystem->EffectManager().EffectsList()[index]->IsCoreEffect())
    {
        AddCORSHeaderAndSendBadRequest(pRequest, "Can't delete core effect");
        return;
    }

    g_ptrSystem->EffectManager().DeleteEffect(index);
    AddCORSHeaderAndSendOKResponse(pRequest);
}

void CWebServer::NextEffect(AsyncWebServerRequest * pRequest)
{
    debugV("NextEffect");
    g_ptrSystem->EffectManager().NextEffect();
    AddCORSHeaderAndSendOKResponse(pRequest);
}

void CWebServer::PreviousEffect(AsyncWebServerRequest * pRequest)
{
    debugV("PreviousEffect");
    g_ptrSystem->EffectManager().PreviousEffect();
    AddCORSHeaderAndSendOKResponse(pRequest);
}

void CWebServer::SendSettingSpecsResponse(AsyncWebServerRequest * pRequest, const std::vector<std::reference_wrapper<SettingSpec>> & settingSpecs)
{
    static size_t jsonBufferSize = JSON_BUFFER_BASE_SIZE;
    bool bufferOverflow;

    do
    {
        bufferOverflow = false;
        auto response = std::make_unique<AsyncJsonResponse>(false, jsonBufferSize);
        auto jsonArray = response->getRoot().to<JsonArray>();

        for (auto& specWrapper : settingSpecs)
        {
            auto& spec = specWrapper.get();
            auto specObject = jsonArray.createNestedObject();

            StaticJsonDocument<384> jsonDoc;

            jsonDoc["name"] = spec.Name;
            jsonDoc["friendlyName"] = spec.FriendlyName;
            if (spec.Description)
                jsonDoc["description"] = spec.Description;
            jsonDoc["type"] = to_value(spec.Type);
            jsonDoc["typeName"] = spec.TypeName();
            if (spec.HasValidation)
                jsonDoc["hasValidation"] = true;
            if (spec.MinimumValue.has_value())
                jsonDoc["minimumValue"] = spec.MinimumValue.value();
            if (spec.MaximumValue.has_value())
                jsonDoc["maximumValue"] = spec.MaximumValue.value();
            switch (spec.Access)
            {
                case SettingSpec::SettingAccess::ReadOnly:
                    jsonDoc["readOnly"] = true;
                    break;

                case SettingSpec::SettingAccess::WriteOnly:
                    jsonDoc["writeOnly"] = true;
                    break;

                default:
                    // Default is read/write, so we don't need to specify that
                    break;
            }

            if (jsonDoc.overflowed())
                debugE("JSON buffer overflow while serializing SettingSpec - object incomplete!");

            if (!specObject.set(jsonDoc.as<JsonObjectConst>()))
            {
                bufferOverflow = true;
                jsonBufferSize += JSON_BUFFER_INCREMENT;
                debugV("JSON response buffer overflow! Increased buffer to %zu bytes", jsonBufferSize);
                break;
            }
        }

        if (!bufferOverflow)
            AddCORSHeaderAndSendResponse(pRequest, response.release());

    } while (bufferOverflow);
}

const std::vector<std::reference_wrapper<SettingSpec>> & CWebServer::LoadDeviceSettingSpecs()
{
    if (deviceSettingSpecs.size() == 0)
    {
        mySettingSpecs.emplace_back(
            "effectInterval",
            "Effect interval",
            "The duration in milliseconds that an individual effect runs, before the next effect is activated.",
            SettingSpec::SettingType::PositiveBigInteger
        );
        deviceSettingSpecs.insert(deviceSettingSpecs.end(), mySettingSpecs.begin(), mySettingSpecs.end());

        auto deviceConfigSpecs = g_ptrSystem->DeviceConfig().GetSettingSpecs();
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
    response->addHeader("Server", "NightDriverStrip");
    auto root = response->getRoot();
    JsonObject jsonObject = root.to<JsonObject>();

    // We get the serialized JSON for the device config, without any sensitive values
    g_ptrSystem->DeviceConfig().SerializeToJSON(jsonObject, false);
    jsonObject["effectInterval"] = g_ptrSystem->EffectManager().GetInterval();

    AddCORSHeaderAndSendResponse(pRequest, response);
}

// Support function that silently sets whatever settings are included in the request passed.
//   Composing a response is left to the invoker!
void CWebServer::SetSettingsIfPresent(AsyncWebServerRequest * pRequest)
{
    auto& deviceConfig = g_ptrSystem->DeviceConfig();

    PushPostParamIfPresent<size_t>(pRequest,"effectInterval", SET_VALUE(g_ptrSystem->EffectManager().SetInterval(value)));
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
    auto effectsList = g_ptrSystem->EffectManager().EffectsList();
    auto effectIndex = GetEffectIndexFromParam(pRequest, post);

    if (effectIndex < 0 || effectIndex >= effectsList.size())
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
        debugV("JSON response buffer overflow! Increased buffer to %zu bytes", jsonBufferSize);
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

bool CWebServer::ApplyEffectSettings(AsyncWebServerRequest * pRequest, std::shared_ptr<LEDStripEffect> & effect)
{
    bool settingChanged = false;

    for (auto& settingSpecWrapper : effect->GetSettingSpecs())
    {
        const String& settingName = settingSpecWrapper.get().Name;
        settingChanged = PushPostParamIfPresent<String>(pRequest, settingName, [&](auto value) { return effect->SetSetting(settingName, value); })
            || settingChanged;
    }

    return settingChanged;
}

void CWebServer::SetEffectSettings(AsyncWebServerRequest * pRequest)
{
    debugV("SetEffectSettings");

    std::shared_ptr<LEDStripEffect> effect;

    if (!CheckAndGetSettingsEffect(pRequest, effect, true))
        return;

    if (ApplyEffectSettings(pRequest, effect))
        SaveEffectManagerConfig();

    SendEffectSettingsResponse(pRequest, effect);
}

// Validate and set one setting. If no validator is available in settingValidators for the setting, validation is skipped.
//   Requests containing more than one known setting are malformed and rejected.
void CWebServer::ValidateAndSetSetting(AsyncWebServerRequest * pRequest)
{
    String paramName;

    for (auto& settingSpecWrapper : LoadDeviceSettingSpecs())
    {
        auto& settingSpec = settingSpecWrapper.get();

        if (pRequest->hasParam(settingSpec.Name, true))
        {
            if (paramName.isEmpty())
                paramName = settingSpec.Name;
            else
            // We found multiple known settings in the request, which we don't allow
            {
                AddCORSHeaderAndSendBadRequest(pRequest, "Malformed request");
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
            AddCORSHeaderAndSendBadRequest(pRequest, validationMessage);
            return;
        }
    }

    // Process the setting as per usual
    SetSettingsIfPresent(pRequest);
    AddCORSHeaderAndSendOKResponse(pRequest);
}

// Number of ms we wait after flushing pending writes
#define WRITE_WAIT_DELAY

// Reset effect config, device config and/or the board itself
void CWebServer::Reset(AsyncWebServerRequest * pRequest)
{
    bool boardResetRequested = IsPostParamTrue(pRequest, "board");
    bool deviceConfigResetRequested = IsPostParamTrue(pRequest, "deviceConfig");
    bool effectsConfigResetRequested = IsPostParamTrue(pRequest, "effectsConfig");

    // We can now let the requester know we're taking care of things without making them wait longer
    AddCORSHeaderAndSendOKResponse(pRequest);

    if (boardResetRequested)
    {
        // Flush any pending writes and make sure nothing is written after. We do this to make sure
        //   that what needs saving is written, but no further writes take place after any requested
        //   config resets have happened.
        g_ptrSystem->JSONWriter().FlushWrites(true);

        // Give the device a few seconds to finish the requested writes - this also gives AsyncWebServer
        //   time to push out the response to the request before the device resets
        delay(3000);
    }

    if (deviceConfigResetRequested)
    {
        debugI("Removing DeviceConfig");
        g_ptrSystem->DeviceConfig().RemovePersisted();
    }

    if (effectsConfigResetRequested)
    {
        debugI("Removing EffectManager config");
        RemoveEffectManagerConfig();
    }

    if (boardResetRequested)
    {
        debugW("Resetting device at API request!");
        throw new std::runtime_error("Resetting device at API request");
    }
}