//+--------------------------------------------------------------------------
//
// File:        FireEffect.h
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
//    Fan effects generally have attributes specific to being arranged as
//    circles, and this code also provides a way to draw into those circles
//    in a bottom up, top down, or sideways direction as well.
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
#include "paletteeffect.h"
#include "effectmanager.h"


// Simple definitions of what direction we're talking about

enum PixelOrder
{
  Sequential  = 0,
  Reverse     = 1,
  BottomUp    = 2,
  TopDown     = 4,
  LeftRight   = 8,
  RightLeft   = 16
};

DEFINE_GRADIENT_PALETTE( gpSeahawks ) 
{
    0,       0,     0,   4,      
    64,      3,    38,  58,      
   128,      0,    21,  50,      
   192,     78,   167,   1,      
   255,     54,    87, 140,      
};

// These tables represent the physical order of LEDs when looking at
// the fan in a particular direction, like top to bottom or left to right

#if INSULATORS || SINGLE_INSULATOR
static const int FanPixelsVertical[FAN_SIZE] =
{
  0, 11, 1, 10, 2, 9, 3, 8, 4, 7, 5, 6
};
#elif TREESET
static const int FanPixelsVertical[FAN_SIZE] =
{
  0, 23, 1, 22, 2, 21, 3, 20, 4, 19, 5, 18, 6, 17, 7, 16, 8, 15, 9, 14, 10, 13, 11, 12
};
#elif FAN_SIZE == 16
static const int FanPixelsVertical[FAN_SIZE] =
{
  0, 15, 1, 14, 2, 13, 3, 12, 4, 11, 5, 10, 6, 9, 7, 8
};
#elif FAN_SIZE == 1
static const int FanPixelsVertical[FAN_SIZE] =
{
  0
};
#endif
// GetFanPixelOrder
// 
// Returns the sequential strip postion of a an LED on the fans based
// on the index and direction specified, like 32nd most TopDown pixel.

int GetFanPixelOrder(int iPos, PixelOrder order = Sequential)
{
  while (iPos < 0)
    iPos += FAN_SIZE;


  if (iPos >= NUM_FANS * FAN_SIZE)
  {
    if (order == TopDown)
      return NUM_LEDS - 1 - (iPos - NUM_FANS * FAN_SIZE);
    else
      return iPos;
  }

  int fPos = iPos % FAN_SIZE;
  int fanBase = iPos - fPos;

  switch (order)
  { 
    case BottomUp:
      return fanBase + ((FanPixelsVertical[fPos] + LED_FAN_OFFSET_BU) % FAN_SIZE);

    case TopDown:
      return fanBase + ((FanPixelsVertical[fPos] + LED_FAN_OFFSET_TD) % FAN_SIZE);

    case LeftRight:
      return fanBase + ((FanPixelsVertical[fPos] + LED_FAN_OFFSET_LR) % FAN_SIZE);

    case RightLeft:
      return fanBase + ((FanPixelsVertical[fPos] + LED_FAN_OFFSET_RL) % FAN_SIZE);

    case Reverse:
      return NUM_LEDS - 1 - iPos;

    case Sequential:
    default:
      return fanBase + fPos;
  }
}




// ClearFanPixels
//
// Clears pixels logically into a fan bank in a direction such as top down rather than
// just straight sequential strip order

void ClearFanPixels(float fPos, float count, PixelOrder order = Sequential, int iFan = 0)
{
    fPos += iFan * FAN_SIZE;
    while (count > 0)
    {
      for (int i = 0; i < NUM_CHANNELS; i++)
        FastLED[i][GetFanPixelOrder(fPos + (int) count, order)] = CRGB::Black;
      count--;
    }
}

// DrawFanPixels
//
// A fan is a ring set with a single ring

