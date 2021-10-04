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
//				Apr-20-2019			Davepl		Adapted from Spectrum Analyzer
//
//---------------------------------------------------------------------------

#pragma once

#include <sys/types.h>
#include <errno.h>
#include <iostream>
#include <vector>
#include <deque>
#include <algorithm>
#include <math.h>
#include <arduinoFFT.h>
#include "globals.h"
#include "network.h"

#include "driver/i2s.h"
#include "driver/adc.h"

#if !ENABLE_AUDIO
#error You must have ENABLE_AUDIO set true to include this header file, as it relies on audio hardware
#endif

void IRAM_ATTR AudioSamplerTaskEntry(void *);

extern float gScaler;					  // Instanteous read of LED display vertical scaling
extern float gLogScale;					  // How exponential the peaks are made to be
extern volatile float gVURatio;			  // Current VU as a ratio to its recent min and max
extern volatile float gVU;		   	      // Instantaneous read of VU value
extern volatile float gPeakVU;		      // How high our peak VU scale is in live mode
extern volatile float gMinVU;		      // How low our peak VU scale is in live mode
extern volatile unsigned long g_cSamples; // Total number of samples successfully collected
extern int g_AudioFPS;					  // Framerate of the audio sampler

#if !ENABLE_AUDIO
extern volatile float gVURatio;			
#endif

#if ENABLE_AUDIO 

#if ENABLE_WEBSERVER
#include "spiffswebserver.h"       		   // Handle web server requests
extern CSPIFFSWebServer g_WebServer;
#endif

#define USE_I2S 1

using namespace std;

// PeakData class
//
// Simple data class that holds the music peaks for up to 32 bands.  When the sound analyzer finishes a pass, its
// results are simplified down to this small class of band peaks.

#define SUPERSAMPLES 2									  // How many supersamples to take
#define SAMPLE_BITS  12									  // Sample resolution (0-4095)
#define MAX_ANALOG_IN ((1 << SAMPLE_BITS) * SUPERSAMPLES) // What our max analog input value is on all analog pins (4096 is default 12 bit resolution)
#define MAX_VU MAX_ANALOG_IN
#define MIN_VU 8

#ifndef GAINDAMPEN
#define GAINDAMPEN  25									  // How slowly brackets narrow in for spectrum bands
#endif

#ifndef VUDAMPEN
#define VUDAMPEN    20 									  // How slowly VU reacts
#endif

#define VUDAMPENMIN 100  								  // How slowly VU min creeps up to test noise floor
#define VUDAMPENMAX 25								      // How slowly VU max drops down to test noise ceiling

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
		for (int band = 0; band < 4; band++)
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

			// If new peak is below the min, it becaomes the new min. Otherwise we drift
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
				_Ratio[band] = (_Max[band]-_Min[band]) / denominator;
		}

		debugV("Min:    %f, %f, %f, %f", _Min[0], _Min[1], _Min[2], _Min[3] );
		debugV("Max:    %f, %f, %f, %f", _Max[0], _Max[1], _Max[2], _Max[3] );
		debugV("Level:  %f, %f, %f, %f", _Level[0], _Level[1], _Level[2], _Level[3] );
		debugV("Ratio:  %f, %f, %f, %f", _Ratio[0], _Ratio[1], _Ratio[2], _Ratio[3] );
		debugV("======");
	}

  public:
    
	PeakData()
    {
        for (int i = 0; i < NUM_BANDS; i++)
            _Level[i] = _Ratio[i] = 0.0f;
    }

	PeakData(float * pFloats)
	{
        for (int i = 0; i < NUM_BANDS; i++)
            _Level[i] =_Ratio[i] = 0.0f;

		SetData(pFloats);
	}

	PeakData & operator=(const PeakData & other)
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

	float Ratio(std::size_t n) const
	{
		return _Ratio[n];
	}
	
	float Max(std::size_t n) const
	{
		return _Max[n];
	}

	void SetData(float * pFloats)
	{
        for (int i = 0; i < NUM_BANDS; i++)
		{
			_Level[i] = pFloats[i];
		}
		UpdateMinMax();
	}
};

extern PeakData g_Peaks;
const size_t    MAX_SAMPLES		   = 256;
const size_t    SAMPLING_FREQUENCY = 44100;

// SampleBuffer
//
// Contains the actual samples; This used to be fairly complicated and used frequent IRQs
// until I switched to I2S, which made things a lot simpler

class SampleBuffer
{
  private:
	arduinoFFT		  _FFT;                 // Perhaps could be static, but might have state info, so one per buffer
	size_t            _MaxSamples;          // Number of samples we will take, must be a power of 2
	size_t            _SamplingFrequency;   // Sampling Frequency should be at least twice that of highest freq sampled
	size_t            _BandCount;
	float			* _vPeaks; 
	int				  _InputPin;
	static float      _oldVU;
	static float      _oldPeakVU;
	static float      _oldMinVU;
	portMUX_TYPE	  _mutex;

