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
//    track of internal efects, which one is active, rotating among them,
//    and fading between them.
//
//
//
// History:     Apr-13-2019         Davepl      Created for NightDriverStrip
//
//---------------------------------------------------------------------------

#pragma once

#include <sys/types.h>
#include <errno.h>
#include <iostream>
#include <set>
#include <algorithm>
#include <math.h>

#include "effectfactories.h"
#include "effects/strip/misceffects.h"
#include "effects/strip/fireeffect.h"

#define JSON_FORMAT_VERSION         1
#define CURRENT_EFFECT_CONFIG_FILE  "/current.cfg"

// Forward references to functions in our accompanying CPP file

void InitSplashEffectManager();
void InitEffectsManager();
void SaveEffectManagerConfig();
void RemoveEffectManagerConfig();

std::shared_ptr<LEDStripEffect> GetSpectrumAnalyzer(CRGB color);
std::shared_ptr<LEDStripEffect> GetSpectrumAnalyzer(CRGB color, CRGB color2);

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
    bool _bShowVU = true;
    CRGB lastManualColor = CRGB::Red;
    bool _clearTempEffectWhenExpired = false;
    bool _newFrameAvailable = false;
    int _effectSetVersion = 1;

    std::vector<std::shared_ptr<GFXBase>> _gfx;
    std::shared_ptr<LEDStripEffect> _tempEffect;

    void construct(bool clearTempEffect)
    {
        _bPlayAll = false;

        if (clearTempEffect && _tempEffect)
        {
            _clearTempEffectWhenExpired = true;

            // This is a hacky way to ensure that we start the correct effect after the temporary one.
            //   The switching to the next effect is taken care of by NextEffect(), which starts with
            //   increasing _iCurrentEffect. We therefore need to decrease it here, to make sure that
            //   the first effect after the temporary one is the one we want (either the then current
            //   one when the chip was powered off, or the one at index 0).
            _iCurrentEffect--;
        }
    }

    void ProduceAndLoadDefaultEffect(const EffectFactories::NumberedFactory& numberedFactory)
    {
        auto pEffect = numberedFactory.CreateEffect();
        if (pEffect)
        {
            // Effects in the default list are core effects. These can be disabled but not deleted.
            pEffect->MarkAsCoreEffect();
            _vEffects.push_back(pEffect);
        }
    }

    // Implementation is in effects.cpp
    void LoadJSONAndMissingEffects(const JsonArrayConst& effectsArray);

    void SaveCurrentEffectIndex();
    bool ReadCurrentEffectIndex(size_t& index);

    void ClearEffects()
    {
        _vEffects.clear();
    }