void DrawFanPixels(float fPos, float count, CRGB color, PixelOrder order = Sequential, int iFan = 0)
{
  fPos += iFan * FAN_SIZE;

  if (fPos + count > NUM_LEDS)
  {
    debugE("DrawFanPixels called with fPos=%f, count=%f, but there are only %d LEDs", fPos, count, NUM_LEDS);
    return;
  }

  if (fPos < 0)
  {
    debugE("Negative fPos in DrawFanPixels");
    return;
  }

  if (count < 0)
  {
    debugE("Negative count in DrawFanPixels");
    return;
  }
  // Calculate how much the first pixel will hold

  float availFirstPixel = 1.0f - (fPos - (long)(fPos));
  float amtFirstPixel = min(availFirstPixel, count);
  float remaining = min(count, FastLED.size()-fPos);
  int iPos = fPos;

  // Blend (add) in the color of the first partial pixel

  if (remaining > 0.0f && amtFirstPixel > 0.0f)
  {
    for (int i = 0; i < NUM_CHANNELS; i++)
      FastLED[i][GetFanPixelOrder(iPos++, order)] += ColorFraction(color, amtFirstPixel);
    remaining -= amtFirstPixel;
  }

  // Now draw any full pixels in the middle

  while (remaining > 1.0f)
  {
    for (int i = 0; i < NUM_CHANNELS; i++)
      FastLED[i][GetFanPixelOrder(iPos++, order)] += color;
    remaining--;
  }

  // Draw tail pixel, up to a single full pixel

  if (remaining > 0.0f)
  {
    for (int i = 0; i < NUM_CHANNELS; i++)
      FastLED[i][GetFanPixelOrder(iPos, order)] += ColorFraction(color, remaining);
  }
}

// DrawRingPixels
// 
// With multiple rings, a fan or insulator becomes a ringset, and this function will
// draw to particular ring within a particular insulator

void DrawRingPixels(float fPos, float count, CRGB color, int iInsulator, int iRing)
{
    for (int i = 0; i < iRing; i++)
        fPos += gRingSizeTable[i];

    DrawFanPixels(fPos, count, color, Sequential, iInsulator); 
}

void FillRingPixels(CRGB color, int iInsulator, int iRing)
{
    DrawRingPixels(0, gRingSizeTable[iRing], color, iInsulator, iRing);
}

class EmptyEffect : public LEDStripEffect
{
    using LEDStripEffect::LEDStripEffect;
    
    virtual void Draw()
    {
        FastLED.clear(false);
        DrawEffect();
        delay(20);
    }

    void DrawEffect()
    {

    }
};

class FanBeatEffect : public LEDStripEffect
{
    using LEDStripEffect::LEDStripEffect;
    
    virtual void Draw()
    {
        fadeToBlackBy(FastLED.leds(), NUM_LEDS, 20);
        DrawEffect();
        delay(20);
    }

    void OnBeat()
    {
        int passes = random(1, mapDouble(gVURatio, 1.0, 2.0, 1, 3));
        passes = gVURatio;
        for (int iPass = 0; iPass < passes; iPass++)
        {
          int iFan = random(0, NUM_FANS);
          int passes = random(1, gVURatio);
          CRGB c = CHSV(random(0, 255), 255, 255);

          for (int iPass = 0; iPass < passes; iPass++)
          {
            DrawFanPixels(0, FAN_SIZE, c, Sequential, iFan++);
          }
        }

        CRGB c = CHSV(random(0, 255), 255, 255);
        for (int i = NUM_FANS * FAN_SIZE; i < NUM_LEDS; i++)
        {
          setPixel(i, c);
        }

    }

    void DrawEffect()
    {
        static bool  latch = false;
        static float minVUSeen = 0.0;

        if (latch)
        {
          if (gVURatio < minVUSeen)
            minVUSeen = gVURatio;
        }

        if (gVURatio < 0.25)             // Crossing center going up
        {
          latch = true;
          minVUSeen = gVURatio;
        }

        if (latch)
        {
            if (gVURatio > 1.5)
            {
              if (randomDouble(1.0, 3.0) < gVURatio)
              {
                //Serial.printf("Beat at: %f\n", gVURatio - minVUSeen);
                latch = false;
                OnBeat();
              }
            }
        }
    }
};

extern void ShowTM1814();

class CountEffect : public LEDStripEffect
{
    using LEDStripEffect::LEDStripEffect;
    
    const int DRAW_LEN = 16;
    const int OPEN_LEN = NUM_FANS * FAN_SIZE - DRAW_LEN;

    virtual void Draw()
    {
        static float i = 0;
        EVERY_N_MILLISECONDS(30)
        {
          i+=0.5f;

          if (i >= OPEN_LEN)
            i -= OPEN_LEN;

          FastLED.clear();
          float t = i;
          for (int z = 0; z < NUM_FANS; z += 3)
          {
            CRGB c = CHSV(z * 48, 255, 255);
            DrawFanPixels(t, DRAW_LEN, c, BottomUp);
            t+= FAN_SIZE * 3;
            if (t >= OPEN_LEN)
              t -= OPEN_LEN;
          }
          #if ATOMISTRING
            ShowTM1814();
          #else
            FastLED.show();
          #endif
        }
    }

