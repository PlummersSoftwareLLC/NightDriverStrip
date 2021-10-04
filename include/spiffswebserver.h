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
//   Server also exposes basic RESTful API for querying variablesl etc.
//
// History:     Jul-12-2018         Davepl      Created
//              Apr-29-2019 		Davepl		Adapted from BigBlueLCD project
//
//---------------------------------------------------------------------------

#pragma once

#include <WiFi.h>
#include <FS.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <SPIFFS.h>
#include <effects/effectmanager.h>
#include <Arduino.h>
#include <vector>

#include <AsyncJson.h>
#include <ArduinoJson.h>


extern unique_ptr<EffectManager> g_pEffectManager;


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
		debugI("GetEffectListText");

		AsyncJsonResponse * response = new AsyncJsonResponse();
		response->addHeader("Server","NightDriverStrip");
		auto j = response->getRoot();

		j["currentEffect"] 		   = g_pEffectManager->GetCurrentEffectIndex();
		j["millisecondsRemaining"] = g_pEffectManager->GetTimeRemainingForCurrentEffect();
		j["effectInterval"] 	   = g_pEffectManager->GetInterval();
		
		for (int i = 0; i < g_pEffectManager->EffectCount(); i++) {
			j["Effects"][i]["name"]    = g_pEffectManager->EffectsList()[i]->FriendlyName();
			j["Effects"][i]["enabled"] = g_pEffectManager->IsEffectEnabled(i);
		}
			
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

	void begin()
	{
		debugI("Connecting Web Endpoints");
		
		_server.on("/getEffectList",  		 HTTP_GET, [this](AsyncWebServerRequest * pRequest)	{ this->GetEffectListText(pRequest); });
		
		_server.on("/nextEffect", 	  		 HTTP_POST, [this](AsyncWebServerRequest * pRequest)	{ this->NextEffect(pRequest); });
		_server.on("/previousEffect", 		 HTTP_POST, [this](AsyncWebServerRequest * pRequest)	{ this->PreviousEffect(pRequest); });

		_server.on("/setCurrentEffectIndex", HTTP_POST, [this](AsyncWebServerRequest * pRequest)	{ this->SetCurrentEffectIndex(pRequest); });
		_server.on("/enableEffect",   		 HTTP_POST, [this](AsyncWebServerRequest * pRequest)	{ this->EnableEffect(pRequest); });
		_server.on("/disableEffect", 		 HTTP_POST, [this](AsyncWebServerRequest * pRequest)	{ this->DisableEffect(pRequest); });

		_server.on("/settings", 		 	 HTTP_POST, [this](AsyncWebServerRequest * pRequest)	{ this->SetSettings(pRequest); });

		_server.serveStatic("/", SPIFFS, "/", "public, max-age=86400").setDefaultFile("index.html");

		_server.onNotFound([](AsyncWebServerRequest *request) 
		{
			if (request->method() == HTTP_OPTIONS) {
				request->send(200);										// Apparently needed for CORS: https://github.com/me-no-dev/ESPAsyncWebServer
			} else {
				debugI("Failed GET for %s\n", request->url().c_str() );
				request->send(404);
			}
		});

		_server.begin();

		debugI("HTTP server started");
	}
};