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

#if ENABLE_WEBSERVER

#include "webserver.h"

#include <AsyncJson.h>
#include <FS.h>
#include <utility>

#include "deviceconfig.h"
#include "effectmanager.h"
#include "gfxbase.h"
#include "improvserial.h"
#include "ledstripeffect.h"
#include "soundanalyzer.h"
#include "systemcontainer.h"
#include "taskmgr.h"
#include "values.h"

namespace
{
    void AppendPins(JsonArray target, const std::array<int8_t, NUM_CHANNELS>& pins)
    {
        for (auto pin : pins)
            target.add(pin);
    }

    void FillUnifiedSettingsJson(JsonObject root)
    {
        auto& deviceConfig = g_ptrSystem->GetDeviceConfig();
        auto& effectManager = g_ptrSystem->GetEffectManager();

        auto device = root["device"].to<JsonObject>();
        device["hostname"] = deviceConfig.GetHostname();
        device["location"] = deviceConfig.GetLocation();
        device["locationIsZip"] = deviceConfig.IsLocationZip();
        device["countryCode"] = deviceConfig.GetCountryCode();
        device["timeZone"] = deviceConfig.GetTimeZone();
        device["use24HourClock"] = deviceConfig.Use24HourClock();
        device["useCelsius"] = deviceConfig.UseCelsius();
        device["ntpServer"] = deviceConfig.GetNTPServer();
        device["rememberCurrentEffect"] = deviceConfig.RememberCurrentEffect();
        device["powerLimit"] = deviceConfig.GetPowerLimit();
        device["brightness"] = deviceConfig.GetBrightness();
        device["globalColor"] = deviceConfig.GlobalColor();
        device["secondColor"] = deviceConfig.SecondColor();
        device["applyGlobalColors"] = deviceConfig.ApplyGlobalColors();
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
        audio["inputPin"] = deviceConfig.GetAudioInputPin();
        audio["compiledDefaultPin"] = DeviceConfig::GetCompiledAudioInputPin();
        audio["mode"] = deviceConfig.GetAudioInputModeName();
        audio["liveApply"] = deviceConfig.SupportsLiveAudioInputReconfigure();
        audio["requiresReboot"] = !deviceConfig.SupportsLiveAudioInputReconfigure();
        audio["supportsPinOverride"] = deviceConfig.SupportsConfigurableAudioInputPin();

        auto topology = root["topology"].to<JsonObject>();
        topology["width"] = deviceConfig.GetMatrixWidth();
        topology["height"] = deviceConfig.GetMatrixHeight();
        topology["serpentine"] = deviceConfig.IsMatrixSerpentine();
        topology["ledCount"] = deviceConfig.GetActiveLEDCount();
        topology["liveApply"] = deviceConfig.SupportsLiveTopology();

        auto outputs = root["outputs"].to<JsonObject>();
        outputs["driver"] = deviceConfig.GetRuntimeDriverName();
        outputs["compiledDriver"] = deviceConfig.GetCompiledDriverName();
        outputs["liveApply"] = deviceConfig.SupportsLiveOutputReconfigure();

        auto ws281x = outputs["ws281x"].to<JsonObject>();
        ws281x["channelCount"] = deviceConfig.GetChannelCount();
        ws281x["compiledMaxChannels"] = DeviceConfig::GetCompiledChannelCount();
        ws281x["colorOrder"] = DeviceConfig::GetColorOrderName(deviceConfig.GetWS281xColorOrder());
        ws281x["compiledColorOrder"] = DeviceConfig::GetColorOrderName(DeviceConfig::GetCompiledWS281xColorOrder());
        AppendPins(ws281x["pins"].to<JsonArray>(), deviceConfig.GetWS281xPins());

        auto effects = root["effects"].to<JsonObject>();
        effects["effectInterval"] = effectManager.GetInterval();
    }

