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
//
// Description:
//
//    Various incarnational of flame effects
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
#include <memory>
#include "colorutils.h"
#include "globals.h"
#include "ledstripeffect.h"

extern AppTime g_AppTime;
extern volatile float gVURatio;

class FireEffect : public LEDStripEffect
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

    unique_ptr<byte []> heat;

    // When diffusing the fire upwards, these control how much to blend in from the cells below (ie: downward neighbors)
    // You can tune these coefficients to control how quickly and smoothly the fire spreads

    static const byte BlendSelf = 0;            // 2
    static const byte BlendNeighbor1 = 1;       // 3
    static const byte BlendNeighbor2 = 2;       // 2
    static const byte BlendNeighbor3 = 0;       // 1

    static const byte BlendTotal = (BlendSelf + BlendNeighbor1 + BlendNeighbor2 + BlendNeighbor3);

    int CellCount() const { return LEDCount * CellsPerLED; } 

  public:

    FireEffect(int ledCount = NUM_LEDS, int cellsPerLED = 1, int cooling = 20, int sparking = 100, int sparks = 3, int sparkHeight = 4,  bool breversed = false, bool bmirrored = false)
        : LEDStripEffect("FireEffect"),
          LEDCount(ledCount),
          CellsPerLED(cellsPerLED),
          Cooling(cooling),
          Sparks(sparks),
          SparkHeight(sparkHeight),
          Sparking(sparking),
          bReversed(breversed),
          bMirrored(bmirrored)
    {
        if (bMirrored)
            LEDCount = LEDCount / 2;

        heat = make_unique<byte []>(CellCount());
    }

    virtual ~FireEffect()
    {
    }

    virtual CRGB GetBlackBodyHeatColor(double temp)
    {
        temp *= 255;
        byte t192 = round((temp/255.0)*191);
 
        // calculate ramp up from
        byte heatramp = t192 & 0x3F; // 0..63
        heatramp <<= 2; // scale up to 0..252

        // figure out which third of the spectrum we're in:
        if( t192 > 0x80) {                     // hottest
            return CRGB(255, 255, heatramp);
        } else if( t192 > 0x40 ) {             // middle
            return CRGB( 255, heatramp, 0);
        } else {                               // coolest
            return CRGB( heatramp, 0, 0);     
        }
    }

    virtual void Draw()
    {
        FastLED.clear(false);
        DrawFire();
    }

    virtual void DrawFire()
    {
        // First cool each cell by a litle bit

        EVERY_N_MILLISECONDS(50)
        {
          for (int i = 0; i < CellCount(); i++)
          {
            int coolingAmount = random(0, Cooling);
            heat[i] = ::max(0, heat[i] - coolingAmount);
          }
        }

        EVERY_N_MILLISECONDS(20)
        {
          // Next drift heat up and diffuse it a little bit
          for (int i = 0; i < CellCount(); i++)
              heat[i] = min(255, (heat[i] * BlendSelf +
                        heat[(i + 1) % CellCount()] * BlendNeighbor1 +
                        heat[(i + 2) % CellCount()] * BlendNeighbor2 +
                        heat[(i + 3) % CellCount()] * BlendNeighbor3)
                        / BlendTotal);
        }

        // Randomly ignite new sparks down in the flame kernel

        EVERY_N_MILLISECONDS(20)
        {
          for (int i = 0 ; i < Sparks; i++)
          {
              if (random(255) < Sparking)
              {
                  int y = CellCount() - 1 - random(SparkHeight * CellsPerLED);
                  heat[y] = random(200, 255);   // Can roll over which actually looks good!
              }
          }
        }

        // Finally, convert heat to a color

        for (int i = 0; i < LEDCount; i++)
        {

            CRGB color = GetBlackBodyHeatColor(heat[i*CellsPerLED]);

            // If we're reversed, we work from the end back.  We don't reverse the bonus pixels

            int j = (!bReversed) ? i : LEDCount - 1 - i;
            setPixels(j, 1, color, false);
            if (bMirrored)
                setPixels(!bReversed ? (2 * LEDCount - 1 - i) : LEDCount + i, 1, color, false);
        }
    }
};

class PaletteFlameEffect : public FireEffect
{
    CRGBPalette256 _palette;

public:
    PaletteFlameEffect(const char *pszName,
                       const CRGBPalette256 &palette,
                       int ledCount = NUM_LEDS,
                       int cellsPerLED = 1,
                       int cooling = 20,         // Was 1.8 for NightDriverStrip
                       int sparking = 100,
                       int sparkHeight = 3,
                       bool reversed = false,
                       bool mirrored = false)
        : FireEffect(ledCount, cellsPerLED, cooling, sparking, sparking, sparkHeight, reversed, mirrored),
          _palette(palette)
    {
    }

