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

#include <ArduinoFFT.h>
#include <driver/i2s.h>
#include <driver/adc.h>

#define MS_PER_SECOND 1000

// These are the audio variables that are referenced by many audio effects.  In order to allow non-audio code to reference them too without
// including all the audio code (such as logging code, etc.), we put the publicly exposed variables into a structure, and then the SoundAnalyzer
// class will inherit them.
//
// In the non-audio case, there's a stub class that includes ONLY the audio variables and none of the code or buffers.
//
// In both cases, the AudioVariables are accessible as g_Analyzer.  It'll just be a stub in the non-audio case

struct AudioVariables
{
    float _VURatio          = 1.0;          // Current VU as a ratio to its recent min and max
    float _VURatioFade      = 1.0;          // Same as gVURatio but with a slow decay
    float _VU               = 0.0;          // Instantaneous read of VU value
    float _PeakVU           = 0.0;          // How high our peak VU scale is in live mode
    float _MinVU            = 0.0;          // How low our peak VU scale is in live mode
    unsigned long _cSamples = 0U;           // Total number of samples successfully collected
    int _AudioFPS           = 0;            // Framerate of the audio sampler
    int _serialFPS          = 0;            // How many serial packets are processed per second
    uint _msLastRemote      = 0;            // When the last Peak data came in from external (ie: WiFi)
};

#if !ENABLE_AUDIO
class SoundAnalyzer : public AudioVariables // Non-audio case.  Inherits only the AudioVariables so that any project can
{                                           //   reference them in g_Analyzer
};

#else // Audio case

void IRAM_ATTR AudioSamplerTaskEntry(void *);
void IRAM_ATTR AudioSerialTaskEntry(void *);

// PeakData class
//
// Simple data class that holds the music peaks for up to 32 bands.  When the sound analyzer finishes a pass, its
// results are simplified down to this small class of band peaks.

#ifndef MIN_VU
#define MIN_VU 2                // Minimum VU value to use for the span when computing VURatio.  Contributes to
#endif                          // how dynamic the music is (smaller values == more dynamic)


#ifndef GAINDAMPEN
    #define GAINDAMPEN 10      // How slowly brackets narrow in for spectrum bands
#endif

#ifndef VUDAMPEN
    #define VUDAMPEN 0 // How slowly VU reacts
#endif

#define VUDAMPENMIN 1 // How slowly VU min creeps up to test noise floor
#define VUDAMPENMAX 1 // How slowly VU max drops down to test noise ceiling

// PeakData
//
// Keeps track of a set of peaks for a sample pass

class PeakData
{

public:

    double _Level[NUM_BANDS];

public:
    typedef enum
    {
        MESMERIZERMIC,
        PCREMOTE,
        M5,
        M5PLUS2
    } MicrophoneType;