    void FillUnifiedSettingsSchemaJson(JsonObject root)
    {
        auto& deviceConfig = g_ptrSystem->GetDeviceConfig();

        auto topology = root["topology"].to<JsonObject>();
        topology["compiledMaxWidth"] =
            DeviceConfig::GetCompiledOutputDriver() == DeviceConfig::OutputDriver::HUB75
                ? DeviceConfig::GetCompiledMatrixWidth()
                : DeviceConfig::GetCompiledLEDCount();
        topology["compiledMaxHeight"] =
            DeviceConfig::GetCompiledOutputDriver() == DeviceConfig::OutputDriver::HUB75
                ? DeviceConfig::GetCompiledMatrixHeight()
                : DeviceConfig::GetCompiledLEDCount();
        topology["compiledNominalWidth"] = DeviceConfig::GetCompiledMatrixWidth();
        topology["compiledNominalHeight"] = DeviceConfig::GetCompiledMatrixHeight();
        topology["compiledMaxLEDs"] = DeviceConfig::GetCompiledLEDCount();
        topology["liveApply"] = deviceConfig.SupportsLiveTopology();
        topology["rejectMessage"] = "recompile needed";

        auto outputs = root["outputs"].to<JsonObject>();
        outputs["compiledDriver"] = deviceConfig.GetCompiledDriverName();
        outputs["liveApply"] = deviceConfig.SupportsLiveOutputReconfigure();
        outputs["rejectMessage"] = "recompile needed";

        auto drivers = outputs["allowedDrivers"].to<JsonArray>();
        drivers.add(deviceConfig.GetCompiledDriverName());

        auto ws281x = outputs["ws281x"].to<JsonObject>();
        const auto compiledChannels = DeviceConfig::GetCompiledChannelCount();
        ws281x["compiledMaxChannels"] = compiledChannels;
        ws281x["compiledMaxLEDs"] = DeviceConfig::GetCompiledLEDCount();
        ws281x["compiledColorOrder"] = DeviceConfig::GetColorOrderName(DeviceConfig::GetCompiledWS281xColorOrder());
        // Concrete list of valid channel counts. Specs that drive a select for
        // channel count point here so the UI doesn't have to derive a range
        // from a bare integer.
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
        AppendPins(ws281x["compiledPins"].to<JsonArray>(), DeviceConfig::GetCompiledWS281xPins());

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
        audio["compiledDefaultPin"] = DeviceConfig::GetCompiledAudioInputPin();
        audio["mode"] = deviceConfig.GetAudioInputModeName();
        audio["liveApply"] = deviceConfig.SupportsLiveAudioInputReconfigure();
        audio["requiresReboot"] = !deviceConfig.SupportsLiveAudioInputReconfigure();
        audio["supportsPinOverride"] = deviceConfig.SupportsConfigurableAudioInputPin();
        audio["rejectMessage"] = "recompile needed";

        // Section catalog. The UI groups settings by spec.section and uses
        // these entries (matched on id) for the section heading and subtitle.
        // Sections without any settings are still listed so the UI can decide
        // whether to render an empty placeholder or skip them.
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

    // Normalizes the unified settings audio pin request into one optional value.
    // The API currently accepts both the legacy flat device.audioInputPin field
    // and the nested device.audio.inputPin field, so validation/apply can consume
    // one resolved value instead of duplicating that lookup logic.

    std::optional<int> GetRequestedUnifiedAudioInputPin(JsonObjectConst device)
    {
        std::optional<int> requestedAudioInputPin;

        if (device[DeviceConfig::AudioInputPinTag].is<int>())
            requestedAudioInputPin = device[DeviceConfig::AudioInputPinTag].as<int>();

        if (device["audio"].is<JsonObjectConst>())
        {
            auto audio = device["audio"].as<JsonObjectConst>();
            if (audio["inputPin"].is<int>())
                requestedAudioInputPin = audio["inputPin"].as<int>();
        }

        return requestedAudioInputPin;
    }

    // Applies a validated runtime topology/output change as one staged transaction.
    //
    // The flow is:
    // 1. Update DeviceConfig in-memory without persisting it.
    // 2. Ask SystemContainer to reconfigure live devices/buffers/effects.
    // 3. If live apply fails, restore the previous runtime config and best-effort
    //    re-apply that old state so the device is not left half-switched.
    // 4. Only persist the new runtime config after the live reconfigure succeeds.
    //
    // This keeps unified settings requests from partially committing topology/output
    // changes when the runtime apply path fails after validation.

    bool ApplyRuntimeConfigTransaction(const DeviceConfig::RuntimeConfig& requestedConfig, String* errorMessage)
    {
        auto& system = *g_ptrSystem;
        auto& deviceConfig = system.GetDeviceConfig();
        const auto previousConfig = deviceConfig.GetRuntimeConfig();

        if (!deviceConfig.SetRuntimeConfig(requestedConfig, true, errorMessage))
            return false;

        if (!system.ApplyRuntimeConfiguration(errorMessage))
        {
            deviceConfig.SetRuntimeConfig(previousConfig, true, nullptr);

            String rollbackError;
            if (!system.ApplyRuntimeConfiguration(&rollbackError))
                debugE("Failed to roll back runtime configuration after apply error: %s", rollbackError.c_str());

            return false;
        }

        if (!deviceConfig.SetRuntimeConfig(requestedConfig, false, errorMessage))
            return false;

        return true;
    }
}

// Static member initializers

// Maps settings for which a validator is available to the invocation thereof
const std::map<String, CWebServer::ValueValidator> CWebServer::settingValidators
{
    { DeviceConfig::OpenWeatherApiKeyTag, [](const String& value) { return g_ptrSystem->GetDeviceConfig().ValidateOpenWeatherAPIKey(value); } },
    { DeviceConfig::PowerLimitTag,        [](const String& value) { return g_ptrSystem->GetDeviceConfig().ValidatePowerLimit(value); } },
    { DeviceConfig::BrightnessTag,        [](const String& value) { return g_ptrSystem->GetDeviceConfig().ValidateBrightness(value); } },
    { DeviceConfig::AudioInputPinTag,     [](const String& value) { return g_ptrSystem->GetDeviceConfig().ValidateAudioInputPin(value.toInt()); } }
};

std::vector<SettingSpec, psram_allocator<SettingSpec>> CWebServer::mySettingSpecs = {};
std::vector<std::reference_wrapper<SettingSpec>> CWebServer::deviceSettingSpecs{};

// Member function template specializations

// Push param that represents a bool. Values considered true are text "true" and any whole number not equal to 0
template<>
bool CWebServer::PushPostParamIfPresent<bool>(const AsyncWebServerRequest * pRequest, const String &paramName, ValueSetter<bool> setter)
{
    return PushPostParamIfPresent<bool>(pRequest, paramName, std::move(setter), [](const AsyncWebParameter * param) constexpr
    {
        return BoolFromText(param->value());
    });
}

// Push param that represents a size_t
template<>
bool CWebServer::PushPostParamIfPresent<size_t>(const AsyncWebServerRequest * pRequest, const String &paramName, ValueSetter<size_t> setter)
{
    return PushPostParamIfPresent<size_t>(pRequest, paramName, std::move(setter), [](const AsyncWebParameter * param) constexpr
    {
        return strtoul(param->value().c_str(), nullptr, 10);
    });
}

// Push param that represents an int
template<>
bool CWebServer::PushPostParamIfPresent<int>(const AsyncWebServerRequest * pRequest, const String &paramName, ValueSetter<int> setter)
{
    return PushPostParamIfPresent<int>(pRequest, paramName, std::move(setter), [](const AsyncWebParameter * param) constexpr
    {
        return std::stoi(param->value().c_str());
    });
}

// Push param that represents a color
template<>
bool CWebServer::PushPostParamIfPresent<CRGB>(const AsyncWebServerRequest * pRequest, const String &paramName, ValueSetter<CRGB> setter)
{
    return PushPostParamIfPresent<CRGB>(pRequest, paramName, std::move(setter), [](const AsyncWebParameter * param) constexpr
    {
        return CRGB(strtoul(param->value().c_str(), nullptr, 10));
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
    [[maybe_unused]] extern const uint8_t html_start[] asm("_binary_site_dist_index_html_gz_start");
    [[maybe_unused]] extern const uint8_t html_end[] asm("_binary_site_dist_index_html_gz_end");
    [[maybe_unused]] extern const uint8_t css_start[] asm("_binary_site_dist_styles_css_gz_start");
    [[maybe_unused]] extern const uint8_t css_end[] asm("_binary_site_dist_styles_css_gz_end");
    [[maybe_unused]] extern const uint8_t js_start[] asm("_binary_site_dist_app_js_gz_start");
    [[maybe_unused]] extern const uint8_t js_end[] asm("_binary_site_dist_app_js_gz_end");
    [[maybe_unused]] extern const uint8_t timezones_start[] asm("_binary_config_timezones_json_start");
    [[maybe_unused]] extern const uint8_t timezones_end[] asm("_binary_config_timezones_json_end");

    auto trimmedTimezonesEnd = timezones_end;
    while (trimmedTimezonesEnd > timezones_start && *(trimmedTimezonesEnd - 1) == 0)
        --trimmedTimezonesEnd;

    EmbeddedWebFile html_file(html_start, html_end, "text/html", "gzip");
    EmbeddedWebFile css_file(css_start, css_end, "text/css", "gzip");
    EmbeddedWebFile js_file(js_start, js_end, "application/javascript", "gzip");
    EmbeddedWebFile timezones_file(timezones_start, trimmedTimezonesEnd, "text/json");

    debugI("Embedded html file size: %zu", (size_t)html_file.length);
    debugI("Embedded css file size: %zu", (size_t)css_file.length);
    debugI("Embedded js file size: %zu", (size_t)js_file.length);
    debugI("Embedded timezones file size: %zu", (size_t)timezones_file.length);

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

    // SPIFFS file requests

    _server.on("/effectsConfig",         HTTP_GET,  [](AsyncWebServerRequest* pRequest) { pRequest->send(SPIFFS, EFFECTS_CONFIG_FILE,   "text/json"); });
    #if ENABLE_IMPROV_LOGGING
        _server.on(IMPROV_LOG_FILE,      HTTP_GET,  [](AsyncWebServerRequest* pRequest) { pRequest->send(SPIFFS, IMPROV_LOG_FILE,       "text/plain"); });
    #endif

    // Instance handler requests

    _server.on("/statistics/static",     HTTP_GET,  [this](AsyncWebServerRequest* pRequest)
                                                    { this->GetStatistics(pRequest, StatisticsType::Static); });
    _server.on("/statistics/dynamic",    HTTP_GET,  [this](AsyncWebServerRequest* pRequest)
                                                    { this->GetStatistics(pRequest, StatisticsType::Dynamic); });
    _server.on("/statistics",            HTTP_GET,  [this](AsyncWebServerRequest* pRequest)
                                                    { this->GetStatistics(pRequest); });
    _server.on("/getStatistics",         HTTP_GET,  [this](AsyncWebServerRequest* pRequest)
                                                    { this->GetStatistics(pRequest); });

    // Static handler requests

    _server.on("/effects",               HTTP_GET,  GetEffectListText);
    _server.on("/getEffectList",         HTTP_GET,  GetEffectListText);
    _server.on("/nextEffect",            HTTP_POST, NextEffect);
    _server.on("/previousEffect",        HTTP_POST, PreviousEffect);

    _server.on("/currentEffect",         HTTP_POST, SetCurrentEffectIndex);
    _server.on("/setCurrentEffectIndex", HTTP_POST, SetCurrentEffectIndex);
    _server.on("/enableEffect",          HTTP_POST, EnableEffect);
    _server.on("/disableEffect",         HTTP_POST, DisableEffect);
    _server.on("/moveEffect",            HTTP_POST, MoveEffect);
    _server.on("/copyEffect",            HTTP_POST, CopyEffect);
    _server.on("/deleteEffect",          HTTP_POST, DeleteEffect);

    _server.on("/settings/effect/specs", HTTP_GET,  GetEffectSettingSpecs);
    _server.on("/settings/effect",       HTTP_GET,  GetEffectSettings);
    _server.on("/settings/effect",       HTTP_POST, SetEffectSettings);
    _server.on("/settings/validated",    HTTP_POST, ValidateAndSetSetting);
    _server.on("/settings/specs",        HTTP_GET,  GetSettingSpecs);
    _server.on("/settings",              HTTP_GET,  GetSettings);
    _server.on("/settings",              HTTP_POST, SetSettings);
    _server.on("/api/v1/settings/schema",HTTP_GET,  GetUnifiedSettingsSchema);
    _server.on("/api/v1/settings",       HTTP_GET,  GetUnifiedSettings);

    auto settingsHandler = new AsyncCallbackJsonWebHandler("/api/v1/settings",
        [](AsyncWebServerRequest* pRequest, JsonVariant& json)
        {
            SetUnifiedSettings(pRequest, json.as<JsonVariantConst>());
        });
    settingsHandler->setMethod(HTTP_POST);
    _server.addHandler(settingsHandler);

    _server.on("/reset",                 HTTP_POST, Reset);

    // Embedded file requests

    ServeEmbeddedFile("/timezones.json", timezones_file);

    #if ENABLE_WEB_UI
        debugI("Web UI URL pathnames enabled");

        ServeEmbeddedFile("/", html_file);
        ServeEmbeddedFile("/index.html", html_file);
        ServeEmbeddedFile("/styles.css", css_file);
        ServeEmbeddedFile("/app.js", js_file);
    #endif

    // Not found handler

    _server.onNotFound([](AsyncWebServerRequest *request)
    {
        if (request->method() == HTTP_OPTIONS) {
            request->send(CWebServer::HttpOk);                                // Apparently needed for CORS: https://github.com/me-no-dev/ESPAsyncWebServer
        } else {
                debugW("Failed GET for %s\n", request->url().c_str() );
            request->send(CWebServer::HttpNotFound);
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

    return strtol(pRequest->getParam("effectIndex", post, false)->value().c_str(), nullptr, 10);
}

void CWebServer::SendBufferOverflowResponse(AsyncWebServerRequest * pRequest)
{
    AddCORSHeaderAndSendResponse(
        pRequest,
        pRequest->beginResponse(
            CWebServer::HttpInternalServerError,
            "text/json",
            "{\"message\": \"JSON response buffer overflow\"}"
        )
    );
}

void CWebServer::GetEffectListText(AsyncWebServerRequest * pRequest)
{
    debugV("GetEffectListText");

    auto response = new AsyncJsonResponse();
    auto& j = response->getRoot();
    auto& effectManager = g_ptrSystem->GetEffectManager();

    j["currentEffect"]         = effectManager.GetCurrentEffectIndex();
    j["millisecondsRemaining"] = effectManager.GetTimeRemainingForCurrentEffect();
    j["eternalInterval"]       = effectManager.IsIntervalEternal();
    j["effectInterval"]        = effectManager.GetInterval();

    for (const auto& effect : effectManager.EffectsList())
    {
        auto effectDoc = CreateJsonDocument();

        effectDoc["name"]    = effect->FriendlyName();
        effectDoc["enabled"] = effect->IsEnabled();
        effectDoc["core"]    = effect->IsCoreEffect();

        if (!j["Effects"].add(effectDoc))
        {
            debugV("JSON response buffer overflow!");
            SendBufferOverflowResponse(pRequest);
            return;
        }
    }

    AddCORSHeaderAndSendResponse(pRequest, response);
}

void CWebServer::GetStatistics(AsyncWebServerRequest * pRequest, StatisticsType statsType) const
{
    debugV("GetStatistics");

    auto response = new AsyncJsonResponse();
    auto& j = response->getRoot();
    const auto& deviceConfig  = g_ptrSystem->GetDeviceConfig();
    const auto activeWidth    = g_ptrSystem->HasEffectManager() ? g_ptrSystem->GetEffectManager().g().GetMatrixWidth() : deviceConfig.GetMatrixWidth();
    const auto activeHeight   = g_ptrSystem->HasEffectManager() ? g_ptrSystem->GetEffectManager().g().GetMatrixHeight() : deviceConfig.GetMatrixHeight();
    const auto activeLEDCount = g_ptrSystem->HasEffectManager() ? g_ptrSystem->GetEffectManager().g().GetLEDCount() : deviceConfig.GetActiveLEDCount();

    if ((statsType & StatisticsType::Static) != StatisticsType::None)
    {
        j["MATRIX_WIDTH"]               = MATRIX_WIDTH;
        j["MATRIX_HEIGHT"]              = MATRIX_HEIGHT;
        j["CONFIGURED_MATRIX_WIDTH"]    = deviceConfig.GetMatrixWidth();
        j["CONFIGURED_MATRIX_HEIGHT"]   = deviceConfig.GetMatrixHeight();
        j["CONFIGURED_NUM_LEDS"]        = deviceConfig.GetActiveLEDCount();
        j["ACTIVE_MATRIX_WIDTH"]        = activeWidth;
        j["ACTIVE_MATRIX_HEIGHT"]       = activeHeight;
        j["ACTIVE_NUM_LEDS"]            = activeLEDCount;
        j["COMPILED_NUM_LEDS"]          = DeviceConfig::GetCompiledLEDCount();
        j["COMPILED_NUM_CHANNELS"]      = DeviceConfig::GetCompiledChannelCount();
        j["ACTIVE_NUM_CHANNELS"]        = deviceConfig.GetChannelCount();
        j["COMPILED_OUTPUT_DRIVER"]     = deviceConfig.GetCompiledDriverName();
        j["ACTIVE_OUTPUT_DRIVER"]       = deviceConfig.GetRuntimeDriverName();
        j["COMPILED_WS281X_COLOR_ORDER"] = DeviceConfig::GetColorOrderName(DeviceConfig::GetCompiledWS281xColorOrder());
        j["CONFIGURED_WS281X_COLOR_ORDER"] = DeviceConfig::GetColorOrderName(deviceConfig.GetWS281xColorOrder());
        j["COMPILED_AUDIO_INPUT_PIN"]   = DeviceConfig::GetCompiledAudioInputPin();
        j["CONFIGURED_AUDIO_INPUT_PIN"] = deviceConfig.GetAudioInputPin();
        j["AUDIO_INPUT_MODE"]           = deviceConfig.GetAudioInputModeName();
        j["FRAMES_SOCKET"]              = !!COLORDATA_WEB_SOCKET_ENABLED;
        j["EFFECTS_SOCKET"]             = !!EFFECTS_WEB_SOCKET_ENABLED;
        j["CHIP_MODEL"]                 = _staticStats.ChipModel;
        j["CHIP_CORES"]                 = _staticStats.ChipCores;
        j["CHIP_SPEED"]                 = _staticStats.CpuFreqMHz;
        j["PROG_SIZE"]                  = _staticStats.SketchSize;
        j["CODE_SIZE"]                  = _staticStats.SketchSize;
        j["FLASH_SIZE"]                 = _staticStats.FlashChipSize;
        j["HEAP_SIZE"]                  = _staticStats.HeapSize;
        j["DMA_SIZE"]                   = _staticStats.DmaHeapSize;
        j["PSRAM_SIZE"]                 = _staticStats.PsramSize;
        j["CODE_FREE"]                  = _staticStats.FreeSketchSpace;
    }

    if ((statsType & StatisticsType::Dynamic) != StatisticsType::None)
    {
        j["LED_FPS"]               = g_Values.FPS;
        j["SERIAL_FPS"]            = g_Analyzer.SerialFPS();
        j["AUDIO_FPS"]             = g_Analyzer.AudioFPS();
        j["HEAP_FREE"]             = ESP.getFreeHeap();
        j["HEAP_MIN"]              = ESP.getMinFreeHeap();
        j["DMA_FREE"]              = heap_caps_get_free_size(MALLOC_CAP_DMA);
        j["DMA_MIN"]               = heap_caps_get_largest_free_block(MALLOC_CAP_DMA);
        j["PSRAM_FREE"]            = ESP.getFreePsram();
        j["PSRAM_MIN"]             = ESP.getMinFreePsram();
        auto& taskManager = g_ptrSystem->GetTaskManager();

        j["CPU_USED"]              = taskManager.GetCPUUsagePercent();
        j["CPU_USED_CORE0"]        = taskManager.GetCPUUsagePercent(0);
        j["CPU_USED_CORE1"]        = taskManager.GetCPUUsagePercent(1);
    }

    AddCORSHeaderAndSendResponse(pRequest, response);
}

void CWebServer::SetCurrentEffectIndex(AsyncWebServerRequest * pRequest)
{
    debugV("SetCurrentEffectIndex");
    PushPostParamIfPresent<size_t>(pRequest, "currentEffectIndex", SET_VALUE(g_ptrSystem->GetEffectManager().SetCurrentEffectIndex(value)));
    AddCORSHeaderAndSendOKResponse(pRequest);
}

void CWebServer::EnableEffect(AsyncWebServerRequest * pRequest)
{
    debugV("EnableEffect");
    PushPostParamIfPresent<size_t>(pRequest, "effectIndex", SET_VALUE(g_ptrSystem->GetEffectManager().EnableEffect(value)));
    AddCORSHeaderAndSendOKResponse(pRequest);
}

void CWebServer::DisableEffect(AsyncWebServerRequest * pRequest)
{
    debugV("DisableEffect");
    PushPostParamIfPresent<size_t>(pRequest, "effectIndex", SET_VALUE(g_ptrSystem->GetEffectManager().DisableEffect(value)));
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

    PushPostParamIfPresent<size_t>(pRequest, "newIndex", SET_VALUE(g_ptrSystem->GetEffectManager().MoveEffect(fromIndex, value)));
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

    auto effect = g_ptrSystem->GetEffectManager().CopyEffect(index);
    if (!effect)
    {
        AddCORSHeaderAndSendOKResponse(pRequest);
        return;
    }

    ApplyEffectSettings(pRequest, effect);

    if (g_ptrSystem->GetEffectManager().AppendEffect(effect))
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

    if (index < g_ptrSystem->GetEffectManager().EffectCount() && g_ptrSystem->GetEffectManager().EffectsList()[index]->IsCoreEffect())
    {
        AddCORSHeaderAndSendBadRequest(pRequest, "Can't delete core effect");
        return;
    }

    g_ptrSystem->GetEffectManager().DeleteEffect(index);
    AddCORSHeaderAndSendOKResponse(pRequest);
}

void CWebServer::NextEffect(AsyncWebServerRequest * pRequest)
{
    debugV("NextEffect");
    g_ptrSystem->GetEffectManager().NextEffect();
    AddCORSHeaderAndSendOKResponse(pRequest);
}

void CWebServer::PreviousEffect(AsyncWebServerRequest * pRequest)
{
    debugV("PreviousEffect");
    g_ptrSystem->GetEffectManager().PreviousEffect();
    AddCORSHeaderAndSendOKResponse(pRequest);
}

void CWebServer::SendSettingSpecsResponse(AsyncWebServerRequest * pRequest, const std::vector<std::reference_wrapper<SettingSpec>> & settingSpecs)
{
    auto response = new AsyncJsonResponse();
    auto jsonArray = response->getRoot().to<JsonArray>();

    for (const auto& specWrapper : settingSpecs)
    {
        const auto& spec = specWrapper.get();
        auto specObject = jsonArray.add<JsonObject>();

        auto jsonDoc = CreateJsonDocument();

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
        if (spec.EmptyAllowed.has_value())
            jsonDoc["emptyAllowed"] = spec.EmptyAllowed.value();
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

        // ---- UI metadata: section, priority, requiresReboot, apiPath, widget ----

        if (spec.Section)
            jsonDoc["section"] = spec.Section;
        if (spec.Priority.has_value())
            jsonDoc["priority"] = spec.Priority.value();
        if (spec.RequiresReboot)
            jsonDoc["requiresReboot"] = true;
        if (spec.ApiPath)
            jsonDoc["apiPath"] = spec.ApiPath;

        if (spec.Widget != SettingSpec::WidgetKind::Default)
        {
            auto widget = jsonDoc["widget"].to<JsonObject>();
            widget["kind"] = spec.WidgetName();

            if (spec.Widget == SettingSpec::WidgetKind::Slider
                && spec.DisplayRawMin.has_value()) // Presence of other Display members is validated at SettingSpec construction
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

        if (jsonDoc.overflowed() || !specObject.set(jsonDoc.as<JsonObjectConst>()))
        {
            debugV("JSON response buffer overflow!");
            SendBufferOverflowResponse(pRequest);
            return;
        }
    }

    AddCORSHeaderAndSendResponse(pRequest, response);
}

const std::vector<std::reference_wrapper<SettingSpec>> & CWebServer::LoadDeviceSettingSpecs()
{
    if (deviceSettingSpecs.empty())
    {
        // effectInterval lives on the EffectManager rather than DeviceConfig,
        // so its spec is owned here. The Widget metadata mirrors the legacy
        // composite "Rotate effects toggle + seconds input" UX in a way the
        // front-end can render generically.
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
    SendSettingSpecsResponse(pRequest, LoadDeviceSettingSpecs());
}

// Responds with current config, excluding any sensitive values
void CWebServer::GetSettings(AsyncWebServerRequest * pRequest)
{
    debugV("GetSettings");

    auto response = new AsyncJsonResponse();
    response->addHeader("Server", "NightDriverStrip");
    auto root = response->getRoot();
    JsonObject jsonObject = root.to<JsonObject>();

    // We get the serialized JSON for the device config, without any sensitive values
    g_ptrSystem->GetDeviceConfig().SerializeToJSON(jsonObject, false);
    jsonObject["effectInterval"] = g_ptrSystem->GetEffectManager().GetInterval();

    AddCORSHeaderAndSendResponse(pRequest, response);
}

void CWebServer::GetUnifiedSettings(AsyncWebServerRequest * pRequest)
{
    auto response = new AsyncJsonResponse();
    auto root = response->getRoot().to<JsonObject>();
    FillUnifiedSettingsJson(root);
    AddCORSHeaderAndSendResponse(pRequest, response);
}

void CWebServer::GetUnifiedSettingsSchema(AsyncWebServerRequest * pRequest)
{
    auto response = new AsyncJsonResponse();
    auto root = response->getRoot().to<JsonObject>();
    FillUnifiedSettingsSchemaJson(root);
    AddCORSHeaderAndSendResponse(pRequest, response);
}

bool CWebServer::ValidateLegacyDeviceSettings(AsyncWebServerRequest * pRequest, String* errorMessage)
{
    auto& deviceConfig = g_ptrSystem->GetDeviceConfig();

    // Validate the constrained settings first so the legacy POST path behaves like the unified JSON API:
    // either the nontrivial request is coherent as a whole, or we reject it before any setters persist a
    // partially applied config.

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

bool CWebServer::ValidateUnifiedDeviceSettings(JsonObjectConst device, String* errorMessage)
{
    auto& deviceConfig = g_ptrSystem->GetDeviceConfig();

    if (device[DeviceConfig::OpenWeatherApiKeyTag].is<String>())
    {
        auto [isValid, validationMessage] = deviceConfig.ValidateOpenWeatherAPIKey(device[DeviceConfig::OpenWeatherApiKeyTag].as<String>());
        if (!isValid)
        {
            if (errorMessage)
                *errorMessage = validationMessage;
            return false;
        }
    }

    if (device[DeviceConfig::PowerLimitTag].is<int>())
    {
        auto [isValid, validationMessage] = deviceConfig.ValidatePowerLimit(device[DeviceConfig::PowerLimitTag].as<int>());
        if (!isValid)
        {
            if (errorMessage)
                *errorMessage = validationMessage;
            return false;
        }
    }

    if (device[DeviceConfig::BrightnessTag].is<int>())
    {
        auto [isValid, validationMessage] = deviceConfig.ValidateBrightness(device[DeviceConfig::BrightnessTag].as<int>());
        if (!isValid)
        {
            if (errorMessage)
                *errorMessage = validationMessage;
            return false;
        }
    }

    std::optional<int> requestedAudioInputPin;

    if (device[DeviceConfig::AudioInputPinTag].is<int>())
        requestedAudioInputPin = device[DeviceConfig::AudioInputPinTag].as<int>();

    if (device["audio"].is<JsonObjectConst>())
    {
        auto audio = device["audio"].as<JsonObjectConst>();
        if (audio["inputPin"].is<int>())
        {
            const int nestedAudioInputPin = audio["inputPin"].as<int>();
            // The API still accepts both the legacy flat key and the structured audio object. Reject
            // conflicting requests explicitly so callers do not get a half-legacy, half-modern winner
            // picked implicitly by field order.
            if (requestedAudioInputPin.has_value() && requestedAudioInputPin.value() != nestedAudioInputPin)
            {
                if (errorMessage)
                    *errorMessage = "Malformed request";
                return false;
            }

            requestedAudioInputPin = nestedAudioInputPin;
        }
    }

    if (requestedAudioInputPin.has_value())
    {
        auto [isValid, validationMessage] = deviceConfig.ValidateAudioInputPin(requestedAudioInputPin.value());
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

// Support function that applies whatever settings are included in the request passed after validating the
// nontrivial ones up front. Returning false lets the caller report a single coherent bad-request error
// instead of half-applying device config and then discovering a later constraint violation.
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
        String runtimeErrorMessage;
        if (!ApplyRuntimeConfigTransaction(runtimeConfig, &runtimeErrorMessage))
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
    PushPostParamIfPresent<int>(pRequest, DeviceConfig::AudioInputPinTag, SET_VALUE(deviceConfig.SetAudioInputPin(value)));

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

// Set settings and return resulting config
void CWebServer::SetSettings(AsyncWebServerRequest * pRequest)
{
    debugV("SetSettings");

    String errorMessage;
    if (!SetSettingsIfPresent(pRequest, &errorMessage))
    {
        AddCORSHeaderAndSendBadRequest(pRequest, errorMessage);
        return;
    }

    // We return the current config in response
    GetSettings(pRequest);
}

void CWebServer::SetUnifiedSettings(AsyncWebServerRequest * pRequest, JsonVariantConst json)
{
    if (!json.is<JsonObjectConst>())
    {
        AddCORSHeaderAndSendBadRequest(pRequest, "Malformed request");
        return;
    }

    auto root = json.as<JsonObjectConst>();
    auto& deviceConfig = g_ptrSystem->GetDeviceConfig();
    auto& effectManager = g_ptrSystem->GetEffectManager();
    const auto previousRuntimeConfig = deviceConfig.GetRuntimeConfig();
    auto runtimeConfig = previousRuntimeConfig;

    if (root["topology"].is<JsonObjectConst>())
    {
        auto topology = root["topology"].as<JsonObjectConst>();
        if (topology["width"].is<uint16_t>()) runtimeConfig.topology.width = topology["width"].as<uint16_t>();
        if (topology["height"].is<uint16_t>()) runtimeConfig.topology.height = topology["height"].as<uint16_t>();
        if (topology["serpentine"].is<bool>()) runtimeConfig.topology.serpentine = topology["serpentine"].as<bool>();
    }

    if (root["outputs"].is<JsonObjectConst>())
    {
        auto outputs = root["outputs"].as<JsonObjectConst>();
        if (outputs["driver"].is<String>())
        {
            const auto driver = outputs["driver"].as<String>();
            runtimeConfig.outputs.driver = driver == "hub75" ? DeviceConfig::OutputDriver::HUB75 : DeviceConfig::OutputDriver::WS281x;
        }

        if (outputs["ws281x"].is<JsonObjectConst>())
        {
            auto ws281x = outputs["ws281x"].as<JsonObjectConst>();
            if (ws281x["channelCount"].is<size_t>()) runtimeConfig.outputs.channelCount = ws281x["channelCount"].as<size_t>();
            if (ws281x["colorOrder"].is<String>())
            {
                const auto colorOrder = ws281x["colorOrder"].as<String>();
                if (colorOrder == "RGB") runtimeConfig.outputs.colorOrder = DeviceConfig::WS281xColorOrder::RGB;
                else if (colorOrder == "RBG") runtimeConfig.outputs.colorOrder = DeviceConfig::WS281xColorOrder::RBG;
                else if (colorOrder == "GRB") runtimeConfig.outputs.colorOrder = DeviceConfig::WS281xColorOrder::GRB;
                else if (colorOrder == "GBR") runtimeConfig.outputs.colorOrder = DeviceConfig::WS281xColorOrder::GBR;
                else if (colorOrder == "BRG") runtimeConfig.outputs.colorOrder = DeviceConfig::WS281xColorOrder::BRG;
                else if (colorOrder == "BGR") runtimeConfig.outputs.colorOrder = DeviceConfig::WS281xColorOrder::BGR;
            }
            if (ws281x["pins"].is<JsonArrayConst>())
            {
                auto pins = ws281x["pins"].as<JsonArrayConst>();
                for (size_t i = 0; i < runtimeConfig.outputs.outputPins.size() && i < pins.size(); ++i)
                {
                    if (pins[i].is<int>())
                        runtimeConfig.outputs.outputPins[i] = pins[i].as<int>();
                }
            }
        }
    }

    auto [runtimeConfigValid, runtimeConfigMessage] = deviceConfig.ValidateRuntimeConfig(runtimeConfig);
    if (!runtimeConfigValid)
    {
        AddCORSHeaderAndSendBadRequest(pRequest, runtimeConfigMessage);
        return;
    }

    if (root["device"].is<JsonObjectConst>())
    {
        auto device = root["device"].as<JsonObjectConst>();
        String validationMessage;
        if (!ValidateUnifiedDeviceSettings(device, &validationMessage))
        {
            AddCORSHeaderAndSendBadRequest(pRequest, validationMessage);
            return;
        }
    }

    const bool runtimeConfigChanged =
        previousRuntimeConfig.topology.width != runtimeConfig.topology.width
        || previousRuntimeConfig.topology.height != runtimeConfig.topology.height
        || previousRuntimeConfig.topology.serpentine != runtimeConfig.topology.serpentine
        || previousRuntimeConfig.outputs.driver != runtimeConfig.outputs.driver
        || previousRuntimeConfig.outputs.channelCount != runtimeConfig.outputs.channelCount
        || previousRuntimeConfig.outputs.outputPins != runtimeConfig.outputs.outputPins
        || previousRuntimeConfig.outputs.colorOrder != runtimeConfig.outputs.colorOrder;

    String errorMessage;
    if (runtimeConfigChanged && !ApplyRuntimeConfigTransaction(runtimeConfig, &errorMessage))
    {
        AddCORSHeaderAndSendBadRequest(pRequest, errorMessage);
        return;
    }

    if (root["device"].is<JsonObjectConst>())
    {
        auto device = root["device"].as<JsonObjectConst>();
        if (device[DeviceConfig::HostnameTag].is<String>()) deviceConfig.SetHostname(device[DeviceConfig::HostnameTag].as<String>());
        if (device[DeviceConfig::LocationTag].is<String>()) deviceConfig.SetLocation(device[DeviceConfig::LocationTag].as<String>());
        if (device[DeviceConfig::LocationIsZipTag].is<bool>()) deviceConfig.SetLocationIsZip(device[DeviceConfig::LocationIsZipTag].as<bool>());
        if (device[DeviceConfig::CountryCodeTag].is<String>()) deviceConfig.SetCountryCode(device[DeviceConfig::CountryCodeTag].as<String>());
        if (device[DeviceConfig::OpenWeatherApiKeyTag].is<String>()) deviceConfig.SetOpenWeatherAPIKey(device[DeviceConfig::OpenWeatherApiKeyTag].as<String>());
        if (device[DeviceConfig::TimeZoneTag].is<String>()) deviceConfig.SetTimeZone(device[DeviceConfig::TimeZoneTag].as<String>());
        if (device[DeviceConfig::Use24HourClockTag].is<bool>()) deviceConfig.Set24HourClock(device[DeviceConfig::Use24HourClockTag].as<bool>());
        if (device[DeviceConfig::UseCelsiusTag].is<bool>()) deviceConfig.SetUseCelsius(device[DeviceConfig::UseCelsiusTag].as<bool>());
        if (device[DeviceConfig::NTPServerTag].is<String>()) deviceConfig.SetNTPServer(device[DeviceConfig::NTPServerTag].as<String>());
        if (device[DeviceConfig::RememberCurrentEffectTag].is<bool>()) deviceConfig.SetRememberCurrentEffect(device[DeviceConfig::RememberCurrentEffectTag].as<bool>());
        if (device[DeviceConfig::PowerLimitTag].is<int>()) deviceConfig.SetPowerLimit(device[DeviceConfig::PowerLimitTag].as<int>());
        if (device[DeviceConfig::BrightnessTag].is<int>()) deviceConfig.SetBrightness(device[DeviceConfig::BrightnessTag].as<int>());
        if (const auto requestedAudioInputPin = GetRequestedUnifiedAudioInputPin(device); requestedAudioInputPin.has_value())
            deviceConfig.SetAudioInputPin(requestedAudioInputPin.value());

        std::optional<CRGB> globalColor = {};
        std::optional<CRGB> secondColor = {};
        if (device[DeviceConfig::GlobalColorTag].is<CRGB>()) globalColor = device[DeviceConfig::GlobalColorTag].as<CRGB>();
        if (device[DeviceConfig::SecondColorTag].is<CRGB>()) secondColor = device[DeviceConfig::SecondColorTag].as<CRGB>();
        deviceConfig.ApplyColorSettings(globalColor, secondColor,
                                        device[DeviceConfig::ClearGlobalColorTag].is<bool>() && device[DeviceConfig::ClearGlobalColorTag].as<bool>(),
                                        device[DeviceConfig::ApplyGlobalColorsTag].is<bool>() && device[DeviceConfig::ApplyGlobalColorsTag].as<bool>());
    }

    if (root["effects"].is<JsonObjectConst>())
    {
        auto effects = root["effects"].as<JsonObjectConst>();
        if (effects["effectInterval"].is<size_t>())
            effectManager.SetInterval(effects["effectInterval"].as<size_t>());
    }

    GetUnifiedSettings(pRequest);
}

bool CWebServer::CheckAndGetSettingsEffect(AsyncWebServerRequest * pRequest, std::shared_ptr<LEDStripEffect> & effect, bool post)
{
    const auto& effectsList = g_ptrSystem->GetEffectManager().EffectsList();
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
    auto response = std::make_unique<AsyncJsonResponse>();
    auto jsonObject = response->getRoot().to<JsonObject>();

    if (effect->SerializeSettingsToJSON(jsonObject))
    {
        AddCORSHeaderAndSendResponse(pRequest, response.release());
        return;
    }

    debugV("JSON response buffer overflow!");
    SendBufferOverflowResponse(pRequest);
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
        const auto& spec = settingSpecWrapper.get();
        // Settings with an ApiPath are addressed via a structured path, not by name — skip them here.
        if (spec.ApiPath)
            continue;
        const String& settingName = spec.Name;
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

        // Settings with an ApiPath are addressed via a structured path, not by name — skip them here.
        if (settingSpec.ApiPath)
            continue;

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
    String errorMessage;
    if (!SetSettingsIfPresent(pRequest, &errorMessage))
    {
        AddCORSHeaderAndSendBadRequest(pRequest, errorMessage);
        return;
    }
    AddCORSHeaderAndSendOKResponse(pRequest);
}

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
        g_ptrSystem->GetJSONWriter().FlushWrites(true);

        // Give the device a few seconds to finish the requested writes - this also gives AsyncWebServer
        //   time to push out the response to the request before the device resets
        delay(3000);
    }

    if (deviceConfigResetRequested)
    {
        debugI("Removing DeviceConfig");
        g_ptrSystem->GetDeviceConfig().RemovePersisted();
    }

    if (effectsConfigResetRequested)
    {
        debugI("Removing EffectManager config");
        RemoveEffectManagerConfig();
    }

    if (boardResetRequested)
    {
        debugW("Resetting device at API request!");
        delay(250);
        ESP.restart();
    }
}

#endif  // ENABLE_WEBSERVER
