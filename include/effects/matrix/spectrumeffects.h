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

#include "esp_attr.h"
#include "effects/strip/musiceffect.h"
#include "effects/strip/particles.h"
#include "values.h"
#include "systemcontainer.h"

#if ENABLE_AUDIO

class InsulatorSpectrumEffect : public LEDStripEffect, public BeatEffectBase, public ParticleSystem<SpinningPaletteRingParticle>
{
    int                    _iLastInsulator = 0;
    const CRGBPalette16 & _Palette;
    CRGB _baseColor = CRGB::Black;

  public:

    InsulatorSpectrumEffect(const String & strName, const CRGBPalette16 & Palette) :
        LEDStripEffect(EFFECT_MATRIX_INSULATOR_SPECTRUM, strName),
        BeatEffectBase(1.50, 0.25),
        ParticleSystem<SpinningPaletteRingParticle>(),
        _Palette(Palette)
    {
    }

    InsulatorSpectrumEffect(const JsonObjectConst& jsonObject) :
        LEDStripEffect(jsonObject),
        BeatEffectBase(1.50, 0.25),
        ParticleSystem<SpinningPaletteRingParticle>(),
        _Palette(jsonObject[PTY_PALETTE].as<CRGBPalette16>())
    {
    }

    virtual bool SerializeToJSON(JsonObject& jsonObject) override
    {
        AllocatedJsonDocument jsonDoc(512);

        JsonObject root = jsonDoc.to<JsonObject>();
        LEDStripEffect::SerializeToJSON(root);

        jsonDoc[PTY_PALETTE] = _Palette;

        return jsonObject.set(jsonDoc.as<JsonObjectConst>());
    }

    virtual void Draw() override
    {
        auto peaks = g_Analyzer.GetPeakData();

        for (int band = 0; band < min(NUM_BANDS, NUM_FANS); band++)
        {
            CRGB color = ColorFromPalette(_Palette, ::map(band, 0, min(NUM_BANDS, NUM_FANS), 0, 255) + beatsin8(1) );
            color = color.fadeToBlackBy(255 - 255 * peaks[band]);
            color = color.fadeToBlackBy((2.0 - g_Analyzer._VURatio) * 228);
            DrawRingPixels(0, FAN_SIZE * peaks[band], color, NUM_FANS-1-band, 0);
        }

        ProcessAudio();
        ParticleSystem<SpinningPaletteRingParticle>::Render(_GFX);

        fadeAllChannelsToBlackBy(min(255.0,2000.0 * g_Values.AppTime.LastFrameTime()));
    }

    virtual void HandleBeat(bool bMajor, float elapsed, float span)
    {
        int iInsulator;
        do
        {
          iInsulator = random(0, NUM_FANS);
        } while (NUM_FANS > 3 && iInsulator == _iLastInsulator);
        _iLastInsulator = iInsulator;


        // REVIEW(davepl) This might look interesting if it didn't erase...
        bool bFlash = g_Analyzer._VURatio > 1.99 && span > 1.9 && elapsed > 0.25;

        _allParticles.push_back(SpinningPaletteRingParticle(iInsulator, 0, _Palette, 256.0/FAN_SIZE, 4, -0.5, RING_SIZE_0, 0, LINEARBLEND, true, 1.0, bFlash ? max(0.12f, elapsed/8) : 0));
    }
};

class VUMeterEffect
{
  protected:

    // DrawVUPixels
    //
    // Draw i-th pixel in row y

    void DrawVUPixels(std::shared_ptr<GFXBase> pGFXChannel, int i, int yVU, int fadeBy = 0, const CRGBPalette16 * pPalette = nullptr)
    {
        if (g_Analyzer.MicMode() == PeakData::PCREMOTE)
            pPalette = &vuPaletteBlue;

        int xHalf = pGFXChannel->width()/2;
        pGFXChannel->setPixel(xHalf-i-1, yVU, ColorFromPalette(pPalette ? *pPalette : vu_gpGreen,  i*(256/xHalf)).fadeToBlackBy(fadeBy));
        pGFXChannel->setPixel(xHalf+i,   yVU, ColorFromPalette(pPalette ? *pPalette : vu_gpGreen, i*(256/xHalf)).fadeToBlackBy(fadeBy));
    }


