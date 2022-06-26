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
#include "effects/strip/musiceffect.h"
#include "effects/strip/particles.h"
#include "screen.h"
#include "gfxbase.h"

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
            CRGB color = ColorFromPalette(_Palette, ::map(band, 0, min(NUM_BANDS, NUM_FANS), 0, 255) + beatsin8(1) );
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

class VUMeterEffect 
{
  protected:

    // DrawVUPixels
    //
    // Draw i-th pixel in row y

    void DrawVUPixels(GFXBase * pGFXChannel, int i, int yVU, int fadeBy = 0, CRGBPalette256 * pPalette = nullptr)
    {
        int xHalf = pGFXChannel->width()/2;
        pGFXChannel->setPixel(xHalf-i-1, yVU, ColorFromPalette(pPalette ? *pPalette : vuPaletteGreen,  i*(256/xHalf)).fadeToBlackBy(fadeBy));
        pGFXChannel->setPixel(xHalf+i,   yVU, ColorFromPalette(pPalette ? *pPalette : vuPaletteGreen, i*(256/xHalf)).fadeToBlackBy(fadeBy));
    }

    // DrawVUMeter
    // 
    // Draws the symmetrical VU meter along with its fading peaks up at the top of the display.
    
    int iPeakVUy = 0;                 // size (in LED pixels) of the VU peak
    unsigned long msPeakVU = 0;       // timestamp in ms when that peak happened so we know how old it is
    double lastVU = 0;
    const double VU_DECAY_PER_SECOND = 3.0;


    void DrawVUMeter(GFXBase * pGFXChannel, int yVU, CRGBPalette256 * pPalette = nullptr)
    {
        const int MAX_FADE = 256;

        pGFXChannel->drawLine(0, 0, MATRIX_WIDTH, 0, CRGB::Black);
        //        fillRect(0, yVU, MATRIX_WIDTH, 1, BLACK16);

        if (iPeakVUy > 1)
        {
            int fade = MAX_FADE * (millis() - msPeakVU) / (float) MS_PER_SECOND;
            DrawVUPixels(pGFXChannel, iPeakVUy,   yVU, fade, pPalette);
            DrawVUPixels(pGFXChannel, iPeakVUy-1, yVU, fade, pPalette);
        }

        if (gVURatio > lastVU)
            lastVU = gVURatio;
        else
            lastVU -= g_AppTime.DeltaTime() * VU_DECAY_PER_SECOND;
        lastVU = max(lastVU, 0.0);
        lastVU = min(lastVU, 2.0);

        int xHalf = pGFXChannel->width()/2-1;
        int bars  = lastVU / 2.0 * xHalf; // map(gVU, 0, MAX_VU/8, 1, xHalf);
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
            DrawVUPixels(pGFXChannel, i, yVU, i > bars ? 255 : 0, pPalette);
    }
};


// SpectrumAnalyzerEffect
//
// An effect that draws an audio spectrum analyzer on a matrix.  It is assumed that the
// matrix is 48x16 using LED Channel 0 only.   Has a VU meter up top and 16 bands.

class SpectrumAnalyzerEffect : public LEDStripEffect, virtual public VUMeterEffect
{
  protected:

    uint8_t   _colorOffset;
    uint16_t  _scrollSpeed;
    uint8_t   _fadeRate;

    CRGBPalette256 _palette;
    float _peak1DecayRate;
    float _peak2DecayRate;

    // DrawBand
    //
    // Draws the bar graph rectangle for a bar and then the white line on top of it

    void DrawBand(uint8_t iBand, CRGB baseColor)
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
            CRGB color = baseColor;
            iRow++;
            pGFXChannel->drawLine(xOffset, y, xOffset+bandWidth-1, y, color);
        }
    
        const int PeakFadeTime_ms = 1000;

        CRGB colorHighlight = CRGB(CRGB::White);
        unsigned long msPeakAge = millis() - g_lastPeak1Time[iBand];
        if (msPeakAge > PeakFadeTime_ms)
            msPeakAge = PeakFadeTime_ms;
        
        float agePercent = (float) msPeakAge / (float) MS_PER_SECOND;
        uint8_t fadeAmount = std::min(255.0f, agePercent * 256);

        colorHighlight = CRGB(CRGB::White).fadeToBlackBy(fadeAmount);

        if (value == 0)
            colorHighlight = baseColor;

        // if decay rate is less than zero we interpret that here to mean "don't draw it at all".  

        if (_peak1DecayRate >= 0.0f)
            pGFXChannel->drawLine(xOffset, max(0, yOffset-1), xOffset + bandWidth - 1, max(0, yOffset-1), colorHighlight);
    }

  public:

    SpectrumAnalyzerEffect(const char   * pszFriendlyName = nullptr, 
                           const CRGBPalette256 & palette = spectrumBasicColors, 
                           uint16_t           scrollSpeed = 0, 
                           uint8_t               fadeRate = 0,
                           float           peak1DecayRate = 2.0,
                           float           peak2DecayRate = 2.0)
        : LEDStripEffect(pszFriendlyName),
          _colorOffset(0),
          _scrollSpeed(scrollSpeed), 
          _fadeRate(fadeRate),
          _palette(palette),
          _peak1DecayRate(peak1DecayRate),
          _peak2DecayRate(peak2DecayRate)
    {
    }

    SpectrumAnalyzerEffect(const char   * pszFriendlyName = nullptr, 
                           CRGB                 baseColor = 0, 
                           uint8_t               fadeRate = 0,
                           float           peak1DecayRate = 2.0,
                           float           peak2DecayRate = 2.0)
        : LEDStripEffect(pszFriendlyName), 
          _colorOffset(0),
          _scrollSpeed(0), 
          _fadeRate(fadeRate),
          _palette(baseColor),
          _peak1DecayRate(peak1DecayRate),
          _peak2DecayRate(peak2DecayRate)

    {
    }

    virtual const char * FriendlyName() const
    {
        return "Spectrum";
    }

    virtual void Draw()
    {
        auto pGFXChannel = _GFX[0].get();

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

        DrawVUMeter(pGFXChannel, 0);
        for (int i = 0; i < NUM_BANDS; i++)
        {
            // Start at 32 because first two bands are usually too dark to use as bar colors.  This winds up selecting a number up
            // to 192 and then adding 64 to it in order to skip those darker 0-64 colors
            CRGB bandColor = _GFX[0].get()->ColorFromCurrentPalette((::map(i, 0, NUM_BANDS, 0, 255) + _colorOffset) % 1892 + 76);
            DrawBand(i, bandColor);
        }
    }
};

