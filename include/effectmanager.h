#pragma once

//+--------------------------------------------------------------------------
//
// File:        EffectManager.h
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
//
// Description:
//
//    Based on my original ESP32LEDStick project this is the class that keeps
//    track of internal effects, which one is active, rotating among them,
//    and fading between them.
//
//
//
// History:     Apr-13-2019         Davepl      Created for NightDriverStrip
//
//---------------------------------------------------------------------------

#include "globals.h"
#include "jsonserializer.h"

#include <algorithm>
#include <atomic>
#include <functional>
#include <memory>
#include <vector>

class GFXBase;
class LEDStripEffect;

#define JSON_FORMAT_VERSION         1
#define CURRENT_EFFECT_CONFIG_FILE  "/current.cfg"

#define INFORM_EVENT_LISTENERS(listeners, function, ...) \
    std::for_each(listeners.begin(), listeners.end(), [&](auto& listener) { std::invoke(&function, listener __VA_OPT__(,) __VA_ARGS__); })

// Forward references to functions in our accompanying CPP file

void InitSplashEffectManager();
void InitEffectsManager();
void SaveEffectManagerConfig();
void RemoveEffectManagerConfig();

// IFrameEventListener
//
// Abstract class that can be used to listen to frame-related events.

class IFrameEventListener
{
public:
    virtual void OnNewFrameAvailable() = 0;
};

// IEffectEventListener
//
// Abstract class that can be used to listen to effect-related events.

class IEffectEventListener
{
public:
    virtual void OnCurrentEffectChanged(size_t currentEffectIndex) = 0;
    virtual void OnEffectListDirty() = 0;
    virtual void OnEffectEnabledStateChanged(size_t effectIndex, bool newState) = 0;
    virtual void OnIntervalChanged(uint interval) = 0;
};

// BaseFrameEventListener
//
// Basic implementation of IFrameEventListener that remembers it's been called and allows
// that recollection to be read and cleared.

class BaseFrameEventListener : public IFrameEventListener
{
    std::atomic<bool> _newFrameAvailable = false;

public:
    void OnNewFrameAvailable() override
    {
        _newFrameAvailable = true;
    }

    bool CheckAndClearNewFrameAvailable()
    {
        return _newFrameAvailable.exchange(false);
    }
};

// EffectManager
//
// Handles keeping track of the effects, which one is active, asking it to draw, etc.

class  EffectManager : public IJSONSerializable
{
    std::vector<std::shared_ptr<LEDStripEffect>> _vEffects;

    size_t _iCurrentEffect = 0;
    uint _effectStartTime;
    uint _effectInterval = 0;
    bool _bPlayAll;
    bool _clearTempEffectWhenExpired = false;
    std::atomic_bool _newFrameAvailable = false;
    String _effectSetHashString = "";
    uint32_t _lastBeatSequence = 0;
    uint32_t _lastNearBeatSequence = 0;

    std::vector<std::shared_ptr<GFXBase>> _gfx;
    std::shared_ptr<LEDStripEffect> _tempEffect;
    std::vector<std::reference_wrapper<IFrameEventListener>> _frameEventListeners;
    std::vector<std::reference_wrapper<IEffectEventListener>> _effectEventListeners;

    void construct(bool clearTempEffect);
    void DispatchBeatIfNeeded();

    // Implementation is in effects.cpp
    void LoadJSONEffects(const JsonArrayConst& effectsArray);

    static void SaveCurrentEffectIndex();
    static bool ReadCurrentEffectIndex(size_t& index);

    void ClearEffects()
    {
        _vEffects.clear();
    }

public:
    static const uint csFadeButtonSpeed = 15 * 1000;
    static const uint csSmoothButtonSpeed = 60 * 1000;

    EffectManager(const std::shared_ptr<LEDStripEffect>& effect, std::vector<std::shared_ptr<GFXBase>>& gfx);
    explicit EffectManager(std::vector<std::shared_ptr<GFXBase>>& gfx);
    EffectManager(const JsonObjectConst& jsonObject, std::vector<std::shared_ptr<GFXBase>>& gfx);
    ~EffectManager();

