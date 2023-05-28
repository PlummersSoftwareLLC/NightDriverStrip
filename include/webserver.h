//+--------------------------------------------------------------------------
//
// File:        webserver.h
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
//   Web server that fulfills requests by serving them from statically
//   files in flash.  Requires the espressif-esp32 WebServer class.
//
//   This class contains an early attempt at exposing a REST api for
//   adjusting effect paramters.  I'm in no way attached to it and it
//   should likely be redone!
//
//   Server also exposes basic RESTful API for querying variables etc.
//
// History:     Jul-12-2018         Davepl      Created
//              Apr-29-2019         Davepl      Adapted from BigBlueLCD project
//              Feb-02-2023         LouisRiel   Removed SPIFF served files with statically linked files
//              Apr-28-2023         Rbergen     Reduce code duplication
//---------------------------------------------------------------------------

#pragma once

#include <map>
#include <WiFi.h>
#include <FS.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <Arduino.h>
#include <AsyncJson.h>
#include <ArduinoJson.h>
#include <HTTPClient.h>
#include "deviceconfig.h"
#include "jsonbase.h"
#include "effects.h"

class CWebServer
{
  private:
    // Template for param to value converter function, used by PushPostParamIfPresent()
    template<typename Tv>
    using ParamValueGetter = std::function<Tv(AsyncWebParameter *param)>;

    // Template for value setting forwarding function, used by PushPostParamIfPresent()
    template<typename Tv>
    using ValueSetter = std::function<bool(Tv)>;

    // Value validating function type, as used by DeviceConfig (and possible others)
    using ValueValidator = std::function<DeviceConfig::ValidateResponse(const String&)>;

    // Device stats that don't change after startup
    struct StaticStatistics
    {
        uint32_t HeapSize;
        size_t DmaHeapSize;
        uint32_t PsramSize;
        const char *ChipModel;
        uint8_t ChipCores;
        uint32_t CpuFreqMHz;
        uint32_t SketchSize;
        uint32_t FreeSketchSpace;
        uint32_t FlashChipSize;
    };

    // Properties of files baked into the image
    struct EmbeddedWebFile : public EmbeddedFile
    {
        // Added to hold the file's MIME type, but could be used for other type types, if desired
        const char *const type;

        EmbeddedWebFile(const uint8_t start[], const uint8_t end[], const char type[]) :
            EmbeddedFile(start, end),
            type(type)
        {}
    };

    static std::vector<SettingSpec> deviceSettingSpecs;
    static const std::map<String, ValueValidator> settingValidators;

    AsyncWebServer _server;
    StaticStatistics _staticStats;

    // Helper functions/templates

    // Convert param value to a specific type and forward it to a setter function that expects that type as an argument
    template<typename Tv>
    static bool PushPostParamIfPresent(AsyncWebServerRequest * pRequest, const String &paramName, ValueSetter<Tv> setter, ParamValueGetter<Tv> getter)
    {
        if (!pRequest->hasParam(paramName, true, false))
            return false;

        debugV("found %s", paramName.c_str());

        AsyncWebParameter *param = pRequest->getParam(paramName, true, false);

        // Extract the value and pass it off to the setter
        return setter(getter(param));
    }

    // Generic param value forwarder. The type argument must be implicitly convertable from String!
    //   Some specializations of this are included in the CPP file
    template<typename Tv>
    static bool PushPostParamIfPresent(AsyncWebServerRequest * pRequest, const String &paramName, ValueSetter<Tv> setter)
    {
        return PushPostParamIfPresent<Tv>(pRequest, paramName, setter, [](AsyncWebParameter * param) { return param->value(); });
    }

    // AddCORSHeaderAndSend(OK)Response
    //
    // Sends a response with CORS headers added
    template<typename Tr>
    static void AddCORSHeaderAndSendResponse(AsyncWebServerRequest * pRequest, Tr * pResponse)
    {
        pResponse->addHeader("Server","NightDriverStrip");
        pResponse->addHeader("Access-Control-Allow-Origin", "*");
        pRequest->send(pResponse);
    }

    // Version for empty response, normally used to finish up things that don't return anything, like "NextEffect"
    static void AddCORSHeaderAndSendOKResponse(AsyncWebServerRequest * pRequest)
    {
        AddCORSHeaderAndSendResponse(pRequest, pRequest->beginResponse(HTTP_CODE_OK));
    }

    // Straightforward support functions

    static bool IsPostParamTrue(AsyncWebServerRequest * pRequest, const String & paramName);
    static const std::vector<SettingSpec> & LoadDeviceSettingSpecs();
    static void SendSettingSpecsResponse(AsyncWebServerRequest * pRequest, const std::vector<SettingSpec> & settingSpecs);
    static void SetSettingsIfPresent(AsyncWebServerRequest * pRequest);
    static long GetEffectIndexFromParam(AsyncWebServerRequest * pRequest, bool post = false);
    static bool CheckAndGetSettingsEffect(AsyncWebServerRequest * pRequest, std::shared_ptr<LEDStripEffect> & effect, bool post = false);
    static void SendEffectSettingsResponse(AsyncWebServerRequest * pRequest, std::shared_ptr<LEDStripEffect> & effect);

