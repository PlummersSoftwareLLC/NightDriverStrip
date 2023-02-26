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

#include <arduinoFFT.h>
#include <driver/i2s.h>
#include <driver/adc.h>
// #include <driver/adc_deprecated.h>

extern DRAM_ATTR bool g_bUpdateStarted; // Has an OTA update started?

#define SUPERSAMPLES 2                                    // How many supersamples to take
#define SAMPLE_BITS 12                                    // Sample resolution (0-4095)
#define MAX_ANALOG_IN ((1 << SAMPLE_BITS) * SUPERSAMPLES) // What our max analog input value is on all analog pins (4096 is default 12 bit resolution)
#define MAX_VU MAX_ANALOG_IN
#define MS_PER_SECOND 1000

// These are the audio variables that are referenced by many audio effects.  In order to allow non-audio code to reference them too without
// including all the audio code (such as logging code, etc), we put the publicly exposed variables into a structure, and then the SoundAnalyzer
// class will inherit them.
//
// In the non-audio case, there's a stub class that includes ONLY the audio variables and none of the code or buffers.
//
// In both cases, the AudioVariables are accessiable as g_Analyzer.  It'll just be a stub in the non-audio case

struct AudioVariables
{
    float _VURatio = 1.0;         // Current VU as a ratio to its recent min and max
    float _VURatioFade = 1.0;     // Same as gVURatio but with a slow decay
    float _VU = 0.0;              // Instantaneous read of VU value
    float _PeakVU = MAX_VU;       // How high our peak VU scale is in live mode
    float _MinVU = 0.0;           // How low our peak VU scale is in live mode
    unsigned long _cSamples = 0U; // Total number of samples successfully collected
    int _AudioFPS = 0;            // Framerate of the audio sampler
    int _serialFPS = 0;           // How many serial packets are processed per second
    uint _msLastRemote = 0;       // When the last Peak data came in from external (ie: WiFi)
};

#if !ENABLE_AUDIO
class SoundAnalyzer : public AudioVariables // Non-audio case.  Inherits only the AudioVariables so that any project can
{                                           //   reference them in g_Analyzer
};

#else // Audio case

#define EXAMPLE_I2S_NUM (I2S_NUM_0)
#define EXAMPLE_I2S_SAMPLE_BITS (I2S_COMM_FORMAT_STAND_I2S | I2S_COMM_FORMAT_STAND_MSB)
#define EXAMPLE_I2S_BUF_DEBUG (0)                                                               // enable display buffer for debug
#define EXAMPLE_I2S_READ_LEN (MAX_SAMPLES)                                                      // I2S read buffer length
#define EXAMPLE_I2S_FORMAT (I2S_CHANNEL_FMT_RIGHT_LEFT)                                         // I2S data format
#define EXAMPLE_I2S_CHANNEL_NUM ((EXAMPLE_I2S_FORMAT < I2S_CHANNEL_FMT_ONLY_RIGHT) ? (2) : (1)) // I2S channel number
#define I2S_ADC_UNIT ADC_UNIT_1                                                                 // I2S built-in ADC unit
#define I2S_ADC_CHANNEL ADC1_CHANNEL_0                                                          // I2S built-in ADC channel

void IRAM_ATTR AudioSamplerTaskEntry(void *);
void IRAM_ATTR AudioSerialTaskEntry(void *);

// PeakData class
//
// Simple data class that holds the music peaks for up to 32 bands.  When the sound analyzer finishes a pass, its
// results are simplified down to this small class of band peaks.

// The MAX9814 mic has different sensitivity than th M5's mic, so each needs its own value here

#if (M5STICKC || M5STICKCPLUS)
#define MIN_VU 128
#else
#define MIN_VU 512
#endif

#ifndef GAINDAMPEN
#define GAINDAMPEN 10      // How slowly brackets narrow in for spectrum bands
#define GAINDAMPENMIN 1000 //   We want the quiet part to adjust quite slowly
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

