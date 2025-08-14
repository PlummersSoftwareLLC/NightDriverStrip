//+--------------------------------------------------------------------------
//
// File:        SoundAnalyzer.h
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
//   Code that samples analog input and can run an FFT on it for stats,
//   and
//
// History:     Sep-12-2018         Davepl      Commented
//              Apr-20-2019         Davepl      Adapted from Spectrum Analyzer
//
//---------------------------------------------------------------------------

#pragma once

#include "globals.h"
#include <algorithm>
#include <numeric>
#include <arduinoFFT.h>
#include <driver/adc.h>
#include <driver/i2s.h>
#include <memory>

#ifndef SPECTRUM_BAND_SCALE_MEL
#define SPECTRUM_BAND_SCALE_MEL 0
#endif

// Default FFT size if not provided by build flags or elsewhere
#ifndef MAX_SAMPLES
#define MAX_SAMPLES 256
#endif

// Per-microphone analyzers can be tuned independently via this struct.
// Defaults below start with Mesmerizer values; others can diverge over time.

// You can adjust the amount of compression (which makes the bar)
struct AudioInputParams {
    float windowPowerCorrection;  // Windowing power correction (Hann ~4.0)
    float energyNoiseAdapt;       // Noise floor rise rate when signal present
    float energyNoiseDecay;       // Noise floor decay factor when below floor
    float energySmoothAlpha;      // One-pole smoother for per-band signal
    float energyEnvDecay;         // Envelope fall (autoscale) per frame
    float energyMinEnv;           // Min envelope to avoid div-by-zero
    float bandCompLow;            // Low-band compensation scalar
    float bandCompHigh;           // High-band compensation scalar
    float frameSilenceGate;       // Gate entire frame if below this after norm
    float normNoiseGate;          // Gate individual bands below this after norm
    float envFloorFromNoise;      // Multiplier on mean noise floor to cap normalization
    float frameSNRGate;           // Gate frame if raw SNR (max/noiseMax) is below this
    float postScale;              // Post-scale for normalized band values (applied before clamp)
    float compressGamma;          // Exponent for compression on normalized v (e.g., 1/3=cuberoot, 1/6=sqrt(cuberoot))
    float quietEnvFloorGate;      // Gate entire frame if envFloor (noiseMean*envFloorFromNoise) < this (0 disables)
};

// Mesmerizer (default) tuning
static constexpr AudioInputParams kParamsMesmerizer{
    4.0f,      // windowPowerCorrection
    0.02f,     // energyNoiseAdapt
    0.98f,     // energyNoiseDecay
    0.25f,     // energySmoothAlpha
    0.99f,     // energyEnvDecay
    0.000001f, // energyMinEnv
    0.25f,     // bandCompLow
    10.0f,     // bandCompHigh
    0.00f,     // frameSilenceGate
    0.00f,     // normNoiseGate
    3.0f,      // envFloorFromNoise (cap auto-gain at ~1/4 of pure-noise)
    0.0f,      // frameSNRGate (require ~3:1 SNR to show frame)
    1.0f,      // postScale (Mesmerizer default)
    0.333333f, // compressGamma (cube root)
    30000000   // quietEnvFloorGate 
};

// PC Remote uses Mesmerizer defaults
static constexpr AudioInputParams kParamsPCRemote = kParamsMesmerizer;

// M5 variants use a higher postScale by default
static constexpr AudioInputParams kParamsM5{
    4.0f, 0.02f, 0.98f, 0.25f, 0.99f, 0.000001f, 0.25f, 10.0f, 0.00f, 0.00f, 3.0f, 0.0f, 1.5f,
    0.16666667f,
    30000000.0f
};
static constexpr AudioInputParams kParamsM5Plus2{
    4.0f, 0.02f, 0.98f, 0.25f, 0.99f, 0.000001f, 0.25f, 10.0f, 0.00f, 0.00f, 3.0f, 0.0f, 1.5f,
    0.16666667f,
    30000000.0f
};

// PeakData
//
// Simple data class that holds the music peaks for up to NUM_BANDS bands. Moved above
// the ENABLE_AUDIO conditional so both stub and full implementations can expose
// a consistent interface returning PeakData.

#ifndef MIN_VU
  #define MIN_VU 0.05f
#endif

#ifndef VUDAMPEN
  #define VUDAMPEN 0
#endif

#define VUDAMPENMIN 1
#define VUDAMPENMAX 1

#ifndef NUM_BANDS
#define NUM_BANDS 1
#endif

class PeakData
{
  public:
    float _Level[NUM_BANDS];

    typedef enum
    {
        MESMERIZERMIC,
        PCREMOTE,
        M5,
        M5PLUS2
    } MicrophoneType;

