//+--------------------------------------------------------------------------
//
// File:        spectrumeffects.h
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
//   Classes for moving and fading little render objects over time, 
//   used as a base for the star and insulator effects
//
// History:     Jul-7-2021         Davepl      Commented
//
//---------------------------------------------------------------------------

#pragma once

#include <sys/types.h>
#include <errno.h>
#include <iostream>
#include <vector>
#include <math.h>
#include <deque>
#include "colorutils.h"
#include "globals.h"
#include "ledstripeffect.h"
#include "soundanalyzer.h"
#include "musiceffect.h"
#include "particles.h"
#include "screen.h"

#if ENABLE_AUDIO

extern AppTime  g_AppTime;
extern PeakData g_Peaks;
extern DRAM_ATTR uint8_t giInfoPage;                   // Which page of the display is being shown
extern DRAM_ATTR bool gbInfoPageDirty;

class InsulatorSpectrumEffect : public virtual BeatEffectBase, public virtual ParticleSystemEffect<SpinningPaletteRingParticle>
{
    int                    _iLastInsulator = 0;
    const CRGBPalette256 & _Palette;
    CRGB _baseColor = CRGB::Black;
    
  public:

    InsulatorSpectrumEffect(const char * pszName, const CRGBPalette256 & Palette)
      : LEDStripEffect(pszName),
        BeatEffectBase(0.25, 1.75, .25),
        ParticleSystemEffect<SpinningPaletteRingParticle>(pszName),
        _Palette(Palette)
    {
    }

    virtual void Draw()
    {
        //fillSolidOnAllChannels(CRGB::Black);
        for (int band = 0; band < min(NUM_BANDS, NUM_FANS); band++) {
            CRGB color = ColorFromPalette(_Palette, map(band, 0, min(NUM_BANDS, NUM_FANS), 0, 255) + beatsin8(1) );
            color = color.fadeToBlackBy(255 - 255 * g_Peaks[band]);
            color = color.fadeToBlackBy((2.0 - gVURatio) * 228);
            DrawRingPixels(0, FAN_SIZE * g_Peaks[band], color, NUM_FANS-1-band, 0);
        }

        BeatEffectBase::Draw();
        ParticleSystemEffect<SpinningPaletteRingParticle>::Draw();        
      
        fadeAllChannelsToBlackBy(min(255.0,2000 * g_AppTime.DeltaTime()));
        delay(30);
    }

    virtual void HandleBeat(bool bMajor, float elapsed, double span)
    {
        int iInsulator;
        do
        {
          iInsulator = random(0, NUM_FANS);
        } while (NUM_FANS > 3 && iInsulator == _iLastInsulator);
        _iLastInsulator = iInsulator;
        

        // REVIEW(davepl) This might look interesting if it didn't erase...
        bool bFlash = gVURatio > 1.99 && span > 1.9 && elapsed > 0.25;

        _allParticles.push_back(SpinningPaletteRingParticle(_GFX, iInsulator, 0, _Palette, 256.0/FAN_SIZE, 4, -0.5, RING_SIZE_0, 0, LINEARBLEND, true, 1.0, bFlash ? max(0.12f, elapsed/8) : 0));
    }
};

class VUMeterEffect : public LEDStripEffect
{
  protected:

    // DrawVUPixels
    //
    // Draw i-th pixel in row y

	void DrawVUPixels(int i, int yVU, int fadeBy = 0)
	{
        auto pGFXChannel = _GFX[0];

		int xHalf = pGFXChannel->width()/2;
		pGFXChannel->drawPixel(xHalf-i-1, yVU, ColorFromPalette(vuPaletteGreen, i*(256/xHalf)).fadeToBlackBy(fadeBy));
		pGFXChannel->drawPixel(xHalf+i,   yVU, ColorFromPalette(vuPaletteGreen, i*(256/xHalf)).fadeToBlackBy(fadeBy));
    }

    // DrawVUMeter
    // 
    // Draws the symmetrical VU meter along with its fading peaks up at the top of the display.

