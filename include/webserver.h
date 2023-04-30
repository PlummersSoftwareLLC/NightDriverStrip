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
#include "effects.h"

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

  public:

    CWebServer()
        : _server(80)
    {
    }

    // AddCORSHeaderAndSendOKResponse
    //
    // Sends an empty OK/200 response; normally used to finish up things that don't return anything, like "NextEffect"

    void AddCORSHeaderAndSendOKResponse(AsyncWebServerRequest * pRequest, const char * strText = nullptr)
    {
        AsyncWebServerResponse * pResponse = (strText == nullptr) ?
                                                    pRequest->beginResponse(200) :
                                                    pRequest->beginResponse(200, "text/json",  strText);
        pResponse->addHeader("Access-Control-Allow-Origin", "*");
        pRequest->send(pResponse);
    }

    void GetEffectListText(AsyncWebServerRequest * pRequest)
    {
        static size_t jsonBufferSize = JSON_BUFFER_BASE_SIZE;
        bool bufferOverflow;
        debugV("GetEffectListText");

        do
        {
            bufferOverflow = false;
            std::unique_ptr<AsyncJsonResponse> response(new AsyncJsonResponse(false, jsonBufferSize));
            response->addHeader("Server","NightDriverStrip");
            auto j = response->getRoot();

            j["currentEffect"]         = g_aptrEffectManager->GetCurrentEffectIndex();
            j["millisecondsRemaining"] = g_aptrEffectManager->GetTimeRemainingForCurrentEffect();
            j["effectInterval"]        = g_aptrEffectManager->GetInterval();
            j["enabledCount"]          = g_aptrEffectManager->EnabledCount();

            for (int i = 0; i < g_aptrEffectManager->EffectCount(); i++)
            {
                DynamicJsonDocument effectDoc(256);
                effectDoc["name"]    = g_aptrEffectManager->EffectsList()[i]->FriendlyName();
                effectDoc["enabled"] = g_aptrEffectManager->IsEffectEnabled(i);

                if (!j["Effects"].add(effectDoc))
                {
                    bufferOverflow = true;
                    jsonBufferSize += JSON_BUFFER_INCREMENT;
                    debugV("JSON reponse buffer overflow! Increased buffer to %zu bytes", jsonBufferSize);
                    break;
                }
            }

            if (!bufferOverflow)
            {
                response->addHeader("Access-Control-Allow-Origin", "*");
                response->setLength();
                // The 'send' call will take ownership of the response, so we need to release it here
                pRequest->send(response.release());
            }
        } while (bufferOverflow);
    }

    void GetStatistics(AsyncWebServerRequest * pRequest)
    {
        debugV("GetStatistics");

        auto response = new AsyncJsonResponse(false, JSON_BUFFER_BASE_SIZE);
        response->addHeader("Server","NightDriverStrip");
        auto j = response->getRoot();

        j["LED_FPS"]               = g_FPS;
        j["SERIAL_FPS"]            = g_Analyzer._serialFPS;
        j["AUDIO_FPS"]             = g_Analyzer._AudioFPS;

        j["HEAP_SIZE"]             = ESP.getHeapSize();
        j["HEAP_FREE"]             = ESP.getFreeHeap();
        j["HEAP_MIN"]              = ESP.getMinFreeHeap();

        j["DMA_SIZE"]              = heap_caps_get_total_size(MALLOC_CAP_DMA);
        j["DMA_FREE"]              = heap_caps_get_free_size(MALLOC_CAP_DMA);
        j["DMA_MIN"]               = heap_caps_get_largest_free_block(MALLOC_CAP_DMA);

        j["PSRAM_SIZE"]            = ESP.getPsramSize();
        j["PSRAM_FREE"]            = ESP.getFreePsram();
        j["PSRAM_MIN"]             = ESP.getMinFreePsram();

        j["CHIP_MODEL"]            = ESP.getChipModel();
        j["CHIP_CORES"]            = ESP.getChipCores();
        j["CHIP_SPEED"]            = ESP.getCpuFreqMHz();
        j["PROG_SIZE"]             = ESP.getSketchSize();

        j["CODE_SIZE"]             = ESP.getSketchSize();
        j["CODE_FREE"]             = ESP.getFreeSketchSpace();
        j["FLASH_SIZE"]            = ESP.getFlashChipSize();

        j["CPU_USED"]              = g_TaskManager.GetCPUUsagePercent();
        j["CPU_USED_CORE0"]        = g_TaskManager.GetCPUUsagePercent(0);
        j["CPU_USED_CORE1"]        = g_TaskManager.GetCPUUsagePercent(1);

        response->setLength();
        response->addHeader("Access-Control-Allow-Origin", "*");
        pRequest->send(response);
    }

    void GetSettings(AsyncWebServerRequest * pRequest);

    void SetSettings(AsyncWebServerRequest * pRequest);

    void Reset(AsyncWebServerRequest * pRequest);

    void SetCurrentEffectIndex(AsyncWebServerRequest * pRequest)
    {
        debugV("SetCurrentEffectIndex");

        /*
        AsyncWebParameter * param = pRequest->getParam(0);
        if (param != nullptr)
        {
            debugV("ParamName: [%s]", param->name().c_str());
            debugV("ParamVal : [%s]", param->value().c_str());
            debugV("IsPost: [%d]", param->isPost());
            debugV("IsFile: [%d]", param->isFile());
        }
        else
        {
            debugV("No args!");
        }
        */

        const String strCurrentEffectIndex = "currentEffectIndex";
        if (pRequest->hasParam(strCurrentEffectIndex, true))
        {
            debugV("currentEffectIndex param found");
            // If found, parse it and pass it off to the EffectManager, who will validate it
            AsyncWebParameter * param = pRequest->getParam(strCurrentEffectIndex, true);
            size_t currentEffectIndex = strtoul(param->value().c_str(), NULL, 10);
            g_aptrEffectManager->SetCurrentEffectIndex(currentEffectIndex);
        }
        // Complete the response so the client knows it can happily proceed now
        AddCORSHeaderAndSendOKResponse(pRequest);
    }

    void EnableEffect(AsyncWebServerRequest * pRequest)
    {
        debugV("EnableEffect");

        // Look for the parameter by name
        const String strEffectIndex = "effectIndex";
        if (pRequest->hasParam(strEffectIndex, true))
        {
            // If found, parse it and pass it off to the EffectManager, who will validate it
            AsyncWebParameter * param = pRequest->getParam(strEffectIndex, true);
            size_t effectIndex = strtoul(param->value().c_str(), NULL, 10);
            g_aptrEffectManager->EnableEffect(effectIndex);
        }
        // Complete the response so the client knows it can happily proceed now
        AddCORSHeaderAndSendOKResponse(pRequest);
    }

    void DisableEffect(AsyncWebServerRequest * pRequest)
    {
        debugV("DisableEffect");

        // Look for the parameter by name
        const String strEffectIndex = "effectIndex";
        if (pRequest->hasParam(strEffectIndex, true))
        {
            // If found, parse it and pass it off to the EffectManager, who will validate it
            AsyncWebParameter * param = pRequest->getParam(strEffectIndex, true);
            size_t effectIndex = strtoul(param->value().c_str(), NULL, 10);
            g_aptrEffectManager->DisableEffect(effectIndex);
            debugV("Disabled Effect %d", effectIndex);
        }
        // Complete the response so the client knows it can happily proceed now
        AddCORSHeaderAndSendOKResponse(pRequest);
    }

    void NextEffect(AsyncWebServerRequest * pRequest)
    {
        debugV("NextEffect");
        g_aptrEffectManager->NextEffect();
        AddCORSHeaderAndSendOKResponse(pRequest);
    }

    void PreviousEffect(AsyncWebServerRequest * pRequest)
    {
        debugV("PreviousEffect");
        g_aptrEffectManager->PreviousEffect();
        AddCORSHeaderAndSendOKResponse(pRequest);
    }

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
        _server.on("/effectsConfig",         HTTP_GET,  [](AsyncWebServerRequest * pRequest)        { pRequest->send(SPIFFS, EFFECTS_CONFIG_FILE, "text/json"); });

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