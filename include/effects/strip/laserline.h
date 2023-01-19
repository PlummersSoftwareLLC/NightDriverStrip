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

#include <sys/types.h>
#include <errno.h>
#include <iostream>
#include <vector>
#include <math.h>
#include "colorutils.h"
#include "globals.h"
#include "ledstripeffect.h"
#include "gfxbase.h"
#if ENABLE_AUDIO
#include "soundanalyzer.h"
#endif


extern AppTime g_AppTime;

class LaserShot
{
    double        _position = 0.0;
    double        _speed    = 10.0;
    double        _size     = 10.0;
    uint8_t       _hue      = 0;
    
public:

    LaserShot(double position, double speed, double size, uint8_t hue)
    {
        _position = position;
        _speed    = speed;
        _size     = size;
        _hue      = hue;
    }

    virtual bool Update(double elapsed)
    {
        _hue += _speed * elapsed;
        _position += _speed * elapsed;
        if (_position > NUM_LEDS)
            return false;
        return true;
    }

    virtual void Draw(std::shared_ptr<GFXBase> pGFX)
    {
        for (double d = 0; d < _size && d + _position < NUM_LEDS; d++)
            pGFX->setPixelsF(_position + d, 1.0, CHSV(_hue + d, 255, 255), true);
    }
};

class LaserLineEffect : public BeatEffectBase2, public LEDStripEffect
{
  private:
    std::vector<LaserShot>      _shots;
    std::shared_ptr<GFXBase>    _gfx;
    double                      _defaultSize;
    double                      _defaultSpeed;

  public:
  
    LaserLineEffect(double speed, double size) : 
        BeatEffectBase2(1.50, 0.00), LEDStripEffect("LaserLine"), _defaultSize(size), _defaultSpeed(speed) 
    {
    }

    virtual bool Init(std::shared_ptr<GFXBase> gfx[NUM_CHANNELS])   
    {
        debugW("Initialized LaserLine Effect");
        _gfx = gfx[0];
        if (!LEDStripEffect::Init(gfx))
            return false;
        return true;
    }

    virtual void Draw() 
    {
        ProcessAudio();
        
        fadeAllChannelsToBlackBy(200);

        auto it = _shots.begin();
        while(it != _shots.end()) 
        {
            it->Draw(_gfx);
            if (!it->Update(g_AppTime.DeltaTime()))
                _shots.erase(it);
            else
                it++;                
        }
    }

    virtual void HandleBeat(bool bMajor, float elapsed, double span)
    {
        _shots.push_back(LaserShot(0.0, _defaultSpeed, _defaultSize, random8()));
    };
};
