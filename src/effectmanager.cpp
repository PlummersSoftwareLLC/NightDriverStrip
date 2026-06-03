//+--------------------------------------------------------------------------
//
// File:        effectmanager.cpp
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
//    Various functions related to EffectManager and its initialization
//
// History:     Sep-26-2023         Rbergen     Extracted from effects.cpp
//---------------------------------------------------------------------------

#include "globals.h"

#include <algorithm>
#include <FS.h>
#include <limits>
#include <set>
#include <SPIFFS.h>

#include "deviceconfig.h"
#include "effectfactories.h"
#include "effectmanager.h"
#include "gfxbase.h"
#include "jsonserializer.h"
#include "ledstripeffect.h"
#include "systemcontainer.h"
#include "websocketserver.h"

#include "effects/strip/misceffects.h"
#include "effects/strip/musiceffect.h"
#if USE_HUB75
#include "hub75gfx.h"
#endif

// Variables we need further down

extern allocated_unique_ptr<EffectFactories> g_ptrEffectFactories;
extern std::map<int, JSONEffectFactory> g_JsonStarryNightEffectFactories;
DRAM_ATTR size_t g_EffectsManagerJSONBufferSize = 0;
extern DRAM_ATTR size_t l_EffectsManagerJSONWriterIndex;
extern DRAM_ATTR size_t l_CurrentEffectWriterIndex;
extern DRAM_ATTR bool l_EffectManagerInitializing;

//
// EffectManager initialization functions
//

#if USE_HUB75

    void InitSplashEffectManager()
    {
        debugW("InitSplashEffectManager");

        g_ptrSystem->SetupEffectManager(make_shared_psram<SplashLogoEffect>(), g_ptrSystem->GetDevices());
    }

#endif

// Declare these here just so InitEffectsManager can refer to them. They're defined elsewhere or further down.

void LoadEffectFactories();
std::optional<JsonObjectConst> LoadEffectsJSONFile(JsonDocument& jsonDoc);
void WriteCurrentEffectIndexFile();

// InitEffectsManager
//
// Initializes the effect manager.  Reboots on failure, since it's not optional
void InitEffectsManager()
{
    debugW("InitEffectsManager...");
    l_EffectManagerInitializing = true;

    LoadEffectFactories();

    l_EffectsManagerJSONWriterIndex = g_ptrSystem->GetJSONWriter().RegisterWriter([]()
    {
        if (!SaveToJSONFile(EFFECTS_CONFIG_FILE, g_ptrSystem->GetEffectManager()) && EFFECT_PERSISTENCE_CRITICAL)
            throw std::runtime_error("Effects serialization failed");
    });
    l_CurrentEffectWriterIndex = g_ptrSystem->GetJSONWriter().RegisterWriter(WriteCurrentEffectIndexFile);

    auto jsonDoc = CreateJsonDocument();
    auto jsonObject = LoadEffectsJSONFile(jsonDoc);
    const bool loadedPersistedEffects = jsonObject.has_value();

    if (jsonObject)
    {
        debugI("Creating EffectManager from JSON config");

        if (g_ptrSystem->HasEffectManager())
            g_ptrSystem->GetEffectManager().DeserializeFromJSON(jsonObject.value());
        else
            g_ptrSystem->SetupEffectManager(jsonObject.value(), g_ptrSystem->GetDevices());
    }
    else
    {
        debugI("Creating EffectManager using default effects");

        if (g_ptrSystem->HasEffectManager())
            g_ptrSystem->GetEffectManager().LoadDefaultEffects();
        else
            g_ptrSystem->SetupEffectManager(g_ptrSystem->GetDevices());
    }

    if (false == g_ptrSystem->GetEffectManager().Init())
        throw std::runtime_error("Could not initialize effect manager");

    // We won't need the default factories anymore, so swipe them from memory
    g_ptrEffectFactories->ClearDefaultFactories();

    #if EFFECTS_WEB_SOCKET_ENABLED
        g_ptrSystem->GetEffectManager().AddEffectEventListener(g_ptrSystem->GetWebSocketServer());
    #endif

    l_EffectManagerInitializing = false;

    if (!loadedPersistedEffects)
    {
        // Persist the default effect set after initialization suppression is
        // lifted. Otherwise first boot, missing config, or an effect-set hash
        // change would run from defaults but never write the new config until
        // some later user mutation happened.
        SaveEffectManagerConfig();
    }
}

//
// EffectManager member function definitions
//

EffectManager::EffectManager(const std::shared_ptr<LEDStripEffect>& effect, std::vector<std::shared_ptr<GFXBase>>& gfx)
    : _gfx(gfx)
{
    debugV("EffectManager Splash Effect Constructor");

    if (effect->Init(_gfx))
        _tempEffect = effect;

    construct(false);
}

EffectManager::EffectManager(std::vector<std::shared_ptr<GFXBase>>& gfx)
    : _gfx(gfx)
{
    debugV("EffectManager Constructor");

    LoadDefaultEffects();
}

EffectManager::EffectManager(const JsonObjectConst& jsonObject, std::vector<std::shared_ptr<GFXBase>>& gfx)
    : _gfx(gfx)
{
    debugV("EffectManager JSON Constructor");

    DeserializeFromJSON(jsonObject);
}

EffectManager::~EffectManager()
{
    ClearRemoteColor();
    ClearEffects();
}