    void DrawVUMeter(int yVU)
    {
        auto pGFXChannel = _GFX[0];

        static int iPeakVUy = 0;        // size (in LED pixels) of the VU peak
        static unsigned long msPeakVU = 0;       // timestamp in ms when that peak happened so we know how old it is

        const int MAX_FADE = 256;

        pGFXChannel->fillRect(0, yVU, MATRIX_WIDTH, 1, BLACK16);

        if (iPeakVUy > 1)
        {
            int fade = MAX_FADE * (millis() - msPeakVU) / (float) MS_PER_SECOND;
            DrawVUPixels(iPeakVUy,   yVU, fade);
            DrawVUPixels(iPeakVUy-1, yVU, fade);
        }

        int xHalf = pGFXChannel->width()/2-1;
        int bars  = gVURatio / 2.0 * xHalf; // map(gVU, 0, MAX_VU/8, 1, xHalf);
        bars = min(bars, xHalf);

        if (bars > iPeakVUy)
        {
            msPeakVU = millis();
            iPeakVUy = bars;
        }
        else if (millis() - msPeakVU > MS_PER_SECOND)
        {
            iPeakVUy = 0;
        }

        for (int i = 0; i < bars; i++)
            DrawVUPixels(i, yVU, i > bars ? 255 : 0);
    }

  public:

    VUMeterEffect(const char * pszName = nullptr)
      : LEDStripEffect(pszName)
    {
    }
};

// SpectrumAnalyzerEffect
//
// An effect that draws an audio spectrum analyzer on a matrix.  It is assumed that the
// matrix is 48x16 using LED Channel 0 only.   Has a VU meter up top and 16 bands.

class SpectrumAnalyzerEffect : public VUMeterEffect
{
  protected:

    uint8_t   _colorOffset;
    uint16_t  _scrollSpeed;
    uint8_t   _fadeRate;

    CRGBPalette256 _palette;

    // DrawBand
    //
    // Draws the bar graph rectangle for a bar and then the white line on top of it

    void DrawBand(byte iBand, uint16_t baseColor)
    {
        auto pGFXChannel = _GFX[0];

        int value  = g_peak1Decay[iBand] * (pGFXChannel->height() - 1);
        int value2 = g_peak2Decay[iBand] *  pGFXChannel->height();

        debugV("Band: %d, Value: %f\n", iBand, g_peak1Decay[iBand] );

        if (value > pGFXChannel->height())
            value = pGFXChannel->height();
    
        if (value2 > pGFXChannel->height())
            value2 = pGFXChannel->height();

        int bandWidth = pGFXChannel->width() / NUM_BANDS;
        int xOffset   = iBand * bandWidth;
        int yOffset   = pGFXChannel->height() - value;
        int yOffset2  = pGFXChannel->height() - value2;
    
        int iRow = 0;
        for (int y = yOffset2; y < pGFXChannel->height(); y++)
        {
            CRGB color = pGFXChannel->from16Bit(baseColor);
            iRow++;
            pGFXChannel->drawLine(xOffset, y, xOffset+bandWidth-1, y, pGFXChannel->to16bit(color));
        }
    
        #if SHADE_BAND_EDGE
            CRGB color = pGFXChannel->from16Bit(baseColor);
            color.fadeToBlackBy(32);
            baseColor = pGFXChannel->to16bit(color);
            pGFXChannel->drawLine(xOffset+bandWidth-1, yOffset2, xOffset+bandWidth-1, pGFXChannel->height(), baseColor);
        #endif

        const int PeakFadeTime_ms = 1000;

        CRGB colorHighlight = CRGB(CRGB::White);
	    unsigned long msPeakAge = millis() - g_lastPeak1Time[iBand];
	    if (msPeakAge > PeakFadeTime_ms)
		    msPeakAge = PeakFadeTime_ms;
	    
        float agePercent = (float) msPeakAge / (float) MS_PER_SECOND;
	    byte fadeAmount = std::min(255.0f, agePercent * 256);

        colorHighlight = CRGB(CRGB::White).fadeToBlackBy(fadeAmount);

        if (value == 0)
		    colorHighlight = pGFXChannel->from16Bit(baseColor);

        // if decay rate is less than zero we interpret that here to mean "don't draw it at all".  

        if (g_peak1DecayRate >= 0.0f)
            pGFXChannel->drawLine(xOffset, max(0, yOffset-1), xOffset + bandWidth - 1, max(0, yOffset-1),pGFXChannel->to16bit(colorHighlight));
    }

  public:

    SpectrumAnalyzerEffect(const char   * pszFriendlyName = nullptr, 
                           const CRGBPalette256 & palette = spectrumBasicColors, 
                           uint16_t           scrollSpeed = 0, 
                           uint8_t               fadeRate = 0,
                           float           peak1DecayRate = 2.0,
                           float           peak2DecayRate = 2.0)
        : VUMeterEffect(pszFriendlyName),
          _colorOffset(0),
          _scrollSpeed(scrollSpeed), 
          _fadeRate(fadeRate),
          _palette(palette)
    {
        g_peak1DecayRate = peak1DecayRate;
        g_peak2DecayRate = peak2DecayRate;
    }