    // DrawVUMeter
    //
    // Draws the symmetrical VU meter along with its fading peaks up at the top of the display.

    int iPeakVUy = 0;                 // size (in LED pixels) of the VU peak
    unsigned long msPeakVU = 0;       // timestamp in ms when that peak happened so we know how old it is

  public:

    inline void EraseVUMeter(std::shared_ptr<GFXBase> pGFXChannel, int start, int yVU) const
    {
        int xHalf = pGFXChannel->width()/2;
        for (int i = start; i <= xHalf; i++)
        {
            pGFXChannel->setPixel(xHalf-i,   yVU, CRGB::Black);
            pGFXChannel->setPixel(xHalf-1+i,   yVU, CRGB::Black);
        }
    }

    void DrawVUMeter(std::shared_ptr<GFXBase> pGFXChannel, int yVU, const CRGBPalette16 * pPalette = nullptr)
    {
        const int MAX_FADE = 256;

        int xHalf = pGFXChannel->width()/2-1;
        int bars  = g_Analyzer._VURatioFade / 2.0 * xHalf; // map(g_Analyzer._VU, 0, MAX_VU/8, 1, xHalf);
        bars = min(bars, xHalf);

        EraseVUMeter(pGFXChannel, bars, yVU);

        if (bars >= iPeakVUy)
        {
            msPeakVU = millis();
            iPeakVUy = bars;
        }
        else if (millis() - msPeakVU > MS_PER_SECOND / 2)
        {
            iPeakVUy = 0;
        }

        if (iPeakVUy > 1)
        {
            int fade = MAX_FADE * (millis() - msPeakVU) / (float) MS_PER_SECOND * 2;
            DrawVUPixels(pGFXChannel, iPeakVUy,   yVU, fade);
            DrawVUPixels(pGFXChannel, iPeakVUy-1, yVU, fade);
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

    uint8_t   _numBars;
    uint8_t   _colorOffset;
    uint16_t  _scrollSpeed;
    uint8_t   _fadeRate;

    const CRGBPalette16 _palette;
    float _peak1DecayRate;
    float _peak2DecayRate;

    bool      _bShowVU;

    virtual size_t DesiredFramesPerSecond() const override
    {
        return 60;
    }

    virtual bool RequiresDoubleBuffering() const override
    {
        return _fadeRate != 0;
    }

    // DrawBar
    //
    // Draws the bar graph rectangle for a bar and then the white line on top of it.  Interpolates odd bars when you
    // have twice as many bars as bands.

    void DrawBar(const uint8_t iBar, CRGB baseColor)
    {
        auto pGFXChannel = _GFX[0];
        int value, value2;

        static_assert(!(NUM_BANDS & 1));     // We assume an even number of bars because we peek ahead from an odd one below

        int iBand = ::map(iBar, 0, _numBars, 0, NUM_BANDS);
        int iNextBand = (iBand + 1) % NUM_BANDS;
        int barsPerBand = _numBars / NUM_BANDS;

        if (barsPerBand >= 2)
        {
            // Interpolate the value for the Nth bar by taking a proportional average of this band and the next band, depending on how
            // far we are in between bars.  ib is the substep, so if you have 64 bars and 16 bands, ib will range from 0 to 3, and for
            // bar 16, for example, it will take all of bar 4 and none of bar 5.  For bar 17, it will take 3/4 of bar 4 and 1/4 of bar 5.

            int ib = iBar % barsPerBand;
            value  = (g_Analyzer._peak1Decay[iBand] * (barsPerBand - ib) + g_Analyzer._peak1Decay[iNextBand] * (ib) ) / barsPerBand * (pGFXChannel->height() - 1);
            value2 = (g_Analyzer._peak2Decay[iBand] * (barsPerBand - ib) + g_Analyzer._peak2Decay[iNextBand] * (ib) ) / barsPerBand *  pGFXChannel->height();
        }
        else
        {
            // One to one case, just use the actual band value we mapped to

            value  = g_Analyzer._peak1Decay[iBand] * (pGFXChannel->height() - 1);
            value2 = g_Analyzer._peak2Decay[iBand] *  pGFXChannel->height();
        }

        debugV("Band: %d, Value: %f\n", iBar, g_Analyzer._peak1Decay[iBar] );

        if (value > pGFXChannel->height())
            value = pGFXChannel->height();

        if (value2 > pGFXChannel->height())
            value2 = pGFXChannel->height();

        int barWidth  = pGFXChannel->width() / _numBars;
        int xOffset   = iBar * barWidth;

        // The top of the bar is normally just matrix height less the value.  Here, however, we "enhance" the bar by pulsing it a bit with
        // the beat of the music.  We do this by taking the value and subtracting a fraction of itself, which makes the bar taller when the
        // beat is higher.  We also subtract a fraction of the VU fade, which makes the bar taller when the VU is higher.  The net effect is
        // that the bar is taller when the beat is higher, and the beat is higher when the VU is higher, so the bar is taller when the VU is
        // higher.

        value *= g_Analyzer.BeatEnhance(BARBEAT_ENHANCE);
        value2 *= g_Analyzer.BeatEnhance(BARBEAT_ENHANCE);

        int yOffset   = pGFXChannel->height() - value ;
        int yOffset2  = pGFXChannel->height() - value2 ;

        for (int y = yOffset2; y < pGFXChannel->height(); y++)
            for (int x = xOffset; x < xOffset + barWidth; x++)
                g()->setPixel(x, y, baseColor);

        // We draw the highlight in white, but if its falling at a different rate than the bar itself,
        // it indicates a free-floating highlight, and those get faded out based on age
        // We draw the bottom row in bar color even when only 1 pixel high so as not to have a white
        // strip as the bottom row (all made up of highlights)

        CRGB colorHighlight = value > 1 ? CRGB::White : baseColor;

        // If a decay rate has been defined and it's different than the rate at which the bar falls
        if (_peak1DecayRate >= 0.0f)
        {
            if (_peak1DecayRate != _peak2DecayRate)
            {
                const int PeakFadeTime_ms = 1000;

                unsigned long msPeakAge = millis() - g_Analyzer._lastPeak1Time[iBand];
                if (msPeakAge > PeakFadeTime_ms)
                    msPeakAge = PeakFadeTime_ms;

                float agePercent = (float) msPeakAge / (float) MS_PER_SECOND;
                uint8_t fadeAmount = std::min(255.0f, agePercent * 256);
                colorHighlight.fadeToBlackBy(fadeAmount);
                pGFXChannel->drawLine(xOffset, max(0, yOffset-1), xOffset + barWidth, max(0, yOffset-1), colorHighlight);
            }
            else
            {
                pGFXChannel->drawLine(xOffset, max(0, yOffset2-1), xOffset + barWidth, max(0, yOffset2-1), colorHighlight);
            }
        }
    }

  public:

    SpectrumAnalyzerEffect(const char   * pszFriendlyName,
                           int                    cNumBars = 12,
                           const CRGBPalette16  & palette = spectrumBasicColors,
                           uint16_t           scrollSpeed = 0,
                           uint8_t               fadeRate = 0,
                           float           peak1DecayRate = 1.0,
                           float           peak2DecayRate = 1.0)
        : LEDStripEffect(EFFECT_MATRIX_SPECTRUM_ANALYZER, pszFriendlyName),
          _numBars(cNumBars),
          _colorOffset(0),
          _scrollSpeed(scrollSpeed),
          _fadeRate(fadeRate),
          _palette(palette),
          _peak1DecayRate(peak1DecayRate),
          _peak2DecayRate(peak2DecayRate)
    {
    }

    SpectrumAnalyzerEffect(const char   * pszFriendlyName,
                           int                    cNumBars = 12,
                           const CRGB &          baseColor = CRGB::Red,
                           uint8_t                fadeRate = 0,
                           float            peak1DecayRate = 1.0,
                           float            peak2DecayRate = 1.0)
        : LEDStripEffect(EFFECT_MATRIX_SPECTRUM_ANALYZER, pszFriendlyName),
          _numBars(cNumBars),
          _colorOffset(0),
          _scrollSpeed(0),
          _fadeRate(fadeRate),
          _palette(baseColor),
          _peak1DecayRate(peak1DecayRate),
          _peak2DecayRate(peak2DecayRate)

    {
    }

    SpectrumAnalyzerEffect(const JsonObjectConst& jsonObject)
        : LEDStripEffect(jsonObject),
          _numBars(jsonObject["nmb"]),
          _colorOffset(0),
          _scrollSpeed(jsonObject[PTY_SPEED]),
          _fadeRate(jsonObject["frt"]),
          _palette(jsonObject[PTY_PALETTE].as<CRGBPalette16>()),
          _peak1DecayRate(jsonObject["pd1"]),
          _peak2DecayRate(jsonObject["pd2"])

    {
    }

    virtual bool SerializeToJSON(JsonObject& jsonObject) override
    {
        AllocatedJsonDocument jsonDoc(512);

        JsonObject root = jsonDoc.to<JsonObject>();
        LEDStripEffect::SerializeToJSON(root);

        jsonDoc[PTY_PALETTE] = _palette;
        jsonDoc["nmb"]       = _numBars;
        jsonDoc[PTY_SPEED]   = _scrollSpeed;
        jsonDoc["frt"]       = _fadeRate;
        jsonDoc["pd1"]       = _peak1DecayRate;
        jsonDoc["pd2"]       = _peak2DecayRate;

        return jsonObject.set(jsonDoc.as<JsonObjectConst>());
    }

    virtual void Start() override
    {
        // The peaks and their decay rates are global, so we load up our values every time we display so they're current

        g_Analyzer._peak1DecayRate = _peak1DecayRate;
        g_Analyzer._peak2DecayRate = _peak2DecayRate;
    }

    virtual void Draw() override
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
            pGFXChannel->Clear();

        for (int i = 0; i < _numBars; i++)
        {
            // We don't use the auto-cycling palette, but we'll use the paused palette if the user has asked for one
            // BUGBUG (davepl) - why is one 240 and one 255?  Perhaps for palette wraparound but shouldnt both be the same?
            //
            // If the palette is scrolling, we do a smooth blend.  Otherwise we do a straight color lookup, which makes the stripes
            // on the USA flag solid red rather than pinkish...

            if (pGFXChannel->IsPalettePaused())
            {
                int q = ::map(i, 0, _numBars, 0, 240) + _colorOffset;
                DrawBar(i, pGFXChannel->ColorFromCurrentPalette(q % 240, 255, _scrollSpeed > 0 ? LINEARBLEND : NOBLEND));
            }
            else
            {
                int q = ::map(i, 0, _numBars, 0, 255) + _colorOffset;
                DrawBar(i, ColorFromPalette(_palette, (q) % 255, 255, _scrollSpeed > 0 ? LINEARBLEND : NOBLEND));
            }
        }
    }
};


// WaveformEffect [MATRIX EFFECT]
//
// Draws a colorful scrolling waveform driven by instantaneous VU as it scrolls

class WaveformEffect : public LEDStripEffect
{
  protected:
    uint8_t                      _iColorOffset = 0;
    uint8_t                      _increment = 0;
    float                        _iPeakVUy = 0;
    unsigned long                _msPeakVU = 0;

