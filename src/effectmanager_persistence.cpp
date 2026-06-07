//+--------------------------------------------------------------------------
//
// File:        effectmanager_persistence.cpp
//
// This file is part of effectmanager.cpp; see that file header for additional context.
//
// Split scope: EffectManager JSON persistence, deserialization, and effect copy/load helpers.
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

extern allocated_unique_ptr<EffectFactories> g_ptrEffectFactories;
DRAM_ATTR size_t l_EffectsManagerJSONWriterIndex = SIZE_MAX;
DRAM_ATTR size_t l_CurrentEffectWriterIndex = SIZE_MAX;
DRAM_ATTR bool l_EffectManagerInitializing = false;

#ifndef NO_EFFECT_PERSISTENCE
    #define NO_EFFECT_PERSISTENCE 0
#endif

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

bool EffectManager::ReadCurrentEffectIndex(size_t& index)
{
    File file = SPIFFS.open(CURRENT_EFFECT_CONFIG_FILE);
    bool readIndex = false;

    if (file)
    {
        if (file.size() > 0)
        {
            debugI("Attempting to read file %s", CURRENT_EFFECT_CONFIG_FILE);

            auto valueString = file.readString();

            if (!valueString.isEmpty())
            {
                index = strtoul(valueString.c_str(), nullptr, 10);
                readIndex = true;
            }
        }

        file.close();
    }

    return readIndex;
}

void EffectManager::SaveCurrentEffectIndex()
{
    if (l_EffectManagerInitializing)
        return;

    if (g_ptrSystem->GetDeviceConfig().RememberCurrentEffect())
        g_ptrSystem->GetJSONWriter().FlagWriter(l_CurrentEffectWriterIndex);
}

void SaveEffectManagerConfig()
{
    if (l_EffectManagerInitializing)
        return;

    debugV("Saving effect manager config...");
    g_ptrSystem->GetJSONWriter().FlagWriter(l_EffectsManagerJSONWriterIndex);
}

void RemoveEffectManagerConfig()
{
    WaitForRenderSwapBeforeFilesystemWrite();
    std::lock_guard filesystemGuard(JSONFilesystemWriteMutex());
    SPIFFS.remove(EFFECTS_CONFIG_FILE);
    SPIFFS.remove(CURRENT_EFFECT_CONFIG_FILE);
}

void WriteCurrentEffectIndexFile()
{
    // Capture the current effect index without holding g_render_mutex to avoid nested lock order issues.
    size_t currentIndex = g_ptrSystem->GetEffectManager().GetCurrentEffectIndex();

    std::lock_guard filesystemGuard(JSONFilesystemWriteMutex());
    WaitForRenderSwapBeforeFilesystemWrite();

    SPIFFS.remove(CURRENT_EFFECT_CONFIG_FILE);

    File file = SPIFFS.open(CURRENT_EFFECT_CONFIG_FILE, FILE_WRITE);

    if (!file)
    {
        debugE("Unable to open file %s for writing!", CURRENT_EFFECT_CONFIG_FILE);
        return;
    }

    auto bytesWritten = file.print(currentIndex);

    file.flush();
    file.close();

    debugI("Number of bytes written to file %s: %zu", CURRENT_EFFECT_CONFIG_FILE, (size_t)bytesWritten);

    if (bytesWritten == 0)
    {
        debugE("Unable to write to file %s!", CURRENT_EFFECT_CONFIG_FILE);
        SPIFFS.remove(CURRENT_EFFECT_CONFIG_FILE);
    }
}

void EffectManager::LoadJSONEffects(const JsonArrayConst& effectsArray)
{
    std::set<int> loadedEffectNumbers;

    // Create effects from JSON objects, using the respective factories in g_EffectFactories
    auto& jsonFactories = g_ptrEffectFactories->GetJSONFactories();

    for (auto effectObject : effectsArray)
    {
        int effectNumber = effectObject[PTY_EFFECTNR];
        auto factoryEntry = jsonFactories.find(effectNumber);

        if (factoryEntry == jsonFactories.end())
            continue;

        auto pEffect = factoryEntry->second(effectObject);
        if (pEffect)
        {
            if (effectObject[PTY_COREEFFECT].as<int>())
                pEffect->MarkAsCoreEffect();

            _vEffects.push_back(pEffect);
            loadedEffectNumbers.insert(effectNumber);
        }
    }
}

