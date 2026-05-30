//+--------------------------------------------------------------------------
//
// File:        effectmanager_playback.cpp
//
// This file is part of effectmanager.cpp; see that file header for additional context.
//
// Split scope: EffectManager playback timing, index/palette navigation, and default-load control.
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

extern allocated_unique_ptr<EffectFactories> g_ptrEffectFactories;
void SaveEffectManagerConfig();

void EffectManager::SetCurrentEffectIndex(size_t i)
{
    std::scoped_lock guard(g_render_mutex, g_effect_manager_mutex);

    if (i >= _vEffects.size())
    {
        debugW("Invalid index for SetCurrentEffectIndex");
        return;
    }
    _iCurrentEffect = i;
    _effectStartTime = millis();

    StartEffect();
    SaveCurrentEffectIndex();

    {
        std::lock_guard listenerGuard(_listenerMutex);
        INFORM_EVENT_LISTENERS(_effectEventListeners, IEffectEventListener::OnCurrentEffectChanged, i);
    }
}

uint EffectManager::GetTimeUsedByCurrentEffect() const
{
    std::lock_guard effectGuard(g_effect_manager_mutex);
    return millis() - _effectStartTime;
}

uint EffectManager::GetTimeRemainingForCurrentEffect() const
{
    std::lock_guard effectGuard(g_effect_manager_mutex);
    if (!_tempEffect && _vEffects.empty())
        return 0;

    // If the Interval is set to zero, we treat that as an infinite interval and don't even look at the time used so far
    uint timeUsedByCurrentEffect = GetTimeUsedByCurrentEffect();
    uint interval = GetEffectiveInterval();

    return timeUsedByCurrentEffect > interval ? 0 : (interval - timeUsedByCurrentEffect);
}

uint EffectManager::GetEffectiveInterval() const
{
    std::lock_guard effectGuard(g_effect_manager_mutex);
    if (!_tempEffect && _vEffects.empty())
        return IsIntervalEternal() ? std::numeric_limits<uint>::max() : _effectInterval;

    const size_t index = _vEffects.empty() ? 0 : std::min(_iCurrentEffect, _vEffects.size() - 1);
    auto& currentEffect = *(_tempEffect ? _tempEffect : _vEffects[index]);
    // This allows you to return a MaximumEffectTime and your effect won't be shown longer than that
    return min((IsIntervalEternal() ? std::numeric_limits<uint>::max() : _effectInterval),
               (currentEffect.HasMaximumEffectTime() ? currentEffect.MaximumEffectTime() : std::numeric_limits<uint>::max()));
}

uint EffectManager::GetInterval() const
{
    std::lock_guard effectGuard(g_effect_manager_mutex);
    return _effectInterval;
}

bool EffectManager::IsIntervalEternal() const
{
    std::lock_guard effectGuard(g_effect_manager_mutex);
    return _effectInterval == 0;
}

void EffectManager::NextPalette()
{
    std::scoped_lock guard(g_render_mutex, g_effect_manager_mutex);
    debugV("EffectManager::NextPalette");
    g().CyclePalette(1);
}

void EffectManager::PreviousPalette()
{
    std::scoped_lock guard(g_render_mutex, g_effect_manager_mutex);
    debugV("EffectManager::PreviousPalette");
    g().CyclePalette(-1);
}

void EffectManager::LoadDefaultEffects()
{
    std::scoped_lock guard(g_render_mutex, g_effect_manager_mutex);

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