    PeakData()
    {
        // BUGBUG (davepl) consider std::fill
        for (auto & i: _Level)
            i = 0.0f;
        // RAIDRAID(robertl): Isn't that just
        // std::fill(_Level.begin(), _Level.end(), 0.0f);
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

    float operator[](std::size_t n) const
    {
        return _Level[n];
    }

    static float GetBandScalar(MicrophoneType mic, int i)
    {
      switch (mic)
      {
        case MESMERIZERMIC:
        {
            static constexpr std::array<float, 16> Scalars16  = {0.4, .5, 0.75, 1.0, 0.6, 0.6, 0.8, 0.8, 1.2, 1.5, 3.0, 3.0, 3.0, 3.0, 3.5, 2.5}; //  {0.08, 0.12, 0.3, 0.35, 0.35, 0.5, 0.6, 0.7, 0.8, 0.9, 1.0, 1.4, 1.4, 1.0, 1.0, 1.0};
            float result = (NUM_BANDS == 16) ? Scalars16[i] : 1.0;
            return result;
        }
        case PCREMOTE:
        {

            static constexpr std::array<float, 16> Scalars16  = {1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0};
            float result = (NUM_BANDS == 16) ? Scalars16[i] : 1.0;
            return result;
        }
        case M5PLUS2:
        {
            static constexpr std::array<float, 16> Scalars16  = {0.5, 1.0, 2.5, 2.2, 1.5, 2.0, 2.0, 2.0, 1.5, 1.5, 1.5, 1.5, 1.0, 0.8, 1.0, 1.0}; 
            float result = (NUM_BANDS == 16) ? Scalars16[i] : 1.0;
            return result;
        }
        default:
        {
            static constexpr std::array<float, 16> Scalars16  = {0.5, .5, 0.8, 1.0, 1.5, 1.2, 1.5, 1.6, 2.0, 2.0, 2.0, 3.0, 3.0, 3.0, 5.0, 2.5}; 
            float result = (NUM_BANDS == 16) ? Scalars16[i] : 1.0;
            return result;
        }
      }
    }

    void ApplyScalars(MicrophoneType mic)
    {
        for (int i = 0; i < NUM_BANDS; i++)
            _Level[i] *= GetBandScalar(mic, i);
    }

    void SetData(const double * pDoubles)
    {
        for (int i = 0; i < NUM_BANDS; i++)
            _Level[i] = pDoubles[i];
    }
};

// SoundAnalyzer
//
// The SoundAnalyzer class uses I2S to read samples from the microphone and then runs an FFT on the
// results to generate the peaks in each band, as well as tracking an overall VU and VU ratio, the
// latter being the ratio of the current VU to the trailing min and max VU.

class SoundAnalyzer : public AudioVariables
{
    static constexpr size_t MAX_SAMPLES = 256;
    std::unique_ptr<int16_t[]> ptrSampleBuffer;

    // I'm old enough I can only hear up to about 12K, but feel free to adjust.  Remember from
    // school that you need to sample at double the frequency you want to process, so 24000 is 12K

    static constexpr size_t SAMPLING_FREQUENCY = 20000;
    static constexpr size_t LOWEST_FREQ = 100;
    static constexpr size_t HIGHEST_FREQ = SAMPLING_FREQUENCY / 2;

    static constexpr size_t _sampling_period_us = PERIOD_FROM_FREQ(SAMPLING_FREQUENCY);

    int      _cutOffsBand[NUM_BANDS];   // The upper frequency for each band
    float    _oldVU;                    // Old VU value for damping
    float    _oldPeakVU;                // Old peak VU value for damping
    float    _oldMinVU;                 // Old min VU value for damping
    double * _vPeaks;                   // The peak value for each band

    PeakData::MicrophoneType _MicMode;

    // GetBandIndex
    //
    // Given a frequency, returns the index of the band that frequency belongs to

    int GetBandIndex(float frequency)
    {
        for (int i = 0; i < NUM_BANDS; i++)
            if (frequency < _cutOffsBand[i])
                return i;

        // If we never found a band that includes the freq under its limit, it's in the top bar
        return NUM_BANDS-1;
    }

    // GetBucketFrequency
    //
    // Given a bucket index, returns the frequency that bucket represents
    
    float GetBucketFrequency(int bin_index)
    {
        float bin_width = SAMPLING_FREQUENCY / (MAX_SAMPLES / 2);
        float frequency = bin_width * bin_index;
        return frequency;
    }

    double * _vReal;
    double * _vImaginary;

    // SampleBuffer::Reset
    //
    // Resets (clears) everything about the buffer except for the time stamp.

    void Reset()
    {
        for (int i = 0; i < MAX_SAMPLES; i++)
        {
            _vReal[i] = 0.0;
            _vImaginary[i] = 0.0f;
        }
        for (int i = 0; i < NUM_BANDS; i++)
            _vPeaks[i] = 0;
    }

    // SampleBuffer::FFT
    //
    // Run the FFT on the sample buffer.  When done the first two buckets are VU data and only the first MAX_SAMPLES/2
    // are valid.  For each bucket afterward you can call BucketFrequency to find out what freq corresponds to what bucket

