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
#include <arduinoFFT.h>
#include <driver/adc.h>
#include <driver/i2s.h>
#include <memory>

#if !ENABLE_AUDIO
#ifndef NUM_BANDS
#define NUM_BANDS 16 // Default value for projects without audio
#endif
#endif

/*  SPECTRUM_BAND_SCALE_MEL switches the band layout from logarithmic Hz spacing to Mel-scale 
    spacing in ComputeBandLayout().

    What it does:

    Off (0): bands are spaced geometrically between fMin and fMax (classic log-eq style).
    On (1): bands are evenly spaced in Mel units (hz→mel→hz), approximating human pitch perception. 
    This allocates more bands to lows/mids and fewer to the extreme highs.
    When to enable:

    If you want perceptually uniform bands (speech/vocals, general music listening).
    If lows/mids should have finer resolution and highs can be coarser.
    If you find the current log spacing still feels “bass heavy” or not human-centric.
    When to leave off:

    If you need more technical/log-frequency uniformity (e.g., octave-like spacing).
    If you rely on the current band distribution or high-frequency detail.
*/

#ifndef SPECTRUM_BAND_SCALE_MEL
#define SPECTRUM_BAND_SCALE_MEL 0
#endif

static constexpr float WINDOW_POWER_CORRECTION  = 4.0f;     // Hann window power correction
static constexpr float ENERGY_NOISE_ADAPT       = 0.02f;
static constexpr float ENERGY_NOISE_DECAY       = 0.98f;
static constexpr float ENERGY_SMOOTH_ALPHA      = 0.25f;
static constexpr float ENERGY_ENV_DECAY         = 0.95f;
static constexpr float ENERGY_MIN_ENV           = 0.000001f;
static constexpr float BAND_COMPENSATION_LOW    = 0.25f;    // Compensation for bass frequencies
static constexpr float BAND_COMPENSATION_HIGH   = 4.5f;     // Compensation for treble frequencies
static constexpr float FRAME_SILENCE_GATE       = 0.05f;    // Zero whole frame if max normalized energy is below this
static constexpr float NORM_NOISE_GATE          = 0.01f;    // Zero individual bands below this after normalization

#ifndef MAX_SAMPLES
#define MAX_SAMPLES 256
#endif

#define MS_PER_SECOND 1000

#if !ENABLE_AUDIO
#ifndef NUM_BANDS
#define NUM_BANDS 1
#endif
#endif

// PeakData
//
// Simple data class that holds the music peaks for up to NUM_BANDS bands. Moved above
// the ENABLE_AUDIO conditional so both stub and full implementations can expose
// a consistent interface returning PeakData.

#ifndef MIN_VU
#define MIN_VU 0.05f
#endif
#ifndef GAINDAMPEN
#define GAINDAMPEN 10
#endif
#ifndef VUDAMPEN
#define VUDAMPEN 0
#endif
#define VUDAMPENMIN 1
#define VUDAMPENMAX 1

class PeakData
{
  public:
    double _Level[NUM_BANDS];

    typedef enum
    {
        MESMERIZERMIC,
        PCREMOTE,
        M5,
        M5PLUS2
    } MicrophoneType;