protected:
    static float _Min[NUM_BANDS];
    static float _Max[NUM_BANDS];
    static float _Last[NUM_BANDS];
    static float _allBandsMax;

    float _Level[NUM_BANDS];
    float _Ratio[NUM_BANDS];

    void UpdateMinMax()
    {
        const bool bScaleAllBands = true;
        _allBandsMax = 0.0f;
        for (int band = 0; band < NUM_BANDS; band++)
        {
            // If new peak is above the max, it becomes the new max.  Otherwise we drift
            // towards it using a weighted average.

            if (_Level[band] < _Last[band])
                _Level[band] = (_Last[band] * GAINDAMPEN + _Level[band]) / (GAINDAMPEN + 1);

            if (_Level[band] > _Max[band])
                _Max[band] = _Level[band];
            else
                _Max[band] = (_Max[band] * GAINDAMPEN + _Level[band]) / (GAINDAMPEN + 1);

            //_Max[band] = max(_Max[band], 0.6f);

            // If new peak is below the min, it becomes the new min. Otherwise we drift
            // toward it using a weighted average

            if (_Level[band] < _Min[band])
                _Min[band] = _Level[band];
            else
                _Min[band] = (_Min[band] * GAINDAMPEN + _Level[band]) / (GAINDAMPEN + 1);

            _Min[band] = min(_Min[band], 0.4f);

            // Keep track of the highest peak in any band above its own min

            if (_Max[band] - _Min[band] > _allBandsMax)
                _allBandsMax = _Max[band] - _Min[band];
        }

        debugV("_allBandsMax: %f", _allBandsMax);

        for (int band = 0; band < NUM_BANDS; band++)
        {
            float denominator = bScaleAllBands ? _allBandsMax : _Max[band] - _Min[band];
            if (denominator <= 0.0f)
                _Ratio[band] = 0.0f;
            else
                _Ratio[band] = (_Max[band] - _Min[band]) / denominator;
        }

        debugV("Min:    %f, %f, %f, %f", _Min[0], _Min[1], _Min[2], _Min[3]);
        debugV("Max:    %f, %f, %f, %f", _Max[0], _Max[1], _Max[2], _Max[3]);
        debugV("Level:  %f, %f, %f, %f", _Level[0], _Level[1], _Level[2], _Level[3]);
        debugV("Ratio:  %f, %f, %f, %f", _Ratio[0], _Ratio[1], _Ratio[2], _Ratio[3]);
        debugV("======");
    }

public:
    typedef enum
    {
        MESMERIZERMIC,
        PCREMOTE,
        M5
    } MicrophoneType;

    PeakData()
    {
        for (int i = 0; i < NUM_BANDS; i++)
            _Level[i] = _Ratio[i] = 0.0f;
    }

    PeakData(float *pFloats)
    {
        for (int i = 0; i < NUM_BANDS; i++)
            _Level[i] = _Ratio[i] = 0.0f;

        SetData(pFloats);
    }

    PeakData &operator=(const PeakData &other)
    {
        for (int i = 0; i < NUM_BANDS; i++)
        {
            _Level[i] = other._Level[i];
            _Ratio[i] = other._Ratio[i];
        }
        _allBandsMax = other._allBandsMax;
        return *this;
    }

    const float operator[](std::size_t n) const
    {
        return _Level[n];
    }

    static double GetBandScalar(MicrophoneType mic, int i)
    {
        switch (mic)
        {
        case MESMERIZERMIC:
        {
            static const double Scalars16[16] = {0.1, 0.36, 0.2, 0.25, 0.45, 0.5, 0.6, 0.7, 0.8, 0.9, 1.0, 1.4, 1.4, 1.0, 1.0, 1.0};
            double result = (NUM_BANDS == 16) ? Scalars16[i] : mapDouble(i, 0, NUM_BANDS - 1, 1.0, 1.0);
            return result;
        }
        case PCREMOTE:
        {
            static const double Scalars16[16] = {1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0};
            double result = (NUM_BANDS == 16) ? Scalars16[i] : mapDouble(i, 0, NUM_BANDS - 1, 1.0, 1.0);
            return result;
        }
        default:
        {
            static const double Scalars12[12] = {0.25, 0.70, 1.2, 1.1, 0.70, 0.5, 0.47, 0.68, 0.77, 0.75, 0.5, 0.72};
            double result = (NUM_BANDS == 12) ? Scalars12[i] : mapDouble(i, 0, NUM_BANDS - 1, 1.0, 1.0);
            return result;
        }
        }
    }

    void ApplyScalars(MicrophoneType mic)
    {
        for (int i = 0; i < NUM_BANDS; i++)
        {
            _Level[i] *= GetBandScalar(mic, i);
        }
    }
    float Ratio(std::size_t n) const
    {
        return _Ratio[n];
    }

    float Max(std::size_t n) const
    {
        return _Max[n];
    }

    void SetData(float *pFloats)
    {
        for (int i = 0; i < NUM_BANDS; i++)
        {
            _Level[i] = pFloats[i];
        }
        UpdateMinMax();
    }
};