bool EffectManager::DeserializeFromJSON(const JsonObjectConst& jsonObject)
{
    std::scoped_lock guard(g_render_mutex, g_effect_manager_mutex);

    ClearEffects();

    // "efs" is the array of serialized effect objects
    JsonArrayConst effectsArray = jsonObject["efs"].as<JsonArrayConst>();

    // Check if the object actually contained an effect config array. If not, load the default effects.
    if (effectsArray.isNull())
    {
        LoadDefaultEffects();
        return true;
    }

    // Check if there's a persisted effect set version, and remember it if so
    if (jsonObject[PTY_EFFECTSETVER].is<String>())
        _effectSetHashString = jsonObject[PTY_EFFECTSETVER].as<String>();

    LoadJSONEffects(effectsArray);

    // If no effects were successfully loaded from JSON, it loads the default effects.
    if (_vEffects.empty())
        LoadDefaultEffects();

    // "eef" was the array of effect enabled flags. They have now been integrated in the effects themselves;
    //   this code is there to "migrate" users who already had a serialized effect config on their device
    if (jsonObject["eef"].is<JsonArrayConst>())
    {
        // Try to load effect enabled state from JSON also, default to "enabled" otherwise
        JsonArrayConst enabledArray = jsonObject["eef"].as<JsonArrayConst>();
        size_t enabledSize = enabledArray.isNull() ? 0 : enabledArray.size();

        for (int i = 0; i < _vEffects.size(); i++)
        {
            if (i >= enabledSize || enabledArray[i] == 1)
                EnableEffect(i, true);
            else
                DisableEffect(i, true);
        }
    }

    // "ivl" contains the effect interval in ms
    SetInterval(jsonObject["ivl"].is<uint>() ? jsonObject["ivl"] : DEFAULT_EFFECT_INTERVAL, true);

    // Try to read the effectindex from its own file. If that fails, "cei" may contain the current effect index instead
    if (!ReadCurrentEffectIndex(_iCurrentEffect) && jsonObject["cei"].is<size_t>())
        _iCurrentEffect = jsonObject["cei"];

    // Make sure that if we read an index, it's sane
    if (_vEffects.empty())
        _iCurrentEffect = 0;
    else if (_iCurrentEffect >= EffectCount())
        _iCurrentEffect = EffectCount() - 1;

    construct(true);

    return true;
}

// SerializeToJSON - Serialize effects to a JSON object.
//
// This function serializes the current state of the LED strip effects into a JSON object.
// It starts by setting the JSON format version ("PTY_VERSION") to a predefined value ("JSON_FORMAT_VERSION")
// that helps in detecting and managing potential future incompatible structural updates.
//
// The function then sets the "ivl" and "cei" fields in the JSON object to the current effect interval
// and the current effect index, respectively.
//
// Next, the function creates a nested array ("efs") in the JSON object to store the effects themselves.
// It iterates through all effects, and for each effect, it creates a nested object in the effects array
// and attempts to serialize the effect into this object. If serialization of any effect fails, the function
// immediately returns false.
//
// If all effects are successfully serialized, the function returns true, indicating successful serialization.

bool EffectManager::SerializeToJSON(JsonObject& jsonObject)
{
    std::lock_guard effectGuard(g_effect_manager_mutex);

    // Set JSON format version to be able to detect and manage future incompatible structural updates
    jsonObject[PTY_VERSION] = JSON_FORMAT_VERSION;
    jsonObject["ivl"] = _effectInterval;
    jsonObject[PTY_PROJECT] = PROJECT_NAME;
    jsonObject[PTY_EFFECTSETVER] = _effectSetHashString;

    // Next, the function creates a nested array ("efs") in the JSON object to store the effects themselves.
    JsonArray effectsArray = jsonObject["efs"].to<JsonArray>();

    for (auto & effect : _vEffects)
    {
        JsonObject effectObject = effectsArray.add<JsonObject>();
        if (!(effect->SerializeToJSON(effectObject)))
            return false;
    }

    return true;
}
