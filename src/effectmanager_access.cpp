//+--------------------------------------------------------------------------
//
// File:        effectmanager_access.cpp
//
// This file is part of effectmanager.cpp; see that file header for additional context.
//
// Split scope: EffectManager accessors, listener management, and read-side queries.
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

void EffectManager::ReportNewFrameAvailable()
{
    // Listener vectors store references whose owners may deregister on other
    // tasks. Hold this mutex while iterating so remove/destruction cannot race
    // a callback dispatch.

    std::lock_guard listenerGuard(_listenerMutex);
    INFORM_EVENT_LISTENERS(_frameEventListeners, IFrameEventListener::OnNewFrameAvailable);
}

void EffectManager::AddFrameEventListener(IFrameEventListener& listener)
{
    std::lock_guard listenerGuard(_listenerMutex);
    _frameEventListeners.emplace_back(listener);
}

void EffectManager::RemoveFrameEventListener(IFrameEventListener& listener)
{
    // Match by address — reference_wrapper has no operator== so we compare
    // the underlying object pointers. erase/remove_if so we drop every entry
    // even if the same listener was registered more than once.
    // Robert may have a better or less obtuse way to do this, but it works
    // and it's not like we have thousands of listeners.  Change at will!

    std::lock_guard listenerGuard(_listenerMutex);
    _frameEventListeners.erase(
        std::remove_if(_frameEventListeners.begin(), _frameEventListeners.end(),
                       [&](const std::reference_wrapper<IFrameEventListener>& w)
                       { return &w.get() == &listener; }),
        _frameEventListeners.end());
}

void EffectManager::AddEffectEventListener(IEffectEventListener& listener)
{
    std::lock_guard listenerGuard(_listenerMutex);
    _effectEventListeners.emplace_back(listener);
}

std::vector<std::shared_ptr<GFXBase>> & EffectManager::GetBaseGraphics()
{
    return _gfx;
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
    std::lock_guard effectGuard(g_effect_manager_mutex);
    if (i >= _vEffects.size())
    {
        debugW("Invalid index for IsEffectEnabled");
        return false;
    }
    return _vEffects[i]->IsEnabled();
}

std::vector<std::shared_ptr<LEDStripEffect>> EffectManager::EffectsList() const
{
    // Return a stable snapshot instead of a reference to the live vector. Web,
    // debug, and render tasks can otherwise hold an iterator/reference while
    // another task reorders or deletes effects.
    std::lock_guard effectGuard(g_effect_manager_mutex);
    return _vEffects;
}

std::shared_ptr<LEDStripEffect> EffectManager::EffectAt(size_t index) const
{
    std::lock_guard effectGuard(g_effect_manager_mutex);
    return index < _vEffects.size() ? _vEffects[index] : nullptr;
}

bool EffectManager::IsCoreEffect(size_t index) const
{
    auto effect = EffectAt(index);
    return effect && effect->IsCoreEffect();
}

size_t EffectManager::EffectCount() const
{
    std::lock_guard effectGuard(g_effect_manager_mutex);
    return _vEffects.size();
}

bool EffectManager::AreEffectsEnabled() const
{
    std::lock_guard effectGuard(g_effect_manager_mutex);
    return std::any_of(_vEffects.begin(), _vEffects.end(), [](const auto& pEffect){ return pEffect->IsEnabled(); } );
}

bool EffectManager::HasCurrentEffect() const
{
    std::lock_guard effectGuard(g_effect_manager_mutex);
    return _tempEffect || !_vEffects.empty();
}

size_t EffectManager::GetCurrentEffectIndex() const
{
    std::lock_guard effectGuard(g_effect_manager_mutex);
    return _iCurrentEffect;
}

LEDStripEffect& EffectManager::GetCurrentEffect() const
{
    std::lock_guard effectGuard(g_effect_manager_mutex);
    if (!_tempEffect && _vEffects.empty())
        throw std::runtime_error("No current effect");

    const size_t index = _vEffects.empty() ? 0 : std::min(_iCurrentEffect, _vEffects.size() - 1);
    return *(_tempEffect ? _tempEffect : _vEffects[index]);
}

String EffectManager::GetCurrentEffectName() const
{
    // Return a copy while holding the effect lock. Returning a reference here
    // was unsafe because another task can switch/delete/reinitialize effects
    // immediately after the lock is released.
    std::lock_guard effectGuard(g_effect_manager_mutex);
    if (_tempEffect)
        return _tempEffect->FriendlyName();

    if (_vEffects.empty())
        return "No Effect";

    return _vEffects[std::min(_iCurrentEffect, _vEffects.size() - 1)]->FriendlyName();
}
