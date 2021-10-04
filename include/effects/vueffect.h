//+--------------------------------------------------------------------------
//
// File:        VUEffect.h
//
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
//    Draws a VU meter based on thec current sound measurements
//
// History:     Apr-17-2019         Davepl      Adapted from NightDriver
//              
//---------------------------------------------------------------------------

#pragma once

#include <sys/types.h>
#include <errno.h>
#include <iostream>
#include <vector>
#include <math.h>
#define fastled_internal 1
#include "FastLED.h"
#include "colorutils.h"
#include "globals.h"
#include "ledstripeffect.h"
#include "colordata.h"

#if ENABLE_AUDIO
#include "soundanalyzer.h"

extern AppTime g_AppTime;

using namespace std;


class VUEffect : public LEDStripEffect
{
private:
    int _colorSpeed;
	
public:

	VUEffect(int colorSpeed = 0) : LEDStripEffect("Double Palette Effect")
	{
        _colorSpeed = colorSpeed;
	}

    virtual bool Init(shared_ptr<LEDMatrixGFX> gfx[NUM_CHANNELS])
    {
        if (!LEDStripEffect::Init(gfx))
            return false;
        return true; 
    }

	virtual const char * FriendlyName() const
	{
		return "VU Effect";
	}

    inline void DrawVUPixels(int i, int fadeBy, const CRGBPalette256 & palette)
    {
        int xHalf = _cLEDs / 2;
        CRGB c = ColorFromPalette(palette, ::map(i, 0, xHalf, 0, 256)).fadeToBlackBy(fadeBy);
        
        if (0 != _colorSpeed)
        {
            nblend(c, ColorFromPalette(RainbowColors_p, beatsin16(_colorSpeed, 0, 256)), 128);
        }

        setPixel(xHalf - i - 1, c);
        setPixel(xHalf + i, c);
    }

    // DrawVURing - Draws a VU meter ring in the specified ring, rotated by the given amount

    inline virtual void DrawVULine()
    {
        //setAll(0,0,0);

        fadeAllChannelsToBlackBy(60);

        static int iPeakVUy = 0;           // size (in LED pixels) of the VU peak
        static unsigned long msPeakVU = 0; // timestamp in ms when that peak happened so we know how old it is

        const int MAX_FADE = 256;

        if (iPeakVUy > 1)
        {
            int fade = MAX_FADE * (millis() - msPeakVU) / (float)MILLIS_PER_SECOND;
            DrawVUPixels(iPeakVUy, fade, vuPaletteGreen);
            DrawVUPixels(iPeakVUy - 1, fade, vuPaletteGreen);
        }

        int xHalf = _cLEDs / 2 - 1;

        // Serial.printf("FPS: %d, VU: %f, gMinVU: %f, gPeakVU: %f, gVURatio: %f\n", g_AudioFPS, gVU, gMinVU, gPeakVU, gVURatio);
		    
        int barsReal = mapDouble(gMinVU, 0, MAX_VU, 1, xHalf);
        int bars = mapDouble(gVU, gMinVU, gPeakVU, 1, xHalf);

        bars = std::min(bars, xHalf);

        if (bars > iPeakVUy)
        {
            msPeakVU = millis();
            iPeakVUy = bars;
        }
        else if (millis() - msPeakVU > MILLIS_PER_SECOND)
        {
            iPeakVUy = 0;
        }

        for (int i = 0; i < bars; i++)
        {
            if (i <= barsReal)          
                DrawVUPixels(i, 0, vuPaletteBlue);
            else 
                DrawVUPixels(i, 0, vuPaletteGreen);
        }
    }	

	virtual void Draw()
	{
		DrawVULine();
	}
};



// VUFlameEffect
//
// A flame effect that responds to the music.  Listens to the audio and
// ignites sparks down in the core based on beat and VU.

class VUFlameEffect : public LEDStripEffect
{
  public:

    enum FlameType { REDX, GREENX, BLUEX, MULTICOLOR };

  protected:

    FlameType _flameType;
    float     _colorOffset;
    double    _lastDraw;
    uint      _frameRate;


    void setPixelWithMirror(int Pixel, CRGB temperature)
    {
        if (_Reversed || _Mirrored)
            setPixel(Pixel, temperature);
        
        if (!_Reversed || _Mirrored)
            setPixel(_cLEDs - 1 - Pixel, temperature);
    }

    // setPixelHeatColor
    //
    // Depending on the color mode, we generate a pixel color here based on the temperature.  For the classic flame that's
    // black up through red then yellow then white.  For the multicolor mode, the palette also cycles.