    PeakData()
    {
        std::fill_n(_Level, NUM_BANDS, 0.0f);
    }
    explicit PeakData(const float *pFloats)
    {
        SetData(pFloats);
    }
    PeakData &operator=(const PeakData &other)
    {
        if (this == &other)
            return *this;
        std::copy(other._Level, other._Level + NUM_BANDS, _Level);
        return *this;
    }
    float operator[](std::size_t n) const { return _Level[n]; }
    void SetData(const float *pFloats)
    {
        std::copy(pFloats, pFloats + NUM_BANDS, _Level);
    }
};

// Return parameter set for a given microphone source.
static inline const AudioInputParams &ParamsFor(PeakData::MicrophoneType t)
{
    switch (t)
    {
        case PeakData::PCREMOTE:  return kParamsPCRemote;
        case PeakData::M5:        return kParamsM5;
        case PeakData::M5PLUS2:   return kParamsM5Plus2;
        case PeakData::MESMERIZERMIC:
        default:                  return kParamsMesmerizer;
    }
}

// Interface for SoundAnalyzer (audio and non-audio variants)
class ISoundAnalyzer
{
  public:
    virtual ~ISoundAnalyzer() = default;
    virtual float VURatio() const = 0;
    virtual float VURatioFade() const = 0;
    virtual float VU() const = 0;
    virtual float PeakVU() const = 0;
    virtual float MinVU() const = 0;
    virtual int AudioFPS() const = 0;
    virtual int SerialFPS() const = 0;
    virtual PeakData::MicrophoneType MicMode() const = 0;
    virtual const PeakData &Peaks() const = 0;
    virtual float Peak2Decay(int band) const = 0;
    virtual const float *Peak2DecayData() const = 0;
    virtual const float *Peak1DecayData() const = 0;
    virtual float Peak1Decay(int band) const = 0;
    virtual const unsigned long *Peak1Times() const = 0;
    virtual unsigned long LastPeak1Time(int band) const = 0;
    virtual void SetPeakDecayRates(float r1, float r2) = 0;
};

#if !ENABLE_AUDIO

#ifndef NUM_BANDS
#define NUM_BANDS 1
#endif

class SoundAnalyzer : public ISoundAnalyzer // Non-audio case stub
{
    PeakData _emptyPeaks; // zero-initialized
  public:
    float VURatio() const override
    {
        return 0.0f;
    }
    float VURatioFade() const override
    {
        return 0.0f;
    }
    float VU() const override
    {
        return 0.0f;
    }
    float PeakVU() const override
    {
        return 0.0f;
    }
    float MinVU() const override
    {
        return 0.0f;
    }
    int AudioFPS() const override
    {
        return 0;
    }
    int SerialFPS() const override
    {
        return 0;
    }
    PeakData::MicrophoneType MicMode() const override
    {
        return PeakData::MESMERIZERMIC;
    }
    const PeakData &Peaks() const override
    {
        return _emptyPeaks;
    }
    float Peak2Decay(int) const override
    {
        return 0.0f;
    }
    const float *Peak2DecayData() const override
    {
        static float zeros[NUM_BANDS] = {0};
        return zeros;
    }
    const float *Peak1DecayData() const override
    {
        static float zeros[NUM_BANDS] = {0};
        return zeros;
    }
    float Peak1Decay(int) const override
    {
        return 0.0f;
    }
    const unsigned long *Peak1Times() const override
    {
        static unsigned long zeros[NUM_BANDS] = {0};
        return zeros;
    }
    unsigned long LastPeak1Time(int) const override
    {
        return 0;
    }
    void SetPeakDecayRates(float, float) override
    {
    }
};

#else // Audio case

void IRAM_ATTR AudioSamplerTaskEntry(void *);
void IRAM_ATTR AudioSerialTaskEntry(void *);

// SoundAnalyzer
//
// The SoundAnalyzer class uses I2S to read samples from the microphone and then runs an FFT on the
// results to generate the peaks in each band, as well as tracking an overall VU and VU ratio, the
// latter being the ratio of the current VU to the trailing min and max VU.

class SoundAnalyzer : public ISoundAnalyzer
{
    // Give internal audio task functions access to private members
    friend void IRAM_ATTR AudioSamplerTaskEntry(void *);
    friend void IRAM_ATTR AudioSerialTaskEntry(void *);

    float _VURatio = 1.0f;
    float _VURatioFade = 1.0f;
    float _VU = 0.0f;
    float _PeakVU = 0.0f;
    float _MinVU = 0.0f;
    unsigned long _cSamples = 0U;
    int _AudioFPS = 0;
    int _serialFPS = 0;
    uint _msLastRemote = 0;

    // I'm old enough I can only hear up to about 12000Hz, but feel free to adjust.  Remember from
    // school that you need to sample at double the frequency you want to process, so 24000 samples is 12000Hz

