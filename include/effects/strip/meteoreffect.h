//+--------------------------------------------------------------------------
//
// File:        MeteorPaletteEffect.h
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
//    Draws flying meteors
//
// History:     Apr-16-2019         Davepl      Created
//
//---------------------------------------------------------------------------

#pragma once

#include "soundanalyzer.h"

#include "ledstripeffect.h"

class MeteorChannel
{
  private:
    struct Meteor {
        float hue;
        float pos;
        float speed;
        bool  movingLeft;
    };

    std::vector<Meteor> _meteors;

  public:

    size_t        meteorCount;
    uint8_t       meteorSize;
    uint8_t       meteorTrailDecay;
    float         meteorSpeedMin;
    float         meteorSpeedMax;
    bool          meteorRandomDecay = true;
    const float   minTimeBetweenBeats = 0.6;

    MeteorChannel() {}

    virtual void Init(std::shared_ptr<GFXBase> pGFX, size_t count = 4, int size = 4, int decay = 3, float minSpeed = 0.5, float maxSpeed = 0.5)
    {
        meteorCount = count;
        meteorSize = size;
        meteorTrailDecay = decay;
        meteorSpeedMin = minSpeed;
        meteorSpeedMax = maxSpeed;

        _meteors.clear();
        _meteors.reserve(meteorCount);

        int hueval = HUE_RED;
        for (size_t i = 0; i < meteorCount; i++)
        {
            Meteor m;
            hueval = (hueval + 48) % 256;
            m.hue = static_cast<float>(hueval);

            // Distribute start positions
            m.pos = meteorCount <= 1 ? 0.0f : static_cast<float>(pGFX->GetLEDCount() / (meteorCount - 1) * i);

            m.speed = random_range(meteorSpeedMin, meteorSpeedMax);

            m.movingLeft = (i & 2); // Initial direction logic preserved from original (bit 1 checks 2,3,6,7...)

            // Special case for exactly 2 meteors: Red endpoints
            if (meteorCount == 2) {
                m.hue = static_cast<float>(HUE_RED);
                m.pos = (i == 0) ? 0.0f : static_cast<float>(pGFX->GetLEDCount() - 1);
            }
            _meteors.push_back(m);
        }
    }

    virtual void Reverse(int iMeteor)
    {
        if (iMeteor >= 0 && iMeteor < _meteors.size()) {
            _meteors[iMeteor].movingLeft = !_meteors[iMeteor].movingLeft;
        }
    }

    virtual void Draw(LEDStripEffect* owner)
    {
        auto pGFX = owner->g(0);
        const int ledCount = static_cast<int>(pGFX->GetLEDCount());
        static CHSV hsv;
        hsv.val = 255;
        hsv.sat = 255;

        // Fade brightness on all channels
        for (int j = 0; j < ledCount; j++)
        {
            if ((!meteorRandomDecay) || (random_range(0, 10) > 2))
            {
                owner->fadePixelToBlackOnAllChannelsBy(j, meteorTrailDecay);
            }
        }

        // Hoist invariant constants out of the loop
        const float minPos = static_cast<float>(meteorSize - 1);
        const float maxPos = static_cast<float>(ledCount - 1);

        for (auto& m : _meteors)
        {
            float spd = m.speed;

            #if ENABLE_AUDIO
                if (g_Analyzer.VURatio() > 1.0f)
                    spd *= g_Analyzer.VURatio();
            #endif

            // Update position based on direction
            m.pos += (m.movingLeft ? -spd : spd);

            // Bounce/Clamp logic
            // Change direction if we hit a wall.
            if (m.pos < minPos) m.movingLeft = false;
            else if (m.pos > maxPos) m.movingLeft = true;

            m.pos = std::clamp(m.pos, minPos, maxPos);

            // Draw the meteor head across all channels
            for (int j = 0; j < meteorSize; j++)
            {
                int x = static_cast<int>(m.pos) - j;
                if (x < ledCount && x >= 0)
                {
                    CRGB rgb;
                    m.hue += 0.025f;
                    if (m.hue >= 255.0f) m.hue -= 255.0f;

                    hsv.hue = static_cast<uint8_t>(m.hue);
                    hsv2rgb_rainbow(hsv, rgb);

                    // Blend with current pixel from primary device, then set on all channels
                    CRGB c = pGFX->getPixel(x);
                    nblend(c, rgb, 75);
                    owner->setPixelOnAllChannels(x, c);
                }
            }
        }
    }
};

class MeteorEffect : public EffectWithId<MeteorEffect>
{
  private:

    std::vector<MeteorChannel> _Meteors;

    int                        _cMeteors;
    uint8_t                    _meteorSize;
    uint8_t                    _meteorTrailDecay;
    float                      _meteorSpeedMin;
    float                      _meteorSpeedMax;

  public:

    MeteorEffect(int cMeteors = 4, unsigned int size = 4, unsigned int decay = 3, float minSpeed = 0.2, float maxSpeed = 0.2)
        : EffectWithId<MeteorEffect>("Color Meteors"),
          _Meteors(),
          _cMeteors(cMeteors),
          _meteorSize(size),
          _meteorTrailDecay(decay),
          _meteorSpeedMin(minSpeed),
          _meteorSpeedMax(maxSpeed)
    {
    }

    MeteorEffect(const JsonObjectConst& jsonObject)
        : EffectWithId<MeteorEffect>(jsonObject),
          _Meteors(),
          _cMeteors(jsonObject["mto"]),
          _meteorSize(jsonObject[PTY_SIZE]),
          _meteorTrailDecay(jsonObject["dcy"]),
          _meteorSpeedMin(jsonObject[PTY_MINSPEED]),
          _meteorSpeedMax(jsonObject[PTY_MAXSPEED])
    {
    }

    bool SerializeToJSON(JsonObject& jsonObject) override
    {
        auto jsonDoc = CreateJsonDocument();

        JsonObject root = jsonDoc.to<JsonObject>();
        LEDStripEffect::SerializeToJSON(root);

        jsonDoc["mto"] = _cMeteors;
        jsonDoc[PTY_SIZE] = _meteorSize;
        jsonDoc["dcy"] = _meteorTrailDecay;
        jsonDoc[PTY_MINSPEED] = _meteorSpeedMin;
        jsonDoc[PTY_MAXSPEED] = _meteorSpeedMax;

        return SetIfNotOverflowed(jsonDoc, jsonObject, __PRETTY_FUNCTION__);
    }

    bool Init(std::vector<std::shared_ptr<GFXBase>>& gfx) override
    {
        if (!LEDStripEffect::Init(gfx))
            return false;

        for (auto& device : gfx)
        {
            MeteorChannel channel;
            channel.Init(device, _cMeteors, _meteorSize, _meteorTrailDecay, _meteorSpeedMin, _meteorSpeedMax);
            _Meteors.push_back(channel);
        }

        return true;
    }

    void Draw() override
    {
        // Draw once using channel 0 state while writing to all channels
        if (!_Meteors.empty())
            _Meteors[0].Draw(this);
    }
};
