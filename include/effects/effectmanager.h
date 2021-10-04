//+--------------------------------------------------------------------------
//
// File:        EffectManager.h
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
//    Based on my original ESP32LEDStick project this is the class that keeps
//    track of internal efects, which one is active, rotating among them,
//    and fading between them.
//
//
//
// History:     Apr-13-2019         Davepl      Created for NightDriverStrip
//              
//---------------------------------------------------------------------------

#pragma once

#include <sys/types.h>
#include <errno.h>
#include <iostream>
#include <memory>
#include <vector>
#include <math.h>
#include "colorutils.h"
#include "ledmatrixgfx.h"
#include "ledstripeffect.h"
#include "globals.h"
#include "misceffects.h"

#define MAX_EFFECTS 32

using namespace std;

extern byte g_Brightness;
extern byte g_Fader;

// References to functions in other C files

void InitEffectsManager();
unique_ptr<LEDStripEffect> GetSpectrumAnalyzer(CRGB color);

// EffectManager
// 
// Handles keeping track of the effects, which one is active, asking it to draw, etc.

class EffectManager
{
	LEDStripEffect ** _ppEffects;
	size_t            _cEffects;

	size_t			 _iCurrentEffect;
	uint    		 _effectStartTime;
	uint    		 _effectInterval;
	bool             _bPlayAll;
	
	unique_ptr<bool []> _abEffectEnabled;
	shared_ptr<LEDMatrixGFX> * _gfx;
	unique_ptr<LEDStripEffect> _pRemoteEffect;

public:

	static const uint csFadeButtonSpeed = 15 * 1000;
	static const uint csSmoothButtonSpeed = 60 * 1000;

	EffectManager(LEDStripEffect ** pEffects, size_t cEffects, shared_ptr<LEDMatrixGFX> * gfx)
		  : _ppEffects(pEffects),
	  	    _cEffects(cEffects),
		    _effectInterval(DEFAULT_EFFECT_INTERVAL),
		    _bPlayAll(false),
			_gfx(gfx)
	{
		debugV("EffectManager Constructor");
		_iCurrentEffect = 0;
		_effectStartTime = millis();
		_abEffectEnabled = make_unique<bool []>(cEffects);
			
		for (int i = 0; i < cEffects; i++)
			EnableEffect(i);
	}

	~EffectManager()
	{
		ClearRemoteColor();
	}

	shared_ptr<LEDMatrixGFX> operator [](size_t index)
	{
		return _gfx[index];
	}

	#if ATOMLIGHT
		static const uint FireEffectIndex = 2;			// Index of the fire effect in the AllEffects table (BUGBUG hardcoded)
		static const uint VUEffectIndex   = 6;			// Index of the fire effect in the AllEffects table (BUGBUG hardcoded)
	#elif FANSET
		static const uint FireEffectIndex = 1;			// Index of the fire effect in the AllEffects table (BUGBUG hardcoded)
	#elif BROOKLYNROOM
		static const uint FireEffectIndex = 2;			// Index of the fire effect in the AllEffects table (BUGBUG hardcoded)
		static const uint VUEffectIndex   = 6;			// Index of the fire effect in the AllEffects table (BUGBUG hardcoded)
	#else
		static const uint FireEffectIndex = 0;			// Index of the fire effect in the AllEffects table (BUGBUG hardcoded)
		static const uint VUEffectIndex   = 0;			// Index of the fire effect in the AllEffects table (BUGBUG hardcoded)
	#endif

	// SetGlobalColor
	// 
	// When a global color is set via the remote, we create a fill effect and assign it as the "remote effect"
	// which takes drawing precedence

	void SetGlobalColor(CRGB color)
	{
		ClearRemoteColor();
		#if SPECTRUM
			_pRemoteEffect = GetSpectrumAnalyzer(color);
		#else
			_pRemoteEffect = make_unique<ColorFillEffect>(color, 1);
		#endif

		if (!_pRemoteEffect->Init(_gfx))
		{
			ClearRemoteColor();
			debugE("Unable to init global color effect\n");
			return;
		}
	}

	void ClearRemoteColor()
	{
		_pRemoteEffect.release();
		_pRemoteEffect = nullptr;	
	}

	void EnableEffect(size_t i)
	{
		if (i >= _cEffects)
		{
			debugW("Invalid index for EnableEffect");
			return;
		}
		_abEffectEnabled[i] = true;
	}

	void DisableEffect(size_t i)
	{
		if (i >= _cEffects)
		{
			debugW("Invalid index for DisableEffect");
			return;
		}
		_abEffectEnabled[i] = false;
	}

	bool IsEffectEnabled(size_t i) const
	{
		if (i >= _cEffects)
		{
			debugW("Invalid index for IsEffectEnabled");
			return false;
		}
		return _abEffectEnabled[i];
	}

	void PlayAll(bool bPlayAll)
	{
		_bPlayAll = bPlayAll;
	}

	uint GetInterval() const
	{
		return _effectInterval;
	}

	void SetInterval(uint interval)
	{
		_effectInterval = interval;
		//_effectStartTime = millis();
	}

	const LEDStripEffect * const * EffectsList() const
	{
		return _ppEffects;
	}

	const size_t EffectCount() const
	{
		return _cEffects;
	}

	const size_t GetCurrentEffectIndex() const
	{
		return _iCurrentEffect;
	}

