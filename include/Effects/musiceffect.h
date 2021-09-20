//+--------------------------------------------------------------------------
//
// File:        MusicEffect.h
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
//    Floating point framerate independent version of the classic Flame effect
//
// History:     Apr-13-2019         Davepl      Adapted from LEDWifiSocket
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
#include "faneffects.h"

extern DRAM_ATTR AppTime g_AppTime;

// BeatEffectBase
//
// A specialization of LEDStripEffect, adds a HandleBeat function that allows apps to 
// draw based on the music beat.  The Draw() function does the audio processing and calls
// HandleBeat() whenever.  Apps are free to draw in both Draw() and HandleBeat().
//
// The constructor allows you to specify the sensitivity by where the latch points are/
// For a highly sensitive (defaults), keep them both close to 1.0.  For a wider beat 
// detection you could use 0.25 and 1.75 for example.

class BeatEffectBase : public virtual LEDStripEffect
{
  protected:

    bool   _latched = false;                          // Have we seen a low val yet?
    double _lastBeat = 0;                             // When was the last beat?

    double _lowLatch;                                 // Low Point at we we set the latch ON
    double _highLatch;                                // High Point we must crest for a beat
    double _minElapsed;                               // Min time between beats or we ignore
    double _lowestSeen;

    double SecondsSinceLastBeat()
    {
      return g_AppTime.CurrentTime() - _lastBeat;
    }

  public:

    using LEDStripEffect::LEDStripEffect;
    
    BeatEffectBase(double lowLatch = 1.0, double highLatch = 1.25, double minElapsed = 0.00)      // Eighth note at 120BPM is .125
     : LEDStripEffect("BeatEffectBase"),
       _lowLatch(lowLatch),
       _highLatch(highLatch),
       _minElapsed(minElapsed),
       _lowestSeen(2.0)
    {
    }

    // When a beat is detected, this is called.  The 'bMajor' indicates whether this is a more important beat, which
    // for now simply means it's been a minimum delay since the last beat.

    virtual void HandleBeat(bool bMajor, float elapsed, double span)
    {
        debugV("BEAT: [%s], gVURatio=%f, since last=%lf, span = %lf\n", bMajor ? "Major" : "Minor", gVURatio, g_AppTime.CurrentTime() - _lastBeat, gVURatio - _lowestSeen);
    }

    // BeatEffectBase::Draw
    //
    // Doesn't actually "draw" anything, but rather it scans the audio VU to detect beats, and when it finds one,
    // it calls the virtual "HandleBeat" function.

    virtual void Draw()
    {
        debugV("BeatEffectBase::Draw");
        double elapsed = SecondsSinceLastBeat();
    
        if (elapsed < _minElapsed)                      // Too soon since last beat to even consider another.  
          return;

        if (gVURatio < _lowestSeen)
            _lowestSeen = gVURatio;

        if (gVURatio < _lowLatch)                       // If we dip below the low latch, we're latched (armed)
            _latched = true;                            //   to trigger a beat if we then go above the high latch

        if (_latched && gVURatio >= _highLatch)
        {
            // When we crest above the high latch point while latched (ie: we see a high value aftering confirming
            // that we've seen a low one) we know it's a round trip on the VU meter and so we call that a beat.  We
            // call the client handler and then reset the latch, then record now as the last beat time.

            HandleBeat(elapsed > 1.0, elapsed, gVURatio - _lowestSeen);
            _lastBeat = g_AppTime.CurrentTime();
            _latched = false;
            _lowestSeen = 2.0;
        }
    }
};

//
// MusicalInsulatorEffect
//

class DoesntWorkMusicalEffect : public BeatEffectBase
{
  protected:

    size_t iLastInsulator = -1;
    CRGB   lastColor;
    deque<int> litInsulators;

  public:
   
    DoesntWorkMusicalEffect()
      : BeatEffectBase("MusicalInsulatorEffect")
    {

    }


    virtual void LightInsulator(int iInsulator, int iRing, CRGB color)
    {
      debugV("Base effect LightInsulator for ring %d of insulator %i", iInsulator, iRing);

      if (iInsulator == -1)
        DrawFanPixels(0, NUM_LEDS, color, Sequential, 0);
      else
        DrawFanPixels(0, FAN_SIZE, color, Sequential, iInsulator);        // Fill one insulator for minor beat
    }