  public:

    WaveformEffect(const String & pszFriendlyName, uint8_t increment = 0)
        : LEDStripEffect(EFFECT_MATRIX_WAVEFORM, pszFriendlyName),
          _increment(increment)
    {
    }

    WaveformEffect(const JsonObjectConst& jsonObject)
        : LEDStripEffect(jsonObject),
          _increment(jsonObject["inc"])
    {
    }

    virtual bool SerializeToJSON(JsonObject& jsonObject) override
    {
        StaticJsonDocument<LEDStripEffect::_jsonSize> jsonDoc;

        JsonObject root = jsonDoc.to<JsonObject>();
        LEDStripEffect::SerializeToJSON(root);

        jsonDoc["inc"] = _increment;

        assert(!jsonDoc.overflowed());

        return jsonObject.set(jsonDoc.as<JsonObjectConst>());
    }

    void DrawSpike(int x, float v, bool bErase = true)
    {
        v = std::min(v, 1.0f);
        v = std::max(v, 0.0f);

        int yTop = (MATRIX_HEIGHT / 2) - v * (MATRIX_HEIGHT  / 2);
        int yBottom = (MATRIX_HEIGHT / 2) + v * (MATRIX_HEIGHT / 2) ;
        if (yTop < 0)
            yTop = 0;
        if (yBottom > MATRIX_HEIGHT - 1)
            yBottom = MATRIX_HEIGHT - 1;

        for (int y=0; y < MATRIX_HEIGHT; y++)
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
                    color = g()->ColorFromCurrentPalette(255-index + ms / 11, 255, LINEARBLEND);
            }

