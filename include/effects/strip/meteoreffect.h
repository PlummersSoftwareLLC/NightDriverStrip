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

class MeteorChannel
{
    std::vector<float> hue;
    std::vector<float> iPos;
    std::vector<bool>  bLeft;
    std::vector<float> speed;
    std::vector<float> lastBeat;

public:

    size_t        meteorCount;
    uint8_t       meteorSize;
    uint8_t       meteorTrailDecay;
    float        meteorSpeedMin;
    float        meteorSpeedMax;
    bool          meteorRandomDecay = true;
    const float  minTimeBetweenBeats = 0.6;

    MeteorChannel()
    {

    }

    virtual void Init(std::shared_ptr<GFXBase> pGFX, size_t meteors = 4, int size = 4, int decay = 3, float minSpeed = 0.5, float maxSpeed = 0.5)
    {
        meteorCount = meteors;
        meteorSize = size;
        meteorTrailDecay = decay;
        meteorSpeedMin = minSpeed;
        meteorSpeedMax = maxSpeed;

        hue.resize(meteors);
        iPos.resize(meteors);
        bLeft.resize(meteors);
        speed.resize(meteors);
        lastBeat.resize(meteors);

        static int hueval = HUE_RED;
        for (int i = 0; i < meteorCount; i++)
        {
            hueval = hueval + 48;
            hueval %= 256;
            hue[i] = hueval;
            iPos[i] = (pGFX->GetLEDCount() / meteorCount) * i;
            speed[i] = random_range(meteorSpeedMin, meteorSpeedMax);
            lastBeat[i] = g_Values.AppTime.FrameStartTime();
            bLeft[i] = i & 2;
        }
    }

    virtual void Reverse(int iMeteor)
    {
        bLeft[iMeteor] = !bLeft[iMeteor];
    }

    virtual void Draw(std::shared_ptr<GFXBase> pGFX)
    {
        static CHSV hsv;
        hsv.val = 255;
        hsv.sat = 240;

        for (int j = 0; j<pGFX->GetLEDCount(); j++)                         // fade brightness all LEDs one step
        {
            if ((!meteorRandomDecay) || (random_range(0, 10)>2))            // BUGBUG Was 5 for everything before atomlight
            {
                CRGB c = pGFX->getPixel(j);
                c.fadeToBlackBy(meteorTrailDecay);
                pGFX->setPixel(j, c);
            }
        }

        for (int i = 0; i < meteorCount; i++)
        {
            float spd = speed[i];

            #if ENABLE_AUDIO
                if (g_Analyzer._VURatio > 1.0)
                    spd *= g_Analyzer._VURatio;
            #endif

            iPos[i] = (bLeft[i]) ? iPos[i]-spd : iPos[i]+spd;
            if (iPos[i]< meteorSize)
            {
                bLeft[i] = false;
                iPos[i] = meteorSize;
            }
            if (iPos[i] >= pGFX->GetLEDCount())
            {
                bLeft[i] = true;
                iPos[i] = pGFX->GetLEDCount()-1;
            }

            for (int j = 0; j < meteorSize; j++)                    // Draw the meteor head
            {
                int x = iPos[i] - j;
                if ((x <= pGFX->GetLEDCount()) && (x >= 1))
                {
                    CRGB rgb;
                    hue[i] = hue[i] + 0.025f;
                    if (hue[i] > 255.0f)
                        hue[i] -= 255.0f;
                    hsv.hue = hue[i];
                    hsv2rgb_rainbow(hsv, rgb);
                    CRGB c = pGFX->getPixel(x);
                    nblend(c, rgb , 75);
                    pGFX->setPixel(x, c);
                }
            }
        }
    }
};

class MeteorEffect : public LEDStripEffect
{
  private:
    std::vector<MeteorChannel> _Meteors;

    int                        _cMeteors;
    uint8_t                    _meteorSize;
    uint8_t                    _meteorTrailDecay;
    float                      _meteorSpeedMin;
    float                      _meteorSpeedMax;

  public:

    MeteorEffect(int cMeteors = 4, uint size = 4, uint decay = 3, float minSpeed = 0.2, float maxSpeed = 0.2)
        : LEDStripEffect(EFFECT_STRIP_METEOR, "Color Meteors"),
          _Meteors(),
          _cMeteors(cMeteors),
          _meteorSize(size),
          _meteorTrailDecay(decay),
          _meteorSpeedMin(minSpeed),
          _meteorSpeedMax(maxSpeed)
    {
    }

    MeteorEffect(const JsonObjectConst& jsonObject)
        : LEDStripEffect(jsonObject),
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
        StaticJsonDocument<LEDStripEffect::_jsonSize + 128> jsonDoc;

        JsonObject root = jsonDoc.to<JsonObject>();
        LEDStripEffect::SerializeToJSON(root);

        jsonDoc["mto"] = _cMeteors;
        jsonDoc[PTY_SIZE] = _meteorSize;
        jsonDoc["dcy"] = _meteorTrailDecay;
        jsonDoc[PTY_MINSPEED] = _meteorSpeedMin;
        jsonDoc[PTY_MAXSPEED] = _meteorSpeedMax;

        assert(!jsonDoc.overflowed());

        return jsonObject.set(jsonDoc.as<JsonObjectConst>());
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
        for (int i = 0; i < _Meteors.size(); i++)
            _Meteors[i].Draw(_GFX[i]);
    }
};