// SoundAnalyzer
//
// The SoundAnalyzer class uses I2S to read samples from the microphone and then runs an FFT on the
// results to generate the peaks in each band, as well as tracking an overall VU and VU ratio, the
// latter being the ratio of the current VU to the trailing min and max VU.

class SoundAnalyzer : public AudioVariables
{
    const size_t MAX_SAMPLES = 512;

    // I'm old enough I can only hear up to about 12K, but feel free to adjust.  Remember from
    // school that you need to sample at doube the frequency you want to process, so 24000 is 12K

    const size_t SAMPLING_FREQUENCY = 24000;
    const size_t _sampling_period_us = PERIOD_FROM_FREQ(SAMPLING_FREQUENCY);

    size_t _MaxSamples;        // Number of samples we will take, must be a power of 2
    size_t _SamplingFrequency; // Sampling Frequency should be at least twice that of highest freq sampled
    size_t _BandCount;
    float *_vPeaks;
    int _InputPin;
    int _cutOffsBand[NUM_BANDS];
    float _oldVU;
    float _oldPeakVU;
    float _oldMinVU;
    uint8_t _inputPin; // Which hardware pin do we actually sample audio from?
    PeakData _Peaks;

    PeakData::MicrophoneType _MicMode = PeakData::M5;

    // BucketFrequency
    //
    // Return the frequency corresponding to the Nth sample bucket.  Skips the first two
    // buckets which are overall amplitude and something else.

    int BucketFrequency(int iBucket) const
    {
        if (iBucket <= 1)
            return 0;

        int iOffset = iBucket - 2;
        return _cutOffsBand[iOffset];
        // return iOffset * (_SamplingFrequency / 2) / (_MaxSamples / 2);
    }

    int GetBandIndex(double frequency)
    {
        int band = -1;
        for (int i = 0; i < NUM_BANDS; i++)
        {
            if (frequency < _cutOffsBand[i])
            {
                band = i;
                break;
            }
        }

        if (band < 0)
        {
            band = 0;
        }
        else if (band >= NUM_BANDS)
        {
            band = NUM_BANDS - 1;
        }

        return band;
    }

    double GetBucketFrequency(int bin_index)
    {
        double bin_width = SAMPLING_FREQUENCY / (MAX_SAMPLES / 2);
        double frequency = bin_width * bin_index;
        return frequency;
    }

    volatile int _cSamples;
    double *_vReal;
    double *_vImaginary;

    // SampleBuffer::Reset
    //
    // Resets (clears) everything about the buffer except for the time stamp.

    void Reset()
    {
        _cSamples = 0;
        for (int i = 0; i < _MaxSamples; i++)
        {
            _vReal[i] = 0.0;
            _vImaginary[i] = 0.0f;
        }
        for (int i = 0; i < _BandCount; i++)
            _vPeaks[i] = 0;
    }

    // SampleBuffer::FFT
    //
    // Run the FFT on the sample buffer.  When done the first two buckets are VU data and only the first MAX_SAMPLES/2
    // are valid.  For each bucket afterwards you can call BucketFrequency to find out what freq corresponds to what bucket

    void FFT()
    {
        arduinoFFT _FFT(_vReal, _vImaginary, _MaxSamples, _SamplingFrequency);
        //_FFT.DCRemoval();
        _FFT.Windowing(FFT_WIN_TYP_RECTANGLE, FFT_FORWARD);
        _FFT.Compute(FFT_FORWARD);
        _FFT.ComplexToMagnitude();
        _FFT.MajorPeak();
    }

    inline bool IsBufferFull() const __attribute__((always_inline))
    {
        return (_cSamples >= _MaxSamples);
    }