	void setPixelHeatColor(int Pixel, byte temperature) 
	{
        // Scale 'heat' down from 0-255 to 0-191
		byte t192 = round((temperature / 255.0) * 191);

		// calculate ramp up from
		byte heatramp = t192 & 0x3F; // 0..63
		heatramp <<= 2; // scale up to 0..252

        CRGB c;

        switch (_flameType)
        {
            case REDX:
            {
                if (t192 > 0x80)
                    c = CRGB(255, 255, heatramp);
                else if (t192 > 0x40)
                    c = CRGB(255, heatramp, 0);
                else 
                    c = CRGB(heatramp, 0, 0);                
                break;
            }
            
            case GREENX:
            {
                if (t192 > 0x80)
                    c = CRGB(255, 255, heatramp);
                else if (t192 > 0x40)
                    c = CRGB(heatramp, 255, 0);
                else 
                    c = CRGB(0, heatramp, 0);                
                break;
            }
                
            case BLUEX:
            {
                if (t192 > 0x80)
                    c = CRGB(255, 255, heatramp);
                else if (t192 > 0x40)
                    c = CRGB(0 , heatramp, 255);
                else 
                    c = CRGB(0, 0, heatramp);                
                break;
            }

            case MULTICOLOR:
            {
                c.setHSV((heatramp+_colorOffset), 255, heatramp);
                break;
            }
        }
        if (!_Mirrored || Pixel < _cLEDs / 2)
            setPixelWithMirror(Pixel, c);
	}    

    bool      _Reversed;
    bool      _Mirrored;
    
  public:

    VUFlameEffect(const char * pszName, FlameType flameType = MULTICOLOR, uint frameRate = 50, bool mirrored = false)
      : LEDStripEffect(pszName),
        _flameType(flameType),
        _colorOffset(0),
        _lastDraw(g_AppTime.FrameStartTime()),
        _frameRate(frameRate),
        _Mirrored(mirrored)   
    {
    }

	virtual void Draw()
	{
		static byte heat[NUM_LEDS];

        // A bit of a HACKy solution; we simply don't want to scroll too fast and the floating point versions look too "smooth"
        // so I'm staying frame-rate dependent on this one.  As long as it doesn't run too fast, it looks fine, so we just bail
        // if it's been too little time since the last time.  TODO:  Could I put this update parts in FixedUpdate() instead if
        // I made this a MonoBehavior perhaps?

        if (g_AppTime.FrameStartTime() - _lastDraw < 1.0/_frameRate)
            return;

        _lastDraw = g_AppTime.FrameStartTime();

        // Cycle the color (used by multicoor mode only)
        _colorOffset = fmod(_colorOffset + 16 * gVURatio, 240); //  * _intensityAdjust.GetValue(), 240);

        // Cool down every cell a little randomly

    	int cooldown;
        for (int i = 0; i < _cLEDs; i++) 
        {
            cooldown = random(0, 2); //  * _coolingAdjust.GetValue();
            if (cooldown > heat[i]) 
                heat[i] = 0;
            else 
                heat[i] = heat[i] - cooldown;
        }

        // Heat from each cell drifts 'up' and diffuses a little

        for (int k = _cLEDs - 1; k >= 2; k--) 
            heat[k] = (heat[k] + heat[k - 1] + heat[k - 2]) / 3;
        

        // Randomly ignite new 'sparks' near the bottom
        // We use the ratio of the VU to its peak, which tells us the absolute volume, so we don't display stuff when really quiet

        float ratio = (gVU - gMinVU) / (MAX_VU - gMinVU);

        for (int frame = 0; frame < 6; frame++)
            if (random(255) < (255 * ratio ) ) 
                heat[random(24)] += random(160, 255);		// This randomly rolls over sometimes of course, which seems inadvertantly essential to the effect

        // Convert heat to LED colors and draws them

        for (int j = 0; j < _cLEDs; j++) 
            setPixelHeatColor(j, heat[j]);
	}
};


class SpectrumEffect : public LEDStripEffect
{
    SpectrumEffect() : LEDStripEffect("Double Palette Effect")
    {

    }
    virtual const char * FriendlyName() const
	{
		return "Spectrum Effect";
	}

    virtual void Draw()
    {
        setAllOnAllChannels(0,0,0);

        for (int band = 0; band < NUM_BANDS; band++)
        {
            CRGB color;
            color.setHSV(band * 32, 255, 255);
            int xStartPixel = (band * _cLEDs) / NUM_BANDS;
            int xLength = _cLEDs * min(1.0, 1.0) / NUM_BANDS;
            for (int xPixel = xStartPixel; xPixel < xStartPixel + xLength; xPixel++)
                setPixel(xPixel, color);
        }
    }
};

#endif // ENABLE_AUDIO
