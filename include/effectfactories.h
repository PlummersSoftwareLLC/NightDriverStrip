#pragma once

//+--------------------------------------------------------------------------
//
// File:        effectfactories.h
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
//    This file contains the effect factory container class and its
//    supporting types.
//
//
// History:     May-25-2023         Rbergen     Created for NightDriverStrip
//
//---------------------------------------------------------------------------

#include "globals.h"

#include <cstdint>
#include <functional>
#include <map>
#include <memory>
#include <type_traits>
#include <vector>

#include <ArduinoJson.h>

#include "effects.h"
#include "hashing.h"

class LEDStripEffect;

// Use std::function so factories can be capturing lambdas (needed for macro-free registration)
using DefaultEffectFactory = std::function<std::shared_ptr<LEDStripEffect>()>;
using JSONEffectFactory = std::function<std::shared_ptr<LEDStripEffect>(const JsonObjectConst&)>;

class EffectFactories
{
  public:
    class NumberedFactory
    {
        EffectId effectId;
        DefaultEffectFactory factory;
        FactoryId factoryId { 0 };

      public:
        bool LoadDisabled = false;

        NumberedFactory(EffectId effectId, const DefaultEffectFactory& factory, FactoryId factoryId)
          : effectId(effectId),
            factory(factory),
            factoryId(factoryId)
        {}

        EffectId EffectID() const { return effectId; }
        std::shared_ptr<LEDStripEffect> CreateEffect() const;
        FactoryId FactoryID() const { return factoryId; }
    };

  private:

    std::vector<NumberedFactory, psram_allocator<NumberedFactory>> defaultFactories;
    std::map<EffectId, JSONEffectFactory, std::less<EffectId>, psram_allocator<std::pair<const EffectId, JSONEffectFactory>>> jsonFactories;
    String hashString;

  public:

    const std::vector<NumberedFactory, psram_allocator<NumberedFactory>>& GetDefaultFactories() const { return defaultFactories; }
    const std::map<EffectId, JSONEffectFactory, std::less<EffectId>, psram_allocator<std::pair<const EffectId, JSONEffectFactory>>>& GetJSONFactories() const { return jsonFactories; }

    NumberedFactory& AddEffect(EffectId effectId, const DefaultEffectFactory& defaultFactory, const JSONEffectFactory& jsonFactory, FactoryId factoryId = 0);

    bool IsEmpty() const { return defaultFactories.empty() && jsonFactories.empty(); }

    void ClearDefaultFactories();

    std::vector<FactoryId> FactoryIDs() const;

    const String& HashString() const;

    const String& HashString(const String& str);
};

// ------------------------------------------------------------
// Factory builder templates (used in effects.cpp RegisterAll calls)
// ------------------------------------------------------------

template<typename T>
using remove_cvref_t = std::remove_cv_t<std::remove_reference_t<T>>;

template<typename T>
constexpr EffectId effect_id_of_type() {
    static_assert(std::is_base_of_v<LEDStripEffect, remove_cvref_t<T>>,
                  "Type must derive from LEDStripEffect");
    return remove_cvref_t<T>::ID;
}

template<typename TEffect, typename... Args>
constexpr FactoryId factory_id_of_instance(const Args&... args)
{
    FactoryId h = fnv1a::hash<FactoryId>("effect");
    h = fnv1a::hash(effect_id_of_type<TEffect>(), h);
    h = fnv1a::hash_pack(h, args...);
    return h;
}

template<typename TEffect, typename... Args>
inline EffectFactories::NumberedFactory& AddEffect(EffectFactories& factories, Args&&... args)
{
    return factories.AddEffect(
        effect_id_of_type<TEffect>(),
        [=]() -> std::shared_ptr<LEDStripEffect> { return make_shared_psram<TEffect>(args...); },
        [](const JsonObjectConst& jsonObject) -> std::shared_ptr<LEDStripEffect> { return make_shared_psram<TEffect>(jsonObject); },
        factory_id_of_instance<TEffect>(args...)
    );
}

template<typename... Adders>
inline void RegisterAll(EffectFactories& factories, Adders&&... adders)
{
    (static_cast<void>(adders(factories)), ...);
}

template<typename TEffect, typename... Args>
inline auto Effect(Args&&... args)
{
    return [=](EffectFactories& factories) -> EffectFactories::NumberedFactory&
    {
        return AddEffect<TEffect>(factories, args...);
    };
}

template<typename F>
inline auto Disabled(F adder)
{
    return [=](EffectFactories& factories) -> EffectFactories::NumberedFactory&
    {
        auto& nf = adder(factories);
        nf.LoadDisabled = true;
        return nf;
    };
}