    virtual CRGB GetBlackBodyHeatColor(double temp)
    {
        temp = min(1.0, temp);
        int index = mapDouble(temp, 0.0, 1.0, 0.0, 240);
        return ColorFromPalette(_palette, index, 255);

        //        uint8_t heatramp = (uint8_t)(t192 & 0x3F);
        //        heatramp <<=2;
    }
};

class ClassicFireEffect : public LEDStripEffect
{
    bool _Mirrored;
    bool _Reversed;
    int  _Cooling;

public:

    ClassicFireEffect(bool mirrored = false, bool reversed = false, int cooling = 5) : LEDStripEffect("Classic Fire")
    {
        _Mirrored = mirrored;
        _Reversed = reversed;
        _Cooling  = cooling;
    }

    virtual const char *FriendlyName() const
    {
        return "Classic Fire";
    }

    virtual void Draw()
    {
        //static double lastDraw = 0;

        //if (g_AppTime.FrameStartTime() - lastDraw < 1.0 / 40.0)
        //    return;
        //lastDraw = g_AppTime.FrameStartTime();

        //  Fire(55, 180, 1);               //  The original
        Fire(_Cooling, 180, 5);
        delay(20);
    }

    void Fire(int Cooling, int Sparking, int Sparks)
    {
        setAllOnAllChannels(0,0,0);

        static byte heat[NUM_LEDS];
        int cooldown;

        // Step 1.  Cool down every cell a little
        for (int i = 0; i < _cLEDs; i++)
        {
            cooldown = random(0, Cooling);

            if (cooldown > heat[i])
            {
                heat[i] = 0;
            }
            else
            {
                heat[i] = heat[i] - cooldown;
            }
        }

        // Step 2.  Heat from each cell drifts 'up' and diffuses a little
        for (int k = _cLEDs - 1; k >= 3; k--)
        {
            heat[k] = (heat[k - 1] + heat[k - 2] + heat[k - 3]) / 3;
        }

        // Step 3.  Randomly ignite new 'sparks' near the bottom
        for (int frame = 0; frame < Sparks; frame++)
        {
            if (random(255) < Sparking)
            {
                int y = random(5);
                heat[y] = heat[y] + random(160, 255); // This randomly rolls over sometimes of course, and that's essential to the effect
            }
        }

        // Step 4.  Convert heat to LED colors
        for (int j = 0; j < _cLEDs; j++)
        {
            setPixelHeatColor(j, heat[j]);
        }

        //for (int channel = 0; channel < NUM_CHANNELS; channel++)
        //    blur1d(_GFX[channel]->GetLEDBuffer(), _cLEDs, 255);
    }

    void setPixelWithMirror(int Pixel, CRGB temperature)
    {
        //Serial.printf("Setting pixel %d to %d, %d, %d\n", Pixel, temperature.r, temperature.g, temperature.b);

        if (_Mirrored)
        {
            int middle = _cLEDs / 2;
            setPixel(middle - Pixel, temperature);
            setPixel(middle + Pixel, temperature);
        }
        else
        {
            if (_Reversed)
                setPixel(_cLEDs - 1 - Pixel, temperature);
            else
                setPixel(Pixel, temperature);
        } 
    }

    void setPixelHeatColor(int Pixel, byte temperature)
    {
        // Scale 'heat' down from 0-255 to 0-191
        byte t192 = round((temperature / 255.0) * 191);

        // calculate ramp up from
        byte heatramp = t192 & 0x3F; // 0..63
        heatramp <<= 2;              // scale up to 0..252

        // figure out which third of the spectrum we're in:
        if (t192 > 0x80)
        { // hottest
            setPixelWithMirror(Pixel, CRGB(255, 255, heatramp));
        }
        else if (t192 > 0x40)
        { // middle
            setPixelWithMirror(Pixel, CRGB(255, heatramp, 0));
        }
        else
        { // coolest
            setPixelWithMirror(Pixel, CRGB(heatramp, 0, 0));
        }
    }


};

class SmoothFireEffect : public LEDStripEffect
{
private:
    bool _Reversed;
    float _Cooling;
    int _Sparks;
    float _Drift;
    int _DriftPasses;
    int _SparkHeight;
    bool _Turbo;
    bool _Mirrored;