public:
    static const uint csFadeButtonSpeed = 15 * 1000;
    static const uint csSmoothButtonSpeed = 60 * 1000;

    EffectManager(std::shared_ptr<LEDStripEffect> effect, std::vector<std::shared_ptr<GFXBase>>& gfx)
        : _gfx(gfx)
    {
        debugV("EffectManager Splash Effect Constructor");

        if (effect->Init(_gfx))
            _tempEffect = effect;

        construct(false);
    }

    explicit EffectManager(std::vector<std::shared_ptr<GFXBase>>& gfx)
        : _gfx(gfx)
    {
        debugV("EffectManager Constructor");

        LoadDefaultEffects();
    }

    EffectManager(const JsonObjectConst& jsonObject, std::vector<std::shared_ptr<GFXBase>>& gfx)
        : _gfx(gfx)
    {
        debugV("EffectManager JSON Constructor");

        DeserializeFromJSON(jsonObject);
    }

    ~EffectManager()
    {
        ClearRemoteColor();
        ClearEffects();
    }

    std::shared_ptr<GFXBase> GetBaseGraphics()
    {
        return _gfx[0];
    }

    bool IsNewFrameAvailable() const
    {
        return _newFrameAvailable;
    }

    void SetNewFrameAvailable(bool available)
    {
        _newFrameAvailable = available;
    }

    // Implementation is in effects.cpp
    void LoadDefaultEffects();

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

    bool DeserializeFromJSON(const JsonObjectConst& jsonObject) override
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
        if (jsonObject.containsKey(PTY_EFFECTSETVER))
            _effectSetVersion = jsonObject[PTY_EFFECTSETVER];

        LoadJSONAndMissingEffects(effectsArray);

        // "eef" was the array of effect enabled flags. They have now been integrated in the effects themselves;
        //   this code is there to "migrate" users who already had a serialized effect config on their device
        if (jsonObject.containsKey("eef"))
        {
            // Try to load effect enabled state from JSON also, default to "enabled" otherwise
            JsonArrayConst enabledArray = jsonObject["eef"].as<JsonArrayConst>();
            int enabledSize = enabledArray.isNull() ? 0 : enabledArray.size();

            for (int i = 0; i < _vEffects.size(); i++)
            {
                if (i >= enabledSize || enabledArray[i] == 1)
                    EnableEffect(i, true);
                else
                    DisableEffect(i, true);
            }
        }

        // "ivl" contains the effect interval in ms
        SetInterval(jsonObject.containsKey("ivl") ? jsonObject["ivl"] : DEFAULT_EFFECT_INTERVAL, true);

        // Try to read the effectindex from its own file. If that fails, "cei" may contain the current effect index instead
        if (!ReadCurrentEffectIndex(_iCurrentEffect) && jsonObject.containsKey("cei"))
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
    // The function creates a nested array ("eef") in the JSON object to store the enabled state of each effect.
    // It iterates through all effects, and for each effect, it adds a value of 1 to the array if the effect
    // is enabled, and 0 if it is not.
    //
    // Next, the function creates another nested array ("efs") in the JSON object to store the effects themselves.
    // It iterates through all effects, and for each effect, it creates a nested object in the effects array
    // and attempts to serialize the effect into this object. If serialization of any effect fails, the function
    // immediately returns false.
    //
    // If all effects are successfully serialized, the function returns true, indicating successful serialization.

    bool SerializeToJSON(JsonObject& jsonObject) override
    {
        // Set JSON format version to be able to detect and manage future incompatible structural updates
        jsonObject[PTY_VERSION] = JSON_FORMAT_VERSION;
        jsonObject["ivl"] = _effectInterval;
        jsonObject[PTY_PROJECT] = PROJECT_NAME;
        jsonObject[PTY_EFFECTSETVER] = _effectSetVersion;

        JsonArray effectsArray = jsonObject.createNestedArray("efs");

        for (auto & effect : _vEffects)
        {
            JsonObject effectObject = effectsArray.createNestedObject();
            if (!(effect->SerializeToJSON(effectObject)))
                return false;
        }

        return true;
    }

    // Must provide at least one drawing instance, like the first matrix or strip we are drawing on
    inline std::shared_ptr<GFXBase> g(int iChannel = 0) const
    {
        return _gfx[iChannel];
    }

    // ShowVU - Control whether VU meter should be draw.  Returns the previous state when set.

    virtual bool ShowVU(bool bShow)
    {
        bool bResult = _bShowVU;
        debugI("Setting ShowVU to %d\n", bShow);
        _bShowVU = bShow;

        // Erase any exising pixels since effects don't all clear each frame
        if (!bShow)
            _gfx[0]->setPixelsF(0, MATRIX_WIDTH, CRGB::Black);

        return bResult;
    }

    virtual bool IsVUVisible() const
    {
        return _bShowVU && GetCurrentEffect().CanDisplayVUMeter();
    }

    // SetGlobalColor
    //
    // When a global color is set via the remote, we create a fill effect and assign it as the "remote effect"
    // which takes drawing precedence

    void SetGlobalColor(CRGB color)
    {
        debugI("Setting Global Color");

        CRGB oldColor = lastManualColor;
        lastManualColor = color;

        #if (USE_HUB75)
                auto pMatrix = g();
                pMatrix->setPalette(CRGBPalette16(oldColor, color));
                pMatrix->PausePalette(true);
        #else
            std::shared_ptr<LEDStripEffect> effect;

            if (color == CRGB(CRGB::White))
                effect = make_shared_psram<ColorFillEffect>(CRGB::White, 1);
            else

                #if ENABLE_AUDIO
                    #if SPECTRUM
                        effect = GetSpectrumAnalyzer(color, oldColor);
                    #else
                        effect = make_shared_psram<MusicalPaletteFire>("Custom Fire", CRGBPalette16(CRGB::Black, color, CRGB::Yellow, CRGB::White), NUM_LEDS, 1, 8, 50, 1, 24, true, false);
                    #endif
                #else
                    effect = make_shared_psram<PaletteFlameEffect>("Custom Fire", CRGBPalette16(CRGB::Black, color, CRGB::Yellow, CRGB::White), NUM_LEDS, 1, 8, 50, 1, 24, true, false);
                #endif

            if (effect->Init(_gfx))
            {
                _tempEffect = effect;
                StartEffect();
            }
        #endif
    }

    void ClearRemoteColor(bool retainRemoteEffect = false)
    {
        if (!retainRemoteEffect)
            _tempEffect = nullptr;

        #if (USE_HUB75)
            g()->PausePalette(false);
        #endif
    }

    void StartEffect()
    {
        // If there's a temporary effect override from the remote control active, we start that, else
        // we start the current regular effect

        std::shared_ptr<LEDStripEffect> & effect = _tempEffect ? _tempEffect : _vEffects[_iCurrentEffect];

        #if USE_HUB75
            auto pMatrix = std::static_pointer_cast<LEDMatrixGFX>(_gfx[0]);
            pMatrix->SetCaption(effect->FriendlyName(), CAPTION_TIME);
        #endif

        effect->Start();

        _effectStartTime = millis();
    }

    void EnableEffect(size_t i, bool skipSave = false)
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
        }
    }

    void DisableEffect(size_t i, bool skipSave = false)
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
                SetGlobalColor(CRGB::Black);

            if (!skipSave)
                SaveEffectManagerConfig();
        }
    }

    bool IsEffectEnabled(size_t i) const
    {
        if (i >= _vEffects.size())
        {
            debugW("Invalid index for IsEffectEnabled");
            return false;
        }
        return _vEffects[i]->IsEnabled();
    }

    void MoveEffect(size_t from, size_t to)
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
    }

    // Creates a copy of an existing effect in the list. Note that the effect is created but not yet added to the effect list;
    //   use the AppendEffect() function for that.
    std::shared_ptr<LEDStripEffect> CopyEffect(size_t index);

    // Adds an effect to the effect list and enables it. If an effect is added that is already in the effect list then the result
    //   is undefined but potentially messy.
    bool AppendEffect(std::shared_ptr<LEDStripEffect>& effect)
    {
        if (!effect->Init(_gfx))
            return false;

        _vEffects.push_back(effect);
        EnableEffect(_vEffects.size() - 1, true);

        SaveEffectManagerConfig();

        return true;
    }

    bool DeleteEffect(size_t index)
    {
        if (index >= _vEffects.size())
        {
            debugW("Invalid index for DeleteEffect");
            return false;
        }

        // We don't allow core effects to be deleted
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

        return true;
    }

    void PlayAll(bool bPlayAll)
    {
        _bPlayAll = bPlayAll;
    }

    void SetInterval(uint interval, bool skipSave = false)
    {
        _effectInterval = interval;

        if (!skipSave)
            SaveEffectManagerConfig();
    }

    const std::vector<std::shared_ptr<LEDStripEffect>> & EffectsList() const
    {
        return _vEffects;
    }

    const size_t EffectCount() const
    {
        return _vEffects.size();
    }

    const bool AreEffectsEnabled() const
    {
        return std::any_of(_vEffects.begin(), _vEffects.end(), [](const auto& pEffect){ return pEffect->IsEnabled(); } );
    }

    const size_t GetCurrentEffectIndex() const
    {
        return _iCurrentEffect;
    }

    LEDStripEffect& GetCurrentEffect() const
    {
        return *(_tempEffect ? _tempEffect : _vEffects[_iCurrentEffect]);
    }

    const String & GetCurrentEffectName() const
    {
        if (_tempEffect)
            return _tempEffect->FriendlyName();

        return _vEffects[_iCurrentEffect]->FriendlyName();
    }

    // Change the current effect; marks the state as needing attention so this get noticed next frame

    void SetCurrentEffectIndex(size_t i)
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
    }

    uint GetTimeUsedByCurrentEffect() const
    {
        return millis() - _effectStartTime;
    }

    uint GetTimeRemainingForCurrentEffect() const
    {
        // If the Interval is set to zero, we treat that as an infinite interval and don't even look at the time used so far
        uint timeUsedByCurrentEffect = GetTimeUsedByCurrentEffect();
        uint interval = GetEffectiveInterval();

        return timeUsedByCurrentEffect > interval ? 0 : (interval - timeUsedByCurrentEffect);
    }

    uint GetEffectiveInterval() const
    {
        auto& currentEffect = GetCurrentEffect();
        // This allows you to return a MaximumEffectTime and your effect won't be shown longer than that
        return min((IsIntervalEternal() ? std::numeric_limits<uint>::max() : _effectInterval),
                   (currentEffect.HasMaximumEffectTime() ? currentEffect.MaximumEffectTime() : std::numeric_limits<uint>::max()));
    }

    uint GetInterval() const
    {
        return _effectInterval;
    }

    bool IsIntervalEternal() const
    {
        return _effectInterval == 0;
    }

    void CheckEffectTimerExpired()
    {
        // If interval is zero, the current effect never expires unless it thas a max effect time set

        if (IsIntervalEternal() && !GetCurrentEffect().HasMaximumEffectTime())
            return;

        if (GetTimeUsedByCurrentEffect() >= GetEffectiveInterval()) // See if it's time for a new effect yet
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

    void NextPalette()
    {
        auto g = _gfx[0].get();
        g->CyclePalette(1);
    }

    void PreviousPalette()
    {
        g()->CyclePalette(-1);
    }
    // Update to the next effect and abort the current effect.

    void NextEffect(bool skipSave = false)
    {
        auto enabled = AreEffectsEnabled();

        do
        {
            _iCurrentEffect++; //   ... if so advance to next effect
            _iCurrentEffect %= EffectCount();
            _effectStartTime = millis();
        } while (enabled && false == _bPlayAll && false == IsEffectEnabled(_iCurrentEffect));

        StartEffect();
        SaveCurrentEffectIndex();
    }

    // Go back to the previous effect and abort the current one.

    void PreviousEffect()
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
    }

    bool Init()
    {

        for (int i = 0; i < _vEffects.size(); i++)
        {
            debugV("About to init effect %s", _vEffects[i]->FriendlyName().c_str());
            if (false == _vEffects[i]->Init(_gfx))
            {
                debugW("Could not initialize effect: %s\n", _vEffects[i]->FriendlyName().c_str());
                return false;
            }
            debugV("Loaded Effect: %s", _vEffects[i]->FriendlyName().c_str());
        }
        debugV("First Effect: %s", GetCurrentEffectName().c_str());
        return true;
    }

    // EffectManager::Update
    //
    // Draws the current effect.  If gUIDirty has been set by an interrupt handler, it is reset here

    void Update()
    {
        if ((_gfx[0])->GetLEDCount() == 0)
            return;

        constexpr auto msFadeTime = EFFECT_CROSS_FADE_TIME;

        CheckEffectTimerExpired();

        // If a remote control effect is set, we draw that, otherwise we draw the regular effect

        if (_tempEffect)
            _tempEffect->Draw();
        else
            _vEffects[_iCurrentEffect]->Draw(); // Draw the currently active effect

        // If we do indeed have multiple effects (BUGBUG what if only a single enabled?) then we
        // fade in and out at the appropriate time based on the time remaining/used by the effect

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

        int r = GetTimeRemainingForCurrentEffect();
        int e = GetTimeUsedByCurrentEffect();

        if (e < msFadeTime)
        {
            g_Values.Fader = 255 * (e / msFadeTime); // Fade in
        }
        else if (r < msFadeTime)
        {
            g_Values.Fader = 255 * (r / msFadeTime); // Fade out
        }
        else
        {
            g_Values.Fader = 255; // No fade, not at start or end
        }
    }
};