    void DrawEffect()
    {

    }

};

class TapeReelEffect : public LEDStripEffect
{
  private:
    float ReelPos[NUM_FANS] = { 0 };
    float ReelDir[NUM_FANS] = { 0 };

  public:

    using LEDStripEffect::LEDStripEffect;
    
    virtual void Draw()
    {
      EVERY_N_MILLISECONDS(250)
      {
        for (int i = 0; i < NUM_FANS; i++)
        {
          if (random(0, 100) < 40)              // 40% Chance of attempting to do something
          {
            int action = random(0, 3);          // Generate a random outcome
            if (action == 0)                     
            {
              ReelDir[i] = 0;                   // 0 -> Stop the Reel
            }
            else if (action == 1)
            {
              if (ReelDir[i] == 0)
              {
                ReelDir[i] = -1;                // 1 -> Spin Backwards, or accel if already doing so
              }
              else
              {
                ReelDir[i] -= .5;
              }
              
            }
            else if (action == 2)
            {
              if (ReelDir[i] == 0)              // 2 -> Spin Forwards, or accel if already doing so
              {
                ReelDir[i] = 1;
              }
              else
              {
                ReelDir[i] += .5;
              }
            }
          }
        }
      }

      EVERY_N_MILLISECONDS(20)                  // Update the reels based on the direction
      {
        for (int i = 0; i < NUM_FANS; i++)
        {
          ReelPos[i] = (ReelPos[i] + ReelDir[i]);
          if (ReelPos[i] < 0)
            ReelPos[i] += FAN_SIZE;
          if (ReelPos[i] >= FAN_SIZE)
            ReelPos[i] -= FAN_SIZE;
        }
      }

      EVERY_N_MILLISECONDS(20)                  // Draw the Effect
      {
        FastLED.clear(false);
        DrawEffect();
      }
    }

    void DrawEffect()
    {
        for (int i = 0; i < NUM_FANS; i++)
        {
          int pos = ReelPos[i];
          DrawFanPixels(i * FAN_SIZE + pos,                                     1, CRGB::White);
          DrawFanPixels(i * FAN_SIZE + ((pos + 1) % FAN_SIZE),                  1, CRGB::Blue);
          DrawFanPixels(i * FAN_SIZE + ((pos + FAN_SIZE / 2) % FAN_SIZE),       1, CRGB::White);
          DrawFanPixels(i * FAN_SIZE + ((pos + FAN_SIZE / 2 + 1) % FAN_SIZE),   1, CRGB::Blue);
        }
    }

};

class PaletteReelEffect : public LEDStripEffect
{
  private:
    float ReelPos[NUM_FANS] = { 0 };
    float ReelDir[NUM_FANS] = { 0 };
    int   ColorOffset[NUM_FANS] = { 0 };
  public:

    using LEDStripEffect::LEDStripEffect;
    
    virtual void Draw()
    {
      EVERY_N_MILLISECONDS(250)
      {
        for (int i = 0; i < NUM_FANS; i++)
        {
          if (random(0, 100) < 50 * gVURatio)              // 40% Chance of attempting to do something
          {
            int action = random(0, 3);          // Generate a random outcome
            if (action == 0 || action == 3)                     
            {
              ReelDir[i] = 0;                   // 0 -> Stop the Reel
            }
            else if (action == 1)
            {
              if (gVURatio > 0.5)
              {
                if (ReelDir[i] == 0)
                {
                  ColorOffset[i] = random(0, 255);
                  ReelDir[i] = -1;                // 1 -> Spin Backwards, or accel if already doing so
                }
                else
                {
                  ReelDir[i] -= .5;
                }
              } 
            }
            else if (action == 2)
            {
              if (gVURatio > 0.5)
              {
                if (ReelDir[i] == 0)              // 2 -> Spin Forwards, or accel if already doing so
                {
                  ColorOffset[i] = random(0, 255);
                  ReelDir[i] = 1;
                }
                else
                {
                  ReelDir[i] += .5;
                }
              }
            }
          }
        }
      }

      EVERY_N_MILLISECONDS(20)                  // Update the reels based on the direction
      {
        for (int i = 0; i < NUM_FANS; i++)
        {
          ReelPos[i] = (ReelPos[i] + ReelDir[i] * (2 + gVURatio));
          if (ReelPos[i] < 0)
            ReelPos[i] += FAN_SIZE;
          if (ReelPos[i] >= FAN_SIZE)
            ReelPos[i] -= FAN_SIZE;
        }
      }

      EVERY_N_MILLISECONDS(20)                  // Draw the Effect
      {
        fadeAllChannelsToBlackBy(20);
        DrawEffect();
      }
    }