    PeakData()
    {
        for (auto &i : _Level)
            i = 0.0f;
    }
    explicit PeakData(double *pDoubles)
    {
        SetData(pDoubles);
    }
    PeakData &operator=(const PeakData &other)
    {
        if (this == &other)
            return *this;
        for (int i = 0; i < NUM_BANDS; i++)
            _Level[i] = other._Level[i];
        return *this;
    }
    float operator[](std::size_t n) const { return _Level[n]; }
    void SetData(const double *pDoubles)
    {
        for (int i = 0; i < NUM_BANDS; i++)
            _Level[i] = pDoubles[i];
    }
};

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

    // Former AudioVariables (now private)
    float _VURatio = 1.0f;
    float _VURatioFade = 1.0f;
    float _VU = 0.0f;
    float _PeakVU = 0.0f;
    float _MinVU = 0.0f;
    unsigned long _cSamples = 0U;
    int _AudioFPS = 0;
    int _serialFPS = 0;
    uint _msLastRemote = 0;

    // I'm old enough I can only hear up to about 12K, but feel free to adjust.  Remember from
    // school that you need to sample at double the frequency you want to process, so 24000 is 12K

    static constexpr size_t SAMPLING_FREQUENCY = 20000;
    static constexpr size_t LOWEST_FREQ = 100;
    static constexpr size_t HIGHEST_FREQ = SAMPLING_FREQUENCY / 2;

    static inline size_t SamplingPeriodMicros()
    {
        return (size_t)PERIOD_FROM_FREQ(SAMPLING_FREQUENCY);
    }

    float _oldVU;     // Old VU value for damping
    float _oldPeakVU; // Old peak VU value for damping
    float _oldMinVU;  // Old min VU value for damping
    double *_vPeaks;  // Normalized band energies 0..1
    int _bandBinStart[NUM_BANDS];
    int _bandBinEnd[NUM_BANDS];
    float _energyMaxEnv = 1.0f;         // adaptive envelope for autoscaling
    float _noiseFloor[NUM_BANDS] = {0}; // adaptive per-band noise floor
    float _rawPrev[NUM_BANDS] = {0};    // previous raw (noise-subtracted) power for smoothing

    // Peak decay internals (private)
    unsigned long _lastPeak1Time[NUM_BANDS] = {0};
    float _peak1Decay[NUM_BANDS] = {0};
    float _peak2Decay[NUM_BANDS] = {0};
    float _peak1DecayRate = 1.25f;
    float _peak2DecayRate = 1.25f;

    PeakData::MicrophoneType _MicMode;

    // Energy spectrum processing (implemented inline below)
    //
    // Calculate a logarithmic scale for the bands like you would find on a graphic equalizer display
    //

  private:
    double *_vReal = nullptr;
    double *_vImaginary = nullptr;
    std::unique_ptr<int16_t[]> ptrSampleBuffer; // sample buffer storage
    PeakData _Peaks; // cached last normalized peaks (moved earlier for inline method visibility)

    void Reset()
    {
        if (!_vReal)
            return;
        for (int i = 0; i < MAX_SAMPLES; i++)
        {
            _vReal[i] = 0.0;
            if (_vImaginary)
                _vImaginary[i] = 0.0;
        }
        for (int i = 0; i < NUM_BANDS; i++)
            _vPeaks[i] = 0.0f;
    }
    void FFT()
    {
        ArduinoFFT<double> _FFT(_vReal, _vImaginary, MAX_SAMPLES, SAMPLING_FREQUENCY);
        _FFT.dcRemoval();
        _FFT.windowing(FFTWindow::Hann, FFTDirection::Forward);
        _FFT.compute(FFTDirection::Forward);
        _FFT.complexToMagnitude();
    }
    void FillBufferI2S()
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
        for (int i = 0; i < MAX_SAMPLES; i++)
            _vReal[i] = ptrSampleBuffer[i];
    }
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
    void ComputeBandLayout()
    {
        const float fMin = 50.0f;
        const float fMax = std::min<float>(12000.0f, SAMPLING_FREQUENCY / 2.0f);
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
            float fracHi = (float)(b + 1) / (float)NUM_BANDS;
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
        float frameMax = 0.0f;
        for (int b = 0; b < NUM_BANDS; b++)
        {
            int start = _bandBinStart[b];
            int end = _bandBinEnd[b];
            if (end <= start)
            {
                _vPeaks[b] = 0.0f;
                continue;
            }
            double sumPower = 0.0;
            for (int k = start; k < end; k++)
            {
                double mag = _vReal[k];
                sumPower += mag * mag;
            }
            int widthBins = end - start;
            double avgPower = (widthBins > 0) ? (sumPower / widthBins) : 0.0;
            avgPower *= WINDOW_POWER_CORRECTION;

            // Apply frequency-dependent compensation
            float compensation = BAND_COMPENSATION_LOW + (BAND_COMPENSATION_HIGH - BAND_COMPENSATION_LOW) * ((float)b / (NUM_BANDS - 1));
            avgPower *= compensation;

            if (avgPower > _noiseFloor[b])
                _noiseFloor[b] = _noiseFloor[b] * (1.0f - ENERGY_NOISE_ADAPT) + (float)avgPower * ENERGY_NOISE_ADAPT;
            else
                _noiseFloor[b] *= ENERGY_NOISE_DECAY;
            float signal = (float)avgPower - _noiseFloor[b];
            if (signal < 0.0f)
                signal = 0.0f;
            float smoothed = _rawPrev[b] * (1.0f - ENERGY_SMOOTH_ALPHA) + signal * ENERGY_SMOOTH_ALPHA;
            _rawPrev[b] = smoothed;
            _vPeaks[b] = smoothed;
            if (smoothed > frameMax)
                frameMax = smoothed;
        }
        if (frameMax > _energyMaxEnv)
            _energyMaxEnv = frameMax;
        else
            _energyMaxEnv = std::max(ENERGY_MIN_ENV, _energyMaxEnv * ENERGY_ENV_DECAY);

        float invEnv = 1.0f / _energyMaxEnv;

        // Frame-level silence gate on normalized values
        float vMaxNorm = 0.0f;
        for (int b = 0; b < NUM_BANDS; ++b)
        {
            float vNorm = _vPeaks[b] * invEnv;
            if (vNorm > vMaxNorm)
                vMaxNorm = vNorm;
        }
        if (vMaxNorm < FRAME_SILENCE_GATE)
        {
            for (int b = 0; b < NUM_BANDS; ++b)
            {
                _vPeaks[b] = 0.0f;
                _Peaks._Level[b] = 0.0f;
            }
            UpdateVU(0.0f);
            return _Peaks;
        }

        double sumNorm = 0.0;
        for (int b = 0; b < NUM_BANDS; b++)
        {
            float v = _vPeaks[b] * invEnv;

            // Per-band normalized noise gate
            if (v < NORM_NOISE_GATE)
                v = 0.0f;
            else
                v = std::sqrt(std::sqrt(std::max(0.0f, v))); // compression above the gate

            if (v > 1.0f)
                v = 1.0f;
            _vPeaks[b] = v;
            _Peaks._Level[b] = v;
            sumNorm += v;
        }
        UpdateVU((float)(sumNorm / NUM_BANDS));
        return _Peaks;
    }

  public:
    inline float VURatio() const override
    {
        return _VURatio;
    }
    inline float VURatioFade() const override
    {
        return _VURatioFade;
    }
    inline float VU() const override
    {
        return _VU;
    }
    inline float PeakVU() const override
    {
        return _PeakVU;
    }
    inline float MinVU() const override
    {
        return _MinVU;
    }
    inline int AudioFPS() const override
    {
        return _AudioFPS;
    }
    inline int SerialFPS() const override
    {
        return _serialFPS;
    }
    inline PeakData::MicrophoneType MicMode() const override
    {
        return _MicMode;
    }
    inline const PeakData &Peaks() const override
    {
        return _Peaks;
    }
    inline float Peak2Decay(int band) const override
    {
        return (band >= 0 && band < NUM_BANDS) ? _peak2Decay[band] : 0.0f;
    }
    inline const float *Peak2DecayData() const override
    {
        return _peak2Decay;
    }
    inline const float *Peak1DecayData() const override
    {
        return _peak1Decay;
    }
    inline float Peak1Decay(int band) const override
    {
        return (band >= 0 && band < NUM_BANDS) ? _peak1Decay[band] : 0.0f;
    }
    inline const unsigned long *Peak1Times() const override
    {
        return _lastPeak1Time;
    }
    inline unsigned long LastPeak1Time(int band) const override
    {
        return (band >= 0 && band < NUM_BANDS) ? _lastPeak1Time[band] : 0;
    }
    inline void SetPeakDecayRates(float r1, float r2) override
    {
        _peak1DecayRate = r1;
        _peak2DecayRate = r2;
    }

    SoundAnalyzer()
    {
        ptrSampleBuffer.reset((int16_t *)heap_caps_malloc(MAX_SAMPLES * sizeof(int16_t), MALLOC_CAP_8BIT));
        if (!ptrSampleBuffer)
            throw std::runtime_error("Failed to allocate sample buffer");
        _vReal = (double *)PreferPSRAMAlloc(MAX_SAMPLES * sizeof(_vReal[0]));
        _vImaginary = (double *)PreferPSRAMAlloc(MAX_SAMPLES * sizeof(_vImaginary[0]));
        _vPeaks = (double *)PreferPSRAMAlloc(NUM_BANDS * sizeof(_vPeaks[0]));
        _oldVU = _oldPeakVU = _oldMinVU = 0.0f;
        ComputeBandLayout();
        Reset();
    }

    ~SoundAnalyzer()
    {
        free(_vReal);
        free(_vImaginary);
        free(_vPeaks);
    }

    // These functions allow access to the last-acquired sample buffer and its size so that
    // effects can draw the waveform or do other things with the raw audio data

    const int16_t *GetSampleBuffer() const
    {
        return ptrSampleBuffer.get();
    }

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

    float BeatEnhance(float amt)
    {
        return ((1.0 - amt) + (_VURatioFade / 2.0) * amt);
    }

    // flash record size, for recording 5 second
    void SampleBufferInitI2S()
    {
        // install and start i2s driver

        debugV("Begin SamplerBufferInitI2S...");

#if USE_M5

        // Can't use speaker and mic at the same time, and speaker defaults on, so turn it off

        M5.Speaker.setVolume(255);
        M5.Speaker.end();
        M5.Mic.begin();

#elif ELECROW || USE_I2S_AUDIO

        const i2s_config_t i2s_config = {.mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX),
                                         .sample_rate = SAMPLING_FREQUENCY,
                                         .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,
                                         .channel_format = I2S_CHANNEL_FMT_ONLY_LEFT,
                                         .communication_format = I2S_COMM_FORMAT_STAND_I2S,
                                         .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
                                         .dma_buf_count = 2,
                                         .dma_buf_len = (int)MAX_SAMPLES,
                                         .use_apll = false};

        // i2s pin configuration
        const i2s_pin_config_t pin_config = {.bck_io_num = I2S_BCLK_PIN,
                                             .ws_io_num = I2S_WS_PIN,
                                             .data_out_num = -1, // not used
                                             .data_in_num = INPUT_PIN};

        ESP_ERROR_CHECK(i2s_driver_install(I2S_NUM_0, &i2s_config, 0, NULL));
        ESP_ERROR_CHECK(i2s_set_pin(I2S_NUM_0, &pin_config));
        ESP_ERROR_CHECK(i2s_start(I2S_NUM_0));

