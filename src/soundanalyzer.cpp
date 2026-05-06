//+--------------------------------------------------------------------------
//
// File:        soundanalyzer.cpp
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
//   Analyzes the audio input
//
// History:     Sep-15-2020     Davepl      Created
//
//---------------------------------------------------------------------------

#include "globals.h"

#include <algorithm>
#include <cmath>
#include <numeric>

#include "soundanalyzer.h"
#include "systemcontainer.h"
#include "values.h"

#if ENABLE_AUDIO

#include <arduinoFFT.h>

#if USE_M5
    #include <M5Unified.h>
#endif

namespace
{
    int GetConfiguredAudioInputPin()
    {
        if (g_ptrSystem)
            return g_ptrSystem->GetConfiguredAudioInputPin();

        return AUDIO_INPUT_PIN;
    }

    template <size_t N>
    float MeanOfHistory(const std::array<float, N>& values, size_t count)
    {
        if (count == 0)
            return 0.0f;

        const float sum = std::accumulate(values.begin(), values.begin() + count, 0.0f);
        return sum / static_cast<float>(count);
    }

    template <size_t N>
    float StdDevOfHistory(const std::array<float, N>& values, size_t count, float mean)
    {
        if (count == 0)
            return 0.0f;

        float variance = 0.0f;
        for (size_t i = 0; i < count; ++i)
        {
            const float delta = values[i] - mean;
            variance += delta * delta;
        }

        return sqrtf(variance / static_cast<float>(count));
    }
}

// SoundAnalyzerBase
//
// Construct analyzer, allocate buffers (PSRAM-preferred), set initial state.
// Throws std::runtime_error on allocation failure. Computes band layout once.
SoundAnalyzerBase::SoundAnalyzerBase()
    : _FFT(_vReal.data(), _vImaginary.data(), MAX_SAMPLES, SAMPLING_FREQUENCY, true)
{
    ptrSampleBuffer.reset((int16_t *)heap_caps_malloc(MAX_SAMPLES * sizeof(int16_t), MALLOC_CAP_8BIT));
    if (!ptrSampleBuffer)
    {
        throw std::runtime_error("Failed to allocate sample buffer");
    }
    _oldVU = _oldPeakVU = _oldMinVU = 0.0f;
    ComputeBandLayout();
    Reset();
}

SoundAnalyzerBase::~SoundAnalyzerBase()
{
    TeardownAudioInput();
}

// TeardownAudioInput
//
// Idempotent counterpart to InitAudioInput. Called by AudioService::Stop()
// during a runtime audio reconfigure, and also by ~SoundAnalyzerBase at
// program end. The _hardwareInstalled guard makes double-uninstall safe.
void SoundAnalyzerBase::TeardownAudioInput()
{
    if (!_hardwareInstalled)
        return;

    debugI("Audio: tearing down audio input driver");

#if IS_IDF5
    // Stop the hardware first to kill any active DMA transfers
    if (_rx_handle)
    {
        const esp_err_t disableErr = i2s_channel_disable(_rx_handle);
        if (disableErr != ESP_OK)
            debugW("Audio: i2s_channel_disable returned %d", (int)disableErr);
        const esp_err_t delErr = i2s_del_channel(_rx_handle);
        if (delErr != ESP_OK)
            debugW("Audio: i2s_del_channel returned %d", (int)delErr);
        _rx_handle = nullptr;
    }
    if (_adc_handle)
    {
        const esp_err_t stopErr = adc_continuous_stop(_adc_handle);
        if (stopErr != ESP_OK)
            debugW("Audio: adc_continuous_stop returned %d", (int)stopErr);
        const esp_err_t deinitErr = adc_continuous_deinit(_adc_handle);
        if (deinitErr != ESP_OK)
            debugW("Audio: adc_continuous_deinit returned %d", (int)deinitErr);
        _adc_handle = nullptr;
    }
#else
    #if !USE_M5
        // Legacy cleanup - i2s_stop terminates DMA. M5Unified manages its own
        // mic teardown in M5.Mic.end() so we don't double-stop the I2S peripheral.
        i2s_stop(I2S_NUM_0);
        i2s_driver_uninstall(I2S_NUM_0);
    #endif
#endif

#if USE_M5
    M5.Mic.end();
#endif

    _hardwareInstalled = false;
}

// Reset
//
// Reset and clear the FFT buffers. Leaves the analyzer in a benign default
// state: VU/ratio metrics restored to their startup values (1.0f) so any
// effect that scales by VURatio() before the sampler has produced its first
// frame still renders something visible rather than going dark or dividing
// by zero. Beat detection is cleared. The audio sampler task overwrites
// these on its first iteration so the values here only matter when audio
// is stopped, has never been started, or is mid-reconfigure.
// AudioService::Stop() relies on this to give direct g_Analyzer readers
// safe-default values during a reconfigure.

void SoundAnalyzerBase::Reset()
{
    debugI("Audio analyzer full reset");
    ResetFrameState();
    _noiseFloor.fill(0.0f);
    _rawPrev.fill(0.0f);
    _livePeaks.fill(0.0f);
    _energyMaxEnv = 0.01f;
    _msLastRemoteAudio = 0;
    _AudioFPS = 0;
    _serialFPS = 0;
    _VURatio = 1.0f;
    _VURatioFade = 1.0f;
    _VU = 0.0f;
    _PeakVU = 0.0f;
    _MinVU = 0.0f;
    _oldVU = 0.0f;
    _oldPeakVU = 0.0f;
    _oldMinVU = 0.0f;
    ResetBeatDetection();
}

void SoundAnalyzerBase::ResetFrameState()
{
    _vReal.fill(0.0f);
    _vImaginary.fill(0.0f);
    _vPeaks.fill(0.0f);
    _Peaks.fill(0.0f);
    _beatPeaks.fill(0.0f);
}

