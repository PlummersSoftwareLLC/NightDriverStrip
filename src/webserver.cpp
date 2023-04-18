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

template<typename Tv>
using PostParamSetterType = std::function<void(Tv)>;

template<typename Tv>
void PushPostParam(AsyncWebServerRequest * pRequest, const char *paramName, PostParamSetterType<Tv> setter)
{
    const String strParamName = paramName;
    if (pRequest->hasParam(strParamName, true, false))
    {
        debugV("found %s", paramName);

        // If found, parse it and pass it off to the setter
        setter(pRequest->getParam(strParamName, true, false)->value());
    }       
}

template<>
void PushPostParam<bool>(AsyncWebServerRequest * pRequest, const char *paramName, PostParamSetterType<bool> setter)
{
    const String strParamName = paramName;
    if (pRequest->hasParam(strParamName, true, false))
    {
        debugV("found %s", paramName);

        // If found, parse it and pass it off to the setter
        AsyncWebParameter * param = pRequest->getParam(strParamName, true, false);
        setter(param->value() == "true" || strtol(param->value().c_str(), NULL, 10));
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