    virtual void HandleBeat(bool bMajor, float elapsed, double span)
    {
      BeatEffectBase::HandleBeat(bMajor, elapsed, span);

      size_t iNewInsulator = -1;

      // First, no matter the beat, we add color to the entire things.  We only add N vibrance so it can add up over time.
      // Multiplying the vibrance by gVURatio ties it directly to the sound

      CRGB b = CHSV(beatsin8(4), 255, 64*gVURatio);
      setPixels(0, NUM_LEDS, b, true);

      // Color is additive, so we start with 200 V so that if the same beat in the same color iis replayed in the same insulator, it gets brighter

      // CRGB c = CHSV(random(0, 255), 255, 200);
      CRGB c = CHSV(beatsin8(2), 255, 200);
      if (!bMajor && iLastInsulator != -1)
      {
          // Repeat the last beat by adding to it
          //iNewInsulator = iLastInsulator;
          c = lastColor;
      }

      if (litInsulators.size() >= NUM_FANS)
      {
          iNewInsulator = litInsulators.back();
          litInsulators.pop_back();
      }
      else
      {
          // Search for a new insulator that's not in the MRU.

          for (int iPass = 0; iPass < NUM_FANS * 10; iPass++)
          {
              size_t i = random(0, NUM_FANS);
              if (litInsulators.end() != std::find(litInsulators.begin(), litInsulators.end(), i))
                  continue;
              iNewInsulator = i;
              litInsulators.push_front(iNewInsulator);
              if (randomDouble(0.0, 1.0) < 0.25)       // Sometimes we put this one on twice just to switch up the order 
                  litInsulators.push_front(iNewInsulator);
              break;
          }
      }
      
      if (iNewInsulator == -1)				// No empty slot could be found!
      {	
        litInsulators.clear();
        setAllOnAllChannels(0,0,0);
        return;
      }
      
      lastColor = c;
      iLastInsulator = iNewInsulator;

      LightInsulator(SecondsSinceLastBeat() > 0.8 ? -1 : iNewInsulator, 0, bMajor ?  RandomSaturatedColor() : c);
    }

    virtual void Draw()
    {
      debugV("BeatEffect2::Draw");
      BeatEffectBase::Draw();

      double elapsed = min(1.0, (g_AppTime.CurrentTime() - _lastBeat));     // Elapsed since last beat, 0.0 ... 1.0
      const int fadeRate = 18;
      fadeAllChannelsToBlackBy(max(1.0, fadeRate - (fadeRate * elapsed)));
      delay(20);
    }    
};

class BeatEffect : public LEDStripEffect
{
  public:

    using LEDStripEffect::LEDStripEffect;

    virtual void OnBeat()
    {

    }

    virtual void Draw()
    {
        static bool  latch = false;
        static float minVUSeen = 0.0;

        if (latch)
        {
          if (gVURatio < minVUSeen)
            minVUSeen = gVURatio;
        }

        if (gVURatio < 0.5)             // Crossing center going down
        {
          latch = true;
          minVUSeen = gVURatio;
        }

        if (latch)
        {
            if (gVURatio > 1.75)      // Going back up
            {
                latch = false;
                OnBeat();
                debugV("Beat: %f, %f\n", gVURatio, gVU);
            }
        }
      
    }
};

class ChannelBeatEffect : public BeatEffect
{
    shared_ptr<LEDMatrixGFX> * _gfx;

  public:

    using BeatEffect::BeatEffect;

    size_t iLastArm = -1;
    double lastBeat = 0;
    CRGB   lastColor;

    deque<int> litArms;

    virtual void Draw()
    {
        BeatEffect::Draw();
        
        EVERY_N_MILLISECONDS(20)
        {
          fadeAllChannelsToBlackBy(10);
        }

        delay(10);
    }


    virtual bool Init(shared_ptr<LEDMatrixGFX> gfx[NUM_CHANNELS])	
    {
        _gfx = gfx;
        if (!LEDStripEffect::Init(gfx))
            return false;
        return true;
    }