void SoundAnalyzerBase::ResetBeatDetection()
{
    debugV("Beat detector reset");
    {
        // Need a mutex because it writes the same shared beat structs that the render thread reads.
        std::lock_guard<std::mutex> lock(_beatInfoMutex);
        _lastBeatInfo = {};
        _lastNearBeatInfo = {};
    }
    _previousBeatPeaks.fill(0.0f);
    _beatScoreBaseline = 0.0f;
    _beatFluxBaseline = 0.0f;
    _beatBassBaseline = 0.0f;
    _beatScoreDeviation = 0.0f;
    _beatFluxDeviation = 0.0f;
    _beatBassDeviation = 0.0f;
    _previousBeatIntervalMs = 500.0f;
    _lastBeatDetectedMs = 0;
    _lastBeatDebugMs = 0;
    _lastSimulatedBeatIndex = 0;
    _hasSimulatedBeat = false;
}

// FFT
//
// Perform the FFT
void SoundAnalyzerBase::FFT()
{
    // _FFT is now a member variable to avoid ctor/dtor overhead per frame.
    // Windowing and computation use either the Hann window or others as configured.
    _FFT.windowing(FFTWindow::Hann, FFTDirection::Forward);
    _FFT.compute(FFTDirection::Forward);
    _FFT.complexToMagnitude();
}

// SampleAudio
//
// Sample the audio
void SoundAnalyzerBase::SampleAudio()
{
    size_t bytesRead = 0;
    const auto inputPin = GetConfiguredAudioInputPin();

    if (inputPin < 0)
        return;

#if USE_M5
    bytesRead = SampleM5();
#else
    // Attempt to sample from all supported backends.
    // Those not active in the current configuration will return 0 immediately.
    if (bytesRead == 0) bytesRead = SampleI2S_Modern();
    if (bytesRead == 0) bytesRead = SampleI2S_Legacy();
    if (bytesRead == 0) bytesRead = SampleADC_Modern();
    if (bytesRead == 0) bytesRead = SampleADC_Legacy();
#endif

    if (bytesRead == 0) return;

    // Use std::transform for better code gen (SIMD) when copying/casting samples to FFT input
    std::transform(ptrSampleBuffer.get(), ptrSampleBuffer.get() + MAX_SAMPLES, _vReal.begin(),
                   [](int16_t s) { return static_cast<float>(s); });
}

// UpdateVU
//
// Update the VU and peak values based on the new sample. Instant rise, dampened fall.
void SoundAnalyzerBase::UpdateVU(float newval)
{
    // If VUDAMPEN is 0, this simplifies to _VU = newval
    _VU = (newval > _oldVU) ? newval : (_oldVU * VUDAMPEN + newval) / (VUDAMPEN + 1);
    _oldVU = _VU;

    _PeakVU = (_VU > _oldPeakVU) ? _VU : (_oldPeakVU * VUDAMPENMAX + _VU) / (VUDAMPENMAX + 1);
    _oldPeakVU = _PeakVU;

    _MinVU = (_VU < _oldMinVU) ? _VU : (_oldMinVU * VUDAMPENMIN + _VU) / (VUDAMPENMIN + 1);
    _oldMinVU = _MinVU;
}

// ComputeBandLayout
//
// Compute the band layout based on the sampling frequency and number of bands
//
// This computes the start and end bins for each band based on the sampling frequency,
// ensuring that the bands are spaced logarithmically or in Mel scale as configured.
// The results are stored in _bandBinStart and _bandBinEnd arrays.
void SoundAnalyzerBase::ComputeBandLayout()
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

// BeatEnhance
//
// Looks like pure voodoo, but it returns the multiplier by which to scale a value to enhance it
// by the current VURatioFade amount.  The amt amount is the amount of your factor that should be
// made up of the VURatioFade multiplier. So passing a 0.75 is a lot of beat enhancement, whereas
// 0.25 is a little bit.
//
// Compute a blend factor using VURatioFade to "pulse" visuals.
// amt in [0..1] controls how strongly the ratio influences the result.
float SoundAnalyzerBase::BeatEnhance(float amt)
{
    return ((1.0f - amt) + (_VURatioFade / 2.0f) * amt);
}

// InitAudioInput
//
// Entry point for configuring board-specific audio input (M5, I2S Digital, or I2S ADC Analog).
// Idempotent: if the hardware is already installed, this is a no-op so a stale Start()
// after an aborted Stop() can't double-install the I2S/ADC driver.

void SoundAnalyzerBase::InitAudioInput()
{
    if (_hardwareInstalled)
    {
        debugI("Audio: InitAudioInput skipped, hardware already installed");
        return;
    }

    const auto inputPin = GetConfiguredAudioInputPin();

    if (inputPin < 0)
    {
        debugI("Audio: input pin < 0, skipping hardware initialization. SimBeat only.");
        return;
    }

    debugV("Begin InitAudioInput...");

#if USE_M5
    InitM5();
#else
    // Digital Microphones
    InitI2S_Modern();
    InitI2S_Legacy();

    // Analog Microphones
    InitADC_Modern();
    InitADC_Legacy();
#endif
    _hardwareInstalled = true;
    debugV("InitAudioInput Complete\n");
}

