//+--------------------------------------------------------------------------
//
// File:        effectmanager_runtime.cpp
//
// This file is part of effectmanager.cpp; see that file header for additional context.
//
// Split scope: EffectManager runtime lifecycle, updates, transitions, and effect list mutations.
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

#if ENABLE_AUDIO
#include "effects/matrix/spectrumeffects.h"
#endif

extern allocated_unique_ptr<EffectFactories> g_ptrEffectFactories;
void SaveEffectManagerConfig();

#if ENABLE_AUDIO

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

void EffectManager::StartEffect()
{
    // Acquire both mutexes atomically. Separate sequential lock_guards form
    // an AB-BA cycle with sites that use scoped_lock(render, effect_mgr).
    std::scoped_lock guard(g_render_mutex, g_effect_manager_mutex);

    // If there's a temporary effect override from the remote control active, we start that, else
    // we start the current regular effect

    if (!_tempEffect && _vEffects.empty())
    {
        _iCurrentEffect = 0;
        _effectStartTime = millis();
        debugV("No effect to start");
        return;
    }

    if (!_vEffects.empty() && _iCurrentEffect >= _vEffects.size())
        _iCurrentEffect = 0;

    auto effect = _tempEffect ? _tempEffect : _vEffects[_iCurrentEffect];

    #if USE_HUB75
        auto& matrix = static_cast<HUB75GFX&>(*_gfx[0]);
        matrix.SetCaption(effect->FriendlyName(), CAPTION_TIME);
    #endif

    // Zero the whites plane on every graphics device at effect-switch.
    //
    // CCT-aware effects (WarmGlowEffect and friends) write to whites[]
    // every frame; effects that don't know about whites write only to
    // leds[]. Without this reset, the previous effect's W-LED state
    // persists through the transition and gets re-rendered by the
    // PixelFormat under the new effect's colors - the strip ends up
    // showing the old warm-white tint behind whatever colors the new
    // effect draws. (leds[] doesn't need to be reset here for the same
    // reason: every well-behaved effect overwrites the pixels it draws
    // each frame, and effects that deliberately persist state across
    // frames - like FireEffect's heat decay - want that persistence.
    // The whites plane has no equivalent persisting-effect right now,
    // so blanket-zeroing it on switch is the right semantic.)

    for (auto& device : _gfx)
    {
        if (device && device->whites)
            memset(device->whites, 0,
                   device->GetLEDCount() * sizeof(CRGBW));
    }

    effect->Start();
    _lastBeatSequence = g_Analyzer.LastBeat().sequence;
    _lastNearBeatSequence = g_Analyzer.LastNearBeat().sequence;
    _effectStartTime = millis();
}

void EffectManager::DispatchBeatIfNeeded()
{
#if ENABLE_AUDIO
    std::lock_guard effectGuard(g_effect_manager_mutex);

    // Beat callbacks are sequenced here so every active effect sees the same
    // detector output, including BeatEffectBase-derived effects via OnBeat().
    if (!_tempEffect && _vEffects.empty())
        return;

    auto& currentEffect = GetCurrentEffect();

    const auto nearBeat = g_Analyzer.LastNearBeat();
    if (nearBeat.sequence != 0 && nearBeat.sequence != _lastNearBeatSequence)
    {
        currentEffect.OnNearBeat(nearBeat);
        _lastNearBeatSequence = nearBeat.sequence;
    }

    const auto beat = g_Analyzer.LastBeat();
    if (beat.sequence != 0 && beat.sequence != _lastBeatSequence)
    {
        currentEffect.OnBeat(beat);
        _lastBeatSequence = beat.sequence;
    }
#endif
}

void EffectManager::SetTempEffect(std::shared_ptr<LEDStripEffect> effect)
{
    std::scoped_lock guard(g_render_mutex, g_effect_manager_mutex);
    _tempEffect = effect;
}

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
    if (_vEffects.empty())
        debugV("No local effects loaded");
    else
        debugV("First Effect: %s", GetCurrentEffectName().c_str());

    if (g_ptrSystem->GetDeviceConfig().ApplyGlobalColors())
        ApplyGlobalPaletteColors();

    return true;
}

bool EffectManager::ReinitializeEffects()
{
    std::scoped_lock guard(g_render_mutex, g_effect_manager_mutex);

    if (!Init())
        return false;

    if (_tempEffect)
    {
        debugV("About to re-init temp effect %s", _tempEffect->FriendlyName().c_str());
        if (!_tempEffect->Init(_gfx))
        {
            debugW("Could not re-initialize temporary effect: %s\n", _tempEffect->FriendlyName().c_str());
            return false;
        }
    }

    StartEffect();
    return true;
}

bool EffectManager::ShowVU(bool bShow)
{
    std::scoped_lock guard(g_render_mutex, g_effect_manager_mutex);

    auto& deviceConfig = g_ptrSystem->GetDeviceConfig();
    bool bResult = deviceConfig.ShowVUMeter();
    debugI("Setting ShowVU to %d\n", bShow);
    deviceConfig.SetShowVUMeter(bShow);

    // Erase any exising pixels since effects don't all clear each frame
    if (!bShow)
        _gfx[0]->setPixelsF(0, _gfx[0]->GetMatrixWidth(), CRGB::Black);

    return bResult;
}

bool EffectManager::IsVUVisible() const
{
    std::lock_guard effectGuard(g_effect_manager_mutex);
    return g_ptrSystem->GetDeviceConfig().ShowVUMeter() &&
           (_tempEffect || !_vEffects.empty()) &&
           GetCurrentEffect().CanDisplayVUMeter();
}


