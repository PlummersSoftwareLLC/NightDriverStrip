#pragma once

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
//   Web server that fulfills requests by serving them from static
//   files in flash.  Requires the Espressif-esp32 WebServer class.
//
//   This class contains an early attempt at exposing a REST api for
//   adjusting effect parameters.  I'm in no way attached to it and it
//   should likely be redone!
//
//   Server also exposes basic RESTful API for querying variables etc.
//
// History:     Jul-12-2018         Davepl      Created
//              Apr-29-2019         Davepl      Adapted from BigBlueLCD project
//              Feb-02-2023         LouisRiel   Removed SPIFF served files with statically linked files
//              Apr-28-2023         Rbergen     Reduce code duplication
//---------------------------------------------------------------------------

#include "globals.h"

#if ENABLE_WEBSERVER

#include <ArduinoJson.h>
#include <ESPAsyncWebServer.h>
#include <atomic>
#include <map>

#include "deviceconfig.h"
#include "iservice.h"
#include "jsonserializer.h"
#include "nd_network.h"

class LEDStripEffect;

class CWebServer : public IService
{
  public:
    static constexpr int HttpOk = 200;
    static constexpr int HttpBadRequest = 400;
    static constexpr int HttpNotFound = 404;
    static constexpr int HttpInternalServerError = 500;

    enum class StatisticsType : uint8_t
    {
        None    = 0,
        Static  = 1 << 0,
        Dynamic = 1 << 1,
        All     = Static | Dynamic
    };

  private:
    // Template for param to value converter function, used by PushPostParamIfPresent()
    template<typename Tv>
    using ParamValueGetter = std::function<Tv(const AsyncWebParameter *param)>;

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