// DecayPeaks
//
// Apply time-based decay to the two peak overlay arrays.
// Called once per frame; uses AppTime.LastFrameTime() and configurable rates.
void SoundAnalyzerBase::DecayPeaks()
{
    // Use reciprocal of AudioFPS for frame-rate independent decay timing
    float audioFrameTime = (_AudioFPS > 0) ? (1.0f / (float)_AudioFPS) : 0.016f;
    if (audioFrameTime <= 0.0f || audioFrameTime > 0.1f) audioFrameTime = 0.016f;

    float decayAmount1 = std::max(0.0f, audioFrameTime * _peak1DecayRate);
    float decayAmount2 = std::max(0.0f, audioFrameTime * _peak2DecayRate);

    // Modernized decay passes using std::transform for potential auto-vectorization
    std::transform(_peak1Decay.begin(), _peak1Decay.end(), _peak1Decay.begin(),
                   [decayAmount1](float v) { return std::max(0.0f, v - decayAmount1); });
    std::transform(_peak2Decay.begin(), _peak2Decay.end(), _peak2Decay.begin(),
                   [decayAmount2](float v) { return std::max(0.0f, v - decayAmount2); });
}

// UpdatePeakData
//
// Update the per-band decay overlays from the latest peaks.
// Rises are limited by VU_REACTIVITY_RATIO; records timestamps on new primary peaks.
void SoundAnalyzerBase::UpdatePeakData()
{
    const float lastFrameTime = g_Values.AppTime.LastFrameTime();
    const float maxInc1 = std::max(0.0f, lastFrameTime * _peak1DecayRate * (float)VU_REACTIVITY_RATIO);
    const float maxInc2 = std::max(0.0f, lastFrameTime * _peak2DecayRate * (float)VU_REACTIVITY_RATIO);
    const auto now = millis();

    for (int i = 0; i < NUM_BANDS; i++)
    {
        // Branchless-style rise limits
        if (_Peaks[i] > _peak1Decay[i])
        {
            _peak1Decay[i] = std::min(_Peaks[i], _peak1Decay[i] + maxInc1);
            _lastPeak1Time[i] = now;
        }
        if (_Peaks[i] > _peak2Decay[i])
        {
            _peak2Decay[i] = std::min(_Peaks[i], _peak2Decay[i] + maxInc2);
        }
    }
}

// SetPeakDecayRates
//
// Configure how quickly the two decay overlays drop over time.
// r1 = fast track, r2 = slow track; higher = faster decay.
void SoundAnalyzerBase::SetPeakDecayRates(float r1, float r2)
{
    _peak1DecayRate = r1;
    _peak2DecayRate = r2;
}

float SoundAnalyzerBase::Peak1Decay(int band) const
{
    if (band < 0 || band >= NUM_BANDS)
        return 0.0f;

    return _peak1Decay[band];
}

float SoundAnalyzerBase::Peak2Decay(int band) const
{
    if (band < 0 || band >= NUM_BANDS)
        return 0.0f;

    return _peak2Decay[band];
}

unsigned long SoundAnalyzerBase::LastPeak1Time(int band) const
{
    if (band < 0 || band >= NUM_BANDS)
        return 0;

    return _lastPeak1Time[band];
}

// SetPeakDataFromRemote
//
// Accept externally provided peaks (e.g., over WiFi) and update internal state.
// Also recomputes VU from the new band values and records source time.
void SoundAnalyzerBase::SetPeakDataFromRemote(const PeakData &peaks)
{
    _msLastRemoteAudio = millis();
    _Peaks = peaks;
    _vPeaks = _Peaks;
    _beatPeaks = _Peaks;
    float sum = std::accumulate(_vPeaks.begin(), _vPeaks.end(), 0.0f);
    UpdateVU(sum / (float)NUM_BANDS);
}

void SoundAnalyzerBase::RecordBeat(uint32_t now, float confidence, float strength, float bass, float mid, float treble, float flux, bool simulated)
{
    std::lock_guard<std::mutex> lock(_beatInfoMutex);
    const float intervalMs = (_lastBeatDetectedMs == 0) ? _previousBeatIntervalMs : static_cast<float>(now - _lastBeatDetectedMs);

    _lastBeatDetectedMs = now;
    _previousBeatIntervalMs = (_lastBeatInfo.sequence == 0)
        ? intervalMs
        : ((_previousBeatIntervalMs * 0.75f) + (intervalMs * 0.25f));

    _lastBeatInfo.sequence++;
    _lastBeatInfo.timestampMs = now;
    _lastBeatInfo.intervalMs = intervalMs;
    _lastBeatInfo.msPerBeat = _previousBeatIntervalMs;
    _lastBeatInfo.bpm = (_previousBeatIntervalMs > 1.0f) ? (60000.0f / _previousBeatIntervalMs) : 0.0f;
    _lastBeatInfo.confidence = confidence;
    _lastBeatInfo.strength = strength;
    _lastBeatInfo.bass = bass;
    _lastBeatInfo.mid = mid;
    _lastBeatInfo.treble = treble;
    _lastBeatInfo.flux = flux;
    _lastBeatInfo.vu = _VU;
    _lastBeatInfo.vuRatio = _VURatio;
    _lastBeatInfo.major = confidence >= 0.95f || strength >= 1.90f;
    _lastBeatInfo.simulated = simulated;
}

void SoundAnalyzerBase::RecordNearBeat(uint32_t now, float score, float strength, float bass, float mid, float treble, float flux)
{
    std::lock_guard<std::mutex> lock(_beatInfoMutex);
    _lastNearBeatInfo.sequence++;
    _lastNearBeatInfo.timestampMs = now;
    _lastNearBeatInfo.intervalMs = 0.0f;
    _lastNearBeatInfo.msPerBeat = _previousBeatIntervalMs;
    _lastNearBeatInfo.bpm = (_previousBeatIntervalMs > 1.0f) ? (60000.0f / _previousBeatIntervalMs) : 0.0f;
    _lastNearBeatInfo.confidence = score;
    _lastNearBeatInfo.strength = strength;
    _lastNearBeatInfo.bass = bass;
    _lastNearBeatInfo.mid = mid;
    _lastNearBeatInfo.treble = treble;
    _lastNearBeatInfo.flux = flux;
    _lastNearBeatInfo.vu = _VU;
    _lastNearBeatInfo.vuRatio = _VURatio;
    _lastNearBeatInfo.major = false;
    _lastNearBeatInfo.simulated = false;
}