#else

        // This block is  TTGO, MESMERIZER, SPECTRUM_WROVER_KIT and other I2S.
        //
        i2s_config_t i2s_config;
        i2s_config.mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX);
// Note: Post IDF4, I2S_MODE_ADC_BUILT_IN is no longer supported
// and it was never available on models after original ESP32-Nothing.
// See: https://github.com/espressif/arduino-esp32/issues/9564
#if defined(I2S_MODE_ADC_BUILT_IN)
        i2s_config.mode |= I2S_MODE_ADC_BUILT_IN;
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

    inline void DecayPeaks()
    {
        float decayAmount1 = std::max(0.0, g_Values.AppTime.LastFrameTime() * _peak1DecayRate);
        float decayAmount2 = std::max(0.0, g_Values.AppTime.LastFrameTime() * _peak2DecayRate);

        for (int iBand = 0; iBand < NUM_BANDS; iBand++)
        {
            _peak1Decay[iBand] -= min(decayAmount1, _peak1Decay[iBand]);
            _peak2Decay[iBand] -= min(decayAmount2, _peak2Decay[iBand]);
        }

        // Manual smoothing if desired

#if ENABLE_AUDIO_SMOOTHING
        for (int iBand = 1; iBand < NUM_BANDS - 1; iBand += 2)
        {
            _peak1Decay[iBand] = (_peak1Decay[iBand] * 2 + _peak1Decay[iBand - 1] + _peak1Decay[iBand + 1]) / 4;
            _peak2Decay[iBand] = (_peak2Decay[iBand] * 2 + _peak2Decay[iBand - 1] + _peak2Decay[iBand + 1]) / 4;
        }
#endif
    }

    // Update the local band peaks from the global sound data.  If we establish a new peak in any band,
    // we reset the peak timestamp on that band

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

    inline void SetPeakData(const PeakData &peaks)
    {
        _msLastRemote = millis();
        _Peaks = peaks;
        for (int i = 0; i < NUM_BANDS; i++)
            _vPeaks[i] = _Peaks._Level[i];
        float sum = 0.0f;
        for (int i = 0; i < NUM_BANDS; i++)
            sum += _vPeaks[i];
        UpdateVU(sum / NUM_BANDS);
    }
    inline const int *BandBinStarts() const
    {
        return _bandBinStart;
    }
    inline const int *BandBinEnds() const
    {
        return _bandBinEnd;
    }
#if ENABLE_AUDIO_DEBUG
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
            Reset();
            FillBufferI2S();
            FFT();
            ProcessPeaksEnergy();
        }
        else
        {
            float sum = 0.0f;
            for (int i = 0; i < NUM_BANDS; i++)
                sum += _Peaks._Level[i];
            _MicMode = PeakData::PCREMOTE;
            UpdateVU(sum / NUM_BANDS);
        }
    }
};
#endif

extern SoundAnalyzer g_Analyzer;