    void FFT()
    {
        ArduinoFFT<double> _FFT(_vReal, _vImaginary, MAX_SAMPLES, SAMPLING_FREQUENCY);
        _FFT.dcRemoval();
        _FFT.windowing(FFTWindow::Blackman, FFTDirection::Forward);
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
            ESP_ERROR_CHECK(i2s_read(I2S_NUM_0, (void *) ptrSampleBuffer.get(), bytesExpected, &bytesRead, 100 / portTICK_PERIOD_MS));
            ESP_ERROR_CHECK(i2s_stop(I2S_NUM_0));
        #endif

        if (bytesRead != bytesExpected)
        {
            debugW("Could only read %u bytes of %u in FillBufferI2S()\n", bytesRead, bytesExpected);
            return;
        }

        for (int i = 0; i < MAX_SAMPLES; i++)
        {
            _vReal[i] = ptrSampleBuffer[i];
        }
    }

    // UpdateVU
    //
    // This function is responsible for updating the Volume Unit (VU) values: the current VU (_VU),
    // the peak VU (_PeakVU), and the minimum VU (_MinVU).
    //
    // Firstly, it updates the current VU (_VU) based on the new incoming value (newval).
    // If the new value is greater than the old VU (_oldVU), it assigns the new value to _VU.
    // Otherwise, it applies a damping calculation that drifts _VU towards the new value.
    //
    // Then, it updates the peak VU (_PeakVU) by checking if the current VU (_VU) has exceeded the previous peak.
    // If it has, it updates _PeakVU to the current VU.
    // Otherwise, it applies a damping calculation that drifts the peak VU towards the current VU.
    //
    // Lastly, it updates the minimum VU (_MinVU) by checking if the current VU has dipped below the previous minimum.
    // If it has, it updates _MinVU to the current VU.
    // Otherwise, it applies another damping calculation that drifts the minimum VU towards the current VU.

    void UpdateVU(float newval)
    {
        if (newval > _oldVU)
            _VU = newval;
        else
            _VU = (_oldVU * VUDAMPEN + newval) / (VUDAMPEN + 1);

        _oldVU = _VU;

        // If we crest above the max VU, update the max VU up to that.  Otherwise drift it towards the new value.

        if (_VU > _PeakVU)
            _PeakVU = _VU;
        else
            _PeakVU = (_oldPeakVU * VUDAMPENMAX + _VU) / (VUDAMPENMAX + 1);
        _oldPeakVU = _PeakVU;

        // If we dip below the min VU, update the min VU down to that.  Otherwise drift it towards the new value.

        if (_VU < _MinVU)
            _MinVU = _VU;
        else
            _MinVU = (_oldMinVU * VUDAMPENMIN + _VU) / (VUDAMPENMIN + 1);
        _oldMinVU = _MinVU;
    }

    // SampleBuffer::ProcessPeaks
    //
    // Runs through and figures out what the peak level is in each of the bands.  Also calculates
    // the overall VU level and adjusts the auto gain.

