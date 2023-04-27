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
//---------------------------------------------------------------------------

#include "globals.h"
#include "webserver.h"

namespace 
{
    bool IsParamTrue(AsyncWebParameter *param)
    {
        return param->value() == "true" || strtol(param->value().c_str(), NULL, 10);
    }

    bool IsPostParamTrue(AsyncWebServerRequest * pRequest, const String &paramName)
    {
        if (!pRequest->hasParam(paramName, true, false))
            return false;
        
        debugV("found %s", paramName.c_str());

        // If found, parse it and pass it off to the setter
        return IsParamTrue(pRequest->getParam(paramName, true, false));
    }

    template<typename Tv>
    using PostParamSetter = std::function<void(Tv)>;

    template<typename Tv>
    void PushPostParam(AsyncWebServerRequest * pRequest, const String &paramName, PostParamSetter<Tv> setter)
    {
        if (pRequest->hasParam(paramName, true, false))
        {
            debugV("found %s", paramName.c_str());

            // If found, parse it and pass it off to the setter
            setter(pRequest->getParam(paramName, true, false)->value());
        }       
    }

    template<>
    void PushPostParam<bool>(AsyncWebServerRequest * pRequest, const String &paramName, PostParamSetter<bool> setter)
    {
        if (pRequest->hasParam(paramName, true, false))
        {
            debugV("found %s", paramName.c_str());

            // If found, parse it and pass it off to the setter
            setter(IsParamTrue(pRequest->getParam(paramName, true, false)));
        }       
    }
}

void CWebServer::GetSettings(AsyncWebServerRequest * pRequest)
{
    debugV("GetSettings");

    auto response = new AsyncJsonResponse(false, JSON_BUFFER_BASE_SIZE);
    response->addHeader("Server","NightDriverStrip");
    auto root = response->getRoot();
    JsonObject jsonObject = root.to<JsonObject>();
    
    g_aptrDeviceConfig->SerializeToJSON(jsonObject);
    jsonObject["effectInterval"] = g_aptrEffectManager->GetInterval();

    response->setLength();
    response->addHeader("Access-Control-Allow-Origin", "*");
    pRequest->send(response);
}

void CWebServer::SetSettings(AsyncWebServerRequest * pRequest)
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

    PushPostParam<const String&>(pRequest, "location", [](const String& value) { g_aptrDeviceConfig->SetLocation(value); });
    PushPostParam<bool>(pRequest, "locationIsZip", [](bool value) { g_aptrDeviceConfig->SetLocationIsZip(value); });
    PushPostParam<const String&>(pRequest, "countryCode", [](const String& value) { g_aptrDeviceConfig->SetCountryCode(value); });
    PushPostParam<const String&>(pRequest, "openWeatherApiKey", [](const String& value) { g_aptrDeviceConfig->SetOpenWeatherAPIKey(value); });
    PushPostParam<const String&>(pRequest, "timeZone", [](const String& value) { g_aptrDeviceConfig->SetTimeZone(value); });
    PushPostParam<bool>(pRequest, "use24HourClock", [](bool value) { g_aptrDeviceConfig->Set24HourClock(value); });
    PushPostParam<bool>(pRequest, "useCelsius", [](bool value) { g_aptrDeviceConfig->SetUseCelsius(value); });

    // We return the current config in response
    GetSettings(pRequest);
}

void CWebServer::Reset(AsyncWebServerRequest * pRequest)
{
    if (IsPostParamTrue(pRequest, "deviceConfig"))
    {
        debugI("Removing DeviceConfig");
        g_aptrDeviceConfig->RemovePersisted();
    }

    if (IsPostParamTrue(pRequest, "effectsConfig"))
    {
        debugI("Removing EffectManager config");
        RemoveEffectManagerConfig();
    }

    bool boardResetRequested = IsPostParamTrue(pRequest, "board");
    
    AddCORSHeaderAndSendOKResponse(pRequest);
    
    if (boardResetRequested) 
    {
        delay(1000);    // Give the response a second to be sent
        throw new std::runtime_error("Resetting device at API request");
    }
}