	// BucketFrequency
	//
	// Return the frequency corresponding to the Nth sample bucket.  Skips the first two 
	// buckets which are overall amplitude and something else.

	int BucketFrequency(int iBucket) const
	{
		if (iBucket <= 1)
			return 0;

		int iOffset = iBucket - 2;
		return iOffset * (_SamplingFrequency / 2) / (_MaxSamples / 2);
	}

	// BandCutoffTable
	//
	// Depending on how many bands we have, returns the cutoffs of where those bands are in the spectrum

	const int * BandCutoffTable(int bandCount);				
	const float * GetBandScalars(int bandCount);

  public:

	volatile int	  _cSamples;
	double			* _vReal;
	double			* _vImaginary;

	SampleBuffer(size_t MaxSamples, size_t BandCount, size_t SamplingFrequency, int InputPin)
	{
		_BandCount		   = BandCount;
		_SamplingFrequency = SamplingFrequency;
		_MaxSamples        = MaxSamples;
		_InputPin          = InputPin;

		_vReal			   = (double *) malloc(MaxSamples * sizeof(_vReal[0]));
		_vImaginary		   = (double *) malloc(MaxSamples * sizeof(_vImaginary[0]));
		_vPeaks			   = (float *)  malloc(BandCount  * sizeof(_vPeaks[0]));

		_oldVU			   = 0.0f;
		_oldPeakVU         = 0.0f;
		_oldMinVU          = 0.0f;
		_mutex = portMUX_INITIALIZER_UNLOCKED;
		vPortCPUInitializeMutex(&_mutex);

#if USE_I2S
		SampleBufferInitI2S();
#endif

		Reset();
	}
	~SampleBuffer()
	{
		free(_vReal);
		free(_vImaginary);
		free(_vPeaks);
	}

	bool TryForImmediateLock()
	{
		return vPortCPUAcquireMutexTimeout(&_mutex, portMUX_TRY_LOCK);
	}

	void WaitForLock() 
	{
		vPortCPUAcquireMutex(&_mutex);
	}

	void ReleaseLock()
	{
		vPortCPUReleaseMutex(&_mutex);
	}
    
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
		#if SHOW_FFT_TIMING
		unsigned long fftStart = millis();
		#endif

		_FFT.Windowing(_vReal, _MaxSamples, FFT_WIN_TYP_RECTANGLE, FFT_FORWARD);
		_FFT.Compute(_vReal, _vImaginary, _MaxSamples, FFT_FORWARD);			// REVIEW(davepl)
		_FFT.ComplexToMagnitude(_vReal, _vImaginary, _MaxSamples);              // This and MajorPeak may only actually need _MaxSamples/2
        _FFT.MajorPeak(_vReal, _MaxSamples, _SamplingFrequency);                //   but I can't tell, and it was no perf win when I tried it.

		#if SHOW_FFT_TIMING
		debugV("FFT took %ld ms at %d FPS", millis() - fftStart, FPS(fftStart, millis()));
		#endif
	}
	
	inline bool IsBufferFull() const __attribute__((always_inline))
	{
		return (_cSamples >= _MaxSamples);
	}