    void DrawEffect()
    {
        for (int i = 0; i < NUM_FANS; i++)
        {
          if (ReelDir[i] != 0)
          {
            int pos = ReelPos[i];
            ClearFanPixels(0, 16, Sequential, i);
            for (int x = 0; x < FAN_SIZE; x++)
            {
                DrawFanPixels(i * FAN_SIZE + ((pos + x) % FAN_SIZE), 1, ColorFromPalette(RainbowColors_p, ColorOffset[i]+x*4, 255, NOBLEND));
            }
          }
        }
    }

};

class PaletteSpinEffect : public LEDStripEffect
{
    const CRGBPalette256 _Palette;
    bool  _bReplaceMagenta;
    double _sparkleChance;

  private:
    float ReelPos[NUM_FANS] = { 0 };
    int   ColorOffset[NUM_FANS] = { 0 };
  public:

    PaletteSpinEffect(const char * pszName, const CRGBPalette256 & palette, bool bReplace, double sparkleChance = 0.0) 
      : LEDStripEffect(pszName), _Palette(palette), _bReplaceMagenta(bReplace), _sparkleChance(sparkleChance)
    {

    }

    virtual void Draw()
    {
      EVERY_N_MILLISECONDS(20)                  // Update the reels based on the direction
      {
        for (int i = 0; i < NUM_FANS; i++)
        {
          ReelPos[i] = (ReelPos[i] + 0.25f);
          if (ReelPos[i] < 0)
            ReelPos[i] += FAN_SIZE;
          if (ReelPos[i] >= FAN_SIZE)
            ReelPos[i] -= FAN_SIZE;
        }
      }

      EVERY_N_MILLISECONDS(20)                  // Draw the Effect
      {
        fadeAllChannelsToBlackBy(20);
        DrawEffect();
      }
    }

    void DrawEffect()
    {
        for (int i = 0; i < NUM_FANS; i++)
        {
            ClearFanPixels(0, FAN_SIZE, Sequential, i);
            for (int x = 0; x < FAN_SIZE; x++)
            {
                float q = fmod(ReelPos[i] + x, FAN_SIZE);
                CRGB c = ColorFromPalette(_Palette, 255.0 * q / FAN_SIZE, 255, NOBLEND);
                if (_bReplaceMagenta && c == CRGB(CRGB::Magenta))
                    c = CRGB(CHSV(beatsin8(2, 0, 255), 255, 255));
                if (randomDouble(0, 1) < _sparkleChance)
                  c = CRGB::White;
                DrawFanPixels(x, 1, c, Sequential, i);
            }
        }
    }
};
class ColorCycleEffect : public LEDStripEffect
{
    PixelOrder _order;

  public:
    using LEDStripEffect::LEDStripEffect;
    
    ColorCycleEffect(PixelOrder order = Sequential) : LEDStripEffect("ColorCylceEffect"), _order(order) 
    {
    }

    virtual void Draw()
    {
        FastLED.clear(false);
        DrawEffect();
    }

    void DrawEffect()
    {
        static byte basehue = 0;
        byte hue = basehue;
        EVERY_N_MILLISECONDS(20) 
        {
            basehue += 1;
        }
        for (int i = 0; i < NUM_LEDS; i++)
          DrawFanPixels(i, 1, CHSV(hue+=8, 255, 255), _order);
    }
};

class ColorCycleEffectBottomUp : public LEDStripEffect
{
  public:
    using LEDStripEffect::LEDStripEffect;
    
    virtual void Draw()
    {
        FastLED.clear(false);
        DrawEffect();
    }

    void DrawEffect()
    {
        static byte basehue = 0;
        byte hue = basehue;
        EVERY_N_MILLISECONDS(20) 
        {
            basehue += 2;
        }
        for (int i = 0; i < NUM_LEDS; i++)
          DrawFanPixels(i, 1, CHSV(hue+=8, 255, 255), BottomUp);
    }

};