    static constexpr size_t SAMPLING_FREQUENCY = 24000;
    static constexpr size_t LOWEST_FREQ = 100;
    static constexpr size_t HIGHEST_FREQ = SAMPLING_FREQUENCY / 2;

    static inline size_t SamplingPeriodMicros()
    {
        return (size_t)PERIOD_FROM_FREQ(SAMPLING_FREQUENCY);
    }

    size_t _clipCount = 0;              // number of clipped samples seen
    float  _dcOffsetEMA = 0.0f;         // exponential moving average of DC offset (abs)
    float  _envEMA = 0.0f;              // EMA of _energyMaxEnv
    size_t _frameGateHits = 0;          // frames gated by params.frameSilenceGate
    size_t _bandGateHits = 0;           // total band gate hits
    size_t _framesProcessed = 0;        // total processed frames

    float   _oldVU;                     // Old VU value for damping
    float   _oldPeakVU;                 // Old peak VU value for damping
    float   _oldMinVU;                  // Old min VU value for damping
    float * _vPeaks;                    // Normalized band energies 0..1
    float * _livePeaks;                 // Attack-limited display peaks per band
    int     _bandBinStart[NUM_BANDS];
    int     _bandBinEnd[NUM_BANDS];
    float   _energyMaxEnv = 1.0f;         // adaptive envelope for autoscaling
    float   _noiseFloor[NUM_BANDS] = {0}; // adaptive per-band noise floor
    float   _rawPrev[NUM_BANDS] = {0};    // previous raw (noise-subtracted) power for smoothing

    // Peak decay internals (private)
    unsigned long _lastPeak1Time[NUM_BANDS] = {0};
    float _peak1Decay[NUM_BANDS] = {0};
    float _peak2Decay[NUM_BANDS] = {0};
    float _peak1DecayRate = 1.25f;
    float _peak2DecayRate = 1.25f;

    PeakData::MicrophoneType _MicMode;
    AudioInputParams _params;                  // Active tuning params for current mic mode

    // Energy spectrum processing (implemented inline below)
    //
    // Calculate a logarithmic scale for the bands like you would find on a graphic equalizer display
    //

  private:
    static constexpr int kBandOffset = 1; // number of lowest source bands to skip in layout
    float *_vReal = nullptr;
    float *_vImaginary = nullptr;
    std::unique_ptr<int16_t[]> ptrSampleBuffer; // sample buffer storage
    PeakData _Peaks; // cached last normalized peaks (moved earlier for inline method visibility)

    // Reset and clear the FFT buffers

    void Reset()
    {
        if (!_vReal)
            return;
        std::fill_n(_vReal, MAX_SAMPLES, 0.0f);
        if (_vImaginary)
            std::fill_n(_vImaginary, MAX_SAMPLES, 0.0f);
        std::fill_n(_vPeaks, NUM_BANDS, 0.0f);
    }

    // Perform the FFT 

    void FFT()
    {
        ArduinoFFT<float> _FFT(_vReal, _vImaginary, MAX_SAMPLES, SAMPLING_FREQUENCY);
        _FFT.dcRemoval();
        _FFT.windowing(FFTWindow::Hann, FFTDirection::Forward);
        _FFT.compute(FFTDirection::Forward);
        _FFT.complexToMagnitude();
    }

    // Sample the audio

    void SampleAudio()
    {
        constexpr auto bytesExpected = MAX_SAMPLES * sizeof(ptrSampleBuffer[0]);
        size_t bytesRead = 0;
#if USE_M5
        if (M5.Mic.record((int16_t *)ptrSampleBuffer.get(), MAX_SAMPLES, SAMPLING_FREQUENCY, false))
            bytesRead = bytesExpected;
#else
        ESP_ERROR_CHECK(i2s_start(I2S_NUM_0));
        ESP_ERROR_CHECK(
            i2s_read(I2S_NUM_0, (void *)ptrSampleBuffer.get(), bytesExpected, &bytesRead, 100 / portTICK_PERIOD_MS));
        ESP_ERROR_CHECK(i2s_stop(I2S_NUM_0));
#endif
        if (bytesRead != bytesExpected)
        {
            debugW("Could only read %u bytes of %u in FillBufferI2S()\n", bytesRead, bytesExpected);
            return;
        }
        int16_t smin = INT16_MAX, smax = INT16_MIN;
        long sum = 0;
        for (int i = 0; i < MAX_SAMPLES; i++)
        {
            int16_t s = ptrSampleBuffer[i];
            _vReal[i] = (float)s;
            if (s < smin) smin = s;
            if (s > smax) smax = s;
            sum += s;
        }
        if (smin <= -32768 || smax >= 32767)
            _clipCount++;
        float dc = fabsf((float)sum / (float)MAX_SAMPLES);
        _dcOffsetEMA = _dcOffsetEMA * 0.95f + dc * 0.05f; // slow EMA
    }

    // Update the VU and peak values based on the new sample