void SoundAnalyzerBase::UpdateBeatDetection()
{
    constexpr float kBaselineAlpha = 0.08f;
    constexpr float kDeviationAlpha = 0.12f;

    const size_t bassBands = std::max<size_t>(1, std::min<size_t>(NUM_BANDS, 3));
    const size_t midBands = std::max<size_t>(1, std::min<size_t>(NUM_BANDS - bassBands, std::max<size_t>(1, NUM_BANDS / 3)));

    float bass = 0.0f;
    float mid = 0.0f;
    float treble = 0.0f;
    float flux = 0.0f;
    float lowFlux = 0.0f;

    for (size_t i = 0; i < NUM_BANDS; ++i)
    {
        const float band = _beatPeaks[i];
        const float delta = std::max(0.0f, band - _previousBeatPeaks[i]);

        if (i < bassBands)
        {
            bass += band;
            lowFlux += delta;
        }
        else if (i < bassBands + midBands)
        {
            mid += band;
        }
        else
        {
            treble += band;
        }

        flux += delta;
    }

    bass /= static_cast<float>(bassBands);
    if (midBands > 0)
        mid /= static_cast<float>(midBands);

    const size_t trebleBands = (NUM_BANDS > bassBands + midBands) ? (NUM_BANDS - bassBands - midBands) : 0;
    if (trebleBands > 0)
        treble /= static_cast<float>(trebleBands);

    flux /= static_cast<float>(NUM_BANDS);
    lowFlux /= static_cast<float>(bassBands);
    const float beatBandGroups = static_cast<float>((bassBands > 0 ? 1 : 0) + (midBands > 0 ? 1 : 0) + (trebleBands > 0 ? 1 : 0));
    const float beatVu = (bass + mid + treble) / std::max(1.0f, beatBandGroups);

    const float score = bass * 0.55f + lowFlux * 1.50f + flux * 1.20f + mid * 0.15f + beatVu * 0.25f;
    const float strength = std::clamp((bass * 1.10f) + (lowFlux * 2.10f) + (flux * 1.35f), 0.0f, 2.5f);
    const float scoreThreshold = _beatScoreBaseline + std::max(0.025f, _beatScoreDeviation * 1.08f);
    const float fluxThreshold = _beatFluxBaseline + std::max(0.010f, _beatFluxDeviation * 1.08f);
    const float bassThreshold = _beatBassBaseline + std::max(0.010f, _beatBassDeviation * 0.82f);

    const uint32_t now = millis();
    const float minIntervalMs = std::clamp(_previousBeatIntervalMs * 0.44f, 170.0f, 650.0f);
    const bool enoughGap = (_lastBeatDetectedMs == 0) || (static_cast<float>(now - _lastBeatDetectedMs) >= minIntervalMs);
    const bool candidate = enoughGap
        && score > scoreThreshold
        && flux > fluxThreshold
        && (bass > bassThreshold
            || (bass + lowFlux) > (bassThreshold * 1.18f)
            || lowFlux > (fluxThreshold * 0.92f)
            || (score > (scoreThreshold * 1.03f) && flux > (fluxThreshold * 1.05f)))
        && strength > 0.10f
        && (bass + lowFlux) > 0.06f;
    const bool nearCandidate = enoughGap
        && score > (scoreThreshold * 0.75f)
        && flux > (fluxThreshold * 0.75f)
        && (bass + lowFlux) > 0.05f;

    if (candidate)
    {
        const float confidence = std::clamp(
            ((score - scoreThreshold) * 1.40f)
            + ((flux - fluxThreshold) * 1.80f)
            + ((bass - bassThreshold) * 1.00f)
            + strength * 0.20f,
            0.0f,
            1.5f);

        RecordBeat(now, confidence, strength, bass, mid, treble, flux, false);
        debugV("Beat detected: seq=%lu bass=%.3f flux=%.3f lowFlux=%.3f score=%.3f strength=%.3f bpm=%.1f",
               (unsigned long)_lastBeatInfo.sequence,
               bass,
               flux,
               lowFlux,
               score,
               strength,
               _lastBeatInfo.bpm);
    }
    else if (nearCandidate)
    {
        RecordNearBeat(now, score, strength, bass, mid, treble, flux);

        if (now - _lastBeatDebugMs >= 250)
        {
            _lastBeatDebugMs = now;
            debugV("Beat near-miss: bass=%.3f/%.3f flux=%.3f/%.3f score=%.3f/%.3f lowFlux=%.3f strength=%.3f gap=%d",
                   bass,
                   bassThreshold,
                   flux,
                   fluxThreshold,
                   score,
                   scoreThreshold,
                   lowFlux,
                   strength,
                   enoughGap ? 1 : 0);
        }
    }

    const float baselineAlpha = candidate ? (kBaselineAlpha * 0.35f) : kBaselineAlpha;
    _beatScoreBaseline += (score - _beatScoreBaseline) * baselineAlpha;
    _beatFluxBaseline += (flux - _beatFluxBaseline) * baselineAlpha;
    _beatBassBaseline += (bass - _beatBassBaseline) * baselineAlpha;

    _beatScoreDeviation += (fabsf(score - _beatScoreBaseline) - _beatScoreDeviation) * kDeviationAlpha;
    _beatFluxDeviation += (fabsf(flux - _beatFluxBaseline) - _beatFluxDeviation) * kDeviationAlpha;
    _beatBassDeviation += (fabsf(bass - _beatBassBaseline) - _beatBassDeviation) * kDeviationAlpha;

    _previousBeatPeaks = _beatPeaks;
}