    // SetTempEffect - Sets a temporary effect to be played until remote changes it.
    //                 The effect must have already had its Init() function called.

    void SetTempEffect(std::shared_ptr<LEDStripEffect> effect);

    // GetBaseGraphics - Returns the vector of GFXBase objects that the effects use to draw

    std::vector<std::shared_ptr<GFXBase>> & GetBaseGraphics();

    void ReportNewFrameAvailable();
    void AddFrameEventListener(IFrameEventListener& listener);
    
    // RemoveFrameEventListener
    //
    // Removes a previously registered frame event listener by address.
    // Safe to call even if the listener was never registered (no-op).
    // Required for any listener whose lifetime is shorter than the
    // EffectManager's (e.g. a listener owned by a service that can be
    // Stop()-ed and have its task exit), since EffectManager stores
    // listeners by reference.

    void RemoveFrameEventListener(IFrameEventListener& listener);
    void AddEffectEventListener(IEffectEventListener& listener);

    void LoadDefaultEffects();
    bool ReinitializeEffects();

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

    bool DeserializeFromJSON(const JsonObjectConst& jsonObject) override;

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

    bool SerializeToJSON(JsonObject& jsonObject) override;

    // Must provide at least one drawing instance, like the first matrix or strip we are drawing on
    GFXBase& g(int iChannel = 0);
    const GFXBase& g(int iChannel = 0) const;

    // ShowVU - Control whether VU meter should be drawn.  Returns the previous state when set.
    virtual bool ShowVU(bool bShow);
    virtual bool IsVUVisible() const;

    // ApplyGlobalColor
    //
    // When a global color is set via the remote, we create a fill effect and assign it as the "remote effect"
    // which takes drawing precedence

    void ApplyGlobalColor(CRGB color);
    void ApplyGlobalPaletteColors();

    void ClearRemoteColor(bool retainRemoteEffect = false);

    void StartEffect();

    void EnableEffect(size_t i, bool skipSave = false);

    void DisableEffect(size_t i, bool skipSave = false);

    bool IsEffectEnabled(size_t i) const;

    void MoveEffect(size_t from, size_t to);

    // Creates a copy of an existing effect in the list. Note that the effect is created but not yet added to the effect list;
    //   use the AppendEffect() function for that.
    std::shared_ptr<LEDStripEffect> CopyEffect(size_t index);

    // Adds an effect to the effect list and enables it. If an effect is added that is already in the effect list then the result
    //   is undefined but potentially messy.
    bool AppendEffect(std::shared_ptr<LEDStripEffect>& effect);

    bool DeleteEffect(size_t index);

    void PlayAll(bool bPlayAll);
    void SetInterval(uint interval, bool skipSave = false);
    const std::vector<std::shared_ptr<LEDStripEffect>> & EffectsList() const;
    size_t EffectCount() const;
    bool AreEffectsEnabled() const;
    size_t GetCurrentEffectIndex() const;
    LEDStripEffect& GetCurrentEffect() const;
    const String & GetCurrentEffectName() const;
    void SetCurrentEffectIndex(size_t i);
    uint GetTimeUsedByCurrentEffect() const;
    uint GetTimeRemainingForCurrentEffect() const;
    uint GetEffectiveInterval() const;
    uint GetInterval() const;
    bool IsIntervalEternal() const;

    void CheckEffectTimerExpired();

    void NextPalette();
    void PreviousPalette();
    // Update to the next effect and abort the current effect.

    void NextEffect(bool skipSave = false);

    // Go back to the previous effect and abort the current one.

    void PreviousEffect();

    bool Init();

    // EffectManager::Update
    //
    // Draws the current effect.  If gUIDirty has been set by an interrupt handler, it is reset here

    void Update();

    void ApplyFadeLogic();
};
