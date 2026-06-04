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

#include <algorithm>
#include <AsyncJson.h>
#include <FS.h>
#include <cstdlib>
#include <limits>
#include <utility>

#include "deviceconfig.h"
#include "effects.h"
#include "effectmanager.h"
#include "gfxbase.h"
#include "improvserial.h"
#include "soundanalyzer.h"
#include "systemcontainer.h"
#include "taskmgr.h"
#include "values.h"
#include "websocketserver.h"   // Stop() tears down our websocket clients first

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
void CWebServer::AddCORSHeaderAndSendResponse<AsyncJsonResponse>(AsyncWebServerRequest * pRequest, AsyncJsonResponse * pResponse) const
{
    pResponse->setLength();
    AddCORSHeaderAndSendResponse<AsyncWebServerResponse>(pRequest, pResponse);
}

CWebServer::CWebServer()
        : _server(NetworkPort::Webserver), _staticStats(),
            _settingValidators({
                { DeviceConfig::OpenWeatherApiKeyTag, [](const String& value) { return g_ptrSystem->GetDeviceConfig().ValidateOpenWeatherAPIKey(value); } },
                { DeviceConfig::PowerLimitTag,        [](const String& value) { return g_ptrSystem->GetDeviceConfig().ValidatePowerLimit(value); } },
                { DeviceConfig::BrightnessTag,        [](const String& value) { return g_ptrSystem->GetDeviceConfig().ValidateBrightness(value); } },
                { DeviceConfig::AudioInputPinTag,     [](const String& value) { return g_ptrSystem->GetDeviceConfig().ValidateAudioInputPin(value.toInt()); } }
            })
{}

// Member function implementations

// IService::Start - delegates to begin(). begin() does the heavy lifting of
// registering route handlers and calling AsyncWebServer::begin(); Start just
// sets the running flag and reports the result.
bool CWebServer::Start()
{
    if (_running.load())
        return true;

    if (_everStarted.load())
    {
        debugW("WebServer: Start() after Stop() - AsyncWebServer/AsyncTCP "
               "may not have fully released its listening socket on this "
               "platform; if subsequent requests fail, a reboot is the "
               "documented workaround.");
    }

    debugI("WebServer: starting on port %d", (int)NetworkPort::Webserver);
    begin();
    _everStarted.store(true);
    _running.store(true);
    return true;
}

// IService::Stop - shut down the AsyncWebServer. Safe to call before Start().
void CWebServer::Stop()
{
    if (!_running.load())
        return;

    debugI("WebServer: stop requested");
    // The WebSocketServer's listening sockets and handler registration are
    // owned here, so its lifecycle is entirely delegated to ours: tear down
    // any websocket client connections first so they don't outlast end().
    #if WEB_SOCKETS_ANY_ENABLED
        if (g_ptrSystem && g_ptrSystem->HasWebSocketServer())
            g_ptrSystem->GetWebSocketServer().Stop();
    #endif
    _server.end();
    _running.store(false);
    debugI("WebServer: stop completed");
}

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

    if (!EnsureDeviceSettingSpecsJson())
        debugW("WebServer: failed to pre-cache device setting specs JSON.");

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

    // Instance handler requests

    _server.on("/effects",               HTTP_GET,  [this](AsyncWebServerRequest* pRequest) { this->GetEffectListText(pRequest); });
    _server.on("/getEffectList",         HTTP_GET,  [this](AsyncWebServerRequest* pRequest) { this->GetEffectListText(pRequest); });
    _server.on("/nextEffect",            HTTP_POST, [this](AsyncWebServerRequest* pRequest) { this->NextEffect(pRequest); });
    _server.on("/previousEffect",        HTTP_POST, [this](AsyncWebServerRequest* pRequest) { this->PreviousEffect(pRequest); });

    _server.on("/currentEffect",         HTTP_POST, [this](AsyncWebServerRequest* pRequest) { this->SetCurrentEffectIndex(pRequest); });
    _server.on("/setCurrentEffectIndex", HTTP_POST, [this](AsyncWebServerRequest* pRequest) { this->SetCurrentEffectIndex(pRequest); });
    _server.on("/enableEffect",          HTTP_POST, [this](AsyncWebServerRequest* pRequest) { this->EnableEffect(pRequest); });
    _server.on("/disableEffect",         HTTP_POST, [this](AsyncWebServerRequest* pRequest) { this->DisableEffect(pRequest); });
    _server.on("/moveEffect",            HTTP_POST, [this](AsyncWebServerRequest* pRequest) { this->MoveEffect(pRequest); });
    _server.on("/copyEffect",            HTTP_POST, [this](AsyncWebServerRequest* pRequest) { this->CopyEffect(pRequest); });
    _server.on("/deleteEffect",          HTTP_POST, [this](AsyncWebServerRequest* pRequest) { this->DeleteEffect(pRequest); });

    _server.on("/settings/effect/specs", HTTP_GET,  [this](AsyncWebServerRequest* pRequest) { this->GetEffectSettingSpecs(pRequest); });
    _server.on("/settings/effect",       HTTP_GET,  [this](AsyncWebServerRequest* pRequest) { this->GetEffectSettings(pRequest); });
    _server.on("/settings/effect",       HTTP_POST, [this](AsyncWebServerRequest* pRequest) { this->SetEffectSettings(pRequest); });
    _server.on("/settings/validated",    HTTP_POST, [this](AsyncWebServerRequest* pRequest) { this->ValidateAndSetSetting(pRequest); });
    _server.on("/settings/specs",        HTTP_GET,  [this](AsyncWebServerRequest* pRequest) { this->GetSettingSpecs(pRequest); });
    _server.on("/settings",              HTTP_GET,  [this](AsyncWebServerRequest* pRequest) { this->GetSettings(pRequest); });
    _server.on("/settings",              HTTP_POST, [this](AsyncWebServerRequest* pRequest) { this->SetSettings(pRequest); });
    _server.on("/api/v1/settings/schema",HTTP_GET,  [this](AsyncWebServerRequest* pRequest) { this->GetUnifiedSettingsSchema(pRequest); });
    _server.on("/api/v1/settings",       HTTP_GET,  [this](AsyncWebServerRequest* pRequest) { this->GetUnifiedSettings(pRequest); });

    auto* settingsHandler = new AsyncCallbackJsonWebHandler("/api/v1/settings",
        [this](AsyncWebServerRequest* pRequest, JsonVariant& json)
        {
            this->SetUnifiedSettings(pRequest, json.as<JsonVariantConst>());
        });
    settingsHandler->setMethod(HTTP_POST);
    _server.addHandler(settingsHandler);

    _server.on("/reset",                 HTTP_POST, [this](AsyncWebServerRequest* pRequest) { this->Reset(pRequest); });

    // Embedded file requests

    _server.on("/healthz", HTTP_GET, [](AsyncWebServerRequest* pRequest) { pRequest->send(CWebServer::HttpOk, "text/plain", "ok"); });
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
            request->send(CWebServer::HttpNotFound, "text/plain", "Not found");
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