// WaveformEffect [MATRIX EFFECT]
//
// Draws a colorful scrolling waveform driven by instantaneous VU as it scrolls

class WaveformEffect : public LEDStripEffect, virtual public VUMeterEffect
{
  protected:
    const CRGBPalette256 *    _pPalette = nullptr;
    uint8_t                   _iColorOffset = 0;
    uint8_t                   _increment = 0;
    double                    _iPeakVUy = 0;
    unsigned long             _msPeakVU = 0;

    double lastVU = 0;
    const double VU_DECAY_PER_SECOND = 3.0;

  public:
    
    WaveformEffect(const char * pszFriendlyName, const CRGBPalette256 * pPalette = nullptr, uint8_t increment = 0) 
        : LEDStripEffect(pszFriendlyName)
    {
        _pPalette = pPalette;
        _increment = increment;
    }

    virtual const char * FriendlyName() const
    {
        return "Waveform Effect";
    }

    void DrawSpike(int x, double v) 
    {
        auto g = g_pDevices[0];

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
                uint16_t ms = millis();

                if (y < 2 || y > (MATRIX_HEIGHT - 2))
                    color  = CRGB::Red;
                else
                    color = g->ColorFromCurrentPalette(255-index + ms / 11, 255, LINEARBLEND); // color = ColorFromPalette(*_pPalette, 255 - index);

                // Sparkles
                //
                // if (random(16)  < 2)
                //    color = CRGB::White;

            }
                
            _GFX[0]->setPixel(x, y, color);
        }
        _iColorOffset = (_iColorOffset + _increment) % 255;

    }

    virtual void Draw()
    {
        _GFX[0]->MoveInwardX(1);                            // Start on Y=1 so we don't shift the VU meter
        
        if (gVURatio > lastVU)
            lastVU = gVURatio;
        else
            lastVU -= g_AppTime.DeltaTime() * 10.0;
        lastVU = max(lastVU, 0.0);
        lastVU = min(lastVU, 2.0);

        DrawSpike(63, lastVU/2.0);
        DrawSpike(0, lastVU/2.0);

        DrawVUMeter(_GFX[0].get(), 0);        
    }
};

class GhostWave : public WaveformEffect
{
    double                    _iPeakVUy = 0;
    unsigned long             _msPeakVU = 0;

  public:

    GhostWave(const char * pszFriendlyName = nullptr, const CRGBPalette256 * pPalette = nullptr, uint8_t increment = 0) 
        : WaveformEffect(pszFriendlyName, pPalette, increment)
    {
    }

    virtual void Draw()
    {
        auto graphics = _GFX[0].get();

        for (int y = 1; y < MATRIX_HEIGHT; y++)
        {
            for (int x = 0; x < MATRIX_WIDTH / 2 - 1; x++)
            {
                graphics->setPixel(x, y, graphics->getPixel(x+1, y));
                graphics->setPixel(MATRIX_WIDTH-x-1, y, graphics->getPixel(MATRIX_WIDTH-x-2, y));
            }
        }
    
        if (gVURatio > lastVU)
            lastVU = gVURatio;
        else
            lastVU -= g_AppTime.DeltaTime() * 20.0;
        lastVU = max(lastVU, 0.0);
        lastVU = min(lastVU, 2.0);

        DrawVUMeter(graphics, 0);
        DrawSpike(MATRIX_WIDTH/2, lastVU / 2.0);
        DrawSpike(MATRIX_WIDTH/2-1, lastVU / 2.0);
        //BlurFrame(32);
    }
};

#endif