    void UpdateVU(float newval)
    {
        if (newval > _oldVU)
            _VU = newval;
        else
            _VU = (_oldVU * VUDAMPEN + newval) / (VUDAMPEN + 1);
        _oldVU = _VU;
        if (_VU > _PeakVU)
            _PeakVU = _VU;
        else
            _PeakVU = (_oldPeakVU * VUDAMPENMAX + _VU) / (VUDAMPENMAX + 1);
        _oldPeakVU = _PeakVU;
        if (_VU < _MinVU)
            _MinVU = _VU;
        else
            _MinVU = (_oldMinVU * VUDAMPENMIN + _VU) / (VUDAMPENMIN + 1);
        _oldMinVU = _MinVU;
    }

    // Compute the band layout based on the sampling frequency and number of bands

    // This computes the start and end bins for each band based on the sampling frequency,
    // ensuring that the bands are spaced logarithmically or in Mel scale as configured.
    // The results are stored in _bandBinStart and _bandBinEnd arrays.

    void ComputeBandLayout()
    {
        const float fMin = LOWEST_FREQ;
        const float fMax = std::min<float>(HIGHEST_FREQ, SAMPLING_FREQUENCY / 2.0f);
        const float binWidth = (float)SAMPLING_FREQUENCY / (MAX_SAMPLES / 2.0f);
        int prevBin = 2;
#if SPECTRUM_BAND_SCALE_MEL
        auto hzToMel = [](float f) { return 2595.0f * log10f(1.0f + f / 700.0f); };
        auto melToHz = [](float m) { return 700.0f * (powf(10.0f, m / 2595.0f) - 1.0f); };
        float melMin = hzToMel(fMin);
        float melMax = hzToMel(fMax);
#endif
        for (int b = 0; b < NUM_BANDS; b++)
        {
            // Shift the effective band index by kBandOffset so logical band 0 starts higher
            const int logicalIdx = b + kBandOffset;
            const float fracHi = (float)(logicalIdx + 1) / (float)(NUM_BANDS + kBandOffset);
#if SPECTRUM_BAND_SCALE_MEL
            float edgeMel = melMin + (melMax - melMin) * fracHi;
            float edgeHiFreq = melToHz(edgeMel);
#else
            float ratio = fMax / fMin;
            float edgeHiFreq = fMin * powf(ratio, fracHi);
#endif
            int hiBin = (int)lroundf(edgeHiFreq / binWidth);
            hiBin = std::clamp(hiBin, prevBin + 1, (int)(MAX_SAMPLES / 2 - 1));
            _bandBinStart[b] = prevBin;
            _bandBinEnd[b] = hiBin;
            prevBin = hiBin;
        }
        _bandBinEnd[NUM_BANDS - 1] = (MAX_SAMPLES / 2 - 1);
    }
    
