//+--------------------------------------------------------------------------
//
// File:        BouncingBallEffect.h
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
//    Draws bouncing balls using a kinematics formula
//
// History:     Apr-17-2019         Davepl      Adapted from NightDriver
//              
//---------------------------------------------------------------------------

#pragma once

#include "effects.h"

extern AppTime g_AppTime;

// BouncingBallEffect
//
// Draws a set of N bouncing balls using a simple little kinematics formula.  Clears the section first.

static const CRGB ballColors[] =
{
    CRGB::Green,
    CRGB::Red,
    CRGB::Blue,
    CRGB::Orange,
    CRGB::Indigo,
    CRGB::Violet
};

class BouncingBallEffect : public LEDStripEffect
{
private:

    size_t  _iOffset;
    size_t  _cLength;
    size_t  _cBalls;
    size_t  _cBallSize;
    bool    _bMirrored;

    static const int BallCount = 3;
    const bool _bErase;

    double Gravity = -9.81;
    double StartHeight = 1;
    double ImpactVelocityStart = sqrt(-2 * Gravity * StartHeight);
    
    std::vector<double> ClockTimeSinceLastBounce;
    std::vector<double> TimeSinceLastBounce;
    std::vector<double> Height;
    std::vector<double> ImpactVelocity;
    std::vector<double> Dampening;
    std::vector<CRGB>   Colors;
    
  public:

    BouncingBallEffect(size_t ballCount = 3, bool bMirrored = true, bool bErase = false, int ballSize = 5)
        : LEDStripEffect(EFFECT_STRIP_BOUNCING_BALL, "Bouncing Balls"),
          _cBalls(ballCount),
          _cBallSize(ballSize),
          _bMirrored(bMirrored),
          _bErase(bErase)
    {
    }

    BouncingBallEffect(const JsonObjectConst&  jsonObject) 
        : LEDStripEffect(jsonObject),
          _cBalls(jsonObject["blc"].as<size_t>()),
          _cBallSize(jsonObject["bls"].as<size_t>()),
          _bMirrored(jsonObject["mir"].as<bool>()),
          _bErase(jsonObject["ers"].as<bool>())
    {
    }

    virtual bool SerializeToJSON(JsonObject& jsonObject) 
    {
        StaticJsonDocument<128> jsonDoc;
        
        jsonDoc["blc"] = _cBalls;
        jsonDoc["bls"] = _cBallSize;
        jsonDoc["mir"] = _bMirrored;
        jsonDoc["ers"] = _bErase;

        JsonObject root = jsonDoc.as<JsonObject>();
        LEDStripEffect::SerializeToJSON(root);

        return jsonObject.set(jsonDoc.as<JsonObjectConst>());
    }

    virtual size_t DesiredFramesPerSecond() const
    {
        return 61;
    }

    virtual bool Init(std::shared_ptr<GFXBase> gfx[NUM_CHANNELS])
    {
        if (!LEDStripEffect::Init(gfx))
            return false;

        _cLength = gfx[0]->GetLEDCount();

        ClockTimeSinceLastBounce.resize(_cBalls);
        TimeSinceLastBounce.resize(_cBalls);
        Height.resize(_cBalls);
        ImpactVelocity.resize(_cBalls);
        Dampening.resize(_cBalls);
        Colors.resize(_cBalls);

        for (size_t i = 0; i < _cBalls; i++)
        {
            Height[i]                   = StartHeight;
            ImpactVelocity[i]           = ImpactVelocityStart;
            ClockTimeSinceLastBounce[i] = g_AppTime.FrameStartTime();
            Dampening[i]                = 1.0 - i / pow(_cBalls, 2);               // Was 0.9
            TimeSinceLastBounce[i]      = 0;
            Colors[i]                   = ballColors[i % ARRAYSIZE(ballColors)];
        }           
        return true; 
    }

    // Draw
    //
    // Draw each of the balls.  When any ball gets too little energy it would just sit at the base so it is re-kicked with new energy.#pragma endregion
    
    virtual void Draw()
    {
        // Erase the drawing area
        if (_bErase)
        {
            setAllOnAllChannels(0,0,0);
        }
        else
        {
            for (int j = 0; j<_cLength; j++)                            // fade brightness all LEDs one step
            {
                if (randomDouble(0, 10)>5) 
                {
                    CRGB c = _GFX[0]->getPixel(j);
                    c.fadeToBlackBy(10);
                    setPixelsOnAllChannels(j, 1, c, false);
                }
            }            
        }

        // Draw each of the the balls
        for (size_t i = 0; i < BallCount; i++)
        {
            TimeSinceLastBounce[i] = (g_AppTime.FrameStartTime() - ClockTimeSinceLastBounce[i]) / 3;        // BUGBUG hardcoded was 3 for NightDriverStrip
            Height[i] = 0.5 * Gravity * pow(TimeSinceLastBounce[i], 2.0) + ImpactVelocity[i] * TimeSinceLastBounce[i];

            if (Height[i] < 0)
            {
                Height[i] = 0;
                ImpactVelocity[i] = Dampening[i] * ImpactVelocity[i];
                ClockTimeSinceLastBounce[i] = g_AppTime.FrameStartTime();;

                if (ImpactVelocity[i] < 0.5 * ImpactVelocityStart)                                    // Was .01 and not multiplied by anything
                    ImpactVelocity[i] = ImpactVelocityStart;
            }

            float position = Height[i] * (_cLength - 1) / StartHeight;
            setPixelsOnAllChannels(position, _cBallSize, Colors[i]);
            if (_bMirrored)
                setPixelsOnAllChannels(_cLength-1-position, _cBallSize, Colors[i], true);
        }
    }
};