        EmbeddedWebFile(const uint8_t* start, const uint8_t* end, const char* type, const char* encoding = nullptr)
            : EmbeddedFile(start, end), type(type), encoding(encoding)
        {
        }
    };

    static std::vector<SettingSpec, psram_allocator<SettingSpec>> mySettingSpecs;
    static std::vector<std::reference_wrapper<SettingSpec>> deviceSettingSpecs;
    static String deviceSettingSpecsJson;
    // Per-channel pin specs synthesized at first /settings/specs request from
    // the compiled channel maximum, with stable backing storage for the
    // const char* fields they hold.

    static const std::map<String, ValueValidator> settingValidators;

    AsyncWebServer _server;
    StaticStatistics _staticStats;
    std::atomic<bool> _running{false};

    // Sticky flag: AsyncWebServer::begin() registers a TCP listener inside
    // LWIP and AsyncTCP that we cannot fully reset from this fork. Once we
    // have called begin() the first time we remember it here so a Stop()
    // followed by Start() can warn loudly rather than silently leak/double-
    // bind the listening socket. (We still do the begin again because the
    // AsyncTCP server is normally reused; this just surfaces the limitation.)

    std::atomic<bool> _everStarted{false};

    // Helper functions/templates

    // Convert param value to a specific type and forward it to a setter function that expects that type as an argument
    template<typename Tv>
    static bool PushPostParamIfPresent(const AsyncWebServerRequest * pRequest, const String & paramName, ValueSetter<Tv> setter, ParamValueGetter<Tv> getter)
    {
        if (!pRequest->hasParam(paramName, true, false))
            return false;

        debugV("found %s", paramName.c_str());

        // Extract the value and pass it off to the setter
        return setter(getter(pRequest->getParam(paramName, true, false)));
    }

    // Generic param value forwarder. The type argument must be implicitly convertable from String!
    //   Some specializations of this are included in the CPP file
    template<typename Tv>
    static bool PushPostParamIfPresent(const AsyncWebServerRequest * pRequest, const String & paramName, ValueSetter<Tv> setter)
    {
        return PushPostParamIfPresent<Tv>(pRequest, paramName, setter, [](const AsyncWebParameter * param) { return param->value(); });
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
        AddCORSHeaderAndSendResponse(pRequest, pRequest->beginResponse(HttpOk));
    }

    static void AddCORSHeaderAndSendBadRequest(AsyncWebServerRequest * pRequest, const String& message)
    {
        AddCORSHeaderAndSendResponse(pRequest, pRequest->beginResponse(HttpBadRequest, "text/json",
            "{\"message\": \"" + message + "\"}"));
    }

    // Straightforward support functions

    static void SendBufferOverflowResponse(AsyncWebServerRequest * pRequest);
    static bool IsPostParamTrue(AsyncWebServerRequest * pRequest, const String & paramName);
    static const std::vector<std::reference_wrapper<SettingSpec>> & LoadDeviceSettingSpecs();
    static bool EnsureDeviceSettingSpecsJson();
    static bool BuildSettingSpecsJson(String& json, const std::vector<std::reference_wrapper<SettingSpec>> & settingSpecs);
    static void SendSettingSpecsResponse(AsyncWebServerRequest * pRequest, const std::vector<std::reference_wrapper<SettingSpec>> & settingSpecs);
    static bool ValidateLegacyDeviceSettings(AsyncWebServerRequest * pRequest, String* errorMessage = nullptr);
    static bool SetSettingsIfPresent(AsyncWebServerRequest * pRequest, String* errorMessage = nullptr);

    // Apply a new audio input pin to DeviceConfig and, when the build supports
    // a live reconfigure, push it through AudioService::Reconfigure() without
    // requiring a reboot. Reverts the persisted pin on failure. Safe to call
    // whether or not g_ptrSystem / AudioService are present. Returns true if
    // either the pin was unchanged or the live reconfigure succeeded.

    static bool ApplyAudioInputPinChange(int oldPin);
    static long GetEffectIndexFromParam(AsyncWebServerRequest * pRequest, bool post = false);
    static bool CheckAndGetSettingsEffect(AsyncWebServerRequest * pRequest, std::shared_ptr<LEDStripEffect> & effect, bool post = false);
    static void SendEffectSettingsResponse(AsyncWebServerRequest * pRequest, std::shared_ptr<LEDStripEffect> & effect);
    static bool ApplyEffectSettings(AsyncWebServerRequest * pRequest, std::shared_ptr<LEDStripEffect> & effect);

    // Endpoint member functions

    static void GetEffectListText(AsyncWebServerRequest * pRequest);
    static void GetSettingSpecs(AsyncWebServerRequest * pRequest);
    static void GetSettings(AsyncWebServerRequest * pRequest);
    static void SetSettings(AsyncWebServerRequest * pRequest);
    static void GetUnifiedSettings(AsyncWebServerRequest * pRequest);
    static void GetUnifiedSettingsSchema(AsyncWebServerRequest * pRequest);
    static void SetUnifiedSettings(AsyncWebServerRequest * pRequest, JsonVariantConst json);
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
    void GetStatistics(AsyncWebServerRequest * pRequest, StatisticsType statsType = StatisticsType::All) const;

    // This registers a handler for GET requests for one of the known files embedded in the firmware.
    void ServeEmbeddedFile(const char strUri[], EmbeddedWebFile &file)
    {
        _server.on(strUri, HTTP_GET, [strUri, file](AsyncWebServerRequest *request)
        {
            AsyncWebServerResponse *response = request->beginResponse(200, file.type, file.contents, file.length);
            if (file.encoding)
            {
                response->addHeader("Content-Encoding", file.encoding);
            }
            response->addHeader("Cache-Control", "no-store, max-age=0");
            response->addHeader("Pragma", "no-cache");

            AddCORSHeaderAndSendResponse(request, response);
        });
    }

  public:
    CWebServer()
        : _server(NetworkPort::Webserver), _staticStats()
    {}

    ~CWebServer() override { Stop(); }

    // IService lifecycle. Start delegates to begin() (which registers the
    // route handlers and calls AsyncWebServer::begin); Stop ends the server.
    // Idempotent on both sides.
    bool Start() override;
    void Stop() override;
    bool IsRunning() const override   { return _running.load(); }
    const char* Name() const override { return "WebServer"; }

    // begin - register page load handlers and start serving pages.
    // Retained for callers that already use the AsyncWebServer-style name;
    // Start() invokes this internally.
    void begin();

    void AddWebSocket(AsyncWebSocket& webSocket)
    {
        _server.addHandler(&webSocket);
    }

    void RemoveWebSocket(AsyncWebSocket& webSocket)
    {
        _server.removeHandler(&webSocket);
    }
};

inline CWebServer::StatisticsType operator|(CWebServer::StatisticsType lhs, CWebServer::StatisticsType rhs)
{
    return static_cast<CWebServer::StatisticsType>(to_value(lhs) | to_value(rhs));
}

inline CWebServer::StatisticsType operator&(CWebServer::StatisticsType lhs, CWebServer::StatisticsType rhs)
{
    return static_cast<CWebServer::StatisticsType>(to_value(lhs) & to_value(rhs));
}


// Set value in lambda using a forwarding function. Always returns true
#define SET_VALUE(functionCall) [&](auto value) { functionCall; return true; }

// Set value in lambda using a forwarding function. Reports success based on function's return value,
//   which must be implicitly convertable to bool
#define CONFIRM_VALUE(functionCall) [&](auto value)->bool { return functionCall; }
#endif  // ENABLE_WEBSERVER