#if ENABLE_AUDIO_DEBUG
// DumpBandLayout
//
// Print per-band [start-end] bin ranges over Serial for debugging.
// Useful to verify spacing and coverage with current config.
void SoundAnalyzerBase::DumpBandLayout() const
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

// SimulateBeatPass
//
// Generates a mock "beat" at the configured BPM to allow visual testing when no mic is present or active.
void SoundAnalyzerBase::SimulateBeatPass()
{
    // --- Beat Timing Calculation ---
    const float beatsPerSecond = _simBPM / 60.0f;
    const float beatPeriodMillis = (beatsPerSecond > 0) ? (1000.0f / beatsPerSecond) : 1000.0f;
    const float beatActiveDurationMillis = beatPeriodMillis * 0.20f; // 20% of the cycle for a punchy beat

    unsigned long currentTime = millis();
    float timeInCycle = fmod(static_cast<float>(currentTime), beatPeriodMillis);
    bool isOnBeat = (timeInCycle < beatActiveDurationMillis);

    float targetVU = 0.0f;
    PeakData simulatedPeaks{}; // Value-initialize to zero

    if (isOnBeat)
    {
        // Use normalized VU range (0.0 - 1.0) to match the processed audio pipeline
        targetVU = 0.85f;
        for (int i = 0; i < NUM_BANDS; ++i)
        {
            // Linear slope in frequency space
            double level = std::max(0.0, 1.0 - (double)i / (NUM_BANDS * 0.85));
            // Apply cube-root compression (0.33) to match the expected traits of Mesmerizer tuning
            simulatedPeaks[i] = static_cast<float>(pow(level, 0.333));
        }
    }
    else
    {
        // Sustained low-level background noise traits (or silence)
        targetVU = 0.02f;
    }

    _Peaks = simulatedPeaks;
    _beatPeaks = simulatedPeaks;
    UpdateVU(targetVU);

    // Simulated beats should fire exactly once per cycle so effects can be
    // tested without depending on heuristic onset detection.
    const uint32_t beatIndex = static_cast<uint32_t>(currentTime / beatPeriodMillis);
    if (isOnBeat && (!_hasSimulatedBeat || beatIndex != _lastSimulatedBeatIndex))
    {
        _hasSimulatedBeat = true;
        _lastSimulatedBeatIndex = beatIndex;
        RecordBeat(currentTime, 1.0f, 2.0f, 0.85f, 0.35f, 0.15f, 0.90f, true);
    }
}

// RunSamplerPass
//
// Perform one audio acquisition/processing step.
// Uses local mic if no recent remote peaks; otherwise trusts remote and only updates VU.
void SoundAnalyzerBase::RunSamplerPass()
{
    if (_simulateBeat)
    {
        SimulateBeatPass();
        return;
    }

    if (millis() - _msLastRemoteAudio > AUDIO_PEAK_REMOTE_TIMEOUT)
    {
        // Use local microphone - type determined at compile time
        ResetFrameState();
        SampleAudio();
        FFT();
        ProcessPeaksEnergy();
    }
    else
    {
        // Using remote data - just update VU from existing peaks
        float sum = std::accumulate(_Peaks.begin(), _Peaks.end(), 0.0f);
        UpdateVU(sum / NUM_BANDS);
    }

    UpdateBeatDetection();
}

// --- Private Initialization Helpers ---

void SoundAnalyzerBase::InitM5()
{
#if USE_M5
    debugI("Audio: Initializing M5Stack Microphone");
    // Can't use speaker and mic at the same time, and speaker defaults on, so turn it off
    M5.Speaker.setVolume(255);
    M5.Speaker.end();
    auto cfg = M5.Mic.config();
    cfg.sample_rate = SAMPLING_FREQUENCY;
    cfg.noise_filter_level = 0;
    cfg.magnification = 8;
    M5.Mic.config(cfg);
    M5.Mic.begin();
#endif
}

void SoundAnalyzerBase::InitI2S_Modern()
{
#if (USE_I2S_AUDIO || ELECROW) && IS_IDF5
    const auto inputPin = GetConfiguredAudioInputPin();
    debugI("Audio: Initializing I2S Digital Mic (Modern) on BCLK:%d WS:%d DIN:%d", I2S_BCLK_PIN, I2S_WS_PIN, inputPin);
    // Digital Microphones (INMP441, etc.) - Standard I2S Mode
    i2s_chan_config_t chan_cfg = I2S_CHANNEL_DEFAULT_CONFIG(I2S_NUM_AUTO, I2S_ROLE_MASTER);
    ESP_ERROR_CHECK(i2s_new_channel(&chan_cfg, NULL, &_rx_handle));

    i2s_std_config_t std_cfg = {
        .clk_cfg = I2S_STD_CLK_DEFAULT_CONFIG(SAMPLING_FREQUENCY),
        .slot_cfg = I2S_STD_PHILIPS_SLOT_DEFAULT_CONFIG(I2S_DATA_BIT_WIDTH_32BIT, I2S_SLOT_MODE_STEREO),
        .gpio_cfg = {
            .mclk = I2S_GPIO_UNUSED,
            .bclk = I2S_BCLK_PIN,
            .ws = I2S_WS_PIN,
            .dout = I2S_GPIO_UNUSED,
            .din = static_cast<gpio_num_t>(inputPin),
        },
    };

    ESP_ERROR_CHECK(i2s_channel_init_std_mode(_rx_handle, &std_cfg));
    ESP_ERROR_CHECK(i2s_channel_enable(_rx_handle));
#endif
}