	LEDStripEffect * GetCurrentEffect() const
	{
		return _ppEffects[_iCurrentEffect];
	}

	const char * GetCurrentEffectName() const
	{
		return _ppEffects[_iCurrentEffect]->FriendlyName();
	}

	// Change the current effect; marks the state as needing attention so this get noticed next frame

	void SetCurrentEffectIndex(size_t i)
	{
		if (i >= _cEffects)
		{
			debugW("Invalid index for SetCurrentEffectIndex");
			return;
		}
		_iCurrentEffect = i;
		_effectStartTime = millis();
	}

	uint GetTimeRemainingForCurrentEffect() const
	{
		if (GetTimeUsedByCurrentEffect() > GetInterval())
			return 0;
		return GetInterval() - GetTimeUsedByCurrentEffect();
	}

	uint GetTimeUsedByCurrentEffect() const
	{
		return millis() - _effectStartTime;
	}

	void CheckEffectTimerExpired()
	{
		if (millis() - _effectStartTime >= GetInterval())			// See if its time for a new effect yet
		{
			debugI("%ldms elapsed: Next Effect", millis() - _effectStartTime);
			NextEffect();
			debugI("Current Effect: %s", GetCurrentEffectName());
		}
	}

	// Update to the next effect and abort the current effect.  

	void NextEffect()
	{
		do
		{
			_iCurrentEffect++;						//   ... if so advance to next effect
			_iCurrentEffect %= EffectCount();
			_effectStartTime = millis();
		} while (false == _bPlayAll && false == IsEffectEnabled(_iCurrentEffect));
	}

	// Go back to the previous effect and abort the current one.  

	void PreviousEffect()
	{
		do
		{
			if (_iCurrentEffect == 0)
				_iCurrentEffect = EffectCount() - 1;

			_iCurrentEffect--;
			_effectStartTime = millis();
		} while (false == _bPlayAll && false == IsEffectEnabled(_iCurrentEffect));
	}

	bool Init()
    {

       	for (int i = 0; i < _cEffects; i++)
        {
    		if (false == _ppEffects[i]->Init(_gfx))
            {
                debugW("Could not initialize effect: %s\n", _ppEffects[i]->FriendlyName());
                return false;
			}
			// First time only, we ensure the data is cleared 
			
			//_ppEffects[i]->setAll(0,0,0);
		}
		debugI("First Effect: %s", GetCurrentEffectName());
        return true;
    }
	
	// EffectManager::Update
	//
	// Draws the current effect.  If gUIDirty has been set by an interrupt handler, it is reset here

	void Update()
    {
		if ((_gfx[0])->GetLEDCount() == 0)
			return;

		const float msFadeTime = EFFECT_CROSS_FADE_TIME;

		CheckEffectTimerExpired();

		// If a remote control effect is set, we draw that, otherwise we draw the regular effect
		
		if (_pRemoteEffect)
		{
			_pRemoteEffect->Draw();
		}
		else
		{
			_ppEffects[_iCurrentEffect]->Draw();	// Draw the currently active effect
	
			// If we do indeed have muliple effects (BUGBUG what if only a single enabled?) then we 
			// fade in and out at the appropriate time based on the time remaining/used by the effect
		
			if (EffectCount() < 2)	
			{
				g_Fader = 255;
				return;
			}

			int r = GetTimeRemainingForCurrentEffect();
			int e = GetTimeUsedByCurrentEffect();

			if (e < msFadeTime)
			{
				g_Fader = 255 * (e / msFadeTime);					// Fade in
			}
			else if (r < msFadeTime)
			{
				g_Fader = 255 * (r / msFadeTime);					// Fade out
			}
			else
			{
				g_Fader = 255;										// No fade, not at start or end
			}
		}
    }
};

// FractionalColor
//
// Returns a fraction of a color; abstracts the fadeToBlack away so that we can later
// do better color correction as needed

inline CRGB ColorFraction(const CRGB colorIn, float fraction)
{
  fraction = min(1.0f, fraction);
  fraction = max(0.0f, fraction);
  return CRGB(colorIn).fadeToBlackBy(255 * (1.0f - fraction));
}

#ifdef FAN_SIZE
inline void RotateForward(int iStart, int length = FAN_SIZE, int count = 1)
{
	std::rotate(&FastLED.leds()[iStart], &FastLED.leds()[iStart + count], &FastLED.leds()[iStart + length]);
}

inline void RotateReverse(int iStart, int length = FAN_SIZE, int count = 1)
{
	std::rotate(&FastLED.leds()[iStart], &FastLED.leds()[iStart + length - count], &FastLED.leds()[iStart + length]);
}

// Rotate
//
// Rotate all the pixels in the buffer forward or back

inline void RotateAll(bool bForward = true, int count = 1)
{
	if (bForward)
		RotateForward(0, count);
	else
		RotateReverse(0, count);
}
#endif

#ifdef FAN_SIZE

// RotateFan
//
// Rotate one circular section within itself, like a single fan

inline void RotateFan(int iFan, bool bForward = true, int count = 1)
{
	if (bForward)
		RotateForward(iFan * FAN_SIZE, FAN_SIZE, count);
	else
		RotateReverse(iFan * FAN_SIZE, FAN_SIZE, count);
}

#endif

