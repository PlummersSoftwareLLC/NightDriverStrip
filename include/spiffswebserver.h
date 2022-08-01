//+--------------------------------------------------------------------------
//
// File:        SPIFFSWebServer.h
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
//   Web server that fulfills requests by serving them from the SPIFFS
//   storage in flash.  Requires the espressif-esp32 WebServer class.
//
//   This class contains an early attempt at exposing a REST api for
//   adjusting effect paramters.  I'm in no way attached to it and it
//   should likely be redone!
//
//   Server also exposes basic RESTful API for querying variables etc.
//
// History:     Jul-12-2018         Davepl      Created
//              Apr-29-2019         Davepl      Adapted from BigBlueLCD project
//
//---------------------------------------------------------------------------

#pragma once

#include <WiFi.h>
#include <FS.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <SPIFFS.h>
#include <effectmanager.h>
#include <Arduino.h>
#include <vector>

#include <AsyncJson.h>
#include <ArduinoJson.h>


extern std::unique_ptr<EffectManager<GFXBase>>g_pEffectManager;
#define JSON_BUFFER_BASE_SIZE 2048
#define JSON_BUFFER_INCREMENT 2048


class CSPIFFSWebServer
{
  private:

    AsyncWebServer _server;

  public:

    CSPIFFSWebServer()
        : _server(80)
    {
    }

    // AddCORSHeaderAndSendOKResponse
    //
    // Sends an empty OK/200 response; normally used to finish up things that don't return anything, like "NextEffect"

    void AddCORSHeaderAndSendOKResponse(AsyncWebServerRequest * pRequest, const char * pszText = nullptr)
    {
        AsyncWebServerResponse * pResponse = (pszText == nullptr) ? 
                                                    pRequest->beginResponse(200) :
                                                    pRequest->beginResponse(200, "text/json",  pszText);    
        pResponse->addHeader("Access-Control-Allow-Origin", "*");
        pRequest->send(pResponse);      
    }

    void GetEffectListText(AsyncWebServerRequest * pRequest)
    {
        static size_t jsonBufferSize = JSON_BUFFER_BASE_SIZE;
        bool bufferOverflow;
        AsyncJsonResponse * response;

        debugI("GetEffectListText");

        do {
            bufferOverflow = false;
            response = new AsyncJsonResponse(false, jsonBufferSize);
            response->addHeader("Server","NightDriverStrip");
            auto j = response->getRoot();

            j["currentEffect"]         = g_pEffectManager->GetCurrentEffectIndex();
            j["millisecondsRemaining"] = g_pEffectManager->GetTimeRemainingForCurrentEffect();
            j["effectInterval"]        = g_pEffectManager->GetInterval();
            j["enabledCount"]          = g_pEffectManager->EnabledCount();

            for (int i = 0; i < g_pEffectManager->EffectCount(); i++) { 
                DynamicJsonDocument effectDoc(256);
                effectDoc["name"]    = g_pEffectManager->EffectsList()[i]->FriendlyName();
                effectDoc["enabled"] = g_pEffectManager->IsEffectEnabled(i);

                if (!j["Effects"].add(effectDoc)) {
                    bufferOverflow = true;
                    delete response;
                    jsonBufferSize += JSON_BUFFER_INCREMENT;
                    debugV("JSON reponse buffer overflow! Increased buffer to %zu bytes", jsonBufferSize);
                    break;
                }
            }       
        } while (bufferOverflow);

        response->addHeader("Access-Control-Allow-Origin", "*");
        response->setLength();
        pRequest->send(response);
    }

    void SetSettings(AsyncWebServerRequest * pRequest)
    {
        debugI("SetSettings");

        // Look for the parameter by name
        const char * pszEffectInterval = "effectInterval";
        if (pRequest->hasParam(pszEffectInterval, true, false))
        {
            debugI("found EffectInterval");
            // If found, parse it and pass it off to the EffectManager, who will validate it
            AsyncWebParameter * param = pRequest->getParam(pszEffectInterval, true, false);
            size_t effectInterval = strtoul(param->value().c_str(), NULL, 10);  
            g_pEffectManager->SetInterval(effectInterval);
        }       
        // Complete the response so the client knows it can happily proceed now
        AddCORSHeaderAndSendOKResponse(pRequest);   
    }

    void SetCurrentEffectIndex(AsyncWebServerRequest * pRequest)
    {
        debugI("SetCurrentEffectIndex");

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

        const char * pszCurrentEffectIndex = "currentEffectIndex";
        if (pRequest->hasParam(pszCurrentEffectIndex, true))
        {
            debugV("currentEffectIndex param found");
            // If found, parse it and pass it off to the EffectManager, who will validate it
            AsyncWebParameter * param = pRequest->getParam(pszCurrentEffectIndex, true);
            size_t currentEffectIndex = strtoul(param->value().c_str(), NULL, 10);  
            g_pEffectManager->SetCurrentEffectIndex(currentEffectIndex);
        }
        // Complete the response so the client knows it can happily proceed now
        AddCORSHeaderAndSendOKResponse(pRequest);   
    }

