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

void AudioSamplerTaskEntry(void *);
void AudioSerialTaskEntry(void *);

#ifndef GAINDAMPEN
    #define GAINDAMPEN 10      // How slowly brackets narrow in for spectrum bands
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
  public:
    // Give internal audio task functions access to private members
    friend void IRAM_ATTR AudioSamplerTaskEntry(void *);
    friend void IRAM_ATTR AudioSerialTaskEntry(void *);

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
    float _VURatio = 1.0f;
    float _VURatioFade = 1.0f;
    float _VU = 0.0f;
    float _PeakVU = 0.0f;
    float _MinVU = 0.0f;
    int _AudioFPS = 0;
    int _serialFPS = 0;
    uint _msLastRemoteAudio = 0;

    static constexpr int kBandOffset = 2; // number of lowest source bands to skip in layout (skip bins 0,1,2)
    std::array<float, MAX_SAMPLES> _vReal{};
    std::array<float, MAX_SAMPLES> _vImaginary{};
    std::unique_ptr<int16_t[]> ptrSampleBuffer; // sample buffer storage
    PeakData _Peaks; // cached last normalized peaks (moved earlier for inline method visibility)

    // The FFT object is now a member variable to avoid ctor/dtor overhead per frame.
    // Declaration order ensures _vReal and _vImaginary address are stable when _FFT is initialized.
    ArduinoFFT<float> _FFT;

    void Reset();
    void FFT();
    void SampleAudio();
    void UpdateVU(float newval);
    void ComputeBandLayout();
    const PeakData & ProcessPeaksEnergy();

  public:
    // Construct analyzer, allocate buffers (PSRAM-preferred), set initial state.
    // Throws std::runtime_error on allocation failure. Computes band layout once.
    SoundAnalyzer();

    // Free any heap/PSRAM buffers allocated by the constructor.
    // Safe to call at shutdown/reset.
    ~SoundAnalyzer();

    // Current beat/level ratio value used by visual effects.
    // Typically maintained by higher-level audio logic.
    // Range ~[0..something], consumer-specific.

    float VURatio() const override
    {
        return _VURatio;
    }

    // Smoothed/decayed version of VURatio for more graceful visuals.
    // Use when you want beat emphasis without sharp jumps.

    float VURatioFade() const override
    {
        return _VURatioFade;
    }

    // Average normalized energy this frame (0..1 after gating/compression).
    // Updated in ProcessPeaksEnergy()/SetPeakDataFromRemote via UpdateVU().

    float VU() const override
    {
        return _VU;
    }

    // Highest recent VU observed (peak hold with damping).
    // Useful for setting adaptive effect ceilings.

    float PeakVU() const override
    {
        return _PeakVU;
    }

    // Lowest recent VU observed (floor with damping).
    // Useful as denominator clamps for normalized ratios.

    float MinVU() const override
    {
        return _MinVU;
    }

    // Measured audio processing frames-per-second.
    // For diagnostics/telemetry; not critical to effects logic.

    int AudioFPS() const override
    {
        return _AudioFPS;
    }

    // Measured serial streaming FPS (if enabled).
    // For diagnostics; may be zero if not used.

    int SerialFPS() const override
    {
        return _serialFPS;
    }

    // Indicates whether peaks came from local mic or remote source.
    // Effects may choose to show status based on this.

    // True if we used remote peaks recently
    inline bool IsRemoteAudioActive() const override { return millis() - _msLastRemoteAudio <= AUDIO_PEAK_REMOTE_TIMEOUT; }

    // Returns the latest per-band normalized peaks (0..1).
    // Pointer remains valid until next ProcessPeaksEnergy/SetPeakDataFromRemote.

    const PeakData &Peaks() const override
    {
        return _Peaks;
    }

    // Returns the slower-decay overlay level for the given band (0..1).
    // Used by some visuals to draw trailing bars/dots.

    float Peak2Decay(int band) const override
    {
        return (band >= 0 && band < NUM_BANDS) ? _peak2Decay[band] : 0.0f;
    }

    float Peak1Decay(int band) const override
    {
        return (band >= 0 && band < NUM_BANDS) ? _peak1Decay[band] : 0.0f;
    }

    unsigned long LastPeak1Time(int band) const override
    {
        return (band >= 0 && band < NUM_BANDS) ? _lastPeak1Time[band] : 0;
    }

    // Configure how quickly the two decay overlays drop over time.
    // r1 = fast track, r2 = slow track; higher = faster decay.

    void SetPeakDecayRates(float r1, float r2) override;

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

    float BeatEnhance(float amt);

    void SampleBufferInitI2S();
    void DecayPeaks();
    void UpdatePeakData();
    void SetPeakDataFromRemote(const PeakData &peaks);
    void RunSamplerPass();

#if ENABLE_AUDIO_DEBUG
    void DumpBandLayout() const;
#endif
};

#endif

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