    // flash record size, for recording 5 second
    void SampleBufferInitI2S()
    {
        // install and start i2s driver

        debugV("Begin SamplerBufferInitI2S...");

#if M5STICKC || M5STICKCPLUS

        i2s_config_t i2s_config =
        {
            .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX | I2S_MODE_PDM),
            .sample_rate = SAMPLING_FREQUENCY,
            .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT, // is fixed at 12bit, stereo, MSB
            .channel_format = I2S_CHANNEL_FMT_ALL_RIGHT,
#if ESP_IDF_VERSION > ESP_IDF_VERSION_VAL(4, 1, 0)
            .communication_format = I2S_COMM_FORMAT_STAND_I2S, // Set the format of the communication.
#else
            .communication_format = I2S_COMM_FORMAT_I2S,
#endif
            .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
            .dma_buf_count = 2,
            .dma_buf_len = 256,
        };

        i2s_pin_config_t pin_config;

#if (ESP_IDF_VERSION > ESP_IDF_VERSION_VAL(4, 3, 0))
        pin_config.mck_io_num = I2S_PIN_NO_CHANGE;
#endif

        pin_config.bck_io_num = I2S_PIN_NO_CHANGE;
        pin_config.ws_io_num = IO_PIN;
        pin_config.data_out_num = I2S_PIN_NO_CHANGE;
        pin_config.data_in_num = INPUT_PIN;

        i2s_driver_install(I2S_NUM_0, &i2s_config, 0, NULL);
        i2s_set_pin(I2S_NUM_0, &pin_config);
        i2s_set_clk(I2S_NUM_0, SAMPLING_FREQUENCY, I2S_BITS_PER_SAMPLE_16BIT, I2S_CHANNEL_MONO);

#elif TTGO || MESMERIZER || SPECTRUM_WROVER_KIT

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
        ESP_ERROR_CHECK(i2s_driver_install(EXAMPLE_I2S_NUM, &i2s_config, 0, NULL));
        ESP_ERROR_CHECK(i2s_set_adc_mode(I2S_ADC_UNIT, I2S_ADC_CHANNEL));

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
        ESP_ERROR_CHECK(i2s_driver_install(EXAMPLE_I2S_NUM, &i2s_config, 0, NULL));
        ESP_ERROR_CHECK(i2s_set_adc_mode(I2S_ADC_UNIT, I2S_ADC_CHANNEL));

#endif