    virtual void OnBeat()
    {
      size_t iNew = -1;
      double elapsed = g_AppTime.CurrentTime() - lastBeat;
      lastBeat = g_AppTime.CurrentTime();

      CRGB b = CHSV(random(0, 255), 255, 80);
      setPixels(0, NUM_LEDS, b, true);

      // Color is additive, so we start with 200 V instead of 255 so that if the same beat in the same color iis replayed in the same insulator, it gets brighter

      CRGB c = CHSV(random(0, 255), 255, 200);

      // gVURatio is better when it's been a while since a beat, so we don't do a repeat beat for big values of it

      if (gVURatio < 1.9 && elapsed < 0.25 && iLastArm != -1)
      {
        iNew = iLastArm;
        c = lastColor;
      }
      else if (litArms.size() >= NUM_CHANNELS)
      {
        iNew = litArms.back();
        litArms.pop_back();
      }
      else
      {
        for (int iPass = 0; iPass < NUM_CHANNELS * 10; iPass++)
        {
          size_t i = random(0, NUM_CHANNELS);
          if (litArms.end() != std::find(litArms.begin(), litArms.end(), i))
            continue;
          iNew = i;
          litArms.push_front(iNew);
          if (randomDouble(0.0, 1.0) < 0.25)       // Sometimes we put this one on twice just to switch up the order 
            litArms.push_front(iNew);
          break;
        }
      }
  
      if (iNew == -1)				// No empty slot could be found!
      {	
        litArms.clear();
        setAllOnAllChannels(0,0,0);
        debugI("No slot!\n");
        return;
      }
      
      lastBeat = g_AppTime.CurrentTime();
      lastColor = c;
      iLastArm = iNew;

       
      //for (int i = 0; i < NUM_LEDS; i++)
      //  _gfx[iNew]->GetLEDBuffer()[i] = c;
    }
};

// InsulatorColorBeatEffect
//
// Uses a very sensitive beat detection.  Fills all pixels blue based on VU, and on beats, fills a random insulator
// with a random color.  Longer beats get more insulators.  Very long beats get everything filled with purple.

class SimpleColorBeat : public BeatEffectBase
{
  protected:

    int _iLastInsulator = -1;
           
    virtual void Draw()
    {
        BeatEffectBase::Draw();

        CRGB c = CRGB::Blue * gVURatio * g_AppTime.DeltaTime() * 0.75;
        setPixels(0, NUM_LEDS, c, true);

        fadeAllChannelsToBlackBy(min(255.0,1000 * g_AppTime.DeltaTime()));
        delay(1);
    }

    virtual void HandleBeat(bool bMajor, float elapsed, double span)
    {
        BeatEffectBase::HandleBeat(bMajor, elapsed, span);

        CRGB c;
        int  cInsulators = 1;

        if (elapsed < 0.10)
        {
          c = CHSV(beatsin8(2), 255, 255);
        }
        else if (elapsed < 0.25)
        {
          c = CHSV(beatsin8(3), 255, 255);
        }
        else
        {
          c = CRGB::Cyan;

          if (elapsed > 0.5)                                // Medium beats fill blue and proceed with insualtor2
          {
            c = CHSV(beatsin8(4), 255, 255);
            cInsulators = random(1, NUM_FANS);
          }
          else if (elapsed > 1.0)                           // Long beats fill purple and return
          {
            c = CRGB::Purple;
            setAllOnAllChannels(c.r, c.g, c.b);
            return;
          }
        }

        int i;
        for (int iPass = 0; iPass < cInsulators; iPass++)
        {
          do {                                              // Pick a different insulator than was used last time by:
            i = random(0, NUM_FANS);                        //  - Starting with a random number
          } while (i == _iLastInsulator);                   //  - Repeating until it doesn't match the last pass
          _iLastInsulator = i;                              // Our current choice forms the new "last" choice for next pass

          DrawFanPixels(0, FAN_SIZE, c, Sequential, i);     // Draw twice to double-saturate our color
          DrawFanPixels(0, FAN_SIZE, c, Sequential, i);
        }
    } 

  public:
  
    using BeatEffectBase::BeatEffectBase;
  
    SimpleColorBeat()
      : BeatEffectBase(1.0, 1.0, 0.25)
    {
    }

};