    PeakData ProcessPeaksEnergy()
    {
        // Band offset handled in ComputeBandLayout so index 0 is the lowest VISIBLE band
         float frameMax = 0.0f;
         float frameSumRaw = 0.0f;
         float noiseSum = 0.0f;
         float noiseMaxAll = 0.0f; // Tracks max noise floor across bands
         // Display gain and band floor constants
         constexpr float kDisplayGain = 1.5f;
         constexpr float kBandFloorMin = 0.01f;
         constexpr float kBandFloorScale = 0.05f;

        // Remember that a slower attack will yield a smoother display but a lower one, as it's always 
        // below the actual for some amount of time

        constexpr float kLiveAttackPerSec = 5.0f;

        // Envelope gating factor and max relative EMA
        float _gateEnv = 1.0f;
        float _vMaxRelEMA = 1.0f;
        // Delta time for attack limiting (replace with actual frame time if available)
        float dt = 0.016f;
        // Allocate _livePeaks if not already done
        if (!_livePeaks) {
            _livePeaks = (float *)PreferPSRAMAlloc(NUM_BANDS * sizeof(_livePeaks[0]));
            for (int i = 0; i < NUM_BANDS; ++i) _livePeaks[i] = 0.0f;
        }
        _framesProcessed++;
        const float binWidth = (float)SAMPLING_FREQUENCY / (MAX_SAMPLES / 2.0f);
        for (int b = 0; b < NUM_BANDS; b++)
        {
            int start = _bandBinStart[b];
            int end = _bandBinEnd[b];
            if (end <= start)
            {
                _vPeaks[b] = 0.0f;
                continue;
            }
            float sumPower = 0.0f;
            for (int k = start; k < end; k++)
            {
                float mag = _vReal[k];
                sumPower += mag * mag;
            }
            int widthBins = end - start;
            float avgPower = (widthBins > 0) ? (sumPower / (float)widthBins) : 0.0f;
            avgPower *= _params.windowPowerCorrection;

            // Apply frequency-dependent compensation
            float compensation = _params.bandCompLow + (_params.bandCompHigh - _params.bandCompLow) * ((float)b / (NUM_BANDS - 1));
            avgPower *= compensation;

            // Track pre-subtraction max for SNR gating
            if (avgPower > frameSumRaw)
                frameSumRaw = avgPower;

            if (avgPower > _noiseFloor[b])
                _noiseFloor[b] = _noiseFloor[b] * (1.0f - _params.energyNoiseAdapt) + (float)avgPower * _params.energyNoiseAdapt;
            else
                _noiseFloor[b] *= _params.energyNoiseDecay;

            // Accumulate noise stats
            noiseSum += _noiseFloor[b];
            if (_noiseFloor[b] > noiseMaxAll)
                noiseMaxAll = _noiseFloor[b];

            float signal = (float)avgPower - _noiseFloor[b];
            if (signal < 0.0f)
                signal = 0.0f;
#if ENABLE_AUDIO_SMOOTHING
            // Weighted average: 0.25 * (2 * current + left + right)
            float left = (b > 0) ? _rawPrev[b - 1] : signal;
            float right = (b < NUM_BANDS - 1) ? _rawPrev[b + 1] : signal;
            float smoothed = 0.125f * (6.0f * signal + left + right);
            _rawPrev[b] = smoothed;
            _vPeaks[b] = smoothed;
            if (smoothed > frameMax)
                frameMax = smoothed;
#else
            _vPeaks[b] = signal;
            if (signal > frameMax)
                frameMax = signal;
#endif
        }
        if (frameMax > _energyMaxEnv)
            _energyMaxEnv = frameMax;
        else
            _energyMaxEnv = std::max(_params.energyMinEnv, _energyMaxEnv * _params.energyEnvDecay);

        _envEMA = _envEMA * 0.95f + _energyMaxEnv * 0.05f;

        // Raw SNR-based frame gate (pre-normalization) to suppress steady HVAC and similar backgrounfd noises

        float snrRaw = frameSumRaw / (noiseMaxAll + 1e-9f);
        if (snrRaw < _params.frameSNRGate)
        {
            _frameGateHits++;
            for (int b = 0; b < NUM_BANDS; ++b)
            {
                _vPeaks[b] = 0.0f;
                _Peaks._Level[b] = 0.0f;
            }
            UpdateVU(0.0f);
            return _Peaks;
        }

        // Anchor normalization to noise-derived floor to avoid auto-gain blow-up
        float noiseMean = noiseSum / (float)NUM_BANDS;
        float envFloor = std::max(_params.energyMinEnv, noiseMean * _params.envFloorFromNoise);
        float normDen = std::max(_energyMaxEnv, envFloor);
        const float invEnv = 1.0f / normDen;
        float sumNorm = 0.0f;

        // Now that layout skips the lowest bins, emit all NUM_BANDS directly with no reindexing
        for (int b = 0; b < NUM_BANDS; b++)
        {
            float vTarget = _vPeaks[b] * invEnv;
            vTarget = cbrtf(std::max(0.0f, vTarget));
            if (vTarget > 1.0f) vTarget = 1.0f;
            vTarget *= _gateEnv;
            vTarget *= kDisplayGain;
            if (vTarget > 1.0f) vTarget = 1.0f;
            const float bandFloor = std::max(kBandFloorMin, kBandFloorScale * _vMaxRelEMA);
            if (vTarget < bandFloor) { vTarget = 0.0f; _bandGateHits++; }
            float vCurr = _livePeaks[b];
            float vNew = vTarget;
            if (vTarget > vCurr)
            {
                const float maxRise = kLiveAttackPerSec * dt;
                vNew = vCurr + std::min(vTarget - vCurr, maxRise);
            }
            if (vNew > 1.0f) vNew = 1.0f;
            if (vNew < 0.0f) vNew = 0.0f;
            _livePeaks[b] = vNew;
            _vPeaks[b] = vNew;
            _Peaks._Level[b] = vNew;
            sumNorm += vNew;
        }
        UpdateVU(sumNorm / (float)NUM_BANDS);
        return _Peaks;
    }

  public:
    // Current beat/level ratio value used by visual effects.
    // Typically maintained by higher-level audio logic.
    // Range ~[0..something], consumer-specific.

    inline float VURatio() const override
    {
        return _VURatio;
    }

    // Smoothed/decayed version of VURatio for more graceful visuals.
    // Use when you want beat emphasis without sharp jumps.

    inline float VURatioFade() const override
    {
        return _VURatioFade;
    }

    // Average normalized energy this frame (0..1 after gating/compression).
    // Updated in ProcessPeaksEnergy()/SetPeakData via UpdateVU().

    inline float VU() const override
    {
        return _VU;
    }

    // Highest recent VU observed (peak hold with damping).
    // Useful for setting adaptive effect ceilings.

