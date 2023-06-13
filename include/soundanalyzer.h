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
#define MAX_VU 32767
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
    float _VURatio          = 1.0;          // Current VU as a ratio to its recent min and max
    float _VURatioFade      = 1.0;          // Same as gVURatio but with a slow decay
    float _VU               = 0.0;          // Instantaneous read of VU value
    float _PeakVU           = MAX_VU;       // How high our peak VU scale is in live mode
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

#define EXAMPLE_I2S_NUM (I2S_NUM_0)
#define EXAMPLE_I2S_FORMAT (I2S_CHANNEL_FMT_RIGHT_LEFT)                                         // I2S data format
#define I2S_ADC_UNIT ADC_UNIT_1                                                                 // I2S built-in ADC unit
#define I2S_ADC_CHANNEL ADC1_CHANNEL_0                                                          // I2S built-in ADC channel

void IRAM_ATTR AudioSamplerTaskEntry(void *);
void IRAM_ATTR AudioSerialTaskEntry(void *);

// PeakData class
//
// Simple data class that holds the music peaks for up to 32 bands.  When the sound analyzer finishes a pass, its
// results are simplified down to this small class of band peaks.

#define MIN_VU 512

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

protected:

    double _Level[NUM_BANDS];

public:
    typedef enum
    {
        MESMERIZERMIC,
        PCREMOTE,
        M5
    } MicrophoneType;

    PeakData()
    {
        for (auto & i: _Level)
            i = 0.0f;
    }

    PeakData(double *pDoubles)
    {
        SetData(pDoubles);
    }

    PeakData &operator=(const PeakData &other)
    {
        for (int i = 0; i < NUM_BANDS; i++)
            _Level[i] = other._Level[i];
        return *this;
    }

    const float operator[](std::size_t n) const
    {
        return _Level[n];
    }

    static float GetBandScalar(MicrophoneType mic, int i)
    {
      switch (mic)
      {
        case MESMERIZERMIC:
        {
            static const float Scalars16[16] = {0.4, .35, 0.4, 0.7, 0.8, 0.8, 1.0, 1.0, 1.2, 1.5, 2.0, 3.0, 3.0, 3.0, 3.5, 3.5}; //  {0.08, 0.12, 0.3, 0.35, 0.35, 0.5, 0.6, 0.7, 0.8, 0.9, 1.0, 1.4, 1.4, 1.0, 1.0, 1.0};
            float result = (NUM_BANDS == 16) ? Scalars16[i] : map(i, 0, NUM_BANDS - 1, 1.0, 1.0);
            return result;
        }
        case PCREMOTE:
        {
            static const float Scalars16[16] = {1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0};
            float result = (NUM_BANDS == 16) ? Scalars16[i] : map(i, 0, NUM_BANDS - 1, 1.0, 1.0);
            return result;
        }
        default:
        {
            static const float Scalars16[16] = {0.4, .35, 0.6, 0.8, 1.2, 0.7, 1.2, 1.6, 2.0, 2.0, 2.0, 3.0, 3.0, 3.0, 4.0, 5.0}; //  {0.08, 0.12, 0.3, 0.35, 0.35, 0.5, 0.6, 0.7, 0.8, 0.9, 1.0, 1.4, 1.4, 1.0, 1.0, 1.0};
            float result = (NUM_BANDS == 16) ? Scalars16[i] : map(i, 0, NUM_BANDS - 1, 1.0, 1.0);
            return result;
        }
      }
    }

    void ApplyScalars(MicrophoneType mic)
    {
        for (int i = 0; i < NUM_BANDS; i++)
            _Level[i] *= GetBandScalar(mic, i);
    }

    void SetData(double *pDoubles)
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
    const size_t MAX_SAMPLES = 256;

    // I'm old enough I can only hear up to about 12K, but feel free to adjust.  Remember from
    // school that you need to sample at doube the frequency you want to process, so 24000 is 12K

    const size_t SAMPLING_FREQUENCY = 24000;
    const size_t LOWEST_FREQ = 40;
    const size_t HIGHEST_FREQ = SAMPLING_FREQUENCY / 2;

    const size_t _sampling_period_us = PERIOD_FROM_FREQ(SAMPLING_FREQUENCY);

    double * _vPeaks;
    int      _cutOffsBand[NUM_BANDS];
    float    _oldVU;
    float    _oldPeakVU;
    float    _oldMinVU;
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
    }

    int GetBandIndex(float frequency)
    {
        for (int i = 0; i < NUM_BANDS; i++)
            if (frequency < _cutOffsBand[i])
                return i;
                   
        // If we never found a band that includes the freq under its limit, it's in the top bar
        return NUM_BANDS-1;
    }

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
    // are valid.  For each bucket afterwards you can call BucketFrequency to find out what freq corresponds to what bucket

    void FFT()
    {
        arduinoFFT _FFT(_vReal, _vImaginary, MAX_SAMPLES, SAMPLING_FREQUENCY);
        _FFT.DCRemoval();
        _FFT.Windowing(FFT_WIN_TYP_RECTANGLE, FFT_FORWARD);
        _FFT.Compute(FFT_FORWARD);
        _FFT.ComplexToMagnitude();
        _FFT.MajorPeak();
    }

    void FillBufferI2S()
    {
        int16_t sampleBuffer[MAX_SAMPLES];
        size_t bytesRead = 0;

        #if M5STICKC || M5STICKCPLUS || M5STACKCORE2
            i2s_read(I2S_NUM_0, (void *)sampleBuffer, sizeof(sampleBuffer), &bytesRead, (100 / portTICK_RATE_MS));
        #else
            ESP_ERROR_CHECK(i2s_adc_enable(EXAMPLE_I2S_NUM));
            ESP_ERROR_CHECK(i2s_read(EXAMPLE_I2S_NUM, (void *)sampleBuffer, sizeof(sampleBuffer), &bytesRead, (100 / portTICK_RATE_MS)));
            ESP_ERROR_CHECK(i2s_adc_disable(EXAMPLE_I2S_NUM));
        #endif

        if (bytesRead != sizeof(sampleBuffer))
        {
            debugW("Could only read %u bytes of %u in FillBufferI2S()\n", bytesRead, sizeof(sampleBuffer));
            return;
        }

        for (int i = 0; i < ARRAYSIZE(sampleBuffer); i++)
            _vReal[i] = sampleBuffer[i];
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

        float averageSum = 0.0f;

        int hitCount[NUM_BANDS] = {0};
        for (int i = 0; i < NUM_BANDS; i++)
            _vPeaks[i] = 0.0f;

        for (int i = 2; i < MAX_SAMPLES / 2; i++)
        {
            int freq = GetBucketFrequency(i-2);
            if (freq >= LOWEST_FREQ)
            {
                // Track the average and the peak value

                averageSum += _vReal[i];

                // If it's above the noise floor, figure out which band this belongs to and
                // if it's a new peak for that band, record that fact

                int iBand = GetBandIndex(freq);
                _vPeaks[iBand] += _vReal[i];
                hitCount[iBand]++;
            }
        }

        // Noise gate - if the signal in this band is below a threshold we define, then we say there's no energy in this band

        for (int i = 0; i < NUM_BANDS; i++)
        {
            _vPeaks[i] /= std::max(1, hitCount[i]);                       // max to avoid div by zero error
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
        debugV("All Bands Peak: %f", allBandsPeak);

        // Normalize all the bands relative to allBandsPeak
        for (int i = 0; i < NUM_BANDS; i++)
            _vPeaks[i] /= allBandsPeak;

        // We'll use the average as the gVU.  I assume the average of the samples tracks sound pressure level, but don't really know...

        float newval = averageSum / (MAX_SAMPLES / 2 - 2);
        debugV("AverageSumL: %f", averageSum);

        UpdateVU(newval);

        EVERY_N_MILLISECONDS(100)
        {
            auto peaks = GetPeakData();
            debugV("Audio Data -- Sum: %0.2f, _MinVU: %f0.2, _PeakVU: %f0.2, _VU: %f, Peak0: %f, Peak1: %f, Peak2: %f, Peak3: %f", averageSum, _MinVU, _PeakVU, _VU, peaks[0], peaks[1], peaks[2], peaks[3]);
        }
    
        return PeakData(_vPeaks);
    }

    //
    // Calculate a logrithmic scale for the bands like you would find on a graphic equalizer display
    //

    void CalculateBandCutoffs(float lowFreq, float highFreq)
    {
        if (NUM_BANDS == 16)
        {
            static int cutOffs16Band[16] =
                {200, 380, 580, 800, 980, 1200, 1360, 1584, 1996, 2412, 3162, 3781, 5312, 6310, 8400, (int)HIGHEST_FREQ};

            for (int i = 0; i < NUM_BANDS; i++)
                _cutOffsBand[i] = cutOffs16Band[i];
        }
        else
        {
            // uses geometric spacing to calculate the upper frequency for each of the 12 bands, starting with a frequency of 200 Hz
            // and ending with a frequency of 12.5 kHz. The spacing ratio r is calculated as the 11th root of the ratio of the maximum
            // frequency to the minimum frequency, and each upper frequency is calculated as f1 * r^(i+1).

            float f1 = LOWEST_FREQ;
            float f2 = HIGHEST_FREQ;
            float r = pow(f2 / f1, 1.0 / (NUM_BANDS - 1));
            for (int i = 0; i < NUM_BANDS; i++)
            {
                _cutOffsBand[i] = round(f1 * pow(r, i + 1));
                debugV("BAND %d: %d\n", i, _cutOffsBand[i]);
            }
        }
    }

    // BandCutoffTable
    //
    // Depending on how many bands we have, returns the cutoffs of where those bands are in the spectrum

    const int *BandCutoffTable(int bandCount)
    {
        return _cutOffsBand;
    }

public:
    SoundAnalyzer()
        : _sampling_period_us(PERIOD_FROM_FREQ(SAMPLING_FREQUENCY))
    {
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

    // flash record size, for recording 5 second
    void SampleBufferInitI2S()
    {
        // install and start i2s driver

        debugV("Begin SamplerBufferInitI2S...");

    #if M5STACKCORE2

        esp_err_t err = ESP_OK;

        i2s_driver_uninstall(Speak_I2S_NUMBER);  // Uninstall the I2S driver.  卸载I2S驱动
        i2s_config_t i2s_config =
        {
            .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX | I2S_MODE_PDM),
            .sample_rate = SAMPLING_FREQUENCY,  // Set the I2S sampling rate.
            .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,  // Fixed 12-bit stereo MSB.
            .channel_format = I2S_CHANNEL_FMT_ONLY_RIGHT,  // Set the channel format.
            .communication_format = I2S_COMM_FORMAT_STAND_I2S,  // Set the format of the communication.
            .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,  // Set the interrupt flag.
            .dma_buf_count = 2,        // DMA buffer count.
            .dma_buf_len = 256,        // DMA buffer length.
        };

        err += i2s_driver_install(Speak_I2S_NUMBER, &i2s_config, 0, NULL);

        i2s_pin_config_t tx_pin_config;
        tx_pin_config.mck_io_num = I2S_PIN_NO_CHANGE;
        tx_pin_config.bck_io_num = CONFIG_I2S_BCK_PIN;            // Link the BCK to the CONFIG_I2S_BCK_PIN pin.
        tx_pin_config.ws_io_num = CONFIG_I2S_LRCK_PIN;
        tx_pin_config.data_out_num = CONFIG_I2S_DATA_PIN;
        tx_pin_config.data_in_num = CONFIG_I2S_DATA_IN_PIN;
        err += i2s_set_pin(Speak_I2S_NUMBER, &tx_pin_config);  // Set the I2S pin number.
        err += i2s_set_clk(Speak_I2S_NUMBER, SAMPLING_FREQUENCY, I2S_BITS_PER_SAMPLE_16BIT, I2S_CHANNEL_MONO);  // Set the clock and bitwidth used by I2S Rx and Tx.

    #elif M5STICKC || M5STICKCPLUS

        i2s_config_t i2s_config =
        {
            .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX | I2S_MODE_PDM),
            .sample_rate = SAMPLING_FREQUENCY,
            .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT, // is fixed at 12bit, stereo, MSB
            .channel_format = I2S_CHANNEL_FMT_ALL_RIGHT,
            .communication_format = I2S_COMM_FORMAT_STAND_I2S, // Set the format of the communication.
            .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
            .dma_buf_count = 2,
            .dma_buf_len = 256,
        };

        i2s_pin_config_t pin_config;

        pin_config.mck_io_num = I2S_PIN_NO_CHANGE;
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
        i2s_config.use_apll = false;
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
        float decayAmount1 = std::max(0.0, g_AppTime.LastFrameTime() * g_peak1DecayRate);
        float decayAmount2 = std::max(0.0, g_AppTime.LastFrameTime() * g_peak2DecayRate);

        for (int iBand = 0; iBand < NUM_BANDS; iBand++)
        {
            g_peak1Decay[iBand] -= min(decayAmount1, g_peak1Decay[iBand]);
            g_peak2Decay[iBand] -= min(decayAmount2, g_peak2Decay[iBand]);
        }

        // Manual smoothing if desired 

        #if ENABLE_AUDIO_SMOOTHING
            for (int iBand = 1; iBand < NUM_BANDS - 1; iBand += 2)
            {
                g_peak1Decay[iBand] = (g_peak1Decay[iBand] + g_peak1Decay[iBand - 1] + g_peak1Decay[iBand + 1]) / 3;
                g_peak2Decay[iBand] = (g_peak2Decay[iBand] + g_peak2Decay[iBand - 1] + g_peak2Decay[iBand + 1]) / 3;
            }
        #endif
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
            #if M5STICKC || M5STICKCPLUS || M5STACKCORE2
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

            // Scale it so that its not always in the top red
            _MicMode = PeakData::PCREMOTE;
            UpdateVU(sum);
        }
    }
};
#endif

extern SoundAnalyzer g_Analyzer;