class ColorCycleEffectTopDown : public LEDStripEffect
{
  public:
    using LEDStripEffect::LEDStripEffect;
    
    virtual void Draw()
    {
        FastLED.clear(false);
        DrawEffect();
    }

    void DrawEffect()
    {
        static byte basehue = 0;
        byte hue = basehue;
        EVERY_N_MILLISECONDS(30) 
        {
            basehue += 1;
        }
        for (int i = 0; i < NUM_LEDS; i++)
          DrawFanPixels(i, 1, CHSV(hue+=4, 255, 255), TopDown);
    }
};

class ColorCycleEffectSequential : public LEDStripEffect
{
  public:
    using LEDStripEffect::LEDStripEffect;
    
    virtual void Draw()
    {
        FastLED.clear(false);
        DrawEffect();
    }

    void DrawEffect()
    {
        static byte basehue = 0;
        byte hue = basehue;
        EVERY_N_MILLISECONDS(30) 
        {
            basehue += 1;
        }
        for (int i = 0; i < NUM_LEDS; i++)
          DrawFanPixels(i, 1, CHSV(hue+=4, 255, 255), Sequential);
    }
};

class SpinningPaletteEffect : public PaletteEffect
{
  int iRotate = 0;

  public:
    using PaletteEffect::PaletteEffect;

    virtual void Draw()
    {
        PaletteEffect::Draw();
        for (int i = 0; i < NUM_FANS; i++)
        {
          RotateFan(i, (i /2) * 2 == i ? true : false, iRotate);
        }
        delay(10);

        EVERY_N_MILLISECONDS(25)
        {
          iRotate = (iRotate + 1) % FAN_SIZE;
        }
    }
};

class ColorCycleEffectRightLeft : public LEDStripEffect
{
  public:
    using LEDStripEffect::LEDStripEffect;
    
    virtual void Draw()
    {
        FastLED.clear(false);
        DrawEffect();
        delay(20);
    }

    void DrawEffect()
    {
        static byte basehue = 0;
        byte hue = basehue;
        basehue += 8;
        for (int i = 0; i < NUM_LEDS; i++)
          DrawFanPixels(i, 1, CHSV(hue+=16, 255, 255), RightLeft);
    }
};

class ColorCycleEffectLeftRight : public LEDStripEffect
{
  public:
    using LEDStripEffect::LEDStripEffect;
    
    virtual void Draw()
    {
        FastLED.clear(false);
        DrawEffect();
        delay(20);
    }

    void DrawEffect()
    {
        static byte basehue = 0;
        byte hue = basehue;
        basehue += 8;
        for (int i = 0; i < NUM_LEDS; i++)
          DrawFanPixels(i, 1, CHSV(hue+=16, 255, 255), LeftRight);
    }
};

class FireFanEffect : public LEDStripEffect
{
  protected:
    int     LEDCount;           // Number of LEDs total
    int     CellsPerLED;
    int     Cooling;            // Rate at which the pixels cool off
    int     Sparks;             // How many sparks will be attempted each frame
    int     SparkHeight;        // If created, max height for a spark
    int     Sparking;           // Probability of a spark each attempt
    bool    bReversed;          // If reversed we draw from 0 outwards
    bool    bMirrored;          // If mirrored we split and duplicate the drawing
    bool    bMulticolor;        // If each arm of the atomlight should have its own color

    PixelOrder Order;

    unique_ptr<byte []> abHeat; // Heat table to map temp to color

    // When diffusing the fire upwards, these control how much to blend in from the cells below (ie: downward neighbors)
    // You can tune these coefficients to control how quickly and smoothly the fire spreads

    static const byte BlendSelf = 0;            // 2
    static const byte BlendNeighbor1 = 1;       // 3
    static const byte BlendNeighbor2 = 1;       // 2
    static const byte BlendNeighbor3 = 0;       // 1

    static const byte BlendTotal = (BlendSelf + BlendNeighbor1 + BlendNeighbor2 + BlendNeighbor3);

    int CellCount() const { return LEDCount * CellsPerLED; } 

  public:

