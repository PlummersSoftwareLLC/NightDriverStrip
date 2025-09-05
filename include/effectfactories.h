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

#pragma once

#include <vector>
#include <map>
#include <functional>
#include <memory>
#include <algorithm>
#include <cstdint>
#include <ArduinoJson.h>
#include <esp_attr.h>
#include "ledstripeffect.h"

// Use std::function so factories can be capturing lambdas (needed for macro-free registration)
using DefaultEffectFactory = std::function<std::shared_ptr<LEDStripEffect>()>;
using JSONEffectFactory = std::function<std::shared_ptr<LEDStripEffect>(const JsonObjectConst&)>;

// -----------------------------------------------------------------------------
// Class: EffectFactories
//
// This class manages a collection of default and JSON effect factories. Each
// factory is associated with a unique effect number. Factories are categorized
// into two types: default and JSON, managed separately using respective containers.
//
// Sub-Structure:
//
// NumberedFactory: This class represents a default factory coupled with its unique
//                  effect number and an optional factory ID. It also includes a flag
//                  that indicates whether the effect that is created using the factory
//                  function should be set to "disabled" immediately after creation.
//                  Besides these member variables, the class includes a function to
//                  create the effect in accordance with an instance's member variables'
//                  values.
//
// Member Variables:
//
// defaultFactories: A vector of NumberedFactory instances. Each NumberedFactory holds an
//                   effect number, a DefaultEffectFactory instance, and an optional factory ID.
// jsonFactories: A map linking each effect number to its corresponding JSONEffectFactory.
// hashString: A string that can store a hash of the factory configuration.
//
// Member Functions:
//
// GetDefaultFactories: Returns a const reference to the vector of default factories.
// GetJSONFactories: Returns a const reference to the map of JSON factories.
// AddEffect: Adds a new effect factory into the collection. It takes four parameters:
//            - An effect number which is an integer.
//            - A DefaultEffectFactory reference.
//            - A JSONEffectFactory reference.
//            - An optional factory ID.
//            It returns a reference to the NumberedFactory that was created around the
//            DefaultEffectFactory.
// IsEmpty: Returns a boolean indicating whether both defaultFactories and jsonFactories are empty.
// ClearDefaultFactories: Clears the vector of default factories and the hash string.
// FactoryIDs: Returns a vector of the factory IDs from the default factories.
// HashString (getter): Returns the stored hash string. Throws an error if it hasn't been set.
// HashString (setter): Sets the hash string.
//
// -----------------------------------------------------------------------------

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

        EffectId EffectID() const
        {
            return effectId;
        }

        std::shared_ptr<LEDStripEffect> CreateEffect() const
        {
            auto pEffect = factory();

            // Disable the effect if we have one and we were asked to do so
            if (pEffect && LoadDisabled)
                pEffect->SetEnabled(false);

            return pEffect;
        }

        FactoryId FactoryID() const
        {
            return factoryId;
        }
    };

  private:

    std::vector<NumberedFactory, psram_allocator<NumberedFactory>> defaultFactories;
    std::map<EffectId, JSONEffectFactory, std::less<EffectId>, psram_allocator<std::pair<const EffectId, JSONEffectFactory>>> jsonFactories;
    String hashString;

  public:

    const std::vector<NumberedFactory, psram_allocator<NumberedFactory>>& GetDefaultFactories() const
    {
        return defaultFactories;
    }

    const std::map<EffectId, JSONEffectFactory, std::less<EffectId>, psram_allocator<std::pair<const EffectId, JSONEffectFactory>>>& GetJSONFactories() const
    {
        return jsonFactories;
    }

    NumberedFactory& AddEffect(EffectId effectId, const DefaultEffectFactory& defaultFactory, const JSONEffectFactory& jsonFactory, FactoryId factoryId = 0)
    {
        auto& numberedFactory = defaultFactories.emplace_back(effectId, defaultFactory, factoryId);
        jsonFactories.try_emplace(effectId, jsonFactory);

        return numberedFactory;
    }

    bool IsEmpty() const
    {
        return defaultFactories.empty() && jsonFactories.empty();
    }

    void ClearDefaultFactories()
    {
        defaultFactories.clear();
        hashString.clear();
    }

    // Return the list of stored factory IDs (callers can hash/order as needed)
    std::vector<FactoryId> FactoryIDs() const
    {
        std::vector<FactoryId> ids;
        ids.reserve(defaultFactories.size());
        for (const auto& nf : defaultFactories)
            ids.push_back(nf.FactoryID());

        return ids;
    }

    const String& HashString() const
    {
        if (hashString.isEmpty())
            throw std::runtime_error("Attempt to retrieve unset hash string");

        return hashString;
    }

    const String& HashString(const String& str)
    {
        // Accept value if length is appropriate for FactoryId or empty
        if (str.length() == fnv1a::hash_string_length<FactoryId>() || str.isEmpty())
            hashString = str;

        return hashString;
    }
};
