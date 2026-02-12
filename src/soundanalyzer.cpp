//+--------------------------------------------------------------------------
//
// File:        soundanalyzer.cpp
//
// NightDriverStrip - (c) 2018 Plummer's Software LLC.  All Rights Reserved.
//
// Description:
//
//   Analyzes the audio input
//
// History:     Sep-15-2020     Davepl      Created
//
//---------------------------------------------------------------------------

#include "soundanalyzer.h"
#include "globals.h"

#if ENABLE_AUDIO

#include <numeric>

// Explicit implementations of template methods for SoundAnalyzer

// SoundAnalyzer
//
// Construct analyzer, allocate buffers (PSRAM-preferred), set initial state.
// Throws std::runtime_error on allocation failure. Computes band layout once.
template<const AudioInputParams& Params>
SoundAnalyzer<Params>::SoundAnalyzer()
{
    ptrSampleBuffer.reset((int16_t *)heap_caps_malloc(MAX_SAMPLES * sizeof(int16_t), MALLOC_CAP_8BIT));
    if (!ptrSampleBuffer)
        throw std::runtime_error("Failed to allocate sample buffer");
    _oldVU = _oldPeakVU = _oldMinVU = 0.0f;
    ComputeBandLayout();
    Reset();
}

// ~SoundAnalyzer
//
// Free any heap/PSRAM buffers allocated by the constructor.
// Safe to call at shutdown/reset.
template<const AudioInputParams& Params>
SoundAnalyzer<Params>::~SoundAnalyzer()
{
    // Stop I2S if it was started
    #if !USE_M5 && (ELECROW || USE_I2S_AUDIO || !defined(USE_I2S_AUDIO))
        i2s_stop(I2S_NUM_0);
        i2s_driver_uninstall(I2S_NUM_0);
    #endif
}

// Reset
//
// Reset and clear the FFT buffers
template<const AudioInputParams& Params>
void SoundAnalyzer<Params>::Reset()
{
    _vReal.fill(0.0f);
    _vImaginary.fill(0.0f);
    _vPeaks.fill(0.0f);
}

// FFT
//
// Perform the FFT
template<const AudioInputParams& Params>
void SoundAnalyzer<Params>::FFT()
{
    ArduinoFFT<float> _FFT(_vReal.data(), _vImaginary.data(), MAX_SAMPLES, SAMPLING_FREQUENCY);
    _FFT.dcRemoval();
    _FFT.windowing(FFTWindow::Hann, FFTDirection::Forward);
    _FFT.compute(FFTDirection::Forward);
    _FFT.complexToMagnitude();
}

// SampleAudio
//
// Sample the audio
template<const AudioInputParams& Params>
void SoundAnalyzer<Params>::SampleAudio()
{
    size_t bytesRead = 0;
#if INPUT_PIN < 0
    return;
#endif
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
            ESP_ERROR_CHECK(i2s_read(I2S_NUM_0, (void *)ptrSampleBuffer.get(), bytesExpected16, &bytesRead, 100 / portTICK_PERIOD_MS));
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

// UpdateVU
//
// Update the VU and peak values based on the new sample
template<const AudioInputParams& Params>
void SoundAnalyzer<Params>::UpdateVU(float newval)
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

// ComputeBandLayout
//
// Compute the band layout based on the sampling frequency and number of bands
//
// This computes the start and end bins for each band based on the sampling frequency,
// ensuring that the bands are spaced logarithmically or in Mel scale as configured.
// The results are stored in _bandBinStart and _bandBinEnd arrays.
template<const AudioInputParams& Params>
void SoundAnalyzer<Params>::ComputeBandLayout()
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

    // Manually clamp back the bands, as they are vastly over-represented

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
        const float bandFloor = std::max(kBandFloorMin * 1.0f, kBandFloorScale); // Fixed kBandFloorMin usage
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

// SampleBufferInitI2S
//
// Configure and start the I2S (or M5) input at SAMPLING_FREQUENCY.
// Board-specific branches set pins and ADC/I2S modes as needed.
template<const AudioInputParams& Params>
void SoundAnalyzer<Params>::SampleBufferInitI2S()
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

#if defined(SOC_I2S_SUPPORTS_ADC)
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

#endif

    debugV("SamplerBufferInitI2S Complete\n");
}

// DecayPeaks
//
// Apply time-based decay to the two peak overlay arrays.
// Called once per frame; uses AppTime.LastFrameTime() and configurable rates.
template<const AudioInputParams& Params>
void SoundAnalyzer<Params>::DecayPeaks()
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
}

// UpdatePeakData
//
// Update the per-band decay overlays from the latest peaks.
// Rises are limited by VU_REACTIVITY_RATIO; records timestamps on new primary peaks.
template<const AudioInputParams& Params>
void SoundAnalyzer<Params>::UpdatePeakData()
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

// SetPeakDecayRates
//
// Configure how quickly the two decay overlays drop over time.
// r1 = fast track, r2 = slow track; higher = faster decay.
template<const AudioInputParams& Params>
void SoundAnalyzer<Params>::SetPeakDecayRates(float r1, float r2)
{
    _peak1DecayRate = r1;
    _peak2DecayRate = r2;
}

// SetPeakDataFromRemote
//
// Accept externally provided peaks (e.g., over WiFi) and update internal state.
// Also recomputes VU from the new band values and records source time.
template<const AudioInputParams& Params>
void SoundAnalyzer<Params>::SetPeakDataFromRemote(const PeakData &peaks)
{
    _msLastRemoteAudio = millis();
    _Peaks = peaks;
    _vPeaks = _Peaks;
    float sum = std::accumulate(_vPeaks.begin(), _vPeaks.end(), 0.0f);
    UpdateVU(sum / NUM_BANDS);
}

#if ENABLE_AUDIO_DEBUG
// DumpBandLayout
//
// Print per-band [start-end] bin ranges over Serial for debugging.
// Useful to verify spacing and coverage with current config.
template<const AudioInputParams& Params>
void SoundAnalyzer<Params>::DumpBandLayout() const
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

// RunSamplerPass
//
// Perform one audio acquisition/processing step.
// Uses local mic if no recent remote peaks; otherwise trusts remote and only updates VU.
template<const AudioInputParams& Params>
void SoundAnalyzer<Params>::RunSamplerPass()
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

// Explicit implementations for all standard AudioInputParams configurations
template class SoundAnalyzer<kParamsMesmerizer>;
template class SoundAnalyzer<kParamsM5>;
template class SoundAnalyzer<kParamsM5Plus2>;
template class SoundAnalyzer<kParamsI2SExternal>;

#endif