    FireFanEffect(int ledCount, 
                  int cellsPerLED = 1, 
                  int cooling = 20, 
                  int sparking = 100, 
                  int sparks = 3, 
                  int sparkHeight = 4, 
                  PixelOrder order = Sequential, 
                  bool breversed = false, 
                  bool bmirrored = false, 
                  bool bmulticolor = false)
        : LEDStripEffect("FireFanEffect"),
          LEDCount(ledCount),
          CellsPerLED(cellsPerLED),
          Cooling(cooling),
          Sparks(sparks),
          SparkHeight(sparkHeight),
          Sparking(sparking),
          bReversed(breversed),
          bMirrored(bmirrored),
          bMulticolor(bmulticolor),
          Order(order)          
    {
        if (bMirrored)
            LEDCount = LEDCount / 2;
        abHeat = make_unique<byte []>(CellCount());
    }

    virtual CRGB MapHeatToColor(byte temperature, int iChannel = 0)
    {
        byte t192 = round((temperature/255.0)*191);
 
        // calculate ramp up from
        byte heatramp = t192 & 0x3F; // 0..63
        heatramp <<= 2; // scale up to 0..252

        int iVariant = iChannel % 4;
        switch (iVariant)
        {
          case 0:

            if( t192 > 0x80)                      // hottest
                return CRGB(255, 255, heatramp);
            else if( t192 > 0x40 )                // middle
                return CRGB( 255, heatramp, 0);
            else                                  // coolest
                return CRGB( heatramp, 0, 0);     

          case 1:

            if( t192 > 0x80)                      // hottest
                return CRGB(255, heatramp, 255);
            else if( t192 > 0x40 )                // middle
                return CRGB( heatramp, 0, heatramp);
            else                                  // coolest
                return CRGB( heatramp  / 2, 0, heatramp / 2);     

          case 2:

            if( t192 > 0x80)                      // hottest
                return CRGB(255, heatramp, 255);
            else if( t192 > 0x40 )                // middle
                return CRGB( heatramp, 255, 0);
            else                                  // coolest
                return CRGB( 0, heatramp, 0);     

          case 3:

            if( t192 > 0x80)                      // hottest
                return CRGB(heatramp, 255, 255);
            else if( t192 > 0x40 )                // middle
                return CRGB( 0, heatramp, 255);
            else                                  // coolest
                return CRGB( 0, 0, heatramp);     

        }
        return CRGB::Red;
    }

    virtual void Draw()
    {
        FastLED.clear(false);
        DrawFire(Order);
    }

    virtual void DrawFire(PixelOrder order = Sequential)
    {
        // First cool each cell by a litle bit

        EVERY_N_MILLISECONDS(50)
        {
          for (int i = 0; i < CellCount(); i++)
          {
            int coolingAmount = random(0, Cooling);
            abHeat[i] = ::max(0, abHeat[i] - coolingAmount);
          }
        }

        EVERY_N_MILLISECONDS(20)
        {
          // Next drift heat up and diffuse it a little bit
          for (int i = 0; i < CellCount(); i++)
              abHeat[i] = min(255, (abHeat[i] * BlendSelf +
                        abHeat[(i + 1) % CellCount()] * BlendNeighbor1 +
                        abHeat[(i + 2) % CellCount()] * BlendNeighbor2 +
                        abHeat[(i + 3) % CellCount()] * BlendNeighbor3)
                        / BlendTotal);
        }

        // Randomly ignite new sparks down in the flame kernel

        EVERY_N_MILLISECONDS(20)
        {
          for (int i = 0 ; i < Sparks; i++)
          {
              if (random(255) < Sparking / 4 + Sparking * gVURatio / 2.0)
              {
                  int y = CellCount() - 1 - random(SparkHeight * CellsPerLED);
                  abHeat[y] = random(200, 255);// heat[y] + random(50, 255);       // Can roll over which actually looks good!
              }
          }
        }

        // Finally, convert heat to a color

        for (int i = 0; i < LEDCount; i++)
        {
            //byte maxv = 0;
            //for (int iCell = 0; iCell < CellsPerLED; iCell++)
            //  maxv = max(maxv, heat[i * CellsPerLED + iCell]);

           
           for (int iChannel = 0; iChannel < NUM_CHANNELS; iChannel++)
           {
              CRGB color = MapHeatToColor(abHeat[i*CellsPerLED], bMulticolor ? iChannel : 0);

              // If we're reversed, we work from the end back.  We don't reverse the bonus pixels

              int j = (!bReversed || i > FAN_SIZE) ? i : LEDCount - 1 - i;
              int x = GetFanPixelOrder(j, order);
              if (x < NUM_LEDS)
              {
                FastLED[iChannel][x] = color;
                if (bMirrored)
                    FastLED[iChannel][!bReversed ? (2 * LEDCount - 1 - i) : LEDCount + i] = color;
              }
           }
        }
    }
};

