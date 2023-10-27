//+--------------------------------------------------------------------------
//
// File:        DoublePaletteEffect.h
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
// Description:
//
//    Draws two intersecting palettes
//
// History:     Apr-16-2019         Davepl      Created
//
//---------------------------------------------------------------------------

#pragma once

#include "effects.h"

class DoublePaletteEffect : public LEDStripEffect
{
  private:

    PaletteEffect   _PaletteEffect1;
    PaletteEffect   _PaletteEffect2;

  public:

    DoublePaletteEffect()
     :  LEDStripEffect(EFFECT_STRIP_DOUBLE_PALETTE, "Double Palette"),
        _PaletteEffect1(RainbowColors_p, 1.0,  0.03,  4.0, 3, 3, LINEARBLEND, false, 0.5),
        _PaletteEffect2(RainbowColors_p, 1.0, -0.03, -4.0, 3, 3, LINEARBLEND, false, 0.5)
    {
    }

    DoublePaletteEffect(const JsonObjectConst&  jsonObject)
      : LEDStripEffect(jsonObject),
        _PaletteEffect1(jsonObject["pt1"].as<JsonObjectConst>()),
        _PaletteEffect2(jsonObject["pt2"].as<JsonObjectConst>())
    {
    }

    bool SerializeToJSON(JsonObject& jsonObject) override
    {
        AllocatedJsonDocument jsonDoc(LEDStripEffect::_jsonSize + 768);

        JsonObject root = jsonDoc.to<JsonObject>();
        LEDStripEffect::SerializeToJSON(root);

        JsonObject paletteObj = jsonDoc.createNestedObject("pt1");
        _PaletteEffect1.SerializeToJSON(paletteObj);
        paletteObj = jsonDoc.createNestedObject("pt2");
        _PaletteEffect2.SerializeToJSON(paletteObj);

        assert(!jsonDoc.overflowed());

        return jsonObject.set(jsonDoc.as<JsonObjectConst>());
    }

    bool Init(std::vector<std::shared_ptr<GFXBase>>& gfx) override
    {
        LEDStripEffect::Init(gfx);
        if (!_PaletteEffect1.Init(gfx) || !_PaletteEffect2.Init(gfx))
            return false;
        return true;
    }

    void Draw() override
    {
        setAllOnAllChannels(0,0,0);
        _PaletteEffect1.Draw();
        _PaletteEffect2.Draw();
    }

};