        debugV("SamplerBufferInitI2S Complete\n");
    }

    void FillBufferI2S()
    {
        int16_t sampleBuffer[MAX_SAMPLES];

        if (IsBufferFull())
        {
            debugW("BUG: FillBUfferI2S found buffer already full.");
            return;
        }
        size_t bytesRead = 0;

#if M5STICKC || M5STICKCPLUS
        ESP_ERROR_CHECK(i2s_read(I2S_NUM_0, (void *)sampleBuffer, sizeof(sampleBuffer), &bytesRead, (100 / portTICK_RATE_MS)));
#else
        ESP_ERROR_CHECK(i2s_adc_enable(EXAMPLE_I2S_NUM));
        ESP_ERROR_CHECK(i2s_read(EXAMPLE_I2S_NUM, (void *)sampleBuffer, sizeof(sampleBuffer), &bytesRead, (100 / portTICK_RATE_MS)));
        ESP_ERROR_CHECK(i2s_adc_disable(EXAMPLE_I2S_NUM));
#endif

        _cSamples = _MaxSamples;
        if (bytesRead != sizeof(sampleBuffer))
        {
            debugW("Could only read %u bytes of %u in FillBufferI2S()\n", bytesRead, sizeof(sampleBuffer));
            return;
        }

        for (int i = 0; i < ARRAYSIZE(sampleBuffer); i++)
        {
#if M5STICKC || M5STICKCPLUS
            _vReal[i] = ::map(sampleBuffer[i], INT16_MIN, INT16_MAX, 0, MAX_VU);
#else
            _vReal[i] = sampleBuffer[i];
#endif
        }
    }

    void UpdateVU(double newval)
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
#ifndef NOISE_CUTOFF
#define NOISE_CUTOFF 50
#endif

        // Find the peak and the average

        float averageSum = 0.0f;
        double samplesPeak = 0.0f;

        for (int i = 2; i < _MaxSamples / 2; i++)
        {
            // Track the average and the peak value

            averageSum += _vReal[i];
            if (_vReal[i] > samplesPeak)
                samplesPeak = _vReal[i];

            // If it's above the noise floor, figure out which band this belongs to and
            // if it's a new peak for that band, record that fact

            int freq = GetBucketFrequency(i);
            int iBand = GetBandIndex(freq);

            if (_vReal[i] > NOISE_CUTOFF)
            {
                if (_vReal[i] > _vPeaks[iBand])
                    _vPeaks[iBand] += _vReal[i];
            }
        }

        for (int i = 0; i < NUM_BANDS; i++)
            _vPeaks[i] *= PeakData::GetBandScalar(_MicMode, i);

        // Print out the low 4 and high 4 bands so we can monitor levels in the debugger if needed
        EVERY_N_SECONDS(1)
        {
            debugW("Raw Peaks: %0.1lf %0.1lf  %0.1lf  %0.1lf <--> %0.1lf  %0.1lf  %0.1lf  %0.1lf",
                   _vPeaks[0], _vPeaks[1], _vPeaks[2], _vPeaks[3], _vPeaks[12], _vPeaks[13], _vPeaks[14], _vPeaks[15]);
        }

        //        for (int i = 0; i < _BandCount; i++)
        //        {
        //            _vPeaks[i] *= GetBandScalar(i);
        //        }

        // If you want the peaks to be a lot more prominent, you can exponentially raise the values
        // and then they'll be scaled back down linearly, but you'd have to adjust allBandsPeak
        // accordingly as well as the value there now is based on no exponential scaling.
        //
        //  _vPeaks[i] = powf(_vPeaks[i], 2.0);

        float allBandsPeak = 0;
        for (int i = 0; i < _BandCount; i++)
            allBandsPeak = max(allBandsPeak, _vPeaks[i]);

        // It's hard to know what to use for a "minimum" volume so I aimed for a light ambient noise background
        // just triggering the bottom pixel, and real silence yielding darkness

        debugV("All Bands Peak: %f", allBandsPeak);
        allBandsPeak = max(NOISE_FLOOR, allBandsPeak);

        auto multiplier = mapDouble(_VURatio, 0.0, 2.0, 1.5, 1.0);

#if MESERMIZER
        // Visual hand-tweaking to get the display to look a little taller
        allBandsPeak *= 1.0;
#endif

        for (int i = 0; i < _BandCount; i++)
            _vPeaks[i] /= allBandsPeak;

        // We'll use the average as the gVU.  I assume the average of the samples tracks sound pressure level, but don't really know...

        float newval = averageSum / (_MaxSamples / 2 - 2);
        debugV("AverageSumL: %f", averageSum);

        UpdateVU(newval);

        EVERY_N_MILLISECONDS(100)
        {
            auto peaks = GetPeakData();
            debugV("Audio Data -- Sum: %0.2f, _MinVU: %f0.2, _PeakVU: %f0.2, _VU: %f, Peak0: %f, Peak1: %f, Peak2: %f, Peak3: %f", averageSum, _MinVU, _PeakVU, _VU, peaks[0], peaks[1], peaks[2], peaks[3]);
        }

        PeakData peaks = GetBandPeaks();
        return peaks;
    }

    //
    // Calculate a logrithmic scale for the bands like you would find on a graphic equalizer display
    //

    void CalculateBandCutoffs(double lowFreq, double highFreq)
    {
        if (NUM_BANDS == 16)
        {
            static int cutOffs16Band[16] =
                {
                    30, 300, 425, 565, 715, 900, 1125, 1400, 1750, 2250, 2800, 3150, 4000, 5000, 6400, 12500};
            for (int i = 0; i < NUM_BANDS; i++)
                _cutOffsBand[i] = cutOffs16Band[i];
        }

        // The difference between each adjacent pair of cutoffs is equal to the geometric mean of the two frequencies.

        double df = pow(highFreq / lowFreq, 1.0 / (NUM_BANDS - 1));

        for (int i = 0; i < NUM_BANDS; i++)
        {
            _cutOffsBand[i] = (int)lowFreq;
            lowFreq *= df;
        }
    }

    PeakData GetSamplePassPeaks()
    {
        return _Peaks;
    }

    // BandCutoffTable
    //
    // Depending on how many bands we have, returns the cutoffs of where those bands are in the spectrum

    const int *BandCutoffTable(int bandCount)
    {
        return _cutOffsBand;
    }

    // SampleBuffer::GetBandPeaks
    //
    // Once the FFT processing is complete you can call this function to get a copy of what each of the
    // peaks in the various bands is

    PeakData GetBandPeaks()
    {
        return PeakData(_vPeaks);
    }