    // Endpoint member functions

    static void GetEffectListText(AsyncWebServerRequest * pRequest);
    static void GetSettingSpecs(AsyncWebServerRequest * pRequest);
    static void GetSettings(AsyncWebServerRequest * pRequest);
    static void SetSettings(AsyncWebServerRequest * pRequest);
    static void GetEffectSettingSpecs(AsyncWebServerRequest * pRequest);
    static void GetEffectSettings(AsyncWebServerRequest * pRequest);
    static void SetEffectSettings(AsyncWebServerRequest * pRequest);
    static void ValidateAndSetSetting(AsyncWebServerRequest * pRequest);
    static void Reset(AsyncWebServerRequest * pRequest);
    static void SetCurrentEffectIndex(AsyncWebServerRequest * pRequest);
    static void EnableEffect(AsyncWebServerRequest * pRequest);
    static void DisableEffect(AsyncWebServerRequest * pRequest);
    static void NextEffect(AsyncWebServerRequest * pRequest);
    static void PreviousEffect(AsyncWebServerRequest * pRequest);

    // Not static because it uses member _staticStats
    void GetStatistics(AsyncWebServerRequest * pRequest);

    // This registers a handler for GET requests for one of the known files embedded in the firmware.
    void ServeEmbeddedFile(const char strUri[], EmbeddedWebFile &file)
    {
        _server.on(strUri, HTTP_GET, [strUri, file](AsyncWebServerRequest *request)
        {
            Serial.printf("GET for: %s\n", strUri);
            AsyncWebServerResponse *response = request->beginResponse_P(200, file.type, file.contents, file.length);
            request->send(response);
        });
    }

  public:

    CWebServer()
        : _server(80)
    {}

    // begin - register page load handlers and start serving pages
    void begin()
    {
        extern const uint8_t html_start[] asm("_binary_site_index_html_start");
        extern const uint8_t html_end[] asm("_binary_site_index_html_end");
        extern const uint8_t jsx_start[] asm("_binary_site_main_jsx_start");
        extern const uint8_t jsx_end[] asm("_binary_site_main_jsx_end");
        extern const uint8_t ico_start[] asm("_binary_site_favicon_ico_start");
        extern const uint8_t ico_end[] asm("_binary_site_favicon_ico_end");
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

        _server.on("/settings/effect/specs", HTTP_GET,  [](AsyncWebServerRequest * pRequest)        { GetEffectSettingSpecs(pRequest); });
        _server.on("/settings/effect",       HTTP_GET,  [](AsyncWebServerRequest * pRequest)        { GetEffectSettings(pRequest); });
        _server.on("/settings/effect",       HTTP_POST, [](AsyncWebServerRequest * pRequest)        { SetEffectSettings(pRequest); });
        _server.on("/settings/validated",    HTTP_POST, [](AsyncWebServerRequest * pRequest)        { ValidateAndSetSetting(pRequest); });
        _server.on("/settings/specs",        HTTP_GET,  [](AsyncWebServerRequest * pRequest)        { GetSettingSpecs(pRequest); });
        _server.on("/settings",              HTTP_GET,  [](AsyncWebServerRequest * pRequest)        { GetSettings(pRequest); });
        _server.on("/settings",              HTTP_POST, [](AsyncWebServerRequest * pRequest)        { SetSettings(pRequest); });
        _server.on("/effectsConfig",         HTTP_GET,  [](AsyncWebServerRequest * pRequest)        { pRequest->send(SPIFFS, EFFECTS_CONFIG_FILE, "text/json"); });

        _server.on("/reset",                 HTTP_POST, [](AsyncWebServerRequest * pRequest)        { Reset(pRequest); });

        EmbeddedWebFile html_file(html_start, html_end, "text/html");
        EmbeddedWebFile jsx_file(jsx_start, jsx_end, "application/javascript");
        EmbeddedWebFile ico_file(ico_start, ico_end, "image/vnd.microsoft.icon");
        EmbeddedWebFile timezones_file(timezones_start, timezones_end - 1, "text/json"); // end - 1 because of zero-termination

        debugI("Embedded html file size: %d", html_file.length);
        debugI("Embedded jsx file size: %d", jsx_file.length);
        debugI("Embedded ico file size: %d", ico_file.length);
        debugI("Embedded timezones file size: %d", timezones_file.length);

        ServeEmbeddedFile("/", html_file);
        ServeEmbeddedFile("/index.html", html_file);
        ServeEmbeddedFile("/main.jsx", jsx_file);
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
};

// Set value in lambda using a forwarding function. Always returns true
#define SET_VALUE(functionCall) [](auto value) { functionCall; return true; }

// Set value in lambda using a forwarding function. Reports success based on function's return value,
//   which must be implicitly convertable to bool
#define CONFIRM_VALUE(functionCall) [](auto value)->bool { return functionCall; }