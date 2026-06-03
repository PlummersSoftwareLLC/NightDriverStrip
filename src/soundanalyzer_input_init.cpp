//+--------------------------------------------------------------------------
//
// File:        soundanalyzer_input_init.cpp
//
// This file is part of soundanalyzer.cpp; see that file header for additional context.
//
// Split scope: SoundAnalyzer input backend initialization routines.
//---------------------------------------------------------------------------


#include "globals.h"

#include <algorithm>
#include <cmath>
#include <numeric>

#include "soundanalyzer.h"
#include "systemcontainer.h"
#include "values.h"

#if ENABLE_AUDIO

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
}

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
    const auto audioInputPin = GetConfiguredAudioInputPin();
    debugI("Audio: Initializing I2S Digital Mic (Modern) on BCLK:%d WS:%d DIN:%d", I2S_BCLK_PIN, I2S_WS_PIN, audioInputPin);
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
            .din = static_cast<gpio_num_t>(audioInputPin),
        },
    };

    ESP_ERROR_CHECK(i2s_channel_init_std_mode(_rx_handle, &std_cfg));
    ESP_ERROR_CHECK(i2s_channel_enable(_rx_handle));
#endif
}

void SoundAnalyzerBase::InitI2S_Legacy()
{
#if (USE_I2S_AUDIO || ELECROW) && !IS_IDF5
    const auto audioInputPin = GetConfiguredAudioInputPin();
    debugI("Audio: Initializing I2S Digital Mic (Legacy) on BCLK:%d WS:%d DIN:%d", I2S_BCLK_PIN, I2S_WS_PIN, audioInputPin);
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
    pinMode(audioInputPin, INPUT);

    const i2s_pin_config_t pin_config = {.bck_io_num = I2S_BCLK_PIN,
                                         .ws_io_num = I2S_WS_PIN,
                                         .data_out_num = I2S_PIN_NO_CHANGE,
                                         .data_in_num = audioInputPin};

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

#endif