#if USE_I2S

	// SampleBuffferInitI2S
	// 
	// Do the initial setup required if we're going to capture the audio with I2S

	//i2s number
	#define EXAMPLE_I2S_NUM           (I2S_NUM_0)
	//i2s sample rate
	#define EXAMPLE_I2S_SAMPLE_BITS   (I2S_COMM_FORMAT_I2S | I2S_COMM_FORMAT_I2S_MSB)
	//enable display buffer for debug
	#define EXAMPLE_I2S_BUF_DEBUG     (0)
	//I2S read buffer length
	#define EXAMPLE_I2S_READ_LEN      (MAX_SAMPLES)
	//I2S data format
	#define EXAMPLE_I2S_FORMAT        (I2S_CHANNEL_FMT_RIGHT_LEFT)
	//I2S channel number
	#define EXAMPLE_I2S_CHANNEL_NUM   ((EXAMPLE_I2S_FORMAT < I2S_CHANNEL_FMT_ONLY_RIGHT) ? (2) : (1))
	//I2S built-in ADC unit
	#define I2S_ADC_UNIT              ADC_UNIT_1
	//I2S built-in ADC channel
	#define I2S_ADC_CHANNEL           ADC1_CHANNEL_0

	//flash record size, for recording 5 second
	void SampleBufferInitI2S()
	{
		//install and start i2s driver

		debugV("Begin SamplerBufferInitI2S...");

		#if M5STICKC || M5STICKCPLUS
			i2s_config_t i2s_config = {
				.mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX | I2S_MODE_PDM),
				.sample_rate =  SAMPLING_FREQUENCY,
				.bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT, // is fixed at 12bit, stereo, MSB
				.channel_format = I2S_CHANNEL_FMT_ALL_RIGHT,
				.communication_format = I2S_COMM_FORMAT_I2S,
				.intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
				.dma_buf_count = 2,
				.dma_buf_len = 256,
			};

			i2s_pin_config_t pin_config;
			pin_config.bck_io_num   = I2S_PIN_NO_CHANGE;
			pin_config.ws_io_num    = IO_PIN;
			pin_config.data_out_num = I2S_PIN_NO_CHANGE;
			pin_config.data_in_num  = INPUT_PIN;
				
			
			i2s_driver_install(I2S_NUM_0, &i2s_config, 0, NULL);
			i2s_set_pin(I2S_NUM_0, &pin_config);
			i2s_set_clk(I2S_NUM_0, SAMPLING_FREQUENCY, I2S_BITS_PER_SAMPLE_16BIT, I2S_CHANNEL_MONO);

		#else
			i2s_config_t i2s_config;
			i2s_config.mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX | I2S_MODE_ADC_BUILT_IN);
			i2s_config.sample_rate = SAMPLING_FREQUENCY;            
			i2s_config.dma_buf_len = MAX_SAMPLES;                   
			i2s_config.bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT;
			i2s_config.channel_format = I2S_CHANNEL_FMT_ONLY_LEFT;  
			i2s_config.use_apll = false,
			i2s_config.communication_format = I2S_COMM_FORMAT_I2S_MSB;
			i2s_config.intr_alloc_flags = ESP_INTR_FLAG_LEVEL1;
			i2s_config.dma_buf_count = 2;

			ESP_ERROR_CHECK( adc_gpio_init(ADC_UNIT_1, ADC_CHANNEL_0) );

			ESP_ERROR_CHECK( i2s_driver_install(EXAMPLE_I2S_NUM, &i2s_config,  0, NULL) );
			ESP_ERROR_CHECK( i2s_set_adc_mode(I2S_ADC_UNIT, I2S_ADC_CHANNEL) );		
		#endif
		debugV("SamplerBufferInitI2S Complete\n");
	}


	void FillBufferI2S()
	{
		int16_t byteBuffer[MAX_SAMPLES];		

		if (IsBufferFull())
		{
			debugW("BUG: FillBUfferI2S found buffer already full.");
			return;
		}
		size_t bytesRead = 0;

		#if M5STICKC || M5STICKCPLUS
		// REVIEW(davepl) - If I do not indicate one less byte of length, i2s_read will corrupt the heap
			ESP_ERROR_CHECK( i2s_read(I2S_NUM_0, (void*) byteBuffer, sizeof(byteBuffer), &bytesRead, (100 / portTICK_RATE_MS)) );
		#else
			ESP_ERROR_CHECK( i2s_adc_enable(EXAMPLE_I2S_NUM) );
			ESP_ERROR_CHECK( i2s_read(EXAMPLE_I2S_NUM, (void*) byteBuffer, sizeof(byteBuffer), &bytesRead, portMAX_DELAY) );
			ESP_ERROR_CHECK( i2s_adc_disable(EXAMPLE_I2S_NUM) );
		#endif

		_cSamples = _MaxSamples;
		if (bytesRead != sizeof(byteBuffer))
		{
			debugW("Could only read %u bytes of %u in FillBufferI2S()\n", bytesRead, sizeof(byteBuffer));
			return;
		}

		for (int i = 0; i < ARRAYSIZE(byteBuffer); i++)
		{
			#if M5STICKC || M5STICKCPLUS
				_vReal[i] = ::map(byteBuffer[i], INT16_MIN, INT16_MAX, 0, MAX_VU);
			#else
				_vReal[i] = byteBuffer[i];
			#endif

		}
	}