    inline float PeakVU() const override
    {
        return _PeakVU;
    }

    // Lowest recent VU observed (floor with damping).
    // Useful as denominator clamps for normalized ratios.

    inline float MinVU() const override
    {
        return _MinVU;
    }

    // Measured audio processing frames-per-second.
    // For diagnostics/telemetry; not critical to effects logic.

    inline int AudioFPS() const override
    {
        return _AudioFPS;
    }

    // Measured serial streaming FPS (if enabled).
    // For diagnostics; may be zero if not used.

    inline int SerialFPS() const override
    {
        return _serialFPS;
    }

    // Indicates whether peaks came from local mic or remote source.
    // Effects may choose to show status based on this.

    inline PeakData::MicrophoneType MicMode() const override
    {
        return _MicMode;
    }

    // Returns the latest per-band normalized peaks (0..1).
    // Pointer remains valid until next ProcessPeaksEnergy/SetPeakData.

    inline const PeakData &Peaks() const override
    {
        return _Peaks;
    }

    // Returns the slower-decay overlay level for the given band (0..1).
    // Used by some visuals to draw trailing bars/dots.

    inline float Peak2Decay(int band) const override
    {
        return (band >= 0 && band < NUM_BANDS) ? _peak2Decay[band] : 0.0f;
    }

    // Direct access to the secondary (slower) decay array.
    // Array length is NUM_BANDS.

    inline const float *Peak2DecayData() const override
    {
        return _peak2Decay;
    }

    // Direct access to the primary (faster) decay array.
    // Array length is NUM_BANDS.

    inline const float *Peak1DecayData() const override
    {
        return _peak1Decay;
    }

    // Returns the faster-decay overlay level for the given band (0..1).
    // Band index is clamped by caller; returns 0 on out-of-range.

    inline float Peak1Decay(int band) const override
    {
        return (band >= 0 && band < NUM_BANDS) ? _peak1Decay[band] : 0.0f;
    }

    // Returns timestamps (ms) of last primary-peak rise per band.
    // Useful for triggering time-based band effects.

    inline const unsigned long *Peak1Times() const override
    {
        return _lastPeak1Time;
    }

    // Helper to read the last primary-peak time for a specific band.
    // Returns 0 if band is out of range.

    inline unsigned long LastPeak1Time(int band) const override
    {
        return (band >= 0 && band < NUM_BANDS) ? _lastPeak1Time[band] : 0;
    }

    // Configure how quickly the two decay overlays drop over time.
    // r1 = fast track, r2 = slow track; higher = faster decay.

    inline void SetPeakDecayRates(float r1, float r2) override
    {
        _peak1DecayRate = r1;
        _peak2DecayRate = r2;
    }

    // Construct analyzer, allocate buffers (PSRAM-preferred), set initial state.
    // Throws std::runtime_error on allocation failure. Computes band layout once.

    SoundAnalyzer()
    {
        ptrSampleBuffer.reset((int16_t *)heap_caps_malloc(MAX_SAMPLES * sizeof(int16_t), MALLOC_CAP_8BIT));
        if (!ptrSampleBuffer)
            throw std::runtime_error("Failed to allocate sample buffer");
        _vReal = (float *)PreferPSRAMAlloc(MAX_SAMPLES * sizeof(_vReal[0]));
        _vImaginary = (float *)PreferPSRAMAlloc(MAX_SAMPLES * sizeof(_vImaginary[0]));
        _vPeaks = (float *)PreferPSRAMAlloc(NUM_BANDS * sizeof(_vPeaks[0]));
        _livePeaks = (float *)PreferPSRAMAlloc(NUM_BANDS * sizeof(_livePeaks[0]));
        if (!_vReal || !_vImaginary || !_vPeaks || !_livePeaks)
            throw std::runtime_error("Failed to allocate FFT buffers");
        for (int i = 0; i < NUM_BANDS; ++i) _livePeaks[i] = 0.0f;
        _oldVU = _oldPeakVU = _oldMinVU = 0.0f;
        _MicMode = PeakData::MESMERIZERMIC;
        _params = ParamsFor(_MicMode);
        /* ValidateAudioConfig(); */
        ComputeBandLayout();
        Reset();
    }

    // Free any heap/PSRAM buffers allocated by the constructor.
    // Safe to call at shutdown/reset.

    ~SoundAnalyzer()
    {
        free(_vReal);
        free(_vImaginary);
        free(_vPeaks);
        free(_livePeaks);
    }

    // These functions allow access to the last-acquired sample buffer and its size so that
    // effects can draw the waveform or do other things with the raw audio data

    // Return pointer to last captured raw samples (int16).
    // Valid until the next FillBufferI2S() call.

    const int16_t *GetSampleBuffer() const
    {
        return ptrSampleBuffer.get();
    }

