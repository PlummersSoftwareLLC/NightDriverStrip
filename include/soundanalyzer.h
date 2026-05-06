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

#include <Arduino.h>
#include <arduinoFFT.h>
#include <array>
#include <memory>
#include <mutex>

#include <esp_idf_version.h>
#define IS_IDF5 (ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(5, 0, 0))

#if IS_IDF5
    #include <driver/i2s_std.h>
    #include <esp_adc/adc_continuous.h>
#else
    #include <driver/adc.h>
    #include <driver/i2s.h>
#endif

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
struct AudioInputParams
{
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
inline constexpr AudioInputParams kParamsMesmerizer
{
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
inline constexpr AudioInputParams kParamsM5
{
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
inline constexpr AudioInputParams kParamsM5Plus2
{
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
inline constexpr AudioInputParams kParamsI2SExternal
{
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

// AudioSerialTaskEntry was migrated into AudioSerialBridge::Run() (ITaskService).

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

// BeatInfo
//
// Beat detection is computed once in the analyzer and published as a compact
// event record so effects can react without rescanning the FFT themselves.
struct BeatInfo
{
    uint32_t sequence = 0;
    uint32_t timestampMs = 0;
    float intervalMs = 0.0f;
    float bpm = 0.0f;
    float msPerBeat = 0.0f;
    float confidence = 0.0f;
    float strength = 0.0f;
    float bass = 0.0f;
    float mid = 0.0f;
    float treble = 0.0f;
    float flux = 0.0f;
    float vu = 0.0f;
    float vuRatio = 0.0f;
    bool major = false;
    bool simulated = false;
};

// Interface for SoundAnalyzer (audio and non-audio variants)
class ISoundAnalyzer
{
  public:
    virtual ~ISoundAnalyzer() = default;

    // --- Core Processing ---
    virtual void RunSamplerPass() = 0;
    virtual void SimulateBeatPass() = 0;
    virtual void SetPeakDecayRates(float r1, float r2) = 0;

    // --- Telemetry & Status ---
    virtual int AudioFPS() const = 0;
    virtual int SerialFPS() const = 0;
    virtual bool IsRemoteAudioActive() const = 0;

    // --- VU Metrics ---
    virtual float VU() const = 0;
    virtual float VURatio() const = 0;
    virtual float VURatioFade() const = 0;
    virtual float PeakVU() const = 0;
    virtual float MinVU() const = 0;

    // --- Spectral Data (Bands) ---
    virtual const PeakData & Peaks() const = 0;
    virtual float Peak1Decay(int band) const = 0;
    virtual float Peak2Decay(int band) const = 0;
    virtual unsigned long LastPeak1Time(int band) const = 0;
    virtual BeatInfo LastBeat() const = 0;
    virtual BeatInfo LastNearBeat() const = 0;

    // --- Simulation & Testing ---
    virtual void SetSimulateBeat(bool) = 0;
    virtual void SetSimulateBPM(int) = 0;
    virtual bool GetSimulateBeat() const = 0;
    virtual int GetSimulateBPM() const = 0;
};

#if !ENABLE_AUDIO

#ifndef NUM_BANDS
#define NUM_BANDS 1
#endif

class SoundAnalyzer : public ISoundAnalyzer // Non-audio case stub
{
    PeakData _emptyPeaks; // zero-initialized
    BeatInfo _beatInfo{};
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

    bool IsRemoteAudioActive() const override
    {
        return false;
    }

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

    BeatInfo LastBeat() const override
    {
        return _beatInfo;
    }

    BeatInfo LastNearBeat() const override
    {
        return _beatInfo;
    }

    void SetPeakDecayRates(float, float) override
    {
    }

    bool GetSimulateBeat() const override
    {
		return false;
	}

    int GetSimulateBPM() const override
	{
		return 0;
	}

    void SetSimulateBeat(bool) override {}
    void SetSimulateBPM(int) override {}
    void RunSamplerPass() override {}
    void SimulateBeatPass() override {}

};

#else // Audio case

// SoundAnalyzerBase
//
// Non-template base class containing hardware-specific logic and shared state
// that does not depend on AudioInputParams tuning.

class SoundAnalyzerBase : public ISoundAnalyzer
{
  public:
    // I'm old enough I can only hear up to about 12000Hz, but feel free to adjust.  Remember from
    // school that you need to sample at double the frequency you want to process, so 24000 samples is 12000Hz

    static constexpr size_t SAMPLING_FREQUENCY = 24000;
    static constexpr size_t LOWEST_FREQ = 100;
    static constexpr size_t HIGHEST_FREQ = SAMPLING_FREQUENCY / 2;

    SoundAnalyzerBase();
    virtual ~SoundAnalyzerBase();

    void InitAudioInput();

    // Idempotent. Stops DMA and uninstalls the I2S/ADC driver if (and only if)
    // InitAudioInput previously installed it. Safe to call multiple times.
    // Used by AudioService::Stop() to release the hardware on disable/reconfigure
    // without waiting for global destruction at program end.

    void TeardownAudioInput();

    // True between a successful InitAudioInput and the next TeardownAudioInput.

    bool IsHardwareInstalled() const { return _hardwareInstalled; }

    void RunSamplerPass() override;
    void SimulateBeatPass() override;
    void SetPeakDecayRates(float r1, float r2) override;

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
    bool IsRemoteAudioActive() const override
    {
        return millis() - _msLastRemoteAudio <= AUDIO_PEAK_REMOTE_TIMEOUT;
    }

    // Average normalized energy this frame (0..1 after gating/compression).
    // Updated in ProcessPeaksEnergy()/SetPeakDataFromRemote via UpdateVU().
    float VU() const override
    {
        return _VU;
    }

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

    // Returns the latest per-band normalized peaks (0..1).
    // Pointer remains valid until next ProcessPeaksEnergy/SetPeakDataFromRemote.
    const PeakData & Peaks() const override
    {
        return _Peaks;
    }

    // Returns the faster-decay overlay level for the given band (0..1).
    // Used by some visuals to draw trailing bars/dots.
    float Peak1Decay(int band) const override;

    // Returns the slower-decay overlay level for the given band (0..1).
    // Used by some visuals to draw trailing bars/dots.
    float Peak2Decay(int band) const override;

    // Returns the timestamp of the last time the primary peak for this band was updated.
    unsigned long LastPeak1Time(int band) const override;

    BeatInfo LastBeat() const override
    {
        std::lock_guard<std::mutex> lock(_beatInfoMutex);
        return _lastBeatInfo;
    }

    BeatInfo LastNearBeat() const override
    {
        std::lock_guard<std::mutex> lock(_beatInfoMutex);
        return _lastNearBeatInfo;
    }

    void SetSimulateBeat(bool b) override
    {
        _simulateBeat = b;
    }
    void SetSimulateBPM(int bpm) override
    {
        _simBPM = bpm;
    }
    bool GetSimulateBeat() const override
    {
        return _simulateBeat;
    }
    int GetSimulateBPM() const override
    {
        return _simBPM;
    }

    void DecayPeaks();
    void UpdatePeakData();
    void SetPeakDataFromRemote(const PeakData &peaks);

    // Return pointer to last captured raw samples (int16).
    // Valid until the next FillBufferI2S() call.
    const int16_t *GetSampleBuffer() const
    {
        return ptrSampleBuffer.get();
    }

    // Return count of samples in the sample buffer (MAX_SAMPLES).
    // Pairs with GetSampleBuffer() when drawing waveforms.
    size_t GetSampleBufferSize() const
    {
        return MAX_SAMPLES;
    }

    // BeatEnhance
    //
    // Looks like pure voodoo, but it returns the multiplier by which to scale a value to enhance it
    // by the current VURatioFade amount.  The amt amount is the amount of your factor that should be
    // made up of the VURatioFade multiplier. So passing a 0.75 is a lot of beat enhancement, whereas
    // 0.25 is a little bit.
    //
    // Compute a blend factor using VURatioFade to "pulse" visuals.
    // amt in [0..1] controls how strongly the ratio influences the result.
    float BeatEnhance(float amt);

    // Zero out VU/peak/ratio state and clear the FFT buffers. Public so that
    // AudioService::Stop() can leave g_Analyzer in a true "silent" state when
    // audio is disabled or being reconfigured. Cheap, safe to call from any
    // thread that already holds the analyzer's locking discipline.

    void Reset();

    // Public writeback setters for the audio task loops. Floats and ints
    // are read atomically on Xtensa for naturally-aligned 4-byte values, so
    // the many-readers / single-writer pattern needs no extra locking.
    // Replaces the prior friend-based direct-member-access pattern that
    // assumed AudioSamplerTaskEntry / AudioSerialTaskEntry were free
    // functions.

    void SetVURatio(float v)     { _VURatio = v; }
    void SetVURatioFade(float v) { _VURatioFade = v; }
    void SetAudioFPS(int fps)    { _AudioFPS = fps; }
    void SetSerialFPS(int fps)   { _serialFPS = fps; }

  protected:

    float _VURatio = 1.0f;
    float _VURatioFade = 1.0f;
    float _VU = 0.0f;
    float _PeakVU = 0.0f;
    float _MinVU = 0.0f;
    int _AudioFPS = 0;
    int _serialFPS = 0;
    uint _msLastRemoteAudio = 0;

    float _oldVU = 0.0f;               // Old VU value for damping
    float _oldPeakVU = 0.0f;           // Old peak VU value for damping
    float _oldMinVU = 0.0f;            // Old min VU value for damping

    PeakData _vPeaks{};                // Normalized band energies 0..1
    PeakData _livePeaks{};             // Attack-limited display peaks per band
    PeakData _Peaks{};                 // cached last normalized peaks
    PeakData _beatPeaks{};             // Beat-only peaks derived before display autoscale/attack limiting
    std::array<int, NUM_BANDS> _bandBinStart{};
    std::array<int, NUM_BANDS> _bandBinEnd{};
    float _energyMaxEnv = 0.01f;       // adaptive envelope for autoscaling (start low for fast adaptation)
    std::array<float, NUM_BANDS> _noiseFloor{}; // adaptive per-band noise floor
    std::array<float, NUM_BANDS> _rawPrev{};    // previous raw (noise-subtracted) power for smoothing

    // --- Beat Simulation State ---
    bool _simulateBeat = false;
    int _simBPM = 120;

    // --- Peak Decay State ---
    // These track peaks over time with decay for visual effects
    std::array<unsigned long, NUM_BANDS> _lastPeak1Time{};
    std::array<float, NUM_BANDS> _peak1Decay{};
    std::array<float, NUM_BANDS> _peak2Decay{};
    float _peak1DecayRate = 1.25f;
    float _peak2DecayRate = 1.25f;

    // Keep beat state next to the analyzer so effects observe one shared pulse.
    BeatInfo _lastBeatInfo{};
    BeatInfo _lastNearBeatInfo{};
    mutable std::mutex _beatInfoMutex;
    PeakData _previousBeatPeaks{};
    float _beatScoreBaseline = 0.0f;
    float _beatFluxBaseline = 0.0f;
    float _beatBassBaseline = 0.0f;
    float _beatScoreDeviation = 0.0f;
    float _beatFluxDeviation = 0.0f;
    float _beatBassDeviation = 0.0f;
    float _previousBeatIntervalMs = 500.0f;
    uint32_t _lastBeatDetectedMs = 0;
    uint32_t _lastBeatDebugMs = 0;
    uint32_t _lastSimulatedBeatIndex = 0;
    bool _hasSimulatedBeat = false;

    static constexpr int kBandOffset = 2; // number of lowest source bands to skip in layout (skip bins 0,1,2)
    std::array<float, MAX_SAMPLES> _vReal{};
    std::array<float, MAX_SAMPLES> _vImaginary{};
    std::unique_ptr<int16_t[]> ptrSampleBuffer; // sample buffer storage

#if IS_IDF5
    i2s_chan_handle_t _rx_handle = nullptr;
    adc_continuous_handle_t _adc_handle = nullptr;
#endif

    // Tracked per-instance so AudioService::Stop can call TeardownAudioInput
    // without risk of double-uninstalling the I2S/ADC driver. Set true on
    // a successful InitAudioInput, cleared by TeardownAudioInput.

    bool _hardwareInstalled = false;

    // The FFT object is now a member variable to avoid ctor/dtor overhead per frame.
    // Declaration order ensures _vReal and _vImaginary address are stable when _FFT is initialized.
    ArduinoFFT<float> _FFT;

    void FFT();
    void SampleAudio();
    void UpdateVU(float newval);
    void ComputeBandLayout();
    void ResetFrameState();
    void ResetBeatDetection();
    void UpdateBeatDetection();
    void RecordBeat(uint32_t now, float confidence, float strength, float bass, float mid, float treble, float flux, bool simulated);
    void RecordNearBeat(uint32_t now, float score, float strength, float bass, float mid, float treble, float flux);

    // Energy spectrum processing (implemented inline or in template)
    //
    // Calculate a logarithmic scale for the bands like you would find on a graphic equalizer display
    virtual const PeakData & ProcessPeaksEnergy() = 0;

    void InitM5();
    void InitI2S_Modern();
    void InitI2S_Legacy();
    void InitADC_Modern();
    void InitADC_Legacy();

    size_t SampleM5();
    size_t SampleI2S_Modern();
    size_t SampleI2S_Legacy();
    size_t SampleADC_Modern();
    size_t SampleADC_Legacy();
};

// SoundAnalyzer
//
// The SoundAnalyzer class uses I2S to read samples from the microphone and then runs an FFT on the
// results to generate the peaks in each band, as well as tracking an overall VU and VU ratio, the
// latter being the ratio of the current VU to the trailing min and max VU.
//
// The template class handles the parameter-specific energy processing logic.

template<const AudioInputParams& Params>
class SoundAnalyzer : public SoundAnalyzerBase
{
  public:
    // Compile-time microphone parameters - bind to template param by reference
    static constexpr const AudioInputParams& _params = Params;

    // Construct analyzer, allocate buffers (PSRAM-preferred), set initial state.
    // Throws std::runtime_error on allocation failure. Computes band layout once.
    SoundAnalyzer() : SoundAnalyzerBase()
    {
    }

  protected:
    // Energy spectrum processing (implemented in soundanalyzer.cpp)
    //
    // Processes the FFT results and extracts energy per frequency band, applying scaling and normalization.
    const PeakData & ProcessPeaksEnergy() override;
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