    float *_Temperatures;

public:
    // Parameter:   Cooling   Sparks    driftPasses  drift sparkHeight   Turbo
    // Calm Fire:     0.75f        2         1         64       8          F
    // Full Red:      0.75f        8         1        128      16          F
    // Good Video:    1.20f       64         1        128      12          F

    SmoothFireEffect(bool reversed = true,
                     float cooling = 1.2f,
                     int sparks = 16,
                     int driftPasses = 1,
                     float drift = 48,
                     int sparkHeight = 12,
                     bool turbo = false,
                     bool mirrored = false)

        : LEDStripEffect("Fire Sound Effect v2"),
          _Reversed(reversed),
          _Cooling(cooling),
          _Sparks(sparks),
          _Drift(drift),
          _DriftPasses(driftPasses),
          _SparkHeight(sparkHeight),
          _Turbo(turbo),
          _Mirrored(mirrored)
    {
    }

    virtual bool Init(shared_ptr<LEDMatrixGFX> gfx[NUM_CHANNELS])
    {
        LEDStripEffect::Init(gfx);
        _Temperatures = (float *)malloc(sizeof(float) * _cLEDs);
        if (!_Temperatures)
        {
            Serial.println("ERROR: Could not allocate memory for FireEffect");
            return false;
        }
        return true;
    }

    ~SmoothFireEffect()
    {
        free(_Temperatures);
    }

    virtual CRGB GetBlackBodyHeatColor(float temp)
    {
        temp = min(1.0f, temp);
        uint8_t temperature = (uint8_t)(255 * temp);
        uint8_t t192 = (uint8_t)((temperature / 255.0f) * 191);

        uint8_t heatramp = (uint8_t)(t192 & 0x3F);
        heatramp <<= 2;

        if (t192 > 0x80)
            return CRGB(255, 255, heatramp);
        else if (t192 > 0x40)
            return CRGB(255, heatramp, 0);
        else
            return CRGB(heatramp, 0, 0);
    }

    //double lastDraw = 0;

    virtual void Draw()
    {
        //if (g_AppTime.FrameStartTime() - lastDraw < 1.0/33.0)
        //    return;
        //lastDraw = g_AppTime.FrameStartTime();

        float deltaTime = (float)g_AppTime.DeltaTime();
        setAllOnAllChannels(0, 0, 0);

        float cooldown = randomDouble(0, _Cooling) * deltaTime;

        for (int i = 0; i < _cLEDs; i++)
            if (cooldown > _Temperatures[i])
                _Temperatures[i] = 0;
            else
                _Temperatures[i] = _Temperatures[i] - cooldown;

        // Heat from each cell drifts 'up' and diffuses a little
        for (int pass = 0; pass < _DriftPasses; pass++)
        {
            for (int k = _cLEDs - 1; k >= 3; k--)
            {
                float amount = 0.2f + gVURatio; // MIN(0.85f, _Drift * deltaTime);
                float c0 = 1.0f - amount;
                float c1 = amount * 0.33;
                float c2 = amount * 0.33;
                float c3 = amount * 0.33;

                _Temperatures[k] = _Temperatures[k] * c0 +
                                   _Temperatures[k - 1] * c1 +
                                   _Temperatures[k - 2] * c2 +
                                   _Temperatures[k - 3] * c3;
            }
        }

        // Randomly ignite new 'sparks' near the bottom
        for (int frame = 0; frame < _Sparks; frame++)
        {
            if (randomDouble(0, 1.0f) < 0.70f)
            {
                // NB: This randomly rolls over sometimes of course, and that's essential to the effect
                int y = randomDouble(0, _SparkHeight);
                _Temperatures[y] = (_Temperatures[y] + randomDouble(0.6f, 1.0f));

                if (!_Turbo)
                    while (_Temperatures[y] > 1.0)
                        _Temperatures[y] -= 1.0f;
                else
                    _Temperatures[y] = min(_Temperatures[y], 1.0f);
            }
        }

        for (uint j = 0; j < _cLEDs; j++)
        {
            CRGB c = GetBlackBodyHeatColor(_Temperatures[j]);
            setPixelWithMirror(j, c);
        }
    }

    void setPixelWithMirror(int Pixel, CRGB temperature)
    {
        if (_Reversed || _Mirrored)
            setPixel(Pixel, temperature);
        
        if (!_Reversed || _Mirrored)
            setPixel(_cLEDs - 1 - Pixel, temperature);
    }