    // Return count of samples in the sample buffer (MAX_SAMPLES).
    // Pairs with GetSampleBuffer() when drawing waveforms.

    const size_t GetSampleBufferSize() const
    {
        return MAX_SAMPLES;
    }

    // BeatEnhance
    //
    // Looks like pure voodoo, but it returns the multiplier by which to scale a value to enhance it
    // by the current VURatioFade amount.  The amt amount is the amount of your factor that should be
    // made up of the VURatioFade multiplier.  So passing a 0.75 is a lot of beat enhancement, whereas
    // 0.25 is a little bit.

    // Compute a blend factor using VURatioFade to "pulse" visuals.
    // amt in [0..1] controls how strongly the ratio influences the result.

    float BeatEnhance(float amt)
    {
        return ((1.0 - amt) + (_VURatioFade / 2.0) * amt);
    }

    // flash record size, for recording 5 second
    // SampleBufferInitI2S
    //
    // install and start i2s driver

    // Configure and start the I2S (or M5) input at SAMPLING_FREQUENCY.
    // Board-specific branches set pins and ADC/I2S modes as needed.

    void SampleBufferInitI2S()
    {
        // install and start i2s driver

        debugV("Begin SamplerBufferInitI2S...");

#if USE_M5

        // Can't use speaker and mic at the same time, and speaker defaults on, so turn it off

        M5.Speaker.setVolume(255);
        M5.Speaker.end();
        auto cfg = M5.Mic.config();
        cfg.sample_rate = SAMPLING_FREQUENCY;
        cfg.noise_filter_level = 0;
        cfg.magnification = 8;
        M5.Mic.config(cfg);
        M5.Mic.begin();

#elif ELECROW || USE_I2S_AUDIO

        const i2s_config_t i2s_config = {.mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX),
                                         .sample_rate = SAMPLING_FREQUENCY,
                                         .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,
                                         .channel_format = I2S_CHANNEL_FMT_ONLY_LEFT,
                                         .communication_format = I2S_COMM_FORMAT_STAND_I2S,
                                         .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
                                         .dma_buf_count = 4,  // Increased from 2 for better buffering
                                         .dma_buf_len = (int)MAX_SAMPLES,
                                         .use_apll = false,
                                         .tx_desc_auto_clear = false,
                                         .fixed_mclk = 0};

        // i2s pin configuration - explicitly set pin modes
        pinMode(I2S_BCLK_PIN, OUTPUT);  // Bit clock is output from ESP32
        pinMode(I2S_WS_PIN, OUTPUT);    // Word select is output from ESP32  
        pinMode(INPUT_PIN, INPUT);      // Data input from microphone
        
        const i2s_pin_config_t pin_config = {.bck_io_num = I2S_BCLK_PIN,
                                             .ws_io_num = I2S_WS_PIN,
                                             .data_out_num = I2S_PIN_NO_CHANGE, // not used
                                             .data_in_num = INPUT_PIN};

        ESP_ERROR_CHECK(i2s_driver_install(I2S_NUM_0, &i2s_config, 0, NULL));
        ESP_ERROR_CHECK(i2s_set_pin(I2S_NUM_0, &pin_config));
        
        // Clear any existing data and start fresh
        ESP_ERROR_CHECK(i2s_zero_dma_buffer(I2S_NUM_0));
        ESP_ERROR_CHECK(i2s_start(I2S_NUM_0));

#else

        // This block is for TTGO, MESMERIZER, SPECTRUM_WROVER_KIT and other projects that
        // use an analog mic connected to the input pin.
        
        static_assert(SOC_I2S_SUPPORTS_ADC, "This ESP32 model does not support ADC built-in mode");

        i2s_config_t i2s_config;
        #if SOC_I2S_SUPPORTS_ADC
            i2s_config.mode = (i2s_mode_t) (I2S_MODE_MASTER | I2S_MODE_RX | I2S_MODE_ADC_BUILT_IN);
        #else
            i2s_config.mode = (i2s_mode_t) (I2S_MODE_MASTER | I2S_MODE_RX);
        #endif

        i2s_config.sample_rate = SAMPLING_FREQUENCY;
        i2s_config.dma_buf_len = MAX_SAMPLES;
        i2s_config.bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT;
        i2s_config.channel_format = I2S_CHANNEL_FMT_ONLY_LEFT;
        i2s_config.use_apll = false;
        i2s_config.communication_format = I2S_COMM_FORMAT_STAND_I2S;
        i2s_config.intr_alloc_flags = ESP_INTR_FLAG_LEVEL1;
        i2s_config.dma_buf_count = 2;

        ESP_ERROR_CHECK(adc1_config_width(ADC_WIDTH_BIT_12));
        ESP_ERROR_CHECK(adc1_config_channel_atten(ADC1_CHANNEL_0, ADC_ATTEN_DB_0));
        ESP_ERROR_CHECK(i2s_driver_install(I2S_NUM_0, &i2s_config, 0, NULL));
        ESP_ERROR_CHECK(i2s_set_adc_mode(ADC_UNIT_1, ADC1_CHANNEL_0));

#endif

