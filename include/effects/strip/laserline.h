//+--------------------------------------------------------------------------
//
// File:        LaserLine.h
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
//    Sound reactive laser "shot" that travels down a strip
//
// History:     Apr-16-2019         Davepl      Created
//
//---------------------------------------------------------------------------

#pragma once

class LaserShot
{
    float         _position = 0.0;
    float         _speed    = 10.0;
    float         _size     = 10.0;
    uint8_t       _hue      = 0;

public:

    LaserShot(float position, float speed, float size, uint8_t hue)
    {
        _position = position;
        _speed    = speed;
        _size     = size;
        _hue      = hue;
    }

    virtual bool Update(float elapsed)
    {
        _hue += _speed * elapsed;
        _position += _speed * elapsed;
        if (_position > NUM_LEDS)
            return false;
        return true;
    }

    virtual void Draw(std::shared_ptr<GFXBase> pGFX)
    {
        for (float d = 0; d < _size && d + _position < NUM_LEDS; d++)
            pGFX->setPixelsF(_position + d, 1.0, CHSV(_hue + d, 255, 255), true);
    }
};

class LaserLineEffect : public BeatEffectBase, public LEDStripEffect
{
  private:
    std::vector<LaserShot>      _shots;
    std::shared_ptr<GFXBase>    _gfx;
    float                      _defaultSize;
    float                      _defaultSpeed;

  public:

    LaserLineEffect(float speed, float size)
        : BeatEffectBase(1.50, 0.00),
          LEDStripEffect(EFFECT_STRIP_LASER_LINE, "LaserLine"),
          _defaultSize(size),
          _defaultSpeed(speed)
    {
    }

    LaserLineEffect(const JsonObjectConst& jsonObject)
        : BeatEffectBase(1.50, 0.00),
          LEDStripEffect(jsonObject),
          _defaultSize(jsonObject[PTY_SIZE]),
          _defaultSpeed(jsonObject[PTY_SPEED])
    {
    }

    bool SerializeToJSON(JsonObject& jsonObject) override
    {
        StaticJsonDocument<LEDStripEffect::_jsonSize> jsonDoc;

        JsonObject root = jsonDoc.to<JsonObject>();
        LEDStripEffect::SerializeToJSON(root);

        jsonDoc[PTY_SIZE] = _defaultSize;
        jsonDoc[PTY_SPEED] = _defaultSpeed;

        assert(!jsonDoc.overflowed());

        return jsonObject.set(jsonDoc.as<JsonObjectConst>());
    }

    bool Init(std::vector<std::shared_ptr<GFXBase>>& gfx) override
    {
        debugW("Initialized LaserLine Effect");
        _gfx = gfx[0];
        if (!LEDStripEffect::Init(gfx))
            return false;
        return true;
    }

    void Draw()
    {
        ProcessAudio();

        fadeAllChannelsToBlackBy(200);

        auto it = _shots.begin();
        while(it != _shots.end())
        {
            it->Draw(_gfx);
            if (!it->Update(g_Values.AppTime.LastFrameTime()))
                _shots.erase(it);
            else
                it++;
        }
    }

    virtual void HandleBeat(bool bMajor, float elapsed, float span)
    {
        _shots.push_back(LaserShot(0.0, _defaultSpeed, _defaultSize, random8()));
    };
};