void EffectManager::ClearRemoteColor(bool retainRemoteEffect)
{
    std::scoped_lock guard(g_render_mutex, g_effect_manager_mutex);

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
    std::scoped_lock guard(g_render_mutex, g_effect_manager_mutex);

    debugI("Setting Global Color: %08lX\n", (unsigned long)(uint32_t)color);

    auto& deviceConfig = g_ptrSystem->GetDeviceConfig();
    deviceConfig.SetColorSettings(color, deviceConfig.GlobalColor());

    ApplyGlobalPaletteColors();
}

void EffectManager::ApplyGlobalPaletteColors()
{
    std::scoped_lock guard(g_render_mutex, g_effect_manager_mutex);

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

void EffectManager::EnableEffect(size_t i, bool skipSave)
{
    // Web, remote, and render tasks all touch the effect list/current effect.
    // Mutations take both locks so a UI request cannot reorder/delete/settings-
    // mutate an effect while the draw loop is inside that effect's Draw().
    std::scoped_lock guard(g_render_mutex, g_effect_manager_mutex);

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

        {
            std::lock_guard listenerGuard(_listenerMutex);
            INFORM_EVENT_LISTENERS(_effectEventListeners, IEffectEventListener::OnEffectEnabledStateChanged, i, true);
        }
    }
}

void EffectManager::DisableEffect(size_t i, bool skipSave)
{
    std::scoped_lock guard(g_render_mutex, g_effect_manager_mutex);

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

        {
            std::lock_guard listenerGuard(_listenerMutex);
            INFORM_EVENT_LISTENERS(_effectEventListeners, IEffectEventListener::OnEffectEnabledStateChanged, i, false);
        }
    }
}

void EffectManager::MoveEffect(size_t from, size_t to)
{
    std::scoped_lock guard(g_render_mutex, g_effect_manager_mutex);

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

    {
        std::lock_guard listenerGuard(_listenerMutex);
        INFORM_EVENT_LISTENERS(_effectEventListeners, IEffectEventListener::OnEffectListDirty);
    }
}

// Adds an effect to the effect list and enables it. If an effect is added that is already in the effect list then the result
//   is undefined but potentially messy.
bool EffectManager::AppendEffect(std::shared_ptr<LEDStripEffect>& effect)
{
    std::scoped_lock guard(g_render_mutex, g_effect_manager_mutex);

    if (!effect->Init(_gfx))
        return false;

    _vEffects.push_back(effect);
    EnableEffect(_vEffects.size() - 1, true);

    SaveEffectManagerConfig();

    {
        std::lock_guard listenerGuard(_listenerMutex);
        INFORM_EVENT_LISTENERS(_effectEventListeners, IEffectEventListener::OnEffectListDirty);
    }

    return true;
}

// Creates a copy of an existing effect in the list. Note that the effect is created but not yet added to the effect list;
//   use the AppendEffect() function for that.
std::shared_ptr<LEDStripEffect> EffectManager::CopyEffect(size_t index)
{
    std::scoped_lock guard(g_render_mutex, g_effect_manager_mutex);

    if (index >= _vEffects.size())
    {
        debugW("Invalid index for CopyEffect");
        return nullptr;
    }

    auto& sourceEffect = _vEffects[index];

    const auto& jsonEffectFactories = g_ptrEffectFactories->GetJSONFactories();
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

// Deletes an effect from the effect list. Note that core effects cannot be deleted.
bool EffectManager::DeleteEffect(size_t index)
{
    std::scoped_lock guard(g_render_mutex, g_effect_manager_mutex);

    if (index >= _vEffects.size())
    {
        debugW("Invalid index for DeleteEffect");
        return false;
    }

    if (_vEffects[index]->IsCoreEffect())
        return false;

    DisableEffect(index, true);

    if (index == _iCurrentEffect && _vEffects.size() > 1)
        NextEffect(true);

    _vEffects.erase(_vEffects.begin() + index);

    if (_vEffects.empty())
    {
        _iCurrentEffect = 0;
    }
    else if (index <= _iCurrentEffect)
    {
        if (_iCurrentEffect > 0)
            _iCurrentEffect--;
        else if (_iCurrentEffect >= _vEffects.size())
            _iCurrentEffect = _vEffects.size() - 1;
    }

    SaveCurrentEffectIndex();
    SaveEffectManagerConfig();

    {
        std::lock_guard listenerGuard(_listenerMutex);
        INFORM_EVENT_LISTENERS(_effectEventListeners, IEffectEventListener::OnEffectListDirty);
    }

    return true;
}

void EffectManager::CheckEffectTimerExpired()
{
    std::scoped_lock guard(g_render_mutex, g_effect_manager_mutex);

    if (!_tempEffect && _vEffects.empty())
        return;

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
    std::scoped_lock guard(g_render_mutex, g_effect_manager_mutex);

    if (_vEffects.empty())
    {
        _iCurrentEffect = 0;
        _effectStartTime = millis();
        return;
    }

    auto enabled = AreEffectsEnabled();

    do
    {
        _iCurrentEffect++;
        _iCurrentEffect %= EffectCount();
        _effectStartTime = millis();
    } while (enabled && false == _bPlayAll && false == IsEffectEnabled(_iCurrentEffect));

    StartEffect();
    if (!skipSave)
        SaveCurrentEffectIndex();

    {
        std::lock_guard listenerGuard(_listenerMutex);
        INFORM_EVENT_LISTENERS(_effectEventListeners, IEffectEventListener::OnCurrentEffectChanged, _iCurrentEffect);
    }
}

// Go back to the previous effect and abort the current one.

void EffectManager::PreviousEffect()
{
    std::scoped_lock guard(g_render_mutex, g_effect_manager_mutex);

    if (_vEffects.empty())
    {
        _iCurrentEffect = 0;
        _effectStartTime = millis();
        return;
    }

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
