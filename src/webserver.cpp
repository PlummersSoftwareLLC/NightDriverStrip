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
//              Apr-28-2023         Rbergen     Reduce code duplication
//---------------------------------------------------------------------------

#include "globals.h"
#include "webserver.h"

// Member function template specialzations

template<>
void CWebServer::PushPostParamIfPresent<bool>(AsyncWebServerRequest * pRequest, const String &paramName, ValueSetter<bool> setter)
{
    PushPostParamIfPresent<bool>(pRequest, paramName, setter, [](AsyncWebParameter * param) constexpr
        {
            const String& value = param->value();
            return value == "true" || strtol(value.c_str(), NULL, 10);
        }
    );
}

template<>
void CWebServer::PushPostParamIfPresent<size_t>(AsyncWebServerRequest * pRequest, const String &paramName, ValueSetter<size_t> setter)
{
    PushPostParamIfPresent<size_t>(pRequest, paramName, setter, [](AsyncWebParameter * param) constexpr { return strtoul(param->value().c_str(), NULL, 10); });
}

template<>
void CWebServer::AddCORSHeaderAndSendResponse<AsyncJsonResponse>(AsyncWebServerRequest * pRequest, AsyncJsonResponse * pResponse)
{
    pResponse->setLength();
    AddCORSHeaderAndSendResponse<AsyncWebServerResponse>(pRequest, pResponse);
}

// Member function implementations

bool CWebServer::IsPostParamTrue(AsyncWebServerRequest * pRequest, const String &paramName)
{
    bool returnValue = false;

    PushPostParamIfPresent<bool>(pRequest, paramName, [&returnValue](auto value) { returnValue = value; });

    return returnValue;
}

void CWebServer::GetEffectListText(AsyncWebServerRequest * pRequest)
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
            AddCORSHeaderAndSendResponse(pRequest, response.release());

    } while (bufferOverflow);
}

void CWebServer::GetStatistics(AsyncWebServerRequest * pRequest)
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

    AddCORSHeaderAndSendResponse(pRequest, response);
}

void CWebServer::SetCurrentEffectIndex(AsyncWebServerRequest * pRequest)
{
    debugV("SetCurrentEffectIndex");
    PushPostParamIfPresent<size_t>(pRequest, "currentEffectIndex", SET_VALUE(g_aptrEffectManager->SetCurrentEffectIndex(value)));
    AddCORSHeaderAndSendOKResponse(pRequest);
}

void CWebServer::EnableEffect(AsyncWebServerRequest * pRequest)
{
    debugV("EnableEffect");
    PushPostParamIfPresent<size_t>(pRequest, "effectIndex", SET_VALUE(g_aptrEffectManager->EnableEffect(value)));
    AddCORSHeaderAndSendOKResponse(pRequest);
}

void CWebServer::DisableEffect(AsyncWebServerRequest * pRequest)
{
    debugV("DisableEffect");
    PushPostParamIfPresent<size_t>(pRequest, "effectIndex", SET_VALUE(g_aptrEffectManager->DisableEffect(value)));
    AddCORSHeaderAndSendOKResponse(pRequest);
}

void CWebServer::NextEffect(AsyncWebServerRequest * pRequest)
{
    debugV("NextEffect");
    g_aptrEffectManager->NextEffect();
    AddCORSHeaderAndSendOKResponse(pRequest);
}

void CWebServer::PreviousEffect(AsyncWebServerRequest * pRequest)
{
    debugV("PreviousEffect");
    g_aptrEffectManager->PreviousEffect();
    AddCORSHeaderAndSendOKResponse(pRequest);
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

    AddCORSHeaderAndSendResponse(pRequest, response);
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

    PushPostParamIfPresent<const String&>(pRequest, DeviceConfig::LocationTag, SET_VALUE(g_aptrDeviceConfig->SetLocation(value)));
    PushPostParamIfPresent<bool>(pRequest, DeviceConfig::LocationIsZipTag, SET_VALUE(g_aptrDeviceConfig->SetLocationIsZip(value)));
    PushPostParamIfPresent<const String&>(pRequest, DeviceConfig::CountryCodeTag, SET_VALUE(g_aptrDeviceConfig->SetCountryCode(value)));
    PushPostParamIfPresent<const String&>(pRequest, DeviceConfig::OpenWeatherApiKeyTag, SET_VALUE(g_aptrDeviceConfig->SetOpenWeatherAPIKey(value)));
    PushPostParamIfPresent<const String&>(pRequest, DeviceConfig::TimeZoneTag, SET_VALUE(g_aptrDeviceConfig->SetTimeZone(value)));
    PushPostParamIfPresent<bool>(pRequest, DeviceConfig::Use24HourClockTag, SET_VALUE(g_aptrDeviceConfig->Set24HourClock(value)));
    PushPostParamIfPresent<bool>(pRequest, DeviceConfig::UseCelsiusTag, SET_VALUE(g_aptrDeviceConfig->SetUseCelsius(value)));

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