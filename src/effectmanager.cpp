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
static DRAM_ATTR size_t l_EffectsManagerJSONWriterIndex = SIZE_MAX;
static DRAM_ATTR size_t l_CurrentEffectWriterIndex = SIZE_MAX;
static DRAM_ATTR bool l_EffectManagerInitializing = false;

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

#ifndef NO_EFFECT_PERSISTENCE
    #define NO_EFFECT_PERSISTENCE 0
#endif

// Load the effects JSON file and check if it's appropriate to use
std::optional<JsonObjectConst> LoadEffectsJSONFile(JsonDocument& jsonDoc)
{
    // If ordered to do so, we ignore whatever is persisted
    if (NO_EFFECT_PERSISTENCE || !LoadJSONFile(EFFECTS_CONFIG_FILE, jsonDoc))
        return {};

    auto jsonObject = jsonDoc.as<JsonObjectConst>();

    // Ignore JSON if it was persisted for a different project
    if (jsonObject[PTY_PROJECT].is<String>()
        && jsonObject[PTY_PROJECT].as<String>() != PROJECT_NAME)
    {
        return {};
    }

    auto jsonVersion = jsonObject[PTY_EFFECTSETVER];

    // Only return the JSON object if the persistent version matches the current one
    if (jsonVersion.is<String>()
        && g_ptrEffectFactories->HashString() == jsonVersion.as<String>())
    {
        return jsonObject;
    }

    return {};
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

// Saves the current effect index to its own file. Note that this function only flags the writer
//   for execution in the background.
void EffectManager::SaveCurrentEffectIndex()
{
    if (l_EffectManagerInitializing)
        return;

    if (g_ptrSystem->GetDeviceConfig().RememberCurrentEffect())
        // Default value for writer index is max value for size_t, so nothing will happen if writer has not yet been registered
        g_ptrSystem->GetJSONWriter().FlagWriter(l_CurrentEffectWriterIndex);
}

//
// Helper functions related to JSON persistence
//

void SaveEffectManagerConfig()
{
    if (l_EffectManagerInitializing)
        return;

    debugV("Saving effect manager config...");
    // Default value for writer index is max value for size_t, so nothing will happen if writer has not yet been registered
    g_ptrSystem->GetJSONWriter().FlagWriter(l_EffectsManagerJSONWriterIndex);
}

void RemoveEffectManagerConfig()
{
    RemoveJSONFile(EFFECTS_CONFIG_FILE);
    // We take the liberty of also removing the file with the current effect config index
    std::lock_guard renderPause(g_render_mutex);
    #if USE_HUB75
        HUB75GFX::WaitForMatrixSwap();
    #endif
    SPIFFS.remove(CURRENT_EFFECT_CONFIG_FILE);
}

void WriteCurrentEffectIndexFile()
{
    SaveCurrentEffectIndex();

    {
        std::lock_guard listenerGuard(_listenerMutex);
        INFORM_EVENT_LISTENERS(_effectEventListeners, IEffectEventListener::OnCurrentEffectChanged, _iCurrentEffect);
    }
}

// EffectManager::Update
//
// Draws the current effect.  If gUIDirty has been set by an interrupt handler, it is reset here

void EffectManager::Update()
{
    std::scoped_lock guard(g_render_mutex, g_effect_manager_mutex);

    if ((_gfx[0])->GetLEDCount() == 0)
        return;

    if (!_tempEffect && _vEffects.empty())
    {
        ApplyFadeLogic();
        return;
    }

    CheckEffectTimerExpired();
    DispatchBeatIfNeeded();

    if (_tempEffect)
        _tempEffect->Draw();
    else
        _vEffects[_iCurrentEffect]->Draw();

    ApplyFadeLogic();
}

void EffectManager::ApplyFadeLogic()
{
    if (EffectCount() < 2)
    {
        g_Values.Fader = 255;
        return;
    }

    if (IsIntervalEternal())
    {
        g_Values.Fader = 255;
        return;
    }

    const int msFadeTime = 2000;
    int r = GetTimeRemainingForCurrentEffect();
    int e = GetTimeUsedByCurrentEffect();

    if (e < msFadeTime)
    {
        g_Values.Fader = 255.0f * ((float)e / msFadeTime); // Fade in
    }
    else if (r < msFadeTime)
    {
        g_Values.Fader = 255.0f * ((float)r / msFadeTime); // Fade out
    }
    else
    {
        g_Values.Fader = 255; // No fade, not at start or end
    }
}
