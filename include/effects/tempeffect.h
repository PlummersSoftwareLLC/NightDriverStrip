//+--------------------------------------------------------------------------
//
// File:        TempEffect.h
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
//    Scratchpad file for developing new effects
//
// History:     Jul-15-2021         Davepl      Created
//
//---------------------------------------------------------------------------

#pragma once

#if ENABLE_AUDIO

#include <sys/types.h>
#include <errno.h>
#include <iostream>
#include <vector>
#include <math.h>
#include "colorutils.h"
#include "globals.h"
#include "ledstripeffect.h" 
#include "faneffects.h"
#include "musiceffect.h"
#if ENABLE_AUDIO
#include "soundanalyzer.h"
#endif
extern DRAM_ATTR AppTime g_AppTime;

class SimpleInsulatorBeatEffect : public BeatEffectBase
{
  protected:

    deque<int> _lit;

    virtual void Draw()
    {
        BeatEffectBase::Draw();
        LEDStripEffect::fadeAllChannelsToBlackBy(min(255.0, g_AppTime.DeltaTime() * 1500));
    }

    virtual void HandleBeat(bool bMajor, float elapsed, double span)
    {
        BeatEffectBase::HandleBeat(bMajor, elapsed, span);

        while (_lit.size() >= NUM_FANS - 1)
            _lit.pop_front();

        size_t i;
        do
        {
            i = random(0, NUM_FANS);
        } while (_lit.end() != std::find(_lit.begin(), _lit.end(), i));
        _lit.push_back(i);
        
        FillRingPixels(RandomSaturatedColor(), i, 0);
    }

  public:

    using BeatEffectBase::BeatEffectBase;
 
    SimpleInsulatorBeatEffect(const char * pszName) 
      : LEDStripEffect(pszName), BeatEffectBase(1.0, 1.0, 0.01)
    {
    }
};

class SimpleInsulatorBeatEffect2 : public BeatEffectBase
{
  protected:

    deque<int> _lit;

    virtual void Draw()
    {
        BeatEffectBase::Draw();
        fadeAllChannelsToBlackBy(min(255.0, g_AppTime.DeltaTime() * 1500));
    }

    virtual void HandleBeat(bool bMajor, float elapsed, double span)
    {
        BeatEffectBase::HandleBeat(bMajor, elapsed, span);

        while (_lit.size() >= NUM_FANS - 1)
            _lit.pop_front();

        size_t i;
        do
        {
            i = random(0, NUM_FANS);
        } while (_lit.end() != std::find(_lit.begin(), _lit.end(), i));
        _lit.push_back(i);

      FillRingPixels(CRGB::Red, i, 0);        
    }

  public:
 
    SimpleInsulatorBeatEffect2(const char * pszName) 
      : LEDStripEffect(pszName), BeatEffectBase()
    {
    }
};

class VUInsulatorsEffect : public LEDStripEffect
{
    int _last = 1;

    using LEDStripEffect::LEDStripEffect;

    void DrawVUPixels(int i, int fadeBy, const CRGBPalette256 & palette)
    {
      CRGB c = ColorFromPalette(palette, ::map(i, 0, _cLEDs, 0, 255)).fadeToBlackBy(fadeBy);
      setPixel(i, c);
    }

    virtual void Draw()
    {
      static int iPeakVUy = 0;              // Where the peak occurred
      static unsigned long msPeakVU = 0;    // Timestamp of when the last big peak was

      setAllOnAllChannels(0, 0 , 0);
      
      const int MAX_FADE = 255;

      if (iPeakVUy > 0)
      {
        int fade = MAX_FADE * ((millis() - msPeakVU) / (float) MILLIS_PER_SECOND);
        fade = min(fade, MAX_FADE);
        DrawVUPixels(iPeakVUy, fade, vuPaletteGreen);
      }

      int bars = ::map(gVU, gMinVU, 150.0, 1, _cLEDs - 1);
      if (bars >= iPeakVUy)
      {
        msPeakVU = millis();
        iPeakVUy = bars;
      }
      else if (millis() - msPeakVU > MILLIS_PER_SECOND * 1)
      {
        iPeakVUy = 0;
      }

      const int weight = 10;
      bars = (_last * weight + bars)  / (_last * (weight + 1));
      bars = max(bars, 1);
      _last = bars;

      for (int i = 0; i < bars; i++)
        DrawVUPixels(i, 0, vuPaletteGreen);
    }  
};

#endif
