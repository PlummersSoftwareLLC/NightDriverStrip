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
#if USE_HUB75
#include "hub75gfx.h"
#endif

// Variables we need further down

extern std::unique_ptr<EffectFactories> g_ptrEffectFactories;
extern std::map<int, JSONEffectFactory> g_JsonStarryNightEffectFactories;
DRAM_ATTR size_t g_EffectsManagerJSONBufferSize = 0;
static DRAM_ATTR size_t l_EffectsManagerJSONWriterIndex = SIZE_MAX;
static DRAM_ATTR size_t l_CurrentEffectWriterIndex = SIZE_MAX;

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

    LoadEffectFactories();

    l_EffectsManagerJSONWriterIndex = g_ptrSystem->GetJSONWriter().RegisterWriter([]()
    {
        if (!SaveToJSONFile(EFFECTS_CONFIG_FILE, g_ptrSystem->GetEffectManager()) && EFFECT_PERSISTENCE_CRITICAL)
            throw std::runtime_error("Effects serialization failed");
    });
    l_CurrentEffectWriterIndex = g_ptrSystem->GetJSONWriter().RegisterWriter(WriteCurrentEffectIndexFile);

    auto jsonDoc = CreateJsonDocument();
    auto jsonObject = LoadEffectsJSONFile(jsonDoc);

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
}

