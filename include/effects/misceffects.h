//+--------------------------------------------------------------------------
//
// File:        misceffects.h
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
//    Draws bouncing balls using a kinematics formula
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
#define FASTLED_INTERNAL 1
#include "FastLED.h"
#include "colorutils.h"
#include "globals.h"
#include "ledstripeffect.h"

// SimpleRainbowTestEffect
//
// Fills the spokes with a rainbow palette, skipping dots as specified

class SimpleRainbowTestEffect : public LEDStripEffect
{
  private:
	uint8_t		_EveryNth;
	uint8_t     _SpeedDivisor;

  public:
  
    SimpleRainbowTestEffect(uint8_t speedDivisor = 8, uint8_t everyNthPixel = 12)
	  : LEDStripEffect("Simple Rainbow"),
		  _EveryNth(everyNthPixel),
	    _SpeedDivisor(speedDivisor)
	{
		debugV("SimpleRainbowTestEffect constructor");
	}

	virtual void Draw() 
    {
        fillRainbowAllChannels(0, _cLEDs, beatsin16(4, 0, 256), 5, _EveryNth);
		delay(10);
    }
	
    virtual const char * FriendlyName() const
    {
        return "Sample Effect";
    }
};

// SimpleRainbowTestEffect
//
// Fills the spokes with a rainbow palette, skipping dots as specified

class RainbowTwinkleEffect : public LEDStripEffect
{
  private:
	float _speedDivisor;
	int   _deltaHue;

  public:
  
    RainbowTwinkleEffect(float speedDivisor = 12.0f, int deltaHue = 14)
	  : LEDStripEffect("RainowFill Rainbow"),
	    _speedDivisor(speedDivisor),
		_deltaHue(deltaHue)
	{
		debugV("RainbowFill constructor");
	}

	virtual void Draw()
	{
		static float hue = 0.0f;
		static unsigned long lastms = millis();

		unsigned long msElapsed = millis() - lastms;
		lastms = millis();

		hue += (float) msElapsed / _speedDivisor;
		hue = fmod(hue, 256.0);
		fillRainbowAllChannels(0, _cLEDs, hue, _deltaHue);

		if (random(0, 1) == 0)
			setPixel(random(0, _cLEDs), CRGB::White);
		delay(10);
	}

    virtual const char * FriendlyName() const
    {
        return "RainbowTwinkle Effect";
    }
};

// RainbowFillEffect
//
// Fills the spokes with a rainbow palette


class RainbowFillEffect : public LEDStripEffect
{
  private:

protected:

	float _speedDivisor;
	int   _deltaHue;

  public:
	
    RainbowFillEffect(float speedDivisor = 12.0f, int deltaHue = 14)
	  : LEDStripEffect("RainowFill Rainbow"),
	    _speedDivisor(speedDivisor),
		_deltaHue(deltaHue)
	{
		debugV("RainbowFill constructor");
	}

	virtual void Draw()
	{
		static float hue = 0.0f;
		static unsigned long lastms = millis();

		unsigned long msElapsed = millis() - lastms;
		lastms = millis();

		hue += (float) msElapsed / _speedDivisor;
		hue = fmod(hue, 256.0);
		fillRainbowAllChannels(0, _cLEDs, hue, _deltaHue);
		delay(10);
	}

    virtual const char * FriendlyName() const
    {
        return "RainbowFill Effect";
    }
};

// RainbowFillEffect
//
// Fills the spokes with a rainbow palette


class ColorFillEffect : public LEDStripEffect
{
  private:

protected:

	int _everyNth;
	CRGB _color;

  public:
	
    ColorFillEffect(CRGB color = CRGB(246,200,160), int everyNth = 10)
	  : LEDStripEffect("Color Fill"),
	    _everyNth(everyNth),
		_color(color)
	{
		debugV("Color Fill constructor");
	}

