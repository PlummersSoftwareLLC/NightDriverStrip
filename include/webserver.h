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
//---------------------------------------------------------------------------

#pragma once

#include <WiFi.h>
#include <FS.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <Arduino.h>
#include <AsyncJson.h>
#include <ArduinoJson.h>
#include "deviceconfig.h"
#include "jsonbase.h"

class CWebServer
{
  private:

    struct EmbeddedFile
    {
        // Embedded file size in bytes
        const size_t length;
        // Contents as bytes
        const uint8_t *const contents;
        // Added to hold the file's MIME type, but could be used for other type types, if desired
        const char *const type;

        EmbeddedFile(const uint8_t start[], const uint8_t end[], const char type[]) :
            length(end - start),
            contents(start),
            type(type)
        {}
    };

    AsyncWebServer _server;

    // Helper functions/templates
    template<typename Tv>
    using ParamValueGetter = std::function<Tv(AsyncWebParameter *param)>;

    template<typename Tv>
    using ValueSetter = std::function<void(Tv)>;

    template<typename Tv>
    static void PushPostParamIfPresent(AsyncWebServerRequest * pRequest, const String &paramName, ValueSetter<Tv> setter, ParamValueGetter<Tv> getter)
    {
        if (!pRequest->hasParam(paramName, true, false))
            return;

        debugV("found %s", paramName.c_str());

        AsyncWebParameter *param = pRequest->getParam(paramName, true, false);

        // Extract the value and pass it off to the setter
        setter(getter(param));
    }

    template<typename Tv>
    static void PushPostParamIfPresent(AsyncWebServerRequest * pRequest, const String &paramName, ValueSetter<Tv> setter)
    {
        PushPostParamIfPresent<Tv>(pRequest, paramName, setter, [](AsyncWebParameter * param) constexpr { return param->value(); });
    }

    // AddCORSHeaderAndSend(OK)Response
    //
    // Sends a response with CORS headers
    template<typename Tr>
    static void AddCORSHeaderAndSendResponse(AsyncWebServerRequest * pRequest, Tr * pResponse)
    {
        pResponse->addHeader("Access-Control-Allow-Origin", "*");
        pRequest->send(pResponse);
    }

    // Version for empty response, normally used to finish up things that don't return anything, like "NextEffect"
    static void AddCORSHeaderAndSendOKResponse(AsyncWebServerRequest * pRequest)
    {
        AddCORSHeaderAndSendResponse(pRequest, pRequest->beginResponse(200));
    }

    static bool IsPostParamTrue(AsyncWebServerRequest * pRequest, const String &paramName);

  public:

    CWebServer()
        : _server(80)
    {
    }

    // Endpoint member functions
    void GetEffectListText(AsyncWebServerRequest * pRequest);
    void GetStatistics(AsyncWebServerRequest * pRequest);
    void GetSettings(AsyncWebServerRequest * pRequest);
    void SetSettings(AsyncWebServerRequest * pRequest);
    void Reset(AsyncWebServerRequest * pRequest);
    void SetCurrentEffectIndex(AsyncWebServerRequest * pRequest);
    void EnableEffect(AsyncWebServerRequest * pRequest);
    void DisableEffect(AsyncWebServerRequest * pRequest);
    void NextEffect(AsyncWebServerRequest * pRequest);
    void PreviousEffect(AsyncWebServerRequest * pRequest);

    // This registers a handler for GET requests for one of the known files embedded in the firmware.
    void ServeEmbeddedFile(const char strUri[], EmbeddedFile &file)
    {
        _server.on(strUri, HTTP_GET, [strUri, file](AsyncWebServerRequest *request)
        {
            Serial.printf("GET for: %s\n", strUri);
            AsyncWebServerResponse *response = request->beginResponse_P(200, file.type, file.contents, file.length);
            request->send(response);
        });
    }

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

        debugI("Connecting Web Endpoints");

        _server.on("/getEffectList",         HTTP_GET,  [this](AsyncWebServerRequest * pRequest)    { this->GetEffectListText(pRequest); });
        _server.on("/getStatistics",         HTTP_GET,  [this](AsyncWebServerRequest * pRequest)    { this->GetStatistics(pRequest); });
        _server.on("/nextEffect",            HTTP_POST, [this](AsyncWebServerRequest * pRequest)    { this->NextEffect(pRequest); });
        _server.on("/previousEffect",        HTTP_POST, [this](AsyncWebServerRequest * pRequest)    { this->PreviousEffect(pRequest); });

        _server.on("/setCurrentEffectIndex", HTTP_POST, [this](AsyncWebServerRequest * pRequest)    { this->SetCurrentEffectIndex(pRequest); });
        _server.on("/enableEffect",          HTTP_POST, [this](AsyncWebServerRequest * pRequest)    { this->EnableEffect(pRequest); });
        _server.on("/disableEffect",         HTTP_POST, [this](AsyncWebServerRequest * pRequest)    { this->DisableEffect(pRequest); });

        _server.on("/settings",              HTTP_GET,  [this](AsyncWebServerRequest * pRequest)    { this->GetSettings(pRequest); });
        _server.on("/settings",              HTTP_POST, [this](AsyncWebServerRequest * pRequest)    { this->SetSettings(pRequest); });

        _server.on("/reset",                 HTTP_POST, [this](AsyncWebServerRequest * pRequest)    { this->Reset(pRequest); });

        EmbeddedFile html_file(html_start, html_end, "text/html");
        EmbeddedFile jsx_file(jsx_start, jsx_end, "application/javascript");
        EmbeddedFile ico_file(ico_start, ico_end, "image/vnd.microsoft.icon");
        EmbeddedFile timezones_file(timezones_start, timezones_end - 1, "text/json"); // end - 1 because of zero-termination

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
                request->send(200);                                     // Apparently needed for CORS: https://github.com/me-no-dev/ESPAsyncWebServer
            } else {
                   debugW("Failed GET for %s\n", request->url().c_str() );
                request->send(404);
            }
        });

        _server.begin();

        debugI("HTTP server started");
    }
};