void EffectManager::StartEffect()
{
    // If there's a temporary effect override from the remote control active, we start that, else
    // we start the current regular effect

    std::shared_ptr<LEDStripEffect> & effect = _tempEffect ? _tempEffect : _vEffects[_iCurrentEffect];

    #if USE_HUB75
        auto& matrix = static_cast<HUB75GFX&>(*_gfx[0]);
        matrix.SetCaption(effect->FriendlyName(), CAPTION_TIME);
    #endif

    effect->Start();
    _effectStartTime = millis();
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

void EffectManager::SetTempEffect(std::shared_ptr<LEDStripEffect> effect)
{
    _tempEffect = effect;
}

std::vector<std::shared_ptr<GFXBase>> & EffectManager::GetBaseGraphics()
{
    return _gfx;
}

void EffectManager::ReportNewFrameAvailable()
{
    INFORM_EVENT_LISTENERS(_frameEventListeners, IFrameEventListener::OnNewFrameAvailable);
}

void EffectManager::AddFrameEventListener(IFrameEventListener& listener)
{
    _frameEventListeners.emplace_back(listener);
}

void EffectManager::AddEffectEventListener(IEffectEventListener& listener)
{
    _effectEventListeners.emplace_back(listener);
}

// Must provide at least one drawing instance, like the first matrix or strip we are drawing on
GFXBase& EffectManager::g(int iChannel)
{
    return *_gfx[iChannel];
}

const GFXBase& EffectManager::g(int iChannel) const
{
    return *_gfx[iChannel];
}

bool EffectManager::IsEffectEnabled(size_t i) const
{
    if (i >= _vEffects.size())
    {
        debugW("Invalid index for IsEffectEnabled");
        return false;
    }
    return _vEffects[i]->IsEnabled();
}

void EffectManager::PlayAll(bool bPlayAll)
{
    _bPlayAll = bPlayAll;
}

void EffectManager::SetInterval(uint interval, bool skipSave)
{
    // Reject/ignore intervals smaller than a second, but allow 0 (infinity)
    if (interval > 0 && interval < 1000)
        return;

    _effectInterval = interval;

    if (!skipSave)
        SaveEffectManagerConfig();

    INFORM_EVENT_LISTENERS(_effectEventListeners, IEffectEventListener::OnIntervalChanged, interval);
}

const std::vector<std::shared_ptr<LEDStripEffect>> & EffectManager::EffectsList() const
{
    return _vEffects;
}

size_t EffectManager::EffectCount() const
{
    return _vEffects.size();
}

bool EffectManager::AreEffectsEnabled() const
{
    return std::any_of(_vEffects.begin(), _vEffects.end(), [](const auto& pEffect){ return pEffect->IsEnabled(); } );
}

size_t EffectManager::GetCurrentEffectIndex() const
{
    return _iCurrentEffect;
}

LEDStripEffect& EffectManager::GetCurrentEffect() const
{
    return *(_tempEffect ? _tempEffect : _vEffects[_iCurrentEffect]);
}

const String & EffectManager::GetCurrentEffectName() const
{
    if (_tempEffect)
        return _tempEffect->FriendlyName();

    return _vEffects[_iCurrentEffect]->FriendlyName();
}

void EffectManager::SetCurrentEffectIndex(size_t i)
{
    if (i >= _vEffects.size())
    {
        debugW("Invalid index for SetCurrentEffectIndex");
        return;
    }
    _iCurrentEffect = i;
    _effectStartTime = millis();

    StartEffect();
    SaveCurrentEffectIndex();

    INFORM_EVENT_LISTENERS(_effectEventListeners, IEffectEventListener::OnCurrentEffectChanged, i);
}

uint EffectManager::GetTimeUsedByCurrentEffect() const
{
    return millis() - _effectStartTime;
}

uint EffectManager::GetTimeRemainingForCurrentEffect() const
{
    // If the Interval is set to zero, we treat that as an infinite interval and don't even look at the time used so far
    uint timeUsedByCurrentEffect = GetTimeUsedByCurrentEffect();
    uint interval = GetEffectiveInterval();

    return timeUsedByCurrentEffect > interval ? 0 : (interval - timeUsedByCurrentEffect);
}

uint EffectManager::GetEffectiveInterval() const
{
    auto& currentEffect = GetCurrentEffect();
    // This allows you to return a MaximumEffectTime and your effect won't be shown longer than that
    return min((IsIntervalEternal() ? std::numeric_limits<uint>::max() : _effectInterval),
               (currentEffect.HasMaximumEffectTime() ? currentEffect.MaximumEffectTime() : std::numeric_limits<uint>::max()));
}

uint EffectManager::GetInterval() const
{
    return _effectInterval;
}

bool EffectManager::IsIntervalEternal() const
{
    return _effectInterval == 0;
}

void EffectManager::NextPalette()
{
    debugV("EffectManager::NextPalette");
    g().CyclePalette(1);
}

void EffectManager::PreviousPalette()
{
    debugV("EffectManager::PreviousPalette");
    g().CyclePalette(-1);
}

void EffectManager::LoadDefaultEffects()
{
    _effectSetHashString = g_ptrEffectFactories->HashString();

    for (const auto &numberedFactory : g_ptrEffectFactories->GetDefaultFactories())
    {
        auto pEffect = numberedFactory.CreateEffect();
        if (pEffect)
        {
            // Effects in the default list are core effects. These can be disabled but not deleted.
            pEffect->MarkAsCoreEffect();
            _vEffects.push_back(pEffect);
        }
    }

    SetInterval(DEFAULT_EFFECT_INTERVAL, true);

    construct(true);
}


// Saves the current effect index to its own file. Note that this function only flags the writer
//   for execution in the background.
void EffectManager::SaveCurrentEffectIndex()
{
    if (g_ptrSystem->GetDeviceConfig().RememberCurrentEffect())
        // Default value for writer index is max value for size_t, so nothing will happen if writer has not yet been registered
        g_ptrSystem->GetJSONWriter().FlagWriter(l_CurrentEffectWriterIndex);
}

// Reads the current effect index from its own file. Returns true if the index was successfully read.
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

// Creates a copy of an existing effect in the list. Note that the effect is created but not yet added to the effect list;
//   use the AppendEffect() function for that.
std::shared_ptr<LEDStripEffect> EffectManager::CopyEffect(size_t index)
{
    if (index >= _vEffects.size())
    {
        debugW("Invalid index for CopyEffect");
        return nullptr;
    }

    auto& sourceEffect = _vEffects[index];

    auto jsonEffectFactories = g_ptrEffectFactories->GetJSONFactories();
    auto factoryEntry = jsonEffectFactories.find(static_cast<int>(sourceEffect->effectId()));

    if (factoryEntry == jsonEffectFactories.end())
        return nullptr;

    auto jsonDoc = CreateJsonDocument();
    auto jsonObject = jsonDoc.to<JsonObject>();

    if (!sourceEffect->SerializeToJSON(jsonObject))
    {
        debugE("Could not serialize effect %s to JSON", sourceEffect->FriendlyName().c_str());
        return nullptr;
    }

    auto copiedEffect = factoryEntry->second(jsonDoc.as<JsonObjectConst>());

    if (!copiedEffect)
        return nullptr;

    copiedEffect->SetEnabled(false);

    return copiedEffect;
}

//
// Helper functions related to JSON persistence
//

void SaveEffectManagerConfig()
{
    debugV("Saving effect manager config...");
    // Default value for writer index is max value for size_t, so nothing will happen if writer has not yet been registered
    g_ptrSystem->GetJSONWriter().FlagWriter(l_EffectsManagerJSONWriterIndex);
}

void RemoveEffectManagerConfig()
{
    RemoveJSONFile(EFFECTS_CONFIG_FILE);
    // We take the liberty of also removing the file with the current effect config index
    SPIFFS.remove(CURRENT_EFFECT_CONFIG_FILE);
}

void WriteCurrentEffectIndexFile()
{
    SPIFFS.remove(CURRENT_EFFECT_CONFIG_FILE);

    File file = SPIFFS.open(CURRENT_EFFECT_CONFIG_FILE, FILE_WRITE);

    if (!file)
    {
        debugE("Unable to open file %s for writing!", CURRENT_EFFECT_CONFIG_FILE);
        return;
    }

    auto bytesWritten = file.print(g_ptrSystem->GetEffectManager().GetCurrentEffectIndex());
    debugI("Number of bytes written to file %s: %zu", CURRENT_EFFECT_CONFIG_FILE, (size_t)bytesWritten);

    file.flush();
    file.close();

    if (bytesWritten == 0)
    {
        debugE("Unable to write to file %s!", CURRENT_EFFECT_CONFIG_FILE);
        SPIFFS.remove(CURRENT_EFFECT_CONFIG_FILE);
    }
}

//
// Other helper functions
//

#if ENABLE_AUDIO

#include "effects/matrix/spectrumeffects.h"

// GetSpectrumAnalyzer
//
// A little factory that makes colored spectrum analyzers

std::shared_ptr<LEDStripEffect> GetSpectrumAnalyzer(CRGB color)
{
    CHSV hueColor = rgb2hsv_approximate(color);
    CRGB color2 = CRGB(CHSV(hueColor.hue + 64, 255, 255));
    auto object = make_shared_psram<SpectrumAnalyzerEffect>("Spectrum Clr", 24, CRGBPalette16(color, color2), true);
    if (object->Init(g_ptrSystem->GetDevices()))
        return object;
    throw std::runtime_error("Could not initialize new spectrum analyzer, one color version!");
}

#endif

#include "effects/strip/fireeffect.h"

bool EffectManager::Init()
{
    for (const auto & _vEffect : _vEffects)
    {
        debugV("About to init effect %s", _vEffect->FriendlyName().c_str());
        if (false == _vEffect->Init(_gfx))
        {
            debugW("Could not initialize effect: %s\n", _vEffect->FriendlyName().c_str());
            return false;
        }
        debugV("Loaded Effect: %s", _vEffect->FriendlyName().c_str());
    }
    debugV("First Effect: %s", GetCurrentEffectName().c_str());

    if (g_ptrSystem->GetDeviceConfig().ApplyGlobalColors())
        ApplyGlobalPaletteColors();

    return true;
}

bool EffectManager::ShowVU(bool bShow)
{
    auto& deviceConfig = g_ptrSystem->GetDeviceConfig();
    bool bResult = deviceConfig.ShowVUMeter();
    debugI("Setting ShowVU to %d\n", bShow);
    deviceConfig.SetShowVUMeter(bShow);

    // Erase any exising pixels since effects don't all clear each frame
    if (!bShow)
        _gfx[0]->setPixelsF(0, MATRIX_WIDTH, CRGB::Black);

    return bResult;
}

bool EffectManager::IsVUVisible() const
{
    return g_ptrSystem->GetDeviceConfig().ShowVUMeter() && GetCurrentEffect().CanDisplayVUMeter();
}


void EffectManager::ClearRemoteColor(bool retainRemoteEffect)
{
    if (!retainRemoteEffect)
        _tempEffect = nullptr;

    #if USE_HUB75
        g().PausePalette(false);
    #endif

    g_ptrSystem->GetDeviceConfig().ClearApplyGlobalColors();
}

// ApplyGlobalColor
//
// When a global color is set via the remote, we create a fill effect and assign it as the "remote effect"
// which takes drawing precedence

void EffectManager::ApplyGlobalColor(CRGB color)
{
    debugI("Setting Global Color: %08lX\n", (unsigned long)(uint32_t)color);

    auto& deviceConfig = g_ptrSystem->GetDeviceConfig();
    deviceConfig.SetColorSettings(color, deviceConfig.GlobalColor());

    ApplyGlobalPaletteColors();
}

void EffectManager::ApplyGlobalPaletteColors()
{
    #if USE_HUB75
        auto& pMatrix = g();
        auto& deviceConfig = g_ptrSystem->GetDeviceConfig();
        auto& globalColor = deviceConfig.GlobalColor();
        auto& secondColor = deviceConfig.SecondColor();

        // If the two colors are the same, we just shift the palette by 64 degrees to create a palette
        // based from where those colors sit on the spectrum
        if (secondColor == globalColor)
        {
            CHSV hsv = rgb2hsv_approximate(globalColor);
            pMatrix.setPalette(CRGBPalette16(globalColor, CRGB(CHSV(hsv.hue + 64, 255, 255))));
        }
        else
        {
            // But if we have two different colors, we create a palette spread between them
            pMatrix.setPalette(CRGBPalette16(secondColor, globalColor));
        }

        pMatrix.PausePalette(true);
    #endif
}

void EffectManager::construct(bool clearTempEffect)
{
    _bPlayAll = false;

    if (clearTempEffect && _tempEffect)
    {
        _clearTempEffectWhenExpired = true;

        // This ensures that we start the correct effect after the temporary one.
        //   The switching to the next effect is taken care of by NextEffect(), which starts with
        //   increasing _iCurrentEffect. We therefore need to set it to the previous effect, to
        //   make sure that the first effect after the temporary one is the one we want (either the
        //   then current one when the chip was powered off, or the one at index 0).
        if (_iCurrentEffect == 0)
            _iCurrentEffect = EffectCount();

        _iCurrentEffect--;
    }
}

// DeserializeFromJSON
//
// This function deserializes LED strip effects from a provided JSON object.
//
// It first clears any existing effects and then attempts to populate the effects vector from
// the provided JSON object, which should contain an array of effects configurations ("efs").
//
// For each effect in the JSON array, it attempts to create an effect from its JSON configuration.
// If an effect is successfully created, it's added to the effects vector.
//
// If no effects are successfully loaded from JSON, it loads the default effects.
//
// If the JSON object includes an "eef" array, the function attempts to load each effect's enabled
// state from it.
// If the index exceeds the "eef" array's size, the effect is enabled by default.
//
// The function also sets the effect interval from the "ivl" field in the JSON object, defaulting
// to a pre-defined value if the field isn't present.
//
// If the JSON object includes a "cei" field, the function sets the current effect index to this
// value. If the value is greater than or equal to the number of effects, it defaults to the last
// effect in the vector.
//
// Lastly, the function calls the construct() method, indicating successful deserialization.

bool EffectManager::DeserializeFromJSON(const JsonObjectConst& jsonObject)
{
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
    {
        LoadDefaultEffects();
        return true;
    }

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
    if (_iCurrentEffect >= EffectCount())
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

void EffectManager::EnableEffect(size_t i, bool skipSave)
{
    if (i >= _vEffects.size())
    {
        debugW("Invalid index for EnableEffect");
        return;
    }

    auto& effect = _vEffects[i];

    if (!effect->IsEnabled())
    {
        if (!AreEffectsEnabled())
            ClearRemoteColor(true);

        effect->SetEnabled(true);

        if (!skipSave)
            SaveEffectManagerConfig();

        INFORM_EVENT_LISTENERS(_effectEventListeners, IEffectEventListener::OnEffectEnabledStateChanged, i, true);
    }
}

void EffectManager::DisableEffect(size_t i, bool skipSave)
{
    if (i >= _vEffects.size())
    {
        debugW("Invalid index for DisableEffect");
        return;
    }

    auto effect = _vEffects[i];

    if (effect->IsEnabled())
    {
        effect->SetEnabled(false);

        if (!AreEffectsEnabled())
            ApplyGlobalColor(CRGB::Black);

        if (!skipSave)
            SaveEffectManagerConfig();

        INFORM_EVENT_LISTENERS(_effectEventListeners, IEffectEventListener::OnEffectEnabledStateChanged, i, false);
    }
}

void EffectManager::MoveEffect(size_t from, size_t to)
{
    if (from >= _vEffects.size() || to >= _vEffects.size())
    {
        debugW("Invalid index for MoveEffect");
        return;
    }

    if (from == to)
        return;
    else if (from < to)
        std::rotate(_vEffects.begin() + from, _vEffects.begin() + from + 1, _vEffects.begin() + to + 1);
    else // from > to
        std::rotate(_vEffects.rend() - from - 1, _vEffects.rend() - from, _vEffects.rend() - to);

    if (from == _iCurrentEffect)
    {
        _iCurrentEffect = to;
        SaveCurrentEffectIndex();
    }
    else if (from < _iCurrentEffect && to >= _iCurrentEffect)
    {
        _iCurrentEffect--;
        SaveCurrentEffectIndex();
    }
    else if (from > _iCurrentEffect && to <= _iCurrentEffect)
    {
        _iCurrentEffect++;
        SaveCurrentEffectIndex();
    }

    SaveEffectManagerConfig();

    INFORM_EVENT_LISTENERS(_effectEventListeners, IEffectEventListener::OnEffectListDirty);
}

// Adds an effect to the effect list and enables it. If an effect is added that is already in the effect list then the result
//   is undefined but potentially messy.
bool EffectManager::AppendEffect(std::shared_ptr<LEDStripEffect>& effect)
{
    if (!effect->Init(_gfx))
        return false;

    _vEffects.push_back(effect);
    EnableEffect(_vEffects.size() - 1, true);

    SaveEffectManagerConfig();

    INFORM_EVENT_LISTENERS(_effectEventListeners, IEffectEventListener::OnEffectListDirty);

    return true;
}

// Deletes an effect from the effect list. Note that core effects cannot be deleted.
bool EffectManager::DeleteEffect(size_t index)
{
    if (index >= _vEffects.size())
    {
        debugW("Invalid index for DeleteEffect");
        return false;
    }

    if (_vEffects[index]->IsCoreEffect())
        return false;

    DisableEffect(index, true);

    if (index == _iCurrentEffect)
        NextEffect();

    _vEffects.erase(_vEffects.begin() + index);

    if (index <= _iCurrentEffect)
    {
        _iCurrentEffect--;
        SaveCurrentEffectIndex();
    }

    SaveEffectManagerConfig();

    INFORM_EVENT_LISTENERS(_effectEventListeners, IEffectEventListener::OnEffectListDirty);

    return true;
}

void EffectManager::CheckEffectTimerExpired()
{
    if (IsIntervalEternal() && !GetCurrentEffect().HasMaximumEffectTime())
        return;

    if (GetTimeUsedByCurrentEffect() >= GetEffectiveInterval())
    {
        if (_clearTempEffectWhenExpired)
        {
            _tempEffect.reset();
            _clearTempEffectWhenExpired = false;
        }

        debugV("%ldms elapsed: Next Effect", millis() - _effectStartTime);
        NextEffect();
        debugV("Current Effect: %s", GetCurrentEffectName().c_str());
    }
}

// Update to the next effect and abort the current effect.

void EffectManager::NextEffect(bool skipSave)
{
    auto enabled = AreEffectsEnabled();

    do
    {
        _iCurrentEffect++;
        _iCurrentEffect %= EffectCount();
        _effectStartTime = millis();
    } while (enabled && false == _bPlayAll && false == IsEffectEnabled(_iCurrentEffect));

    StartEffect();
    SaveCurrentEffectIndex();

    INFORM_EVENT_LISTENERS(_effectEventListeners, IEffectEventListener::OnCurrentEffectChanged, _iCurrentEffect);
}

// Go back to the previous effect and abort the current one.

void EffectManager::PreviousEffect()
{
    auto enabled = AreEffectsEnabled();

    do
    {
        if (_iCurrentEffect == 0)
            _iCurrentEffect = EffectCount();

        _iCurrentEffect--;
        _effectStartTime = millis();
    } while (enabled && false == _bPlayAll && false == IsEffectEnabled(_iCurrentEffect));

    StartEffect();
    SaveCurrentEffectIndex();

    INFORM_EVENT_LISTENERS(_effectEventListeners, IEffectEventListener::OnCurrentEffectChanged, _iCurrentEffect);
}

// EffectManager::Update
//
// Draws the current effect.  If gUIDirty has been set by an interrupt handler, it is reset here

void EffectManager::Update()
{
    if ((_gfx[0])->GetLEDCount() == 0)
        return;

    CheckEffectTimerExpired();

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