    PeakData ProcessPeaks()
    {
        // Find the peak and the average

        double averageSum = 0.0f;

        for (int i = 0; i < NUM_BANDS; i++)
            _vPeaks[i] = 0.0f;

        for (int i = 2; i < MAX_SAMPLES / 2; i++)
        {
            _vReal[i] = _vReal[i] / MAX_SAMPLES * AUDIO_MIC_SCALAR;

            int freq = GetBucketFrequency(i-2);
            if (freq >= LOWEST_FREQ)
            {
                // Track the average and the peak value

                double vVal = _vReal[i];
                averageSum += vVal;

                // If it's above the noise floor, figure out which band this belongs to and
                // if it's a new peak for that band, record that fact

                int iBand = GetBandIndex(freq);
                if (vVal > _vPeaks[iBand])
                    _vPeaks[iBand] = _vReal[i];
            }
        }
        averageSum = averageSum / (MAX_SAMPLES / 2 - 2);

        // Noise gate - if the signal in this band is below a threshold we define, then we say there's no energy in this band

        for (int i = 0; i < NUM_BANDS; i++)
        {
            _vPeaks[i] *= PeakData::GetBandScalar(_MicMode, i);
            if (_vPeaks[i] < NOISE_CUTOFF)
                _vPeaks[i] = 0.0f;
        }

        // Print out the low 4 and high 4 bands so we can monitor levels in the debugger if needed
        EVERY_N_SECONDS(1)
        {
            debugV("Raw Peaks: %0.1lf %0.1lf  %0.1lf  %0.1lf <--> %0.1lf  %0.1lf  %0.1lf  %0.1lf",
                   _vPeaks[0], _vPeaks[1], _vPeaks[2], _vPeaks[3], _vPeaks[12], _vPeaks[13], _vPeaks[14], _vPeaks[15]);
        }
        // If you want the peaks to be a lot more prominent, you can exponentially raise the values
        // and then they'll be scaled back down linearly, but you'd have to adjust allBandsPeak
        // accordingly as well as the value there now is based on no exponential scaling.
        //
        //

        #if SCALE_AUDIO_EXPONENTIAL
            for (int i = 0; i < NUM_BANDS; i++)
                _vPeaks[i] = powf(_vPeaks[i], 2.0);
        #endif

        double allBandsPeak = 0;
        for (int i = 0; i < NUM_BANDS; i++)
            allBandsPeak = max(allBandsPeak, _vPeaks[i]);

        // It's hard to know what to use for a "minimum" volume so I aimed for a light ambient noise background
        // just triggering the bottom pixel, and real silence yielding darkness

        allBandsPeak = std::max((double)NOISE_FLOOR, allBandsPeak);
        debugV("All Bands Peak: %lf", allBandsPeak);

        // Normalize all the bands relative to allBandsPeak
        for (int i = 0; i < NUM_BANDS; i++)
            _vPeaks[i] /= allBandsPeak;

        if (_VURatio < 1.0)

            for (int i = 0; i < NUM_BANDS; i++)
                _vPeaks[i] = max(0.0, _vPeaks[i]);

        // We'll use the average as the gVU.  I assume the average of the samples tracks sound pressure level, but don't really know...

        float newval = averageSum;

        debugV("AverageSum : %f", averageSum);
        debugV("Newval     : %f", newval);

        UpdateVU(newval);

        EVERY_N_MILLISECONDS(100)
        {
            auto peaks = GetPeakData();
            debugV("Audio Data -- Sum: %0.2f, _MinVU: %0.2f, _PeakVU: %0.2f, _VU: %f, Peak0: %f, Peak1: %f, Peak2: %f, Peak3: %f", averageSum, _MinVU, _PeakVU, _VU, peaks[0], peaks[1], peaks[2], peaks[3]);
        }

        return PeakData(_vPeaks);
    }

    //
    // Calculate a logarithmic scale for the bands like you would find on a graphic equalizer display
    //

    void CalculateBandCutoffs(float lowFreq, float highFreq)
    {
        if (NUM_BANDS == 16)
        {
            static constexpr std::array<int, 16> cutOffs16Band  =
            {
                200, 380, 580, 780, 980, 1200, 1600, 1800, 2000, 2412, 3162, 3781, 5312, 6310, 8400, (int)HIGHEST_FREQ
            };

            for (int i = 0; i < NUM_BANDS; i++)
                _cutOffsBand[i] = cutOffs16Band[i];
        }
        else
        {
            // Calculate the logarithmic spacing for the frequency bands
            float f1 = LOWEST_FREQ;
            float f2 = HIGHEST_FREQ;

            // Calculate the ratio based on logarithmic scale
            float log_f1 = log10(f1);
            float log_f2 = log10(f2);
            float delta = (log_f2 - log_f1) / (NUM_BANDS - 1);

            for (int i = 0; i < NUM_BANDS; i++)
            {
                // Calculate the upper frequency for each band
                _cutOffsBand[i] = round(pow(10, log_f1 + delta * (i + 1)));
                debugV("BAND %d: %d\n", i, _cutOffsBand[i]);
            }
        }
    }

public:

    PeakData _Peaks;                    // The peak data for the last sample pass

