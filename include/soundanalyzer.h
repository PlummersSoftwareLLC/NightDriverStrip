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
//              Aug-09-2025         Davepl      Substantively rewritten to use C++20 features
//
//---------------------------------------------------------------------------

#pragma once

#include "globals.h"
#include <algorithm>
#include <numeric>
#include <array>
#include <arduinoFFT.h>
#include <driver/adc.h>
#include <driver/i2s.h>
#include <memory>

#ifndef SPECTRUM_BAND_SCALE_MEL
#define SPECTRUM_BAND_SCALE_MEL 1
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
    float energyEnvDecay;         // Envelope fall (autoscale) per frame
    float energyMinEnv;           // Min envelope to avoid div-by-zero
    float bandCompHigh;           // High-band compensation scalar
    float envFloorFromNoise;      // Multiplier on mean noise floor to cap normalization
    float frameSNRGate;           // Gate frame if raw SNR (max/noiseMax) is below this
    float postScale;              // Post-scale for normalized band values (applied before clamp)
    float compressGamma;          // Exponent for compression on normalized v (e.g., 1/3=cuberoot, 1/6=sqrt(cuberoot))
    float quietEnvFloorGate;      // Gate entire frame if envFloor (noiseMean*envFloorFromNoise) < this (0 disables)
    float liveAttackPerSec;       // Max rise per second for live peaks (frame-rate independent attack limit)
};

// Mesmerizer (default) tuning
inline constexpr AudioInputParams kParamsMesmerizer{
    4.0f,      // windowPowerCorrection
    0.02f,     // energyNoiseAdapt
    0.98f,     // energyNoiseDecay
    0.99f,     // energyEnvDecay
    0.000001f, // energyMinEnv
    1.0f,      // bandCompHigh
    3.0f,      // envFloorFromNoise (cap auto-gain at ~1/4 of pure-noise)
    0.0f,      // frameSNRGate (require ~3:1 SNR to show frame)
    1.0f,      // postScale (Mesmerizer default)
    (1.0/3.0), // compressGamma (cube root)
    1500000,   // quietEnvFloorGate (cutoff gates all ssound when below this level)
    1000.0f    // liveAttackPerSec (very fast attack)
};

// PC Remote uses Mesmerizer defaults
inline constexpr AudioInputParams kParamsPCRemote = kParamsMesmerizer;

// M5 variants use a higher postScale by default
inline constexpr AudioInputParams kParamsM5{
    4.0f,      // windowPowerCorrection
    0.02f,     // energyNoiseAdapt
    0.98f,     // energyNoiseDecay
    0.95f,     // energyEnvDecay (faster decay for quicker adaptation)
    0.01f,     // energyMinEnv (higher floor for faster startup)
    1.5f,      // bandCompHigh (boost upper bands for M5)
    3.0f,      // envFloorFromNoise
    0.0f,      // frameSNRGate
    1.0f,      // postScale
    (1.0/8.0), // compressGamma (cube root)
    1000000,   // quietEnvFloorGate (cutoff gates all ssound when below this level)
    1000.0f    // liveAttackPerSec
};
inline constexpr AudioInputParams kParamsM5Plus2{
    4.0f,      // windowPowerCorrection
    0.02f,     // energyNoiseAdapt
    0.98f,     // energyNoiseDecay
    0.95f,     // energyEnvDecay (faster decay for quicker adaptation)
    0.01f,     // energyMinEnv (higher floor for faster startup)
    1.0f,      // bandCompHigh (no attenuation for high bands)
    3.0f,      // envFloorFromNoise
    0.0f,      // frameSNRGate
    1.0f,      // postScale
    (1.0/3.0), // compressGamma (cube root)
    1500000,   // quietEnvFloorGate (cutoff gates all ssound when below this level)
    1000.0f    // liveAttackPerSec
};