	virtual void Draw()
	{
		fillSolidOnAllChannels(CRGB::Black);
		fillSolidOnAllChannels(_color, 0, NUM_LEDS, _everyNth);
	}

    virtual const char * FriendlyName() const
    {
        return "Color Fill Effect";
    }
};

class StatusEffect : public LEDStripEffect
{
  protected:

	int  _everyNth;
	CRGB _color;

  public:
	
    StatusEffect(CRGB color = CRGB(246,200,160), int everyNth = 10)
	  : LEDStripEffect("Color Fill"),
	    _everyNth(everyNth),
		_color(color)
	{
		debugV("Status Fill constructor");
	}

	virtual void Draw()
	{
		CRGB color = _color;

		if (g_bUpdateStarted)
			color = CRGB::Purple;
		else if (!WiFi.isConnected())
			color = CRGB::Red;
		else if (!NTPTimeClient::HasClockBeenSet())
			color = CRGB::Green;

		fillSolidOnAllChannels(CRGB::Black);
		fillSolidOnAllChannels(color, 0, 0, _everyNth);
	}

    virtual const char * FriendlyName() const
    {
        return "Status Fill Effect";
    }
};

#if BELT
static const CRGB TwinkleColors[] = 
{
	CRGB::Red,
	CRGB::Green,
	CRGB::Blue,
	CRGB::White
};
#else
static const CRGB TwinkleColors[] = 
{
    CRGB(238, 51, 39),      // Red
    CRGB(0, 172, 87),       // Green
    CRGB(250, 164, 25),     // Yellow
    CRGB(0, 131, 203)       // Blue
};
#endif

class TwinkleEffect : public LEDStripEffect
{
  protected:

	int  _countToDraw;
	int  _fadeFactor;
	int  _updateSpeed;

  public:
	
    TwinkleEffect(int countToDraw = NUM_LEDS / 2, byte fadeFactor = 10, int updateSpeed = 10)
	  : LEDStripEffect("Twinkle"),
	  	_countToDraw(countToDraw),
		_fadeFactor(fadeFactor),
		_updateSpeed(updateSpeed)
	{
	}

	const int Count = 99;
	int buffer[99] = { 0 };

	std::deque<size_t> litPixels;

	virtual void Draw()
	{
		CRGB * pPixels = _GFX[0]->GetLEDBuffer();
		EVERY_N_MILLISECONDS(_updateSpeed)
		{
			if (litPixels.size() > _countToDraw)
			{
				size_t i = litPixels.back();
				litPixels.pop_back();
				pPixels[i] = CRGB::Black;
			}
		
			// Pick a random pixel and put it in the TOP slot
			int iNew = -1;
			for (int iPass = 0; iPass < NUM_LEDS * 10; iPass++)
			{
				size_t i = random(0, NUM_LEDS);
				if (pPixels[i] != CRGB(0,0,0))
					continue;
				iNew = i;
				break;
			}
			if (iNew == -1)				// No empty slot could be found!
			{	
				litPixels.clear();
				setAllOnAllChannels(0,0,0);
				return;
			}
			
			assert(litPixels.end() == find(litPixels.begin(), litPixels.end(), iNew));
			pPixels[iNew] = TwinkleColors[random(0, ARRAYSIZE(TwinkleColors))];

			if (pPixels[iNew] == CRGB(0,0,0))
			{
				debugI("Just set pixel %d to %d,%d,%d but shows as black", iNew, pPixels[iNew].r, pPixels[iNew].g, pPixels[iNew].b );
			}
			
			
			litPixels.push_front(iNew);
		}

		EVERY_N_MILLISECONDS(20)
		{
			fadeToBlackBy(FastLED.leds(), NUM_LEDS, _fadeFactor);
		}
	}
};

class PoliceEffect : public LEDStripEffect
{
	typedef enum { INNERRED, OUTERRED, INNERBLUE, OUTERBLUE, MIXED, STROBE } lampStates;
	const lampStates highestState = STROBE;

	virtual void Draw()
	{
		

	}
};
