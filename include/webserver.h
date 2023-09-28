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
#include <SPIFFS.h>
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
        uint32_t HeapSize       = 0;
        size_t DmaHeapSize      = 0;
        uint32_t PsramSize      = 0;
        const char *ChipModel   = nullptr;
        uint8_t ChipCores       = 0;
        uint32_t CpuFreqMHz     = 0;
        uint32_t SketchSize     = 0;
        uint32_t FreeSketchSpace= 0;
        uint32_t FlashChipSize  = 0;
    };

    // Properties of files baked into the image
    struct EmbeddedWebFile : public EmbeddedFile
    {
        // Added to hold the file's MIME type, but could be used for other type types, if desired
        const char *const type;
        const char *const encoding;

        EmbeddedWebFile(const uint8_t start[], const uint8_t end[], const char type[], const char encoding[])
            : EmbeddedFile(start, end), type(type), encoding(encoding)
        {
        }
    };

    static std::vector<SettingSpec, psram_allocator<SettingSpec>> mySettingSpecs;
    static std::vector<std::reference_wrapper<SettingSpec>> deviceSettingSpecs;
    static const std::map<String, ValueValidator> settingValidators;

    AsyncWebServer _server;
    StaticStatistics _staticStats;

    // Helper functions/templates

    // Convert param value to a specific type and forward it to a setter function that expects that type as an argument
    template<typename Tv>
    static bool PushPostParamIfPresent(AsyncWebServerRequest * pRequest, const String & paramName, ValueSetter<Tv> setter, ParamValueGetter<Tv> getter)
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
    static bool PushPostParamIfPresent(AsyncWebServerRequest * pRequest, const String & paramName, ValueSetter<Tv> setter)
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

    static void AddCORSHeaderAndSendBadRequest(AsyncWebServerRequest * pRequest, const String& message)
    {
        AddCORSHeaderAndSendResponse(pRequest, pRequest->beginResponse(HTTP_CODE_BAD_REQUEST, "text/json",
            "{\"message\": \"" + message + "\"}"));
    }

    // Straightforward support functions

    static bool IsPostParamTrue(AsyncWebServerRequest * pRequest, const String & paramName);
    static const std::vector<std::reference_wrapper<SettingSpec>> & LoadDeviceSettingSpecs();
    static void SendSettingSpecsResponse(AsyncWebServerRequest * pRequest, const std::vector<std::reference_wrapper<SettingSpec>> & settingSpecs);
    static void SetSettingsIfPresent(AsyncWebServerRequest * pRequest);
    static long GetEffectIndexFromParam(AsyncWebServerRequest * pRequest, bool post = false);
    static bool CheckAndGetSettingsEffect(AsyncWebServerRequest * pRequest, std::shared_ptr<LEDStripEffect> & effect, bool post = false);
    static void SendEffectSettingsResponse(AsyncWebServerRequest * pRequest, std::shared_ptr<LEDStripEffect> & effect);
    static bool ApplyEffectSettings(AsyncWebServerRequest * pRequest, std::shared_ptr<LEDStripEffect> & effect);

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
    static void MoveEffect(AsyncWebServerRequest * pRequest);
    static void CopyEffect(AsyncWebServerRequest * pRequest);
    static void DeleteEffect(AsyncWebServerRequest * pRequest);
    static void NextEffect(AsyncWebServerRequest * pRequest);
    static void PreviousEffect(AsyncWebServerRequest * pRequest);

    // Not static because it uses member _staticStats
    void GetStatistics(AsyncWebServerRequest * pRequest);

    // This registers a handler for GET requests for one of the known files embedded in the firmware.
    void ServeEmbeddedFile(const char strUri[], EmbeddedWebFile &file)
    {
        _server.on(strUri, HTTP_GET, [strUri, file](AsyncWebServerRequest *request) {
            Serial.printf("GET for: %s\n", strUri);
            AsyncWebServerResponse *response = request->beginResponse_P(200, file.type, file.contents, file.length);
            if (file.encoding[0])
            {
                response->addHeader("Content-Encoding", file.encoding);
            }
            request->send(response);
        });
    }

  public:

    CWebServer()
        : _server(80), _staticStats()
    {}

    // begin - register page load handlers and start serving pages
    void begin();
};

// Set value in lambda using a forwarding function. Always returns true
#define SET_VALUE(functionCall) [&](auto value) { functionCall; return true; }

// Set value in lambda using a forwarding function. Reports success based on function's return value,
//   which must be implicitly convertable to bool
#define CONFIRM_VALUE(functionCall) [&](auto value)->bool { return functionCall; }