        debugV("SamplerBufferInitI2S Complete\n");
    }

    // DecayPeaks
    //
    // Every so many ms we decay the peaks by a given amount

    // Apply time-based decay to the two peak overlay arrays.
    // Called once per frame; uses AppTime.LastFrameTime() and configurable rates.

    inline void DecayPeaks()
    {
        float decayAmount1 = std::max(0.0, g_Values.AppTime.LastFrameTime() * _peak1DecayRate);
        float decayAmount2 = std::max(0.0, g_Values.AppTime.LastFrameTime() * _peak2DecayRate);

        for (int iBand = 0; iBand < NUM_BANDS; iBand++)
        {
            _peak1Decay[iBand] -= min(decayAmount1, _peak1Decay[iBand]);
            _peak2Decay[iBand] -= min(decayAmount2, _peak2Decay[iBand]);
        }

        // Removed old smoothing of decay overlays; smoothing now applies to live peaks in ProcessPeaksEnergy()
    }

    // Update the per-band decay overlays from the latest peaks.
    // Rises are limited by VU_REACTIVITY_RATIO; records timestamps on new primary peaks.

    inline void UpdatePeakData()
    {
        for (int i = 0; i < NUM_BANDS; i++)
        {
            if (_Peaks[i] > _peak1Decay[i])
            {
                const float maxIncrease =
                    std::max(0.0, g_Values.AppTime.LastFrameTime() * _peak1DecayRate * VU_REACTIVITY_RATIO);
                _peak1Decay[i] = std::min(_Peaks[i], _peak1Decay[i] + maxIncrease);
                _lastPeak1Time[i] = millis();
            }
            if (_Peaks[i] > _peak2Decay[i])
            {
                const float maxIncrease =
                    std::max(0.0, g_Values.AppTime.LastFrameTime() * _peak2DecayRate * VU_REACTIVITY_RATIO);
                _peak2Decay[i] = std::min(_Peaks[i], _peak2Decay[i] + maxIncrease);
            }
        }
    }

    // Accept externally provided peaks (e.g., over WiFi) and update internal state.
    // Also recomputes VU from the new band values and records source time.

    inline void SetPeakData(const PeakData &peaks)
    {
        _msLastRemote = millis();
        _Peaks = peaks;
        std::copy(&_Peaks._Level[0], &_Peaks._Level[0] + NUM_BANDS, _vPeaks);
        float sum = std::accumulate(_vPeaks, _vPeaks + NUM_BANDS, 0.0f);
        UpdateVU(sum / NUM_BANDS);
    }

    // Expose computed band start indices (inclusive) for diagnostics.
    // Length is NUM_BANDS; pairs with BandBinEnds().

    inline const int *BandBinStarts() const
    {
        return _bandBinStart;
    }

    // Expose computed band end indices (exclusive) for diagnostics.
    // Length is NUM_BANDS; pairs with BandBinStarts().

    inline const int *BandBinEnds() const
    {
        return _bandBinEnd;
    }
#if ENABLE_AUDIO_DEBUG
    // Print per-band [start-end] bin ranges over Serial for debugging.
    // Useful to verify spacing and coverage with current config.

    void DumpBandLayout() const
    {
        Serial.println("Band layout (start-end):");
        for (int b = 0; b < NUM_BANDS; b++)
        {
            Serial.print(b);
            Serial.print(": ");
            Serial.print(_bandBinStart[b]);
            Serial.print('-');
            Serial.println(_bandBinEnd[b]);
        }
    }
#endif

    //
    // RunSamplerPass
    //

    // Perform one audio acquisition/processing step.
    // Uses local mic if no recent remote peaks; otherwise trusts remote and only updates VU.

    inline void RunSamplerPass()
    {
        if (millis() - _msLastRemote > AUDIO_PEAK_REMOTE_TIMEOUT)
        {
#if M5STICKCPLUS2
            _MicMode = PeakData::M5PLUS2;
#elif USE_M5
            _MicMode = PeakData::M5;
#else
            _MicMode = PeakData::MESMERIZERMIC;
#endif
            _params = ParamsFor(_MicMode);
            Reset();
            SampleAudio();
            FFT();
            ProcessPeaksEnergy();
        }
        else
        {
            float sum = std::accumulate(&_Peaks._Level[0], &_Peaks._Level[0] + NUM_BANDS, 0.0f);
            _MicMode = PeakData::PCREMOTE;
            _params = ParamsFor(_MicMode);
            UpdateVU(sum / NUM_BANDS);
        }
    }
};
#endif

extern SoundAnalyzer g_Analyzer;
