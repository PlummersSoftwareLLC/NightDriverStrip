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

#define JSON_BUFFER_BASE_SIZE 2048
#define JSON_BUFFER_INCREMENT 2048

class CWebServer
{
  private:

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

        do {
            bufferOverflow = false;
            auto response = new AsyncJsonResponse(false, jsonBufferSize);
            response->addHeader("Server","NightDriverStrip");
            auto j = response->getRoot();

            j["currentEffect"]         = g_aptrEffectManager->GetCurrentEffectIndex();
            j["millisecondsRemaining"] = g_aptrEffectManager->GetTimeRemainingForCurrentEffect();
            j["effectInterval"]        = g_aptrEffectManager->GetInterval();
            j["enabledCount"]          = g_aptrEffectManager->EnabledCount();

            for (int i = 0; i < g_aptrEffectManager->EffectCount(); i++) { 
                DynamicJsonDocument effectDoc(256);
                effectDoc["name"]    = g_aptrEffectManager->EffectsList()[i]->FriendlyName();
                effectDoc["enabled"] = g_aptrEffectManager->IsEffectEnabled(i);

                if (!j["Effects"].add(effectDoc)) {
                    bufferOverflow = true;
                    jsonBufferSize += JSON_BUFFER_INCREMENT;
                    delete response;
                    debugV("JSON reponse buffer overflow! Increased buffer to %zu bytes", jsonBufferSize);
                    break;
                }
            }       

            response->addHeader("Access-Control-Allow-Origin", "*");
            response->setLength();
            pRequest->send(response);
        } while (bufferOverflow);
    }

    void GetStatistics(AsyncWebServerRequest * pRequest)
    {
        static size_t jsonBufferSize = JSON_BUFFER_BASE_SIZE;

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

    void SetSettings(AsyncWebServerRequest * pRequest)
    {
        debugV("SetSettings");

        // Look for the parameter by name
        const String strEffectInterval = "effectInterval";
        if (pRequest->hasParam(strEffectInterval, true, false))
        {
            debugV("found EffectInterval");
            // If found, parse it and pass it off to the EffectManager, who will validate it
            AsyncWebParameter * param = pRequest->getParam(strEffectInterval, true, false);
            size_t effectInterval = strtoul(param->value().c_str(), NULL, 10);  
            g_aptrEffectManager->SetInterval(effectInterval);
        }       
        // Complete the response so the client knows it can happily proceed now
        AddCORSHeaderAndSendOKResponse(pRequest);   
    }

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

    void begin()
    {
        debugI("Connecting Web Endpoints");
        extern const char html_start[] asm("_binary_data_index_html_start");
        extern const char jsx_start[] asm("_binary_data_main_jsx_start");
        
        _server.on("/getEffectList",         HTTP_GET, [this](AsyncWebServerRequest * pRequest) { this->GetEffectListText(pRequest); });
        _server.on("/getStatistics",         HTTP_GET, [this](AsyncWebServerRequest * pRequest) { this->GetStatistics(pRequest); });
        _server.on("/nextEffect",            HTTP_POST, [this](AsyncWebServerRequest * pRequest)    { this->NextEffect(pRequest); });
        _server.on("/previousEffect",        HTTP_POST, [this](AsyncWebServerRequest * pRequest)    { this->PreviousEffect(pRequest); });

        _server.on("/setCurrentEffectIndex", HTTP_POST, [this](AsyncWebServerRequest * pRequest)    { this->SetCurrentEffectIndex(pRequest); });
        _server.on("/enableEffect",          HTTP_POST, [this](AsyncWebServerRequest * pRequest)    { this->EnableEffect(pRequest); });
        _server.on("/disableEffect",         HTTP_POST, [this](AsyncWebServerRequest * pRequest)    { this->DisableEffect(pRequest); });

        _server.on("/settings",              HTTP_POST, [this](AsyncWebServerRequest * pRequest)    { this->SetSettings(pRequest); });

        _server.on("/", HTTP_GET, [this](AsyncWebServerRequest * pRequest) { pRequest->send(200, "text/html", html_start);});
        _server.on("/main.jsx", HTTP_GET, [this](AsyncWebServerRequest * pRequest) { pRequest->send(200, "application/javascript", jsx_start); });

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