void SoundAnalyzerBase::InitI2S_Legacy()
{
#if (USE_I2S_AUDIO || ELECROW) && !IS_IDF5
    const auto inputPin = GetConfiguredAudioInputPin();
    debugI("Audio: Initializing I2S Digital Mic (Legacy) on BCLK:%d WS:%d DIN:%d", I2S_BCLK_PIN, I2S_WS_PIN, inputPin);
    const i2s_config_t i2s_config = {.mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX),
                                     .sample_rate = SAMPLING_FREQUENCY,
                                     .bits_per_sample = I2S_BITS_PER_SAMPLE_32BIT,
                                     .channel_format = I2S_CHANNEL_FMT_RIGHT_LEFT,
                                     .communication_format = I2S_COMM_FORMAT_STAND_I2S,
                                     .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
                                     .dma_buf_count = 4,
                                     .dma_buf_len = (int)MAX_SAMPLES,
                                     .use_apll = false,
                                     .tx_desc_auto_clear = false,
                                     .fixed_mclk = 0};

    pinMode(I2S_BCLK_PIN, OUTPUT);
    pinMode(I2S_WS_PIN, OUTPUT);
    pinMode(inputPin, INPUT);

    const i2s_pin_config_t pin_config = {.bck_io_num = I2S_BCLK_PIN,
                                         .ws_io_num = I2S_WS_PIN,
                                         .data_out_num = I2S_PIN_NO_CHANGE,
                                         .data_in_num = inputPin};

    ESP_ERROR_CHECK(i2s_driver_install(I2S_NUM_0, &i2s_config, 0, NULL));
    ESP_ERROR_CHECK(i2s_set_pin(I2S_NUM_0, &pin_config));
    ESP_ERROR_CHECK(i2s_zero_dma_buffer(I2S_NUM_0));
    ESP_ERROR_CHECK(i2s_start(I2S_NUM_0));
#endif
}

void SoundAnalyzerBase::InitADC_Modern()
{
#if !USE_M5 && !USE_I2S_AUDIO && IS_IDF5
    debugI("Audio: Initializing I2S ADC Analog Mic (Modern) on Channel 0");
    adc_continuous_handle_cfg_t adc_config = {
        .max_store_buf_size = 1024,
        .conv_frame_size = MAX_SAMPLES * sizeof(uint16_t),
    };
    ESP_ERROR_CHECK(adc_continuous_new_handle(&adc_config, &_adc_handle));

    adc_continuous_config_t dig_cfg = {
        .sample_freq_hz = SAMPLING_FREQUENCY,
        .conv_mode = ADC_CONV_SINGLE_UNIT_1, // Using ADC1
        .format = ADC_DIGI_OUTPUT_FORMAT_TYPE2,
    };

    // Configure pattern: channel, attenuation, etc.
    adc_digi_pattern_config_t adc_pattern[1] = {0};
    adc_pattern[0].atten = ADC_ATTEN_DB_12; // 12dB (formerly 11dB) for full range ~3.3V
    adc_pattern[0].channel = ADC_CHANNEL_0; // FIXED for now, ideally map from AUDIO_INPUT_PIN
    adc_pattern[0].unit = ADC_UNIT_1;
    adc_pattern[0].bit_width = ADC_BITWIDTH_12;

    dig_cfg.adc_pattern = adc_pattern;
    dig_cfg.pattern_num = 1;

    ESP_ERROR_CHECK(adc_continuous_config(_adc_handle, &dig_cfg));
    ESP_ERROR_CHECK(adc_continuous_start(_adc_handle));
#endif
}

void SoundAnalyzerBase::InitADC_Legacy()
{
#if !USE_M5 && !USE_I2S_AUDIO && !IS_IDF5 && defined(SOC_I2S_SUPPORTS_ADC)
    debugI("Audio: Initializing I2S ADC Analog Mic (Legacy) on Channel 0");
    static_assert(SOC_I2S_SUPPORTS_ADC, "This ESP32 model does not support ADC built-in mode");

    const i2s_config_t i2s_config = {
        .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX | I2S_MODE_ADC_BUILT_IN),
        .sample_rate = SAMPLING_FREQUENCY,
        .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,
        .channel_format = I2S_CHANNEL_FMT_ONLY_LEFT,
        .communication_format = I2S_COMM_FORMAT_STAND_I2S,
        .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
        .dma_buf_count = 2,
        .dma_buf_len = MAX_SAMPLES,
        .use_apll = false,
        .tx_desc_auto_clear = false,
        .fixed_mclk = 0
    };

    ESP_ERROR_CHECK(adc1_config_width(ADC_WIDTH_BIT_12));
    ESP_ERROR_CHECK(adc1_config_channel_atten(ADC1_CHANNEL_0, ADC_ATTEN_DB_0));
    ESP_ERROR_CHECK(i2s_driver_install(I2S_NUM_0, &i2s_config, 0, NULL));
    ESP_ERROR_CHECK(i2s_set_adc_mode(ADC_UNIT_1, ADC1_CHANNEL_0));
#endif
}

// --- Private Sampling Helpers ---

size_t SoundAnalyzerBase::SampleM5()
{
    size_t bytesRead = 0;
#if USE_M5
    constexpr auto bytesExpected = MAX_SAMPLES * sizeof(ptrSampleBuffer[0]);
    if (M5.Mic.record((int16_t *)ptrSampleBuffer.get(), MAX_SAMPLES, SAMPLING_FREQUENCY, false))
    {
        bytesRead = bytesExpected;
    }
#endif
    return bytesRead;
}

