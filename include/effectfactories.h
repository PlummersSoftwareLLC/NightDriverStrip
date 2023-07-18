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
#include <ArduinoJson.h>
#include <esp_attr.h>
#include "ledstripeffect.h"

using DefaultEffectFactory = std::shared_ptr<LEDStripEffect>(*)();
using JSONEffectFactory = std::shared_ptr<LEDStripEffect>(*)(const JsonObjectConst&);

// -----------------------------------------------------------------------------
// Class: EffectFactories
// 
// This class manages a collection of default and JSON effect factories. Each 
// factory is associated with a unique effect number. Factories are categorized 
// into two types: default and JSON, managed separately using respective containers.
//
// Sub-Structure:
//
// NumberedFactory: This struct represents a default factory coupled with its unique effect number.
//
// Member Variables:
//
// defaultFactories: A vector of NumberedFactory instances. Each NumberedFactory holds an 
//                   effect number and a DefaultEffectFactory instance.
// jsonFactories: A map linking each effect number to its corresponding JSONEffectFactory.
//
// Member Functions:
//
// GetDefaultFactories: Returns a const reference to the vector of default factories.
// GetJSONFactories: Returns a const reference to the map of JSON factories.
// AddEffect: Adds a new effect into the collection. It takes three parameters:
//            - An effect number which is an integer.
//            - A DefaultEffectFactory reference.
//            - A JSONEffectFactory reference.
// IsEmpty: Returns a boolean indicating whether both defaultFactories and jsonFactories are empty.
// ClearDefaultFactories: Clears the vector of default factories.
//
// Note: The 'extern DRAM_ATTR EffectFactories g_EffectFactories;' line at the bottom of this code 
//       indicates that an EffectFactories object named 'g_EffectFactories' is declared elsewhere, 
//       and it is available for use throughout the program.
// -----------------------------------------------------------------------------

class EffectFactories
{
  public:

    struct NumberedFactory
    {
        int EffectNumber;
        DefaultEffectFactory Factory;

        NumberedFactory(int effectNumber, const DefaultEffectFactory& factory)
          : EffectNumber(effectNumber),
            Factory(factory)
        {}

        NumberedFactory()
        {}
    };

  private:

    std::vector<NumberedFactory, psram_allocator<NumberedFactory>> defaultFactories;
    std::map<int, JSONEffectFactory> jsonFactories;

  public:

    const std::vector<NumberedFactory, psram_allocator<NumberedFactory>>& GetDefaultFactories()
    {
        return defaultFactories;
    }

    const std::map<int, JSONEffectFactory>& GetJSONFactories()
    {
        return jsonFactories;
    }

    void AddEffect(int effectNumber, const DefaultEffectFactory& defaultFactory, const JSONEffectFactory& jsonFactory)
    {
        defaultFactories.emplace_back(effectNumber, defaultFactory);

        if (jsonFactories.count(effectNumber) == 0)
            jsonFactories[effectNumber] = jsonFactory;
    }

    bool IsEmpty()
    {
        return defaultFactories.size() == 0 && jsonFactories.size() == 0;
    }

    void ClearDefaultFactories()
    {
      defaultFactories.clear();
    }
};