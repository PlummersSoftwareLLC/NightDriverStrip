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

    std::vector<NumberedFactory> defaultFactories;
    std::map<int, JSONEffectFactory> jsonFactories;

  public:

    const std::vector<NumberedFactory>& GetDefaultFactories()
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

extern DRAM_ATTR EffectFactories g_EffectFactories;