    SpectrumAnalyzerEffect(const char   * pszFriendlyName = nullptr, 
                           CRGB                 baseColor = 0, 
                           uint8_t               fadeRate = 0,
                           float           peak1DecayRate = 2.0,
                           float           peak2DecayRate = 2.0)
        : VUMeterEffect(pszFriendlyName),
          _colorOffset(0),
          _scrollSpeed(0), 
          _fadeRate(fadeRate),
          _palette(baseColor)
    {
    }

    virtual void Draw()
    {
        auto pGFXChannel = _GFX[0];

        if (_scrollSpeed > 0)
        {
            EVERY_N_MILLISECONDS(_scrollSpeed)
            {
                _colorOffset+=2;
            }
        }

        if (_fadeRate)
            fadeAllChannelsToBlackBy(_fadeRate);
        else
            fillSolidOnAllChannels(CRGB::Black);

  
        std::lock_guard<std::mutex> guard(Screen::_screenMutex);

        DrawVUMeter(0);
        for (int i = 0; i < NUM_BANDS; i++)
        {
            CRGB bandColor = ColorFromPalette(_palette, (::map(i, 0, NUM_BANDS, 0, 255) + _colorOffset) % 256);
            DrawBand(i, pGFXChannel->to16bit(bandColor));
        }
    }
};

// WaveformEffect [MATRIX EFFECT]
//
// Draws a colorful scrolling waveform driven by instantaneous VU as it scrolls

class WaveformEffect : public VUMeterEffect
{
  protected:
    const CRGBPalette256 * _pPalette = nullptr;
    byte                   _iColorOffset = 0;
    byte                   _increment = 0;

  public:
	
    WaveformEffect(const char * pszFriendlyName, const CRGBPalette256 * pPalette = nullptr, byte increment = 0) 
        : VUMeterEffect(pszFriendlyName)
	{
        _pPalette = pPalette;
        _increment = increment;
    }

    void DrawSpike(int x, double v) 
    {
        int yTop = (MATRIX_HEIGHT / 2) - v * (MATRIX_HEIGHT  / 2 - 1);
        int yBottom = (MATRIX_HEIGHT / 2) + v * (MATRIX_HEIGHT / 2) ;
        if (yTop < 1)
            yTop = 1;
        if (yBottom > MATRIX_HEIGHT - 1)
            yBottom = MATRIX_HEIGHT - 1;

        for (int y=1; y < MATRIX_HEIGHT; y++)
        {
            int x1 = abs(MATRIX_HEIGHT / 2 - y);
            int dx = 256 / std::max(1, (MATRIX_HEIGHT / 2));
            CRGB color = CRGB::Black;
            // Invert index so that a rainbow ends up with red at the end, which would match
            // our red VU pixels
            auto index = (x1 * dx +_iColorOffset) % 256;
            if (y >= yTop && y <= yBottom )
            {
                if (y < 2 || y > (MATRIX_HEIGHT - 2))
                    color  = CRGB::Red;
                else
                    color = ColorFromPalette(*_pPalette, 255 - index);

                // Sparkles
                //
                // if (random(16)  < 2)
                //    color = CRGB::White;

            }
                
            _GFX[0]->drawPixel(x, y, color);
        }
        _iColorOffset = (_iColorOffset + _increment) % 255;

    }

    virtual void Draw()
	{
        auto leds = _GFX[0]->GetLEDBuffer();

        memcpy(&leds[0], &leds[1], sizeof(leds[0]) * NUM_LEDS-1);
        DrawVUMeter(0);        
        DrawSpike(63, gVURatio);
	}
};

class GhostWave : public WaveformEffect
{
    public:

    GhostWave(const char * pszFriendlyName = nullptr, const CRGBPalette256 * pPalette = nullptr, byte increment = 0) 
        : WaveformEffect(pszFriendlyName, pPalette, increment)
    {
    }

	virtual void Draw()
	{
        auto graphics = _GFX[0];
        auto peak = gVURatio / 2;

        for (int y = 1; y < MATRIX_HEIGHT; y++)
        {
            for (int x = 0; x < MATRIX_WIDTH / 2 - 1; x++)
            {
                graphics->drawPixel(x, y, graphics->getPixel(x+1, y));
                graphics->drawPixel(MATRIX_WIDTH-x-1, y, graphics->getPixel(MATRIX_WIDTH-x-2, y));
            }
        }
    
        DrawVUMeter(0);
        DrawSpike(MATRIX_WIDTH/2, peak);
        DrawSpike(MATRIX_WIDTH/2-1, peak);
        //BlurFrame(32);
	}
};

#endif
