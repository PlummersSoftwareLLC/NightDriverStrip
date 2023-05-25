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

#define JSON_FORMAT_VERSION 1

extern uint8_t g_Brightness;
extern uint8_t g_Fader;

// References to functions in other C files

void InitSplashEffectManager();
void InitEffectsManager();
void SaveEffectManagerConfig();
void RemoveEffectManagerConfig();
std::shared_ptr<LEDStripEffect> GetSpectrumAnalyzer(CRGB color);
std::shared_ptr<LEDStripEffect> GetSpectrumAnalyzer(CRGB color, CRGB color2);
extern DRAM_ATTR std::shared_ptr<GFXBase> g_aptrDevices[NUM_CHANNELS];

// EffectManager
//
// Handles keeping track of the effects, which one is active, asking it to draw, etc.

template <typename GFXTYPE>
class EffectManager : public IJSONSerializable
{
    std::vector<std::shared_ptr<LEDStripEffect>> _vEffects;
    size_t _cEnabled = 0;

    size_t _iCurrentEffect = 0;
    uint _effectStartTime;
    uint _effectInterval = 0;
    bool _bPlayAll;
    bool _bShowVU = true;
    CRGB lastManualColor = CRGB::Red;
    bool _clearTempEffectWhenExpired = false;

    std::shared_ptr<GFXTYPE> * _gfx;
    std::shared_ptr<LEDStripEffect> _tempEffect;

    void construct(bool clearTempEffect)
    {
        _bPlayAll = false;

        if (clearTempEffect && _tempEffect)
        {
            _clearTempEffectWhenExpired = true;

            // This is a hacky way to ensure that we start the correct effect after the temporary one
            _iCurrentEffect--;
        }
    }

    void LoadJSONAndMissingEffects(const JsonArrayConst& effectsArray)
    {
        std::set<int> loadedEffectNumbers;

        // Create effects from JSON objects, using the respective factories in g_EffectFactories
        auto& jsonFactories = g_EffectFactories.GetJSONFactories();

        for (auto effectObject : effectsArray)
        {
            int effectNumber = effectObject[PTY_EFFECTNR];
            auto factoryEntry = jsonFactories.find(effectNumber);

            if (factoryEntry == jsonFactories.end())
                continue;

            auto pEffect = factoryEntry->second(effectObject);

            if (pEffect != nullptr)
            {
                _vEffects.push_back(pEffect);
                loadedEffectNumbers.insert(effectNumber);
            }
        }

        // Now add missing effects from the default factory list
        auto &defaultFactories = g_EffectFactories.GetDefaultFactories();

        // We iterate manually, so we can use where we are as the starting point for a later inner loop
        for (auto iter = defaultFactories.begin(); iter != defaultFactories.end(); iter++)
        {
            int effectNumber = iter->EffectNumber;

            // If we've already loaded this effect (number) from JSON, we can move on to check the next one
            if (loadedEffectNumbers.count(effectNumber))
                continue;

            // We found an effect (number) in the default list that we have not yet loaded from JSON.
            //   So, we go through the rest of the default factory list to create and add to our effects
            //   list all instances of this effect.
            std::for_each(iter, defaultFactories.end(), [&](const EffectFactories::NumberedFactory& numberedFactory)
                {
                    if (numberedFactory.EffectNumber != effectNumber)
                        return;

                    auto pEffect = numberedFactory.Factory();
                    if (pEffect != nullptr)
                        _vEffects.push_back(pEffect);
                }
            );

            // Register that we added this effect number, so we don't add the respective effects more than once
            loadedEffectNumbers.insert(effectNumber);
        }
    }

    void ClearEffects()
    {
        _vEffects.clear();
        _cEnabled = 0;
    }

public:
    static const uint csFadeButtonSpeed = 15 * 1000;
    static const uint csSmoothButtonSpeed = 60 * 1000;

    EffectManager(std::shared_ptr<LEDStripEffect> effect, std::shared_ptr<GFXTYPE> gfx [])
        : _gfx(gfx)
    {
        debugV("EffectManager Splash Effect Constructor");

        if (effect->Init(_gfx))
            _tempEffect = effect;

        construct(false);
    }

    EffectManager(std::shared_ptr<GFXTYPE> gfx [])
        : _gfx(gfx)
    {
        debugV("EffectManager Constructor");

        LoadDefaultEffects();
    }

    EffectManager(const JsonObjectConst& jsonObject, std::shared_ptr<GFXTYPE> gfx [])
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