size_t SoundAnalyzerBase::SampleI2S_Modern()
{
    size_t bytesReadTotal = 0;
#if (USE_I2S_AUDIO || ELECROW) && IS_IDF5
    static int32_t tempBuffer[MAX_SAMPLES * 2];
    constexpr int kChannels = 2;
    size_t bytesToRead = MAX_SAMPLES * kChannels * sizeof(int32_t);
    size_t bytesRead = 0;

    esp_err_t err = i2s_channel_read(_rx_handle, (void *)tempBuffer, bytesToRead, &bytesRead, 100 / portTICK_PERIOD_MS);
    if (err != ESP_OK) return 0;

    for (int i = 0; i < MAX_SAMPLES; i++)
    {
        if (i * kChannels >= (bytesRead / 4)) break;
        int32_t s32 = tempBuffer[i * kChannels]; // Left channel
        ptrSampleBuffer[i] = (int16_t)std::clamp(s32 >> 15, -32768, 32767);
    }
    bytesReadTotal = bytesRead / kChannels / 2; // Rough approximation of output samples converted to bytes
#endif
    return bytesReadTotal;
}

size_t SoundAnalyzerBase::SampleI2S_Legacy()
{
    size_t bytesRead = 0;
#if (USE_I2S_AUDIO || ELECROW) && !IS_IDF5
    constexpr int kChannels = 2; // RIGHT + LEFT
    constexpr auto wordsToRead = MAX_SAMPLES * kChannels;
    constexpr auto bytesExpected32 = wordsToRead * sizeof(int32_t);
    static int32_t raw32[wordsToRead];

    ESP_ERROR_CHECK(i2s_read(I2S_NUM_0, (void *)raw32, bytesExpected32, &bytesRead, 100 / portTICK_PERIOD_MS));
    if (bytesRead != bytesExpected32)
    {
        debugW("Only read %u of %u bytes from I2S\n", bytesRead, bytesExpected32);
        return bytesRead;
    }

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

    for (int i = 0; i < MAX_SAMPLES; i++)
    {
        int32_t v = raw32[i * kChannels + s_chanIndex];
        int32_t scaled = (v >> 15);
        ptrSampleBuffer[i] = (int16_t)std::clamp(scaled, (int32_t)INT16_MIN, (int32_t)INT16_MAX);
    }
    bytesRead = MAX_SAMPLES * sizeof(int16_t); // effectively valid now
#endif
    return bytesRead;
}

size_t SoundAnalyzerBase::SampleADC_Modern()
{
    size_t ret_num = 0;
#if !USE_M5 && !USE_I2S_AUDIO && IS_IDF5
    constexpr size_t bytesToRead = MAX_SAMPLES * sizeof(uint16_t);
    esp_err_t err = adc_continuous_read(_adc_handle, (uint8_t*)ptrSampleBuffer.get(), bytesToRead, (uint32_t*)&ret_num, 0);

    if (err == ESP_OK)
    {
        for (int i = 0; i < MAX_SAMPLES; i++)
        {
            if (i * 2 >= ret_num) break;
            uint16_t val = ptrSampleBuffer[i];
            uint16_t data = val & 0xFFF; // Keep 12 bits
            ptrSampleBuffer[i] = (int16_t)((data - 2048) * 16);
        }
    }
#endif
    return ret_num;
}

size_t SoundAnalyzerBase::SampleADC_Legacy()
{
    size_t bytesRead = 0;
#if !USE_M5 && !USE_I2S_AUDIO && !IS_IDF5 && defined(SOC_I2S_SUPPORTS_ADC)
    constexpr auto bytesExpected16 = MAX_SAMPLES * sizeof(ptrSampleBuffer[0]);
    ESP_ERROR_CHECK(i2s_read(I2S_NUM_0, (void *)ptrSampleBuffer.get(), bytesExpected16, &bytesRead, 100 / portTICK_PERIOD_MS));
    if (bytesRead != bytesExpected16)
    {
        debugW("Could only read %u bytes of %u in FillBufferI2S()\n", bytesRead, bytesExpected16);
        return bytesRead;
    }
#endif
    return bytesRead;
}

// SoundAnalyzer<Params>
//
// Explicit implementations of template methods for SoundAnalyzer