    SoundAnalyzer()
    {
        ptrSampleBuffer.reset( (int16_t *)heap_caps_malloc(MAX_SAMPLES * sizeof(int16_t), MALLOC_CAP_8BIT) );
        if (!ptrSampleBuffer)
            throw std::runtime_error("Failed to allocate sample buffer");

        _vReal      = (double *)PreferPSRAMAlloc(MAX_SAMPLES * sizeof(_vReal[0]));
        _vImaginary = (double *)PreferPSRAMAlloc(MAX_SAMPLES * sizeof(_vImaginary[0]));
        _vPeaks     = (double *)PreferPSRAMAlloc(NUM_BANDS  * sizeof(_vPeaks[0]));

        _oldVU = 0.0f;
        _oldPeakVU = 0.0f;
        _oldMinVU = 0.0f;

        CalculateBandCutoffs(LOWEST_FREQ, SAMPLING_FREQUENCY / 2.0);
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
    
    const int16_t * GetSampleBuffer() const
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
        
    #elif ELECROW 

        const i2s_config_t i2s_config = {
                .mode = (i2s_mode_t) (I2S_MODE_MASTER | I2S_MODE_RX),
                .sample_rate = SAMPLING_FREQUENCY,
                .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,
                .channel_format = I2S_CHANNEL_FMT_ONLY_LEFT,
                .communication_format = I2S_COMM_FORMAT_STAND_I2S,
                .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
                .dma_buf_count = 2,
                .dma_buf_len = (int) MAX_SAMPLES,
                .use_apll = false
            };

            // i2s pin configuration
            const i2s_pin_config_t pin_config = {
                .bck_io_num = 39,
                .ws_io_num = 38,
                .data_out_num = -1,  // not used
                .data_in_num = INPUT_PIN
            };

            ESP_ERROR_CHECK( i2s_driver_install(I2S_NUM_0, &i2s_config, 0, NULL) );
            ESP_ERROR_CHECK( i2s_set_pin(I2S_NUM_0, &pin_config) );
            ESP_ERROR_CHECK( i2s_start(I2S_NUM_0) );

    #elif TTGO || MESMERIZER || SPECTRUM_WROVER_KIT 

        i2s_config_t i2s_config;
        i2s_config.mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX | I2S_MODE_ADC_BUILT_IN);
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
        ESP_ERROR_CHECK(i2s_driver_install(I2S_NUM_0, &i2s_config, 0, nullptr));
        ESP_ERROR_CHECK(i2s_set_adc_mode(ADC_UNIT_1, ADC1_CHANNEL_0));

    #else

        i2s_config_t i2s_config;
        i2s_config.mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX | I2S_MODE_ADC_BUILT_IN);
        i2s_config.sample_rate = SAMPLING_FREQUENCY;
        i2s_config.dma_buf_len = MAX_SAMPLES;
        i2s_config.bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT;
        i2s_config.channel_format = I2S_CHANNEL_FMT_ONLY_LEFT;
        i2s_config.use_apll = false,
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

    PeakData::MicrophoneType MicMode()
    {
        return _MicMode;
    }

    unsigned long _lastPeak1Time[NUM_BANDS] = {0};
    float _peak1Decay[NUM_BANDS] = {0};
    float _peak2Decay[NUM_BANDS] = {0};
    float _peak1DecayRate = 1.25f;
    float _peak2DecayRate = 1.25f;

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
                const float maxIncrease = std::max(0.0, g_Values.AppTime.LastFrameTime() * _peak1DecayRate * VU_REACTIVITY_RATIO);  
                _peak1Decay[i] = std::min(_Peaks[i], _peak1Decay[i] + maxIncrease);
                _lastPeak1Time[i] = millis();
            }
            if (_Peaks[i] > _peak2Decay[i])
            {
                const float maxIncrease = std::max(0.0, g_Values.AppTime.LastFrameTime() * _peak2DecayRate * VU_REACTIVITY_RATIO);
                _peak2Decay[i] = std::min(_Peaks[i], _peak2Decay[i] + maxIncrease);
            }
        }
    }

    inline PeakData GetPeakData() const
    {
        return _Peaks;
    }

    inline void SetPeakData(const PeakData &peaks)
    {
        debugV("Manually setting peaks!");
        Serial.print(" #");
        _msLastRemote = millis();
        _Peaks = peaks;
    }

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
            _Peaks = ProcessPeaks();
        }
        else
        {
            // Calculate a total VU from the band data
            float sum = 0.0f;
            for (int i = 0; i < NUM_BANDS; i++)
                sum += _Peaks[i];

            // Scale it so that it is not always in the top red
            _MicMode = PeakData::PCREMOTE;
            UpdateVU(sum / NUM_BANDS);
        }
    }
};
#endif

extern SoundAnalyzer g_Analyzer;