    void LoadDefaultEffects()
    {
        for (auto &numberedFactory : g_EffectFactories.GetDefaultFactories())
        {
            auto pEffect = numberedFactory.Factory();
            if (pEffect != nullptr)
                _vEffects.push_back(pEffect);
        }

        for (int i = 0; i < _vEffects.size(); i++)
            EnableEffect(i, true);

        SetInterval(DEFAULT_EFFECT_INTERVAL, true);

        construct(true);
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
    // If the JSON object includes an "eef" array, the function attempts to load each effect's enabled state from it.
    // If the index exceeds the "eef" array's size, the effect is enabled by default.
    //
    // The function also sets the effect interval from the "ivl" field in the JSON object, defaulting to a pre-defined value if the field isn't present.
    //
    // If the JSON object includes a "cei" field, the function sets the current effect index to this value.
    // If the value is greater than or equal to the number of effects, it defaults to the last effect in the vector.
    //
    // Lastly, the function calls the construct() method, indicating successful deserialization.

    virtual bool DeserializeFromJSON(const JsonObjectConst& jsonObject) override
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

        // "cei" contains the current effect index
        if (jsonObject.containsKey("cei"))
        {
            _iCurrentEffect = jsonObject["cei"];
            if (_iCurrentEffect >= EffectCount())
                _iCurrentEffect = EffectCount() - 1;
        }

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

    virtual bool SerializeToJSON(JsonObject& jsonObject) override
    {
        // Set JSON format version to be able to detect and manage future incompatible structural updates
        jsonObject[PTY_VERSION] = JSON_FORMAT_VERSION;

        jsonObject["ivl"] = _effectInterval;
        jsonObject["cei"] = _iCurrentEffect;

        JsonArray effectsArray = jsonObject.createNestedArray("efs");

        for (auto & effect : _vEffects)
        {
            JsonObject effectObject = effectsArray.createNestedObject();
            if (!(effect->SerializeToJSON(effectObject)))
                return false;
        }

        return true;
    }

    std::shared_ptr<GFXTYPE> operator[](size_t index) const
    {
        return _gfx[index];
    }

    // Must provide at least one drawing instance, like the first matrix or strip we are drawing on
    inline std::shared_ptr<GFXTYPE> g() const
    {
        return _gfx[0];
    }

    // ShowVU - Control whether VU meter should be draw.  Returns the previous state when set.

    virtual bool ShowVU(bool bShow)
    {
        bool bResult = _bShowVU;
        debugW("Setting ShowVU to %d\n", bShow);
        _bShowVU = bShow;

        // Erase any exising pixels since effects don't all clear each frame
        if (!bShow)
            _gfx[0]->setPixelsF(0, MATRIX_WIDTH, CRGB::Black);

        return bResult;
    }

    virtual bool IsVUVisible() const
    {
        return _bShowVU && GetCurrentEffect()->CanDisplayVUMeter();
    }

    // SetGlobalColor
    //
    // When a global color is set via the remote, we create a fill effect and assign it as the "remote effect"
    // which takes drawing precedence

    void SetGlobalColor(CRGB color)
    {
        debugW("Setting Global Color");

        CRGB oldColor = lastManualColor;
        lastManualColor = color;

        #if (USE_MATRIX)
                auto pMatrix = g();
                pMatrix->setPalette(CRGBPalette16(oldColor, color));
                pMatrix->PausePalette(true);
        #else
            std::shared_ptr<LEDStripEffect> effect;

            if (color == CRGB(CRGB::White))
                effect = std::make_shared<ColorFillEffect>(CRGB::White, 1);
            else

                #if ENABLE_AUDIO
                    #if SPECTRUM
                        effect = GetSpectrumAnalyzer(color, oldColor);
                    #else
                        effect = std::make_shared<MusicalPaletteFire>("Custom Fire", CRGBPalette16(CRGB::Black, color, CRGB::Yellow, CRGB::White), NUM_LEDS, 1, 8, 50, 1, 24, true, false);
                    #endif
                #else
                    effect = std::make_shared<PaletteFlameEffect>("Custom Fire", CRGBPalette16(CRGB::Black, color, CRGB::Yellow, CRGB::White), NUM_LEDS, 1, 8, 50, 1, 24, true, false);
                #endif

            if (effect->Init(g_aptrDevices))
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

        #if (USE_MATRIX)
            auto pMatrix = (*this)[0];
            pMatrix->PausePalette(false);
        #endif
    }

    void StartEffect()
    {
        // If there's a temporary effect override from the remote control active, we start that, else
        // we start the current regular effect

        std::shared_ptr<LEDStripEffect> & effect = _tempEffect ? _tempEffect : _vEffects[_iCurrentEffect];

        #if USE_MATRIX
            auto pMatrix = std::static_pointer_cast<LEDMatrixGFX>(_gfx[0]);
            pMatrix->SetCaption(effect->FriendlyName(), 3000);
            pMatrix->setLeds(LEDMatrixGFX::GetMatrixBackBuffer());
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
            effect->SetEnabled(true);

            if (_cEnabled < 1)
            {
                ClearRemoteColor(true);
            }
            _cEnabled++;

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

            _cEnabled--;
            if (_cEnabled < 1)
            {
                SetGlobalColor(CRGB::Black);
            }

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

    const size_t EnabledCount() const
    {
        return _cEnabled;
    }

    const size_t GetCurrentEffectIndex() const
    {
        return _iCurrentEffect;
    }

    const std::shared_ptr<LEDStripEffect> GetCurrentEffect() const
    {
        return _tempEffect ? _tempEffect : _vEffects[_iCurrentEffect];
    }

    const String & GetCurrentEffectName() const
    {
        if (_tempEffect)
            return _tempEffect->FriendlyName();

        return _vEffects[_iCurrentEffect]->FriendlyName();
    }

    // Change the current effect; marks the state as needing attention so this get noticed next frame

    void SetCurrentEffectIndex(size_t i, bool skipSave = false)
    {
        if (i >= _vEffects.size())
        {
            debugW("Invalid index for SetCurrentEffectIndex");
            return;
        }
        _iCurrentEffect = i;
        _effectStartTime = millis();

        StartEffect();

        if (!skipSave)
            SaveEffectManagerConfig();
    }

    uint GetTimeUsedByCurrentEffect() const
    {
        return millis() - _effectStartTime;
    }

    uint GetTimeRemainingForCurrentEffect() const
    {
        // If the Interval is set to zero, we treat that as an infinite interval and don't even look at the time used so far
        uint timeUsedByCurrentEffect = GetTimeUsedByCurrentEffect();
        uint interval = GetInterval();

        return timeUsedByCurrentEffect > interval ? 0 : (interval - timeUsedByCurrentEffect);
    }

    uint GetInterval() const
    {
        // This allows you to return a MaximumEffectTime and your effect won't be shown longer than that
        return min((_effectInterval == 0 ? std::numeric_limits<uint>::max() : _effectInterval), GetCurrentEffect()->MaximumEffectTime());
    }

    void CheckEffectTimerExpired()
    {
        // If interval is zero, the current effect never expires unless it thas a max effect time set

        if (_effectInterval == 0 && !GetCurrentEffect()->HasMaximumEffectTime())
            return;

        if (GetTimeUsedByCurrentEffect() >= GetInterval()) // See if it's time for a new effect yet
        {
            if (_clearTempEffectWhenExpired)
            {
                _tempEffect.reset();
                _clearTempEffectWhenExpired = false;
            }

            debugV("%ldms elapsed: Next Effect", millis() - _effectStartTime);
            NextEffect();
            debugV("Current Effect: %s", GetCurrentEffectName());
        }
    }

    void NextPalette()
    {
        auto g = _gfx[0].get();
        g->CyclePalette(1);
    }

    void PreviousPalette()
    {
        auto g = _gfx[0];
        g->CyclePalette(-1);
    }
    // Update to the next effect and abort the current effect.

    void NextEffect()
    {
        do
        {
            _iCurrentEffect++; //   ... if so advance to next effect
            _iCurrentEffect %= EffectCount();
            _effectStartTime = millis();
        } while (0 < _cEnabled && false == _bPlayAll && false == IsEffectEnabled(_iCurrentEffect));

        StartEffect();
        SaveEffectManagerConfig();
    }

    // Go back to the previous effect and abort the current one.

    void PreviousEffect()
    {
        do
        {
            if (_iCurrentEffect == 0)
                _iCurrentEffect = EffectCount() - 1;

            _iCurrentEffect--;
            _effectStartTime = millis();
        } while (0 < _cEnabled && false == _bPlayAll && false == IsEffectEnabled(_iCurrentEffect));

        StartEffect();
        SaveEffectManagerConfig();
    }

    bool Init()
    {

        for (int i = 0; i < _vEffects.size(); i++)
        {
            debugV("About to init effect %s", _vEffects[i]->FriendlyName());
            if (false == _vEffects[i]->Init(_gfx))
            {
                debugW("Could not initialize effect: %s\n", _vEffects[i]->FriendlyName());
                return false;
            }
            debugV("Loaded Effect: %s", _vEffects[i]->FriendlyName());

            // First time only, we ensure the data is cleared

            //_vEffects[i]->setAll(0,0,0);
        }
        debugV("First Effect: %s", GetCurrentEffectName());
        return true;
    }

    // EffectManager::Update
    //
    // Draws the current effect.  If gUIDirty has been set by an interrupt handler, it is reset here

    void Update()
    {
        if ((_gfx[0])->GetLEDCount() == 0)
            return;

        const float msFadeTime = EFFECT_CROSS_FADE_TIME;

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
            g_Fader = 255;
            return;
        }

        if (_effectInterval == 0)
        {
            g_Fader = 255;
            return;
        }

        int r = GetTimeRemainingForCurrentEffect();
        int e = GetTimeUsedByCurrentEffect();

        if (e < msFadeTime)
        {
            g_Fader = 255 * (e / msFadeTime); // Fade in
        }
        else if (r < msFadeTime)
        {
            g_Fader = 255 * (r / msFadeTime); // Fade out
        }
        else
        {
            g_Fader = 255; // No fade, not at start or end
        }
    }
};

extern DRAM_ATTR std::unique_ptr<EffectManager<GFXBase>> g_ptrEffectManager;
