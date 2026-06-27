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
#include "values.h"

#if ENABLE_AUDIO

#include <arduinoFFT.h>

#if USE_M5
    #include <M5Unified.h>
#endif

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
#if IS_IDF5
    // Stop the hardware first to kill any active DMA transfers
    if (_rx_handle)
    {
        i2s_channel_disable(_rx_handle);
        i2s_del_channel(_rx_handle);
    }
    if (_adc_handle)
    {
        adc_continuous_stop(_adc_handle);
        adc_continuous_deinit(_adc_handle);
    }
#else
    // Legacy cleanup - i2s_stop terminates DMA
    i2s_stop(I2S_NUM_0);
    i2s_driver_uninstall(I2S_NUM_0);
#endif
}

// Reset
//
// Reset and clear the FFT buffers
void SoundAnalyzerBase::Reset()
{
    _vReal.fill(0.0f);
    _vImaginary.fill(0.0f);
    _vPeaks.fill(0.0f);
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

#if INPUT_PIN < 0
    return;
#endif

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

    // Calculate DC offset (mean) of the current frame to prevent windowing artifacts
    float mean = 0.0f;
    for (int i = 0; i < MAX_SAMPLES; i++) {
        mean += ptrSampleBuffer[i];
    }
    mean /= MAX_SAMPLES;

    // Use std::transform for better code gen (SIMD) when copying/casting samples to FFT input
    // Subtract the mean to remove DC offset before windowing
    std::transform(ptrSampleBuffer.get(), ptrSampleBuffer.get() + MAX_SAMPLES, _vReal.begin(),
                   [mean](int16_t s) { return static_cast<float>(s) - mean; });
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
void SoundAnalyzerBase::InitAudioInput()
{
#if INPUT_PIN < 0
    debugI("Audio: INPUT_PIN < 0, skipping hardware initialization. SimBeat only.");
    return;
#endif

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

// SetPeakDataFromRemote
//
// Accept externally provided peaks (e.g., over WiFi) and update internal state.
// Also recomputes VU from the new band values and records source time.
void SoundAnalyzerBase::SetPeakDataFromRemote(const PeakData &peaks)
{
    _msLastRemoteAudio = millis();
    _Peaks = peaks;
    _vPeaks = _Peaks;
    float sum = std::accumulate(_vPeaks.begin(), _vPeaks.end(), 0.0f);
    UpdateVU(sum / (float)NUM_BANDS);
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
    UpdateVU(targetVU);
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
        Reset();
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
    debugI("Audio: Initializing I2S Digital Mic (Modern) on BCLK:%d WS:%d DIN:%d", I2S_BCLK_PIN, I2S_WS_PIN, INPUT_PIN);
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
            .din = INPUT_PIN,
        },
    };

    ESP_ERROR_CHECK(i2s_channel_init_std_mode(_rx_handle, &std_cfg));
    ESP_ERROR_CHECK(i2s_channel_enable(_rx_handle));
#endif
}

void SoundAnalyzerBase::InitI2S_Legacy()
{
#if (USE_I2S_AUDIO || ELECROW) && !IS_IDF5
    debugI("Audio: Initializing I2S Digital Mic (Legacy) on BCLK:%d WS:%d DIN:%d", I2S_BCLK_PIN, I2S_WS_PIN, INPUT_PIN);
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
    pinMode(INPUT_PIN, INPUT);

    const i2s_pin_config_t pin_config = {.bck_io_num = I2S_BCLK_PIN,
                                         .ws_io_num = I2S_WS_PIN,
                                         .data_out_num = I2S_PIN_NO_CHANGE,
                                         .data_in_num = INPUT_PIN};

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
    adc_pattern[0].channel = ADC_CHANNEL_0; // FIXED for now, ideally map from INPUT_PIN
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
    // Use 100ms timeout instead of 0 to wait for a full buffer
    esp_err_t err = adc_continuous_read(_adc_handle, (uint8_t*)ptrSampleBuffer.get(), bytesToRead, (uint32_t*)&ret_num, 100);

    // Process only if we got a full buffer
    if (ret_num == bytesToRead)
    {
        for (int i = 0; i < MAX_SAMPLES; i++)
        {
            uint16_t val = ptrSampleBuffer[i];
            uint16_t data = val & 0xFFF; // Keep 12 bits
            // Do NOT scale by 16. Legacy ADC returned unscaled 12-bit values (0-4095).
            // Scaling by 16 increases energy by 256x, which defeats the quietEnvFloorGate!
            ptrSampleBuffer[i] = (int16_t)(data - 2048);
        }
    }
    else
    {
        // If we timed out or got partial data, discard it to prevent FFT discontinuities
        ret_num = 0;
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
            _vPeaks[b] = smoothed;
            if (smoothed > frameMax)
            {
                frameMax = smoothed;
            }
        #else
            _vPeaks[b] = signal;
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
        float vTarget = std::clamp(powf(std::max(0.0f, _vPeaks[b] * invEnv), _params.compressGamma), 0.0f, 1.0f);
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