class BlueFireFanEffect : public FireFanEffect
{
    using FireFanEffect::FireFanEffect;

    virtual CRGB MapHeatToColor(byte temperature, int iChannel = 0)
    {
      byte t192 = round((temperature/255.0)*191);
      byte heatramp = t192 & 0x3F; // 0..63
      heatramp <<= 2; // scale up to 0..252

      CHSV hsv(HUE_BLUE, 255, heatramp);
      CRGB rgb;
      hsv2rgb_rainbow(hsv, rgb);
      return rgb;
    }
};

class GreenFireFanEffect : public FireFanEffect
{
    using FireFanEffect::FireFanEffect;

    virtual CRGB MapHeatToColor(byte temperature, int iChannel = 0)
    {
      byte t192 = round((temperature/255.0)*191);
      byte heatramp = t192 & 0x3F; // 0..63
      heatramp <<= 2; // scale up to 0..252

      CHSV hsv(HUE_GREEN, 255, heatramp);
      CRGB rgb;
      hsv2rgb_rainbow(hsv, rgb);
      return rgb;
    }
};

class MusicFireEffect : public FireFanEffect
{
      float     _colorOffset;

    using FireFanEffect::FireFanEffect;

    void OnBeat(double intensity)
    {
      for (int i = 0; i < intensity * 3; i++)
      {
        abHeat[random(0, NUM_FANS*FAN_SIZE)] = random(200, 255);
      }
    }

    void HandleAudio()
    {
      static bool  latch = false;
      static float minVUSeen = 0.0;

      if (latch)
      {
        if (gVURatio < minVUSeen)
          minVUSeen = gVURatio;
      }

      if (gVURatio < 0.25)             // We've seen a "low" value, so set the latch
      {
        latch = true;
        minVUSeen = gVURatio;
      }

      if (latch)
      {
          if (gVURatio > 1.5)
          {
            if (randomDouble(1.0, 3.0) < gVURatio)
            {
              //Serial.printf("Beat at: %f\n", gVURatio - minVUSeen);
              latch = false;
              OnBeat(gVURatio - minVUSeen);
            }
          }
      }
    }

    virtual CRGB MapHeatToColor(byte temperature, int iChannel = 0)
    {
              // Scale 'heat' down from 0-255 to 0-191
      byte t192 = round((temperature / 255.0) * 191);

      // calculate ramp up from
      byte heatramp = t192 & 0x3F; // 0..63
      heatramp <<= 2; // scale up to 0..252
      CRGB c;
      c.setHSV((heatramp+_colorOffset*0), 255, heatramp);
      c.fadeToBlackBy(252-heatramp);
      return c;
    }

    virtual void Draw()
    {
        // Cycle the color (used by multicoor mode only)
        _colorOffset = fmod(_colorOffset + 16 * gVURatio, 240); //  * _intensityAdjust.GetValue(), 240);

        FastLED.clear(false);
        DrawFire(Sequential);
        HandleAudio();
        delay(20);
    }
};

class RGBRollAround : public LEDStripEffect
{
  int iRotate = 0;

  public:
    using LEDStripEffect::LEDStripEffect;

    virtual void DrawColor(CRGB color, int phase)
    {
        const int lineLen = FAN_SIZE;
        int q = beatsin16(24, 0, NUM_LEDS-lineLen, 0, phase);
        DrawFanPixels(q, lineLen, color, BottomUp);   
    }

    virtual void Draw()
    {
        FastLED.clear();
        DrawColor(CRGB::Red, 0);
        DrawColor(CRGB::Green, 16383);
        DrawColor(CRGB::Blue, 32767);

    }
};

class HueTest : public LEDStripEffect
{
  int iRotate = 0;

  public:
    using LEDStripEffect::LEDStripEffect;

    virtual void Draw()
    {
        FastLED.clear();
        int iFan = 0;
        for (int sat = 255; sat >= 0 && iFan < NUM_FANS; sat -= 32)
        {
            DrawFanPixels(0, FAN_SIZE, CRGB( CHSV(HUE_RED, sat, 255)), Sequential, iFan++);
        }
    }
};
