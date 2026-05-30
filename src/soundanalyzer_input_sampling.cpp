//+--------------------------------------------------------------------------
//
// File:        soundanalyzer_input_sampling.cpp
//
// This file is part of soundanalyzer.cpp; see that file header for additional context.
//
// Split scope: SoundAnalyzer sampling routines for M5, I2S, and ADC backends.
//---------------------------------------------------------------------------


#include "globals.h"

#include <algorithm>
#include <cmath>
#include <numeric>

#include "soundanalyzer.h"
#include "systemcontainer.h"
#include "values.h"

#if ENABLE_AUDIO

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
    if (err != ESP_OK)
        return 0;

    for (int i = 0; i < MAX_SAMPLES; i++)
    {
        if (i * kChannels >= (bytesRead / 4))
            break;
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
    esp_err_t err = adc_continuous_read(_adc_handle, (uint8_t *)ptrSampleBuffer.get(), bytesToRead, (uint32_t *)&ret_num, 0);

    if (err == ESP_OK)
    {
        for (int i = 0; i < MAX_SAMPLES; i++)
        {
            if (i * 2 >= ret_num)
                break;
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

#endif