#endif

    // SampleBuffer::AcquireSampleAnalogRead
    //
    // IRQ calls here through the IRQ stub

	void AcquireSampleAnalogRead()
	{
		if (false == IsBufferFull())
		{ 
			//_vReal[_cSamples] = adcEnd(_InputPin);
			_vReal[_cSamples] = analogRead(_InputPin);
			_vImaginary[_cSamples] = 0;
			_cSamples++;
			g_cSamples++;

			if (_cSamples % 16 == 0)
				delay(1);
		}
	}

	void FillBufferAnalogRead()
	{
		while (false == IsBufferFull())
			AcquireSampleAnalogRead();
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

			if (_vReal[i] > NOISE_CUTOFF)
			{
				int freq = BucketFrequency(i);

				int iBand = 0;
				while (iBand < _BandCount - 1)
				{
					if (freq < BandCutoffTable(_BandCount)[iBand])
						break;
					iBand++;
				}
				if (iBand > _BandCount-1)
					iBand = _BandCount-1;
				if (_vReal[i] > _vPeaks[iBand])
					_vPeaks[iBand] = _vReal[i];
			}
		}

		for (int i = 0; i < _BandCount; i++)
		    _vPeaks[i] *= GetBandScalars(_BandCount)[i];

		// If you want the peaks to be a lot more prominent, you can exponentially raise the values
		// and then they'll be scaled back down linearly, but you'd have to adjust allBandsPeak 
		// accordingly as well as the value there now is based on no exponential scaling.
		//
		//	_vPeaks[i] = powf(_vPeaks[i], 2.0);

		float allBandsPeak = 0;
		for (int i = 0; i < _BandCount; i++)
			allBandsPeak = max(allBandsPeak, _vPeaks[i]);			
		
		// It's hard to know what to use for a "minimum" volume so I aimed for a light ambient noise background
		// just triggering the bottom pixel, and real silence yielding darkness
		
		debugV("All Bands Peak: %f", allBandsPeak);
		allBandsPeak = max(NOISE_FLOOR, allBandsPeak);				

		for (int i = 0; i < _BandCount; i++)
			_vPeaks[i] /= allBandsPeak;


		// We'll use the average as the gVU.  I assume the average of the samples tracks sound pressure level, but don't really know...

		float newval = averageSum / (_MaxSamples / 2 - 2);
		debugV("AverageSumL: %f", averageSum);

		if (newval > _oldVU)
			gVU = newval;
		else
			gVU = (_oldVU * VUDAMPEN + newval) / (VUDAMPEN + 1);
	
		_oldVU = gVU;

		debugV("PEAK: %lf, VU: %f", samplesPeak, gVU);

		// If we crest above the max VU, update the max VU up to that.  Otherwise drift it towards the new value.

		if (gVU > gPeakVU)
			gPeakVU = gVU; 
		else
			gPeakVU = (_oldPeakVU * VUDAMPENMAX + gVU) / (VUDAMPENMAX+1);	
		_oldPeakVU = gPeakVU;

		// If we dip below the min VU, update the min VU down to that.  Otherwise drift it towards the new value.

		if (gVU < gMinVU)
			gMinVU = gVU; 
		else
			gMinVU = (_oldMinVU * VUDAMPENMIN + gVU) / (VUDAMPENMIN+1);
		_oldMinVU = gMinVU;

		EVERY_N_MILLISECONDS(100)
		{
			debugV("Audio Data -- Sum: %0.2f, gMinVU: %f0.2, gPeakVU: %f0.2, gVU: %f, Peak0: %f, Peak1: %f, Peak2: %f, Peak3: %f", averageSum, gMinVU, gPeakVU, gVU, g_Peaks[0], g_Peaks[1], g_Peaks[2], g_Peaks[3]);
		}

		return GetBandPeaks();
	}
    
    // SampleBuffer::GetBandPeaks
    //
    // Once the FFT processing is complete you can call this function to get a copy of what each of the
    // peaks in the various bands is

	PeakData GetBandPeaks()
	{
		return PeakData(_vPeaks);
	}

};

// SoundAnalyzer
//
// The SoundAnalyzer class uses I2S to read samples from the microphone and then runs an FFT on the
// results to generate the peaks in each band, as well as tracking an overall VU and VU ratio, the
// latter being the reatio of the current VU to the trailing min and max VU.

class SoundAnalyzer
{
  private:

	SampleBuffer    _buffer;																// A front buffer and a back buffer
	unsigned int	_sampling_period_us = PERIOD_FROM_FREQ(SAMPLING_FREQUENCY);
	uint8_t			_inputPin;																// Which hardware pin do we actually sample audio from?
															
  public:

	SoundAnalyzer(uint8_t inputPin)
		: _buffer(MAX_SAMPLES, NUM_BANDS, SAMPLING_FREQUENCY, INPUT_PIN),
 		  _sampling_period_us(PERIOD_FROM_FREQ(SAMPLING_FREQUENCY)),
		  _inputPin(inputPin)
	{
	}

	//
    // RunSamplerPass
    //

    PeakData RunSamplerPass()
	{
		_buffer.Reset();
		_buffer.FillBufferI2S();
    	_buffer.FFT();

		return _buffer.ProcessPeaks();
	}
};

#endif // ENABLE_AUDIO
