//+--------------------------------------------------------------------------
//
// File:        LEDStripEffect.h
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
//    Defines the base class for effects that run locally on the chip
//
// History:     Apr-13-2019         Davepl      Created for NightDriverStrip
//              
//---------------------------------------------------------------------------

#pragma once

#include <sys/types.h>
#include <errno.h>
#include <iostream>
#include <vector>
#include <math.h>
#include "globals.h"
#include "colorutils.h"
#include "ntptimeclient.h"
#include "effectmanager.h"
#include "gfxbase.h"
#include "ledstripeffect.h"

#include <deque>
#include <memory>

extern bool                      g_bUpdateStarted;

// LEDStripEffect
//
// Base class for an LED strip effect.  At a minimum they must draw themselves and provide a unique name.

class LEDStripEffect
{
  protected:

    size_t _cLEDs;
    String _friendlyName;

    std::shared_ptr<GFXBase> _GFX[NUM_CHANNELS];
    inline static double randomDouble(double lower, double upper)
    {
        double result = (lower + ((upper - lower) * rand()) / RAND_MAX);
        return result;
    }

  public:

    LEDStripEffect(const char * pszName)
    {
        if (pszName)
            _friendlyName = pszName;
    }

    virtual ~LEDStripEffect()
    {
    }

    virtual bool Init(std::shared_ptr<GFXBase> gfx[NUM_CHANNELS])               // There are up to 8 channel in play per effect and when we
    {           
        debugW("Init %s", _friendlyName.c_str());                                                    //   start up, we are given copies to their graphics interfaces
        for (int i = 0; i < NUM_CHANNELS; i++)                      //   so that we can call them directly later from other calls
        {
            _GFX[i] = gfx[i];    
        }
        debugW("Get LED Count");
        _cLEDs = _GFX[0]->GetLEDCount();   
        debugI("Init Effect %s with %d LEDs\n", _friendlyName.c_str(), _cLEDs);
        return true;  
    }
    
    inline std::shared_ptr<GFXBase> graphics() const
    {
        return _GFX[0];
    }

    virtual void Start() {}                                         // Optional method called when time to clean/init the effect
    virtual void Draw() = 0;                                        // Your effect must implement these
    
    virtual const char *FriendlyName() const
    {
        return _friendlyName.c_str();
    }

    virtual size_t DesiredFramesPerSecond() const
    {
        return 31;
    }
    
    static inline CRGB RandomRainbowColor()
    {
        static const CRGB colors[] =
            {
                CRGB::Green,
                CRGB::Red,
                CRGB::Blue,
                CRGB::Orange,
                CRGB::Indigo,
                CRGB::Violet
            };
        int randomColorIndex = (int)randomDouble(0, ARRAYSIZE(colors));
        return colors[randomColorIndex];
    }

    static inline CRGB RandomSaturatedColor()
    {
        CRGB c;
        c.setHSV((uint8_t)randomDouble(0, 255), 255, 255);
        return c;
    }

    void fillSolidOnAllChannels(CRGB color, int iStart = 0, int numToFill = 0,  uint everyN = 1)
    {
        if (!_GFX[0])
            throw new std::runtime_error("Graphcis not set up properly");

        if (numToFill == 0)
            numToFill = _cLEDs-iStart;

        if (iStart + numToFill > _cLEDs)
        {
            printf("Boundary Exceeded in FillRainbow");
            return;
        }

        for (int n = 0; n < NUM_CHANNELS; n++)
        {            
            for (int i = iStart; i < numToFill; i+= everyN)
                _GFX[n]->setPixel(i, color);
               
        }
    }

    void ClearFrameOnAllChannels()
    {
        for (int i = 0; i < NUM_CHANNELS; i++)
        {
            _GFX[i]->Clear();
        }
    }
    
    void fillRainbowAllChannels(int iStart, int numToFill, uint8_t initialhue, uint8_t deltahue, uint8_t everyNth = 1) 
    {
        if (iStart + numToFill > _cLEDs)
        {
            printf("Boundary Exceeded in FillRainbow");
            return;
        }

        CHSV hsv;
        hsv.hue = initialhue;
        hsv.val = 255;
        hsv.sat = 240;
        for (int i = 0; i < numToFill; i+=everyNth)
        {
            CRGB rgb;
            hsv2rgb_rainbow(hsv, rgb);
            setPixelOnAllChannels(iStart + i, rgb);
            hsv.hue += deltahue;
            for (int q = 1; q < everyNth; q++)
                _GFX[q]->setPixel(iStart + i + q, CRGB::Black);
        }
    }

    inline void fadePixelToBlackOnAllChannelsBy(int pixel, uint8_t fadeValue) const
    {
        for (int i = 0; i < NUM_CHANNELS; i++)
        {
            CRGB crgb = _GFX[i]->getPixel(pixel);
            crgb.fadeToBlackBy(fadeValue);
            _GFX[i]->setPixel(pixel, crgb);
        }
    }

    inline void fadeAllChannelsToBlackBy(uint8_t fadeValue) const
    {
        for (int i = 0; i < _cLEDs; i++)
            fadePixelToBlackOnAllChannelsBy(i, fadeValue);
    }

    inline void setAllOnAllChannels(uint8_t r, uint8_t g, uint8_t b) const
    {
        for (int n = 0; n < NUM_CHANNELS; n++)    
            for (int i = 0; i < _cLEDs; i++)
                _GFX[n]->setPixel(i, r, g, b);
    }

    inline void setPixelOnAllChannels(int i, CRGB c)
    {       
        for (int j = 0; j < NUM_CHANNELS; j++)  
            _GFX[j]->setPixel(i, c);
    }

    inline void setPixelsOnAllChannels(float fPos, float count, CRGB c, bool bMerge = false) const
    {       
        for (int i = 0; i < NUM_CHANNELS; i++)
            _GFX[i]->setPixelsF(fPos, count, c, bMerge);
    }
};