            bErase ? g()->setPixel(x, y, color) : g()->drawPixel(x, y, color);

        }
        _iColorOffset = (_iColorOffset + _increment) % 255;

    }

    virtual size_t DesiredFramesPerSecond() const override
    {
        // I found a pleasing scroll speed to be 24-30, not much faster or its too mesmerizing :-)
        return 24;
    }

    virtual void Draw() override
    {
        int top = g_ptrSystem->EffectManager().IsVUVisible() ? 1 : 0;
        g()->MoveInwardX(top);                            // Start on Y=1 so we don't shift the VU meter
        DrawSpike(MATRIX_WIDTH-1, g_Analyzer._VURatio/2.0);
        DrawSpike(0, g_Analyzer._VURatio/2.0);
    }
};

class GhostWave : public WaveformEffect
{
    uint8_t                   _blur     = 0;
    bool                      _erase    = true;
    int                       _fade     = 0;

    void construct()
    {
        _effectNumber = EFFECT_MATRIX_GHOST_WAVE;
    }
  public:

    GhostWave(const String & pszFriendlyName, uint8_t increment = 0, uint8_t blur = 0, bool erase = true, int fade = 0)
        : WaveformEffect(pszFriendlyName, increment),
          _blur(blur),
          _erase(erase),
          _fade(fade)
    {
        construct();
    }