// I2S External microphones (INMP441, etc.) use higher postScale and less aggressive gating
inline constexpr AudioInputParams kParamsI2SExternal{
    4.0f,           // windowPowerCorrection (increased from 4.0f for more gain)
    0.02f,          // energyNoiseAdapt (slower noise adaptation)
    0.98f,          // energyNoiseDecay (slower noise floor decay)
    0.95f,          // energyEnvDecay (slower envelope decay)
    0.01f,          // energyMinEnv (higher floor to prevent over-normalization with quiet I2S mics)
    1.0f,           // bandCompHigh (no attenuation for high bands)
    3.0f,           // envFloorFromNoise
    0.0f,           // frameSNRGate (disable SNR gating for external mics)
    1.5f,           // postScale (much higher gain for external I2S mics - was 4.0f)
    (1.0/3.0),      // compressGamma (cube root)
    1500000,        // quietEnvFloorGate (cutoff gates all ssound when below this level)
    1000.0f         // liveAttackPerSec
};

// AudioInputParams instances for different microphone types - used directly as template parameters

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

// Replace previous PeakData class with a direct alias to std::array
using PeakData = std::array<float, NUM_BANDS>;

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
    virtual bool IsRemoteAudioActive() const = 0;
    virtual const PeakData & Peaks() const = 0;
    virtual float Peak2Decay(int band) const = 0;
    virtual float Peak1Decay(int band) const = 0;
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
    bool IsRemoteAudioActive() const override { return false; }
    const PeakData &Peaks() const override
    {
        return _emptyPeaks;
    }
    float Peak2Decay(int) const override
    {
        return 0.0f;
    }
    float Peak1Decay(int) const override
    {
        return 0.0f;
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

template<const AudioInputParams& Params>
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
    int _AudioFPS = 0;
    int _serialFPS = 0;
    uint _msLastRemoteAudio = 0;

    // I'm old enough I can only hear up to about 12000Hz, but feel free to adjust.  Remember from
    // school that you need to sample at double the frequency you want to process, so 24000 samples is 12000Hz

    static constexpr size_t SAMPLING_FREQUENCY = 24000;
    static constexpr size_t LOWEST_FREQ = 100;
    static constexpr size_t HIGHEST_FREQ = SAMPLING_FREQUENCY / 2;

    float   _oldVU;                     // Old VU value for damping
    float   _oldPeakVU;                 // Old peak VU value for damping
    float   _oldMinVU;                  // Old min VU value for damping
    PeakData _vPeaks{};    // Normalized band energies 0..1
    PeakData _livePeaks{}; // Attack-limited display peaks per band
    std::array<int, NUM_BANDS> _bandBinStart{};
    std::array<int, NUM_BANDS> _bandBinEnd{};
    float   _energyMaxEnv = 0.01f;        // adaptive envelope for autoscaling (start low for fast adaptation)
    std::array<float, NUM_BANDS> _noiseFloor{}; // adaptive per-band noise floor
    std::array<float, NUM_BANDS> _rawPrev{};    // previous raw (noise-subtracted) power for smoothing

    // Peak decay internals (private)
    std::array<unsigned long, NUM_BANDS> _lastPeak1Time{};
    std::array<float, NUM_BANDS> _peak1Decay{};
    std::array<float, NUM_BANDS> _peak2Decay{};
    float _peak1DecayRate = 1.25f;
    float _peak2DecayRate = 1.25f;

    // Compile-time microphone parameters - bind to template param by reference
    static constexpr const AudioInputParams& _params = Params;

    // Energy spectrum processing (implemented inline below)
    //
    // Calculate a logarithmic scale for the bands like you would find on a graphic equalizer display
    //

  private:
    static constexpr int kBandOffset = 2; // number of lowest source bands to skip in layout (skip bins 0,1,2)
    std::array<float, MAX_SAMPLES> _vReal{};
    std::array<float, MAX_SAMPLES> _vImaginary{};
    std::unique_ptr<int16_t[]> ptrSampleBuffer; // sample buffer storage
    PeakData _Peaks; // cached last normalized peaks (moved earlier for inline method visibility)

    // Reset and clear the FFT buffers

    void Reset()
    {
        _vReal.fill(0.0f);
        _vImaginary.fill(0.0f);
        _vPeaks.fill(0.0f);
    }

    // Perform the FFT 

    void FFT()
    {
        ArduinoFFT<float> _FFT(_vReal.data(), _vImaginary.data(), MAX_SAMPLES, SAMPLING_FREQUENCY);
        _FFT.dcRemoval();
        _FFT.windowing(FFTWindow::Hann, FFTDirection::Forward);
        _FFT.compute(FFTDirection::Forward);
        _FFT.complexToMagnitude();
    }

    // Sample the audio

    void SampleAudio()
    {
        size_t bytesRead = 0;
        #if USE_M5
            // M5 path unchanged; fills ptrSampleBuffer with int16 samples
            constexpr auto bytesExpected = MAX_SAMPLES * sizeof(ptrSampleBuffer[0]);
            if (M5.Mic.record((int16_t *)ptrSampleBuffer.get(), MAX_SAMPLES, SAMPLING_FREQUENCY, false))
                bytesRead = bytesExpected;
            if (bytesRead != bytesExpected)
            {
                debugW("Could only read %u bytes of %u in FillBufferI2S()\n", bytesRead, bytesExpected);
                return;
            }
        #elif ELECROW || USE_I2S_AUDIO
                // External I2S microphones like INMP441 produce 24-bit samples in 32-bit words.
                // Read stereo frames (RIGHT_LEFT) and auto-select whichever channel carries the mic.
                {
                    constexpr int kChannels = 2; // RIGHT + LEFT
                    constexpr auto wordsToRead = MAX_SAMPLES * kChannels;
                    constexpr auto bytesExpected32 = wordsToRead * sizeof(int32_t);
                    static int32_t raw32[wordsToRead];
                    // I2S is already running continuously; just read from DMA buffers
                    ESP_ERROR_CHECK(i2s_read(I2S_NUM_0, (void *)raw32, bytesExpected32, &bytesRead, 100 / portTICK_PERIOD_MS));
                    if (bytesRead != bytesExpected32)
                    {
                        debugW("Only read %u of %u bytes from I2S\n", bytesRead, bytesExpected32);
                        return;
                    }
                    // Decide which channel has signal (0 or 1 within each frame)
                    static int s_chanIndex = -1;
                    if (s_chanIndex < 0)
                    {
                        long long sumAbs[2] = {0, 0};
                        for (int i = 0; i < MAX_SAMPLES; ++i)
                        {
                            int32_t r0 = raw32[i * kChannels + 0];
                            int32_t r1 = raw32[i * kChannels + 1];
                            sumAbs[0] += llabs((long long)r0);
                            sumAbs[1] += llabs((long long)r1);
                        }
                        s_chanIndex = (sumAbs[1] > sumAbs[0]) ? 1 : 0;
                    }
                    // Downconvert 24-bit left-justified samples from the chosen channel to signed 16-bit
                    // INMP441 produces 24-bit samples left-justified in 32-bit words (bits [31:8])
                    // For better dynamic range, we shift by 16 to get the top 16 bits of the 24-bit sample
                    // Apply additional scaling since INMP441 may have lower sensitivity than expected
                    for (int i = 0; i < MAX_SAMPLES; i++)
                    {
                        int32_t v = raw32[i * kChannels + s_chanIndex];
                        // Take top 16 bits and apply 2x scaling for INMP441 sensitivity compensation
                        int32_t scaled = (v >> 15);  // Shift by 15 instead of 16 for 2x gain
                        ptrSampleBuffer[i] = (int16_t)std::clamp(scaled, (int32_t)INT16_MIN, (int32_t)INT16_MAX);
                    }
                }
        #else
            // ADC or generic 16-bit I2S sources
            {
                constexpr auto bytesExpected16 = MAX_SAMPLES * sizeof(ptrSampleBuffer[0]);
                // I2S is already running continuously; just read from DMA buffers
                ESP_ERROR_CHECK(                i2s_read(I2S_NUM_0, (void *)ptrSampleBuffer.get(), bytesExpected16, &bytesRead, 100 / portTICK_PERIOD_MS));
                if (bytesRead != bytesExpected16)
                {
                    debugW("Could only read %u bytes of %u in FillBufferI2S()\n", bytesRead, bytesExpected16);
                    return;
                }
            }
        
        #endif
        
        // Compute stats and copy into FFT input buffer
        for (int i = 0; i < MAX_SAMPLES; i++)
        {
            int16_t s = ptrSampleBuffer[i];
            _vReal[i] = (float)s;
        }
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
        int prevBin = 4;  // Start at bin 4 to skip DC (bin 0), very low freq (bins 1-3)
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
         const float kDisplayGain = _params.postScale;  // Use postScale from AudioInputParams instead of hardcoded value
         constexpr float kBandFloorMin = 0.01f;
         constexpr float kBandFloorScale = 0.05f;

        // Attack rate from mic params (frame-rate independent)
        const float kLiveAttackPerSec = _params.liveAttackPerSec;
        // Use reciprocal of AudioFPS for frame-rate independent attack limiting
        float dt = (_AudioFPS > 0) ? (1.0f / (float)_AudioFPS) : 0.016f;
        // Fallback to reasonable default if FPS is invalid
        if (dt <= 0.0f || dt > 0.1f) 
            dt = 0.016f;
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

            // Apply aggressive logarithmic bass suppression with steeper initial curve
            float bandRatio = (float)b / (NUM_BANDS - 1);  // 0.0 to 1.0 across all bands
            
            // Two-stage suppression: moderate suppression on first few bands, then quick recovery
            float suppression;
            if (bandRatio < 0.1f) {  // First 10% of bands get moderate suppression
                // Quadratic suppression for the very first bands (band 0 gets maximum suppression)
                float localRatio = bandRatio / 0.1f;  // 0.0 to 1.0 over first 10% of spectrum
                suppression = 0.005f + (0.05f - 0.005f) * (localRatio * localRatio);  // 0.005 to 0.05 (99.5% to 95% attenuation)
            } else if (bandRatio < 0.4f) {  // Next 30% get moderate suppression with quick recovery
                // Exponential recovery from 0.05 to bandCompHigh over bands 10-40%
                float localRatio = (bandRatio - 0.1f) / 0.3f;  // 0.0 to 1.0 over 10-40% of spectrum
                suppression = 0.05f * expf(localRatio * logf(_params.bandCompHigh / 0.05f));
            } else {
                // Full gain for upper 60% of spectrum
                suppression = _params.bandCompHigh;
            }
            avgPower *= suppression;

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
                float smoothed = 0.25f * (2.0f * signal + left + right);
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

        // Manually clamp back the bass bands, as they are vastly over-represented

        if (frameMax > _energyMaxEnv)
            _energyMaxEnv = frameMax;
        else
            _energyMaxEnv = std::max(_params.energyMinEnv, _energyMaxEnv * _params.energyEnvDecay);

        // Raw SNR-based frame gate (pre-normalization) to suppress steady HVAC and similar backgrounfd noises

        float snrRaw = frameSumRaw / (noiseMaxAll + 1e-9f);
        if (snrRaw < _params.frameSNRGate)
        {
            for (int b = 0; b < NUM_BANDS; ++b)
            {
                _vPeaks[b] = 0.0f;
                _Peaks[b] = 0.0f;
            }
            UpdateVU(0.0f);
            return _Peaks;
        }

        // Anchor normalization to noise-derived floor to avoid auto-gain blow-up
        float noiseMean = noiseSum / (float)NUM_BANDS;
        float envFloorRaw = noiseMean * _params.envFloorFromNoise; // raw (unclamped) env floor
        // Quiet environment gate: if the derived env floor is below a configured threshold, suppress the whole frame
        if (_params.quietEnvFloorGate > 0.0f && envFloorRaw < _params.quietEnvFloorGate)
        {
            for (int b = 0; b < NUM_BANDS; ++b)
            {
                _vPeaks[b] = 0.0f;
                _Peaks[b] = 0.0f;
            }
            UpdateVU(0.0f);
            return _Peaks;
        }

        float envFloor = std::max(_params.energyMinEnv, envFloorRaw);
        float normDen = std::max(_energyMaxEnv, envFloor);
        const float invEnv = 1.0f / normDen;
        float sumNorm = 0.0f;

        // Now that layout skips the lowest bins, emit all NUM_BANDS directly with no reindexing
        for (int b = 0; b < NUM_BANDS; b++)
        {
            float vTarget = _vPeaks[b] * invEnv;
            vTarget = powf(std::max(0.0f, vTarget), _params.compressGamma);
            if (vTarget > 1.0f) vTarget = 1.0f;
            vTarget *= kDisplayGain;
            if (vTarget > 1.0f) vTarget = 1.0f;
            const float bandFloor = std::max(kBandFloorMin, kBandFloorScale);
            if (vTarget < bandFloor) { vTarget = 0.0f; }
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
            _Peaks[b] = vNew;
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
    // Updated in ProcessPeaksEnergy()/SetPeakDataFromRemote via UpdateVU().

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

    // True if we used remote peaks recently
    inline bool IsRemoteAudioActive() const override { return millis() - _msLastRemoteAudio <= AUDIO_PEAK_REMOTE_TIMEOUT; }

    // Returns the latest per-band normalized peaks (0..1).
    // Pointer remains valid until next ProcessPeaksEnergy/SetPeakDataFromRemote.

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

    inline float Peak1Decay(int band) const override
    {
        return (band >= 0 && band < NUM_BANDS) ? _peak1Decay[band] : 0.0f;
    }

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
        _oldVU = _oldPeakVU = _oldMinVU = 0.0f;
        ComputeBandLayout();
        Reset();
    }

    // Free any heap/PSRAM buffers allocated by the constructor.
    // Safe to call at shutdown/reset.

    ~SoundAnalyzer()
    {
        // Stop I2S if it was started
        #if !USE_M5 && (ELECROW || USE_I2S_AUDIO || !defined(USE_I2S_AUDIO))
            i2s_stop(I2S_NUM_0);
            i2s_driver_uninstall(I2S_NUM_0);
        #endif
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
    // made up of the VURatioFade multiplier. So passing a 0.75 is a lot of beat enhancement, whereas
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

#elif USE_I2S_AUDIO

        const i2s_config_t i2s_config = {.mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX),
                                         .sample_rate = SAMPLING_FREQUENCY,
                                         .bits_per_sample = I2S_BITS_PER_SAMPLE_32BIT,
                                         .channel_format = I2S_CHANNEL_FMT_RIGHT_LEFT,
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
        
        // Clear any existing data and start I2S running continuously
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
        // Use reciprocal of AudioFPS for frame-rate independent decay timing
        float audioFrameTime = (_AudioFPS > 0) ? (1.0f / (float)_AudioFPS) : 0.016f;
        if (audioFrameTime <= 0.0f || audioFrameTime > 0.1f) audioFrameTime = 0.016f;
        
        float decayAmount1 = std::max(0.0f, audioFrameTime * _peak1DecayRate);
        float decayAmount2 = std::max(0.0f, audioFrameTime * _peak2DecayRate);

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

    inline void SetPeakDataFromRemote(const PeakData &peaks)
    {
        _msLastRemoteAudio = millis();
        _Peaks = peaks;
        _vPeaks = _Peaks;
        float sum = accumulate(_vPeaks);
        UpdateVU(sum / NUM_BANDS);
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

    // Perform one audio acquisition/processing step.
    // Simplified - no runtime microphone switching needed
    inline void RunSamplerPass()
    {
        if (millis() - _msLastRemoteAudio > AUDIO_PEAK_REMOTE_TIMEOUT)
        {
            // Use local microphone - type determined at compile time
            Reset();
            SampleAudio();
            FFT();
            ProcessPeaksEnergy();
        }
        else
        {
            // Using remote data - just update VU from existing peaks
            float sum = accumulate(_Peaks);
            UpdateVU(sum / NUM_BANDS);
        }
    }
};
#endif

// Project-specific SoundAnalyzer type selection using AudioInputParams directly
#if ENABLE_AUDIO
    #if M5STICKCPLUS2
    using ProjectSoundAnalyzer = SoundAnalyzer<kParamsM5Plus2>;
    #elif USE_M5
    using ProjectSoundAnalyzer = SoundAnalyzer<kParamsM5>;
    #elif USE_I2S_AUDIO
    using ProjectSoundAnalyzer = SoundAnalyzer<kParamsI2SExternal>;
    #else
    using ProjectSoundAnalyzer = SoundAnalyzer<kParamsMesmerizer>;
    #endif
#else
    using ProjectSoundAnalyzer = SoundAnalyzer;
#endif

extern ProjectSoundAnalyzer g_Analyzer;