    void EnableEffect(AsyncWebServerRequest * pRequest)
    {
        debugI("EnableEffect");

        // Look for the parameter by name
        const char * pszEffectIndex = "effectIndex";
        if (pRequest->hasParam(pszEffectIndex, true))
        {
            // If found, parse it and pass it off to the EffectManager, who will validate it
            AsyncWebParameter * param = pRequest->getParam(pszEffectIndex, true);
            size_t effectIndex = strtoul(param->value().c_str(), NULL, 10); 
            g_pEffectManager->EnableEffect(effectIndex);
        }
        // Complete the response so the client knows it can happily proceed now
        AddCORSHeaderAndSendOKResponse(pRequest);   
    }

    void DisableEffect(AsyncWebServerRequest * pRequest)
    {
        debugI("DisableEffect");

        // Look for the parameter by name
        const char * pszEffectIndex = "effectIndex";
        if (pRequest->hasParam(pszEffectIndex, true))
        {
            // If found, parse it and pass it off to the EffectManager, who will validate it
            AsyncWebParameter * param = pRequest->getParam(pszEffectIndex, true);
            size_t effectIndex = strtoul(param->value().c_str(), NULL, 10); 
            g_pEffectManager->DisableEffect(effectIndex);
            debugV("Disabled Effect %d", effectIndex);
        }
        // Complete the response so the client knows it can happily proceed now
        AddCORSHeaderAndSendOKResponse(pRequest);   
    }

    void NextEffect(AsyncWebServerRequest * pRequest)
    {
        debugI("NextEffect");
        g_pEffectManager->NextEffect();
        AddCORSHeaderAndSendOKResponse(pRequest);
    }

    void PreviousEffect(AsyncWebServerRequest * pRequest)
    {
        debugI("PreviousEffect");
        g_pEffectManager->PreviousEffect();
        AddCORSHeaderAndSendOKResponse(pRequest);
    }

    // begin - register page load handlers and start serving pages

    // The default method of fulfilling a file doesn't work on large files because it tries to hold
    // the entire thing in RAM, and it chokes.  So, for files that are too large to serve from RAM, 
    // I use this function.  It registers a file-specific handler and then does chunk based IO.
    
    void ServeLargeStaticFile(const char pszName[], const char pszType[])
    {
       _server.on(pszName, HTTP_GET, [pszName, pszType](AsyncWebServerRequest *request)
        {
            Serial.printf("GET for: %s\n", pszName);
            File SPIFFSfile = SPIFFS.open(pszName, FILE_READ);
            if (SPIFFSfile)
            {
                Serial.printf("[HTTP]\tOpening [%d]\r\n", SPIFFSfile);
            } 
            else 
            {
                Serial.printf("[HTTP]\tSPIFFS File DOESN'T exists [%d] <<< ERROR !!!\r\n", SPIFFSfile);
            }
            AsyncWebServerResponse *response = 
                request->beginChunkedResponse(pszType, [SPIFFSfile](uint8_t *buffer, size_t maxLen, size_t index) -> size_t 
                {
                    auto localHandle = SPIFFSfile;
                    Serial.printf("[HTTP]  [%6d]    INDEX [%6d]    BUFFER_MAX_LENGTH [%6d]\r\n", index, localHandle.size(), maxLen);
                    size_t len = localHandle.read(buffer, maxLen);
                    Serial.printf(">> Succcessful read of %d\n", len);
                    if (len == 0)
                    {
                        Serial.printf("Closing [%d]\n", SPIFFSfile);
                        localHandle.close();
                    }
                    return len;
                }
            );
            request->send(response);
        });
        
    }    

    void begin()
    {
        debugI("Connecting Web Endpoints");
        
        _server.on("/getEffectList",         HTTP_GET, [this](AsyncWebServerRequest * pRequest) { this->GetEffectListText(pRequest); });
        
        _server.on("/nextEffect",            HTTP_POST, [this](AsyncWebServerRequest * pRequest)    { this->NextEffect(pRequest); });
        _server.on("/previousEffect",        HTTP_POST, [this](AsyncWebServerRequest * pRequest)    { this->PreviousEffect(pRequest); });

        _server.on("/setCurrentEffectIndex", HTTP_POST, [this](AsyncWebServerRequest * pRequest)    { this->SetCurrentEffectIndex(pRequest); });
        _server.on("/enableEffect",          HTTP_POST, [this](AsyncWebServerRequest * pRequest)    { this->EnableEffect(pRequest); });
        _server.on("/disableEffect",         HTTP_POST, [this](AsyncWebServerRequest * pRequest)    { this->DisableEffect(pRequest); });

        _server.on("/settings",              HTTP_POST, [this](AsyncWebServerRequest * pRequest)    { this->SetSettings(pRequest); });

        // Extra-large files must be manually served like this per Issue #770 in AsyncWebServer lib

        ServeLargeStaticFile("/static/js/main.js", "text/javascript");
        //ServeLargeStaticFile("/static/js/main.js.LICENSE.txt", "text/plain");
        //ServeLargeStaticFile("/static/css/main.b4f33716.css", "text/css");

        _server.serveStatic("/", SPIFFS, "/", "public, max-age=2").setDefaultFile("index.html");        // BUGBUG Fix lifetime to 86400 or something like that

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