//+--------------------------------------------------------------------------
//
// File:        webserver_unified.cpp
//
// NightDriverStrip - (c) 2018 Plummer's Software LLC.  All Rights Reserved.
//
// This file is part of webserver.cpp; see that file header for additional context.
//
// Split scope: unified settings API handlers for webserver routes.
//---------------------------------------------------------------------------


#include "globals.h"

#if ENABLE_WEBSERVER

#include "webserver.h"

#include <AsyncJson.h>

#include "deviceconfig.h"
#include "effectmanager.h"
#include "systemcontainer.h"

void CWebServer::GetUnifiedSettings(AsyncWebServerRequest * pRequest)
{
    auto response = new AsyncJsonResponse();
    auto root = response->getRoot().to<JsonObject>();
    auto& deviceConfig = g_ptrSystem->GetDeviceConfig();
    deviceConfig.SerializeUnifiedSettings(root);
    root["effects"]["effectInterval"] = g_ptrSystem->GetEffectManager().GetInterval();
    AddCORSHeaderAndSendResponse(pRequest, response);
}

void CWebServer::GetUnifiedSettingsSchema(AsyncWebServerRequest * pRequest)
{
    auto response = new AsyncJsonResponse();
    auto root = response->getRoot().to<JsonObject>();
    g_ptrSystem->GetDeviceConfig().SerializeUnifiedSettingsSchema(root);
    AddCORSHeaderAndSendResponse(pRequest, response);
}

void CWebServer::SetUnifiedSettings(AsyncWebServerRequest * pRequest, JsonVariantConst json)
{
    if (!json.is<JsonObjectConst>())
    {
        AddCORSHeaderAndSendBadRequest(pRequest, "Malformed request");
        return;
    }

    auto root = json.as<JsonObjectConst>();
    auto& deviceConfig = g_ptrSystem->GetDeviceConfig();
    DeviceConfig::UnifiedSettingsRequest unifiedRequest;

    auto [isValid, validationMessage] = deviceConfig.ParseAndValidateUnifiedSettings(root, unifiedRequest);
    if (!isValid)
    {
        AddCORSHeaderAndSendBadRequest(pRequest, validationMessage);
        return;
    }

    if (unifiedRequest.runtimeConfigTouched)
    {
        auto [runtimeConfigApplied, runtimeErrorMessage] = g_ptrSystem->ApplyRuntimeConfigurationTransaction(unifiedRequest.requestedRuntimeConfig);
        if (!runtimeConfigApplied)
        {
            AddCORSHeaderAndSendBadRequest(pRequest, runtimeErrorMessage);
            return;
        }
    }

    String applyErrorMessage;
    if (!deviceConfig.ApplyUnifiedDeviceSettings(unifiedRequest, &applyErrorMessage))
    {
        AddCORSHeaderAndSendBadRequest(pRequest, applyErrorMessage);
        return;
    }

    auto& effectManager = g_ptrSystem->GetEffectManager();

    if (root["effects"].is<JsonObjectConst>())
    {
        auto effects = root["effects"].as<JsonObjectConst>();
        if (effects["effectInterval"].is<size_t>())
            effectManager.SetInterval(effects["effectInterval"].as<size_t>());
    }

    GetUnifiedSettings(pRequest);
}

#endif  // ENABLE_WEBSERVER