    virtual const char *FriendlyName() const
    {
        return "Smooth Fire Effect";
    }
};

class BaseFireEffect : public LEDStripEffect
{
  protected:
    int     LEDCount;           // Number of LEDs total
    int     CellCount;          // How many heat cells to represent entire flame
    int     Cooling;            // Rate at which the pixels cool off
    int     Sparks;             // How many sparks will be attempted each frame
    int     SparkHeight;        // If created, max height for a spark
    int     Sparking;           // Probability of a spark each attempt
    bool    bReversed;          // If reversed we draw from 0 outwards
    bool    bMirrored;          // If mirrored we split and duplicate the drawing
    unique_ptr<byte []> heat;

    // When diffusing the fire upwards, these control how much to blend in from the cells below (ie: downward neighbors)
    // You can tune these coefficients to control how quickly and smoothly the fire spreads

    static const byte BlendSelf = 0;            // 2
    static const byte BlendNeighbor1 = 1;       // 3
    static const byte BlendNeighbor2 = 2;       // 2
    static const byte BlendNeighbor3 = 0;       // 1

    static const byte BlendTotal = (BlendSelf + BlendNeighbor1 + BlendNeighbor2 + BlendNeighbor3);

  public:

    BaseFireEffect(int ledCount, int cellsPerLED = 1, int cooling = 20, int sparking = 100, int sparks = 3, int sparkHeight = 4, bool breversed = false, bool bmirrored = false)
        : LEDStripEffect("BaseFireEffect"),
          Cooling(cooling),
          Sparks(sparks),
          SparkHeight(sparkHeight),
          Sparking(sparking),
          bReversed(breversed),
          bMirrored(bmirrored)
    {
        LEDCount = bmirrored ? LEDCount / 2 : LEDCount;
        CellCount = LEDCount * cellsPerLED;

        heat = make_unique<byte []>(CellCount);
    }

    virtual ~BaseFireEffect()
    {
    }

    virtual CRGB MapHeatToColor(byte temperature)
    {
        byte t192 = round((temperature/255.0)*191);
 
        // calculate ramp up from
        byte heatramp = t192 & 0x3F; // 0..63
        heatramp <<= 2; // scale up to 0..252

        // figure out which third of the spectrum we're in:
        if( t192 > 0x80) {                     // hottest
            return CRGB(255, 255, heatramp);
        } else if( t192 > 0x40 ) {             // middle
            return CRGB( 255, heatramp, 0);
        } else {                               // coolest
            return CRGB( heatramp, 0, 0);     
        }
    }

    virtual void Draw()
    {
        FastLED.showColor(CRGB::Red);
        return;
        FastLED.clear(false);
        DrawFire();
        delay(120);
    }

    virtual void DrawFire()
    {
        // First cool each cell by a litle bit
        for (int i = 0; i < CellCount; i++)
            heat[i] = max(0L, heat[i] - random(0, ((Cooling * 10) / CellCount) + 2));

        // Next drift heat up and diffuse it a little bit
        for (int i = 0; i < CellCount; i++)
            heat[i] = min(255, (heat[i] * BlendSelf +
                       heat[(i + 1) % CellCount] * BlendNeighbor1 +
                       heat[(i + 2) % CellCount] * BlendNeighbor2 +
                       heat[(i + 3) % CellCount] * BlendNeighbor3)
                      / BlendTotal);

        // Randomly ignite new sparks down in the flame kernel

        for (int i = 0 ; i < Sparks; i++)
        {
            if (random(255) < Sparking)
            {
                int y = CellCount - 1 - random(SparkHeight * CellCount / LEDCount);
                heat[y] = random(200, 255);// heat[y] + random(50, 255);       // Can roll over which actually looks good!
            }
        }

        // Finally, convert heat to a color

        int cellsPerLED = CellCount / LEDCount;
        for (int i = 0; i < LEDCount; i++)
        {
            int sum = 0;
            for (int iCell = 0; iCell < cellsPerLED; iCell++)
              sum += heat[i * cellsPerLED + iCell];
            int avg = sum / cellsPerLED;
            CRGB color = MapHeatToColor(heat[avg]);
            int j = bReversed ? (LEDCount - 1 - i) : i;
            setPixels(j, 1, color, true);
            if (bMirrored)
                setPixels(!bReversed ? (2 * LEDCount - 1 - i) : LEDCount + i, 1, color, true);
        }
    }
};