    GhostWave(const JsonObjectConst& jsonObject)
        : WaveformEffect(jsonObject),
          _blur(jsonObject[PTY_BLUR]),
          _erase(jsonObject[PTY_ERASE]),
          _fade(jsonObject[PTY_FADE])
    {
        construct();
    }

    virtual bool SerializeToJSON(JsonObject& jsonObject) override
    {
        StaticJsonDocument<LEDStripEffect::_jsonSize> jsonDoc;

        JsonObject root = jsonDoc.to<JsonObject>();
        LEDStripEffect::SerializeToJSON(root);

        jsonDoc[PTY_BLUR] = _blur;
        jsonDoc[PTY_ERASE] = _erase;
        jsonDoc[PTY_FADE] = _fade;

        assert(!jsonDoc.overflowed());

        return jsonObject.set(jsonDoc.as<JsonObjectConst>());
    }

    virtual bool RequiresDoubleBuffering() const override
    {
        return true;
    }

    virtual size_t DesiredFramesPerSecond() const override
    {
        // Looks cool at the low-50s it can actually achieve
        return _blur > 0 ? 45 : 30;
    }

    virtual void Draw() override
    {
        auto& effectManager = g_ptrSystem->EffectManager();
        int top = effectManager.IsVUVisible() ? 1 : 0;

        g()->MoveOutwardsX(top);

        if (_fade)
            g()->DimAll(255-_fade);

        if (_blur)
            g()->blur2d(g()->leds, MATRIX_WIDTH, 0, MATRIX_HEIGHT, 1, _blur);

        // VURatio is too fast, VURatioFade looks too slow, but averaged between them is just right

        float audioLevel = (g_Analyzer._VURatioFade + g_Analyzer._VURatio) / 2;

        // Offsetting by 0.25, which is a very low ratio, helps keep the line thin when sound is low
        //audioLevel = (audioLevel - 0.25) / 1.75;

        // Now pulse it by some amount based on the beat
        audioLevel = audioLevel * g_Analyzer.BeatEnhance(SPECTRUMBARBEAT_ENHANCE);

        DrawSpike(MATRIX_WIDTH/2, audioLevel, _erase);
        DrawSpike(MATRIX_WIDTH/2-1, audioLevel, _erase);
    }
};

// SpectrumBarEffect
//
// Draws an approximation of the waveform by mirroring the spectrum analyzer bars in four quadrants

class SpectrumBarEffect : public LEDStripEffect
{
    byte _hueIncrement = 0;
    byte _scrollIncrement = 0;
    byte _hueStep = 0;

    void construct()
    {
        _effectNumber = EFFECT_MATRIX_SPECTRUMBAR;
    }

    public:

    SpectrumBarEffect(const char   * pszFriendlyName, byte hueStep = 16, byte hueIncrement = 4, byte scrollIncrement = 0)
        :LEDStripEffect(EFFECT_MATRIX_SPECTRUMBAR, pszFriendlyName),
        _hueIncrement(hueIncrement),
        _scrollIncrement(scrollIncrement),
        _hueStep(hueStep)
    {
    }

    SpectrumBarEffect(const JsonObjectConst& jsonObject)
        : LEDStripEffect(jsonObject),
          _hueIncrement(jsonObject[PTY_DELTAHUE]),
          _scrollIncrement(jsonObject[PTY_SPEED]),
          _hueStep(jsonObject[PTY_HUESTEP])
    {
    }

    virtual bool SerializeToJSON(JsonObject& jsonObject) override
    {
        StaticJsonDocument<LEDStripEffect::_jsonSize> jsonDoc;

        JsonObject root = jsonDoc.to<JsonObject>();
        LEDStripEffect::SerializeToJSON(root);

        jsonDoc[PTY_SPEED]    = _scrollIncrement;
        jsonDoc[PTY_DELTAHUE] = _hueIncrement;
        jsonDoc[PTY_HUESTEP]  = _hueStep;

        assert(!jsonDoc.overflowed());

        return jsonObject.set(jsonDoc.as<JsonObjectConst>());
    }

    virtual size_t DesiredFramesPerSecond() const override
    {
        return 45;
    }

    virtual bool RequiresDoubleBuffering() const override
    {
        return true;
    }

    void DrawGraph()
    {
        constexpr size_t halfHeight = MATRIX_HEIGHT / 2;
        constexpr size_t halfWidth  = MATRIX_WIDTH  / 2;

        // We step the hue ever 30ms
        static byte hue = 0;
        EVERY_N_MILLISECONDS(30)
            hue -= _hueIncrement;

        // We scroll the bars ever 50ms
        static byte offset = 0;
        EVERY_N_MILLISECONDS(50)
            offset += _scrollIncrement;

        for (int iBand = 0; iBand < NUM_BANDS; iBand++)
        {
            // Draw the spike

            auto value =  g_Analyzer.BeatEnhance(SPECTRUMBARBEAT_ENHANCE) * g_Analyzer._peak1Decay[iBand];
            auto top    = std::max(0.0f, halfHeight - value * halfHeight);
            auto bottom = std::min(MATRIX_HEIGHT-1.0f, halfHeight + value * halfHeight + 1);
            auto x1     = halfWidth - ((iBand * 2 + offset) % halfWidth);
            auto x2     = halfWidth + ((iBand * 2 + offset) % halfWidth);

            if (value == 0.0f)
                bottom = top;

            if (x1 < 0 || x2 >= MATRIX_WIDTH)
                break;

            CRGB  color = g()->IsPalettePaused() ? g()->ColorFromCurrentPalette() : CHSV(hue + iBand * _hueStep, 255, 255);
            g()->drawLine(x1, top, x1, bottom, color);
            g()->drawLine(x2, top, x2, bottom, color);
        }
        g()->drawLine(0, halfHeight, MATRIX_WIDTH - 1, halfHeight, CRGB::Grey);
     }

    virtual void Start() override
    {
        constexpr auto kPeakDecaySpectrumBar = 2.5;

        // Set the peak decay rates to something that looks good for this effect

        g_Analyzer._peak1DecayRate = kPeakDecaySpectrumBar;
        g_Analyzer._peak2DecayRate = kPeakDecaySpectrumBar;

        // This effect doesn't clear during drawing, so we need to clear to start the frame

        g()->Clear();
    }

    virtual void Draw() override
    {
        // Rather than clearing the screen, we fade it out quickly, which gives a nice persistence of vision effect
        // as the bars fade back to black once the line has receeded

        g()->DimAll(200);
        DrawGraph();
    }
};

#endif