// ProcessPeaksEnergy
//
// Processes the FFT results and extracts energy per frequency band, applying scaling and normalization.
template<const AudioInputParams& Params>
const PeakData & SoundAnalyzer<Params>::ProcessPeaksEnergy()
{
    PeakData rawSignals{};

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
    {
        dt = 0.016f;
    }

    for (int b = 0; b < NUM_BANDS; b++)
    {
        int start = _bandBinStart[b];
        int end = _bandBinEnd[b];
        if (end <= start)
        {
            _vPeaks[b] = 0.0f;
            rawSignals[b] = 0.0f;
            continue;
        }

        // Sum of squares using inner_product (hipster voodoo for power calculation)
        float sumPower = std::inner_product(_vReal.begin() + start, _vReal.begin() + end,
                                           _vReal.begin() + start, 0.0f);

        int widthBins = end - start;
        float avgPower = (widthBins > 0) ? (sumPower / (float)widthBins) : 0.0f;
        avgPower *= _params.windowPowerCorrection;

        // Apply aggressive logarithmic bass suppression with steeper initial curve
        float bandRatio = (float)b / (NUM_BANDS - 1);  // 0.0 to 1.0 across all bands

        // Two-stage suppression: moderate suppression on first few bands, then quick recovery
        float suppression;
        if (bandRatio < 0.1f)
        {  // First 10% of bands get moderate suppression
            // Quadratic suppression for the very first bands (band 0 gets maximum suppression)
            float localRatio = bandRatio / 0.1f;  // 0.0 to 1.0 over first 10% of spectrum
            suppression = 0.005f + (0.05f - 0.005f) * (localRatio * localRatio);  // 0.005 to 0.05 (99.5% to 95% attenuation)
        }
        else if (bandRatio < 0.4f)
        {  // Next 30% get moderate suppression with quick recovery
            // Exponential recovery from 0.05 to bandCompHigh over bands 10-40%
            float localRatio = (bandRatio - 0.1f) / 0.3f;  // 0.0 to 1.0 over 10-40% of spectrum
            suppression = 0.05f * expf(localRatio * logf(_params.bandCompHigh / 0.05f));
        }
        else
        {
            // Full gain for upper 60% of spectrum
            suppression = _params.bandCompHigh;
        }
        avgPower *= suppression;

        // Track pre-subtraction max for SNR gating
        if (avgPower > frameSumRaw)
        {
            frameSumRaw = avgPower;
        }

        if (avgPower > _noiseFloor[b])
        {
            _noiseFloor[b] = _noiseFloor[b] * (1.0f - _params.energyNoiseAdapt) + (float)avgPower * _params.energyNoiseAdapt;
        }
        else
        {
            _noiseFloor[b] *= _params.energyNoiseDecay;
        }

        // Accumulate noise stats
        noiseSum += _noiseFloor[b];
        noiseMaxAll = std::max(noiseMaxAll, _noiseFloor[b]);

        float signal = std::max(0.0f, (float)avgPower - _noiseFloor[b]);

        #if ENABLE_AUDIO_SMOOTHING
            // Weighted average: 0.25 * (2 * current + left + right)
            float left = (b > 0) ? _rawPrev[b - 1] : signal;
            float right = (b < NUM_BANDS - 1) ? _rawPrev[b + 1] : signal;
            float smoothed = 0.25f * (2.0f * signal + left + right);
            _rawPrev[b] = smoothed;
            rawSignals[b] = smoothed;
            if (smoothed > frameMax)
            {
                frameMax = smoothed;
            }
        #else
            rawSignals[b] = signal;
            if (signal > frameMax)
            {
                frameMax = signal;
            }
        #endif
    }

    // Manually clamp back the bands, as they are vastly over-represented

    if (frameMax > _energyMaxEnv)
    {
        _energyMaxEnv = frameMax;
    }
    else
    {
        _energyMaxEnv = std::max(_params.energyMinEnv, _energyMaxEnv * _params.energyEnvDecay);
    }

    // Raw SNR-based frame gate (pre-normalization) to suppress steady HVAC and similar backgrounfd noises

    float snrRaw = frameSumRaw / (noiseMaxAll + 1e-9f);
    if (snrRaw < _params.frameSNRGate)
    {
        _vPeaks.fill(0.0f);
        _Peaks.fill(0.0f);
        _beatPeaks.fill(0.0f);
        UpdateVU(0.0f);
        return _Peaks;
    }

    // Anchor normalization to noise-derived floor to avoid auto-gain blow-up
    float noiseMean = noiseSum / (float)NUM_BANDS;
    float envFloorRaw = noiseMean * _params.envFloorFromNoise; // raw (unclamped) env floor
    // Quiet environment gate: if the derived env floor is below a configured threshold, suppress the whole frame
    if (_params.quietEnvFloorGate > 0.0f && envFloorRaw < _params.quietEnvFloorGate)
    {
        _vPeaks.fill(0.0f);
        _Peaks.fill(0.0f);
        _beatPeaks.fill(0.0f);
        UpdateVU(0.0f);
        return _Peaks;
    }

    float envFloor = std::max(_params.energyMinEnv, envFloorRaw);
    float normDen = std::max(_energyMaxEnv, envFloor);
    const float invEnv = 1.0f / normDen;
    const float beatNoiseRef = std::max(noiseMean, _params.energyMinEnv);
    float sumNorm = 0.0f;

    // Now that layout skips the lowest bins, emit all NUM_BANDS directly with no reindexing
    for (int b = 0; b < NUM_BANDS; b++)
    {
        // Beat detection runs from the pre-display signal so it is not pushed
        // around by the same adaptive envelope and attack limiting used to make
        // the spectrum/VU visuals look good.
        const float beatReference = std::max({_noiseFloor[b], beatNoiseRef, _params.energyMinEnv});
        const float beatRatio = std::max(0.0f, rawSignals[b]) / (beatReference + 1e-9f);
        _beatPeaks[b] = std::clamp(sqrtf(beatRatio), 0.0f, 2.0f);

        float vTarget = std::clamp(powf(std::max(0.0f, rawSignals[b] * invEnv), _params.compressGamma), 0.0f, 1.0f);
        vTarget = std::clamp(vTarget * kDisplayGain, 0.0f, 1.0f);

        const float bandFloor = std::max(kBandFloorMin * 1.0f, kBandFloorScale); // Fixed kBandFloorMin usage
        if (vTarget < bandFloor)
        {
            vTarget = 0.0f;
        }
        float vCurr = _livePeaks[b];
        float vNew = vTarget;
        if (vTarget > vCurr)
        {
            const float maxRise = kLiveAttackPerSec * dt;
            vNew = std::min(vTarget, vCurr + maxRise);
        }
        vNew = std::clamp(vNew, 0.0f, 1.0f);
        _livePeaks[b] = vNew;
        _vPeaks[b] = vNew;
        _Peaks[b] = vNew;
        sumNorm += vNew;
    }
    UpdateVU(sumNorm / (float)NUM_BANDS);
    return _Peaks;
}

// Explicit implementations for all standard AudioInputParams configurations
template class SoundAnalyzer<kParamsMesmerizer>;
template class SoundAnalyzer<kParamsM5>;
template class SoundAnalyzer<kParamsM5Plus2>;
template class SoundAnalyzer<kParamsI2SExternal>;

#endif

ProjectSoundAnalyzer g_Analyzer;