public:
    SoundAnalyzer(uint8_t inputPin)
        : _sampling_period_us(PERIOD_FROM_FREQ(SAMPLING_FREQUENCY)),
          _inputPin(inputPin)
    {
        _BandCount = NUM_BANDS;
        _SamplingFrequency = SAMPLING_FREQUENCY;
        _MaxSamples = MAX_SAMPLES;
        _InputPin = INPUT_PIN;

        _vReal = (double *)malloc(_MaxSamples * sizeof(_vReal[0]));
        _vImaginary = (double *)malloc(_MaxSamples * sizeof(_vImaginary[0]));
        _vPeaks = (float *)malloc(_BandCount * sizeof(_vPeaks[0]));

        _oldVU = 0.0f;
        _oldPeakVU = 0.0f;
        _oldMinVU = 0.0f;

        SampleBufferInitI2S();
        CalculateBandCutoffs(200.0, SAMPLING_FREQUENCY / 2.0);
        Reset();
    }

    ~SoundAnalyzer()
    {
        free(_vReal);
        free(_vImaginary);
        free(_vPeaks);
    }

    PeakData::MicrophoneType MicMode()
    {
        return _MicMode;
    }

    unsigned long g_lastPeak1Time[NUM_BANDS] = {0};
    float g_peak1Decay[NUM_BANDS] = {0};
    float g_peak2Decay[NUM_BANDS] = {0};
    float g_peak1DecayRate = 1.25f;
    float g_peak2DecayRate = 1.25f;

    // DecayPeaks
    //
    // Every so many ms we decay the peaks by a given amount

    inline void DecayPeaks()
    {
        // REVIEW(davepl) Can be updated to use the frame timers from g_AppTime

        static unsigned long lastDecay = 0;
        float seconds = (millis() - lastDecay) / (float)MS_PER_SECOND;
        lastDecay = millis();

        float decayAmount1 = std::max(0.0f, seconds * g_peak1DecayRate);
        float decayAmount2 = std::max(0.0f, seconds * g_peak2DecayRate);

        for (int iBand = 0; iBand < NUM_BANDS; iBand++)
        {
            g_peak1Decay[iBand] -= min(decayAmount1, g_peak1Decay[iBand]);
            g_peak2Decay[iBand] -= min(decayAmount2, g_peak2Decay[iBand]);
        }

        /* Manual smoothing if desired */

        for (int iBand = 1; iBand < NUM_BANDS - 1; iBand += 2)
        {
            g_peak1Decay[iBand] = (g_peak1Decay[iBand-1] + g_peak1Decay[iBand+1]) / 2;
            g_peak2Decay[iBand] = (g_peak2Decay[iBand-1] + g_peak2Decay[iBand+1]) / 2;
        }
    }

    // Update the local band peaks from the global sound data.  If we establish a new peak in any band,
    // we reset the peak timestamp on that band

    inline void UpdatePeakData()
    {
        for (int i = 0; i < NUM_BANDS; i++)
        {
            if (_Peaks[i] > g_peak1Decay[i])
            {
                g_peak1Decay[i] = _Peaks[i];
                g_lastPeak1Time[i] = millis();
            }
            if (_Peaks[i] > g_peak2Decay[i])
            {
                g_peak2Decay[i] = _Peaks[i];
            }
        }
    }

    inline PeakData GetPeakData()
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
#if M5STICKC || M5STICKCPLUS
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
            // Calculate an average VU from the band data
            float sum = 0.0f;
            for (int i = 0; i < NUM_BANDS; i++)
                sum += _Peaks[i];

            // Scale it so that its not always in the top red
            _MicMode = PeakData::PCREMOTE;
            UpdateVU(0.25 * MAX_VU * sum / NUM_BANDS);
        }
    }
};
#endif

extern SoundAnalyzer g_Analyzer;
