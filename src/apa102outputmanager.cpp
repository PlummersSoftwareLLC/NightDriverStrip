//+--------------------------------------------------------------------------
//
// File:        apa102outputmanager.cpp
//
// NightDriverStrip - (c) 2026 Plummer's Software LLC.  All Rights Reserved.
//
// APA102/SK9822 runtime output manager. Uses the ESP32 hardware SPI master
// with DMA so the render task hands the prepared frame off in one shot
// instead of toggling GPIOs per bit. One SPI host per channel (SPI2_HOST,
// then SPI3_HOST), so at most two simultaneous APA102 channels.
//
//---------------------------------------------------------------------------

#include "globals.h"

#if USE_APA102

#include "apa102outputmanager.h"

#include <algorithm>
#include <cstring>
#include <esp_heap_caps.h>
#include <driver/spi_common.h>

#include "gfxbase.h"
#include "pixelformat.h"
#include "ws281xgfx.h"

namespace
{
    #ifndef APA102_GLOBAL_BRIGHTNESS
        #define APA102_GLOBAL_BRIGHTNESS 31
    #endif

    #ifndef APA102_SPI_HZ
        // 10 MHz is comfortable for both APA102 and SK9822 across typical wiring.
        // Bump per-env if your harness is short and clean.
        #define APA102_SPI_HZ (10 * 1000 * 1000)
    #endif

    constexpr uint8_t ClampGlobalBrightness(uint8_t brightness)
    {
        if (brightness == 0)
            return 0;

        if (brightness > 31)
            return 31;

        return brightness;
    }

    constexpr uint8_t kGlobalBrightness = ClampGlobalBrightness(APA102_GLOBAL_BRIGHTNESS);

    constexpr size_t kStartFrameBytes = 4;
    constexpr size_t kBytesPerPixel   = 4;

    constexpr size_t EndFrameBytes(size_t ledCount)
    {
        // APA102 needs at least ledCount / 2 extra clock pulses after the pixel
        // frames; use ceil(N/16) bytes with a small fixed floor for short strips.
        const size_t needed = (ledCount + 15) / 16;
        return needed < 4 ? 4 : needed;
    }

    constexpr size_t FrameBufferSize(size_t ledCount)
    {
        return kStartFrameBytes + kBytesPerPixel * ledCount + EndFrameBytes(ledCount);
    }

    constexpr spi_host_device_t HostForChannel(size_t channelIndex)
    {
        return channelIndex == 0 ? SPI2_HOST : SPI3_HOST;
    }

    void LogRuntimeAPA102Configuration(const DeviceConfig& config, const std::vector<std::shared_ptr<GFXBase>>& devices, const char* reason)
    {
        debugI("APA102 config (%s): path=spi-dma@%uHz driver=%s channels=%zu matrix=%ux%u serpentine=%d colorOrder=%s leds=%zu",
               reason ? reason : "update",
               static_cast<unsigned>(APA102_SPI_HZ),
               config.GetRuntimeDriverName().c_str(),
               config.GetChannelCount(),
               static_cast<unsigned>(config.GetMatrixWidth()),
               static_cast<unsigned>(config.GetMatrixHeight()),
               config.IsMatrixSerpentine(),
               DeviceConfig::GetColorOrderName(config.GetWS281xColorOrder()).c_str(),
               config.GetActiveLEDCount());

        const auto& dataPins = config.GetAPA102DataPins();
        const auto& clockPins = config.GetAPA102ClockPins();
        for (size_t channel = 0; channel < config.GetChannelCount() && channel < devices.size(); ++channel)
        {
            const auto& graphics = *devices[channel];
            debugI("APA102 channel %zu: host=SPI%d data=%d clock=%d leds=%zu matrix=%ux%u serpentine=%d buffer=%p",
                   channel,
                   HostForChannel(channel) == SPI2_HOST ? 2 : 3,
                   dataPins[channel],
                   clockPins[channel],
                   graphics.GetLEDCount(),
                   static_cast<unsigned>(graphics.GetMatrixWidth()),
                   static_cast<unsigned>(graphics.GetMatrixHeight()),
                   graphics.IsSerpentine(),
                   graphics.leds);
        }
    }
}

APA102OutputManager::APA102OutputManager()
{
}

APA102OutputManager::~APA102OutputManager()
{
    Reset();
}

void APA102OutputManager::Reset()
{
    std::lock_guard guard(WS281xGFX::TransportMutex());

    for (size_t i = 0; i < _channels.size(); ++i)
        ReleaseChannel(i);

    _activeChannelCount = 0;
    _activeLEDCount = 0;
    _colorOrder = DeviceConfig::GetCompiledWS281xColorOrder();
}

bool APA102OutputManager::ConfigureChannel(size_t channelIndex, int8_t dataPin, int8_t clockPin, size_t ledCount, String* errorMessage)
{
    auto& state = _channels[channelIndex];

    const size_t requiredBytes = FrameBufferSize(ledCount);
    const bool pinsChanged   = state.dataPin != dataPin || state.clockPin != clockPin;
    const bool bufferTooSmall = state.bufferSize < requiredBytes;

    // If the bus is already initialized but anything material changed, tear it down before re-binding.
    if (state.active && (pinsChanged || bufferTooSmall))
        ReleaseChannel(channelIndex);

    if (!state.active)
    {
        state.host = HostForChannel(channelIndex);

        spi_bus_config_t busCfg = {};
        busCfg.mosi_io_num     = dataPin;
        busCfg.miso_io_num     = -1;
        busCfg.sclk_io_num     = clockPin;
        busCfg.quadwp_io_num   = -1;
        busCfg.quadhd_io_num   = -1;
        busCfg.max_transfer_sz = static_cast<int>(requiredBytes);

        esp_err_t err = spi_bus_initialize(state.host, &busCfg, SPI_DMA_CH_AUTO);
        if (err != ESP_OK)
        {
            if (errorMessage)
                *errorMessage = String("spi_bus_initialize failed: ") + esp_err_to_name(err);
            return false;
        }

        spi_device_interface_config_t devCfg = {};
        devCfg.clock_speed_hz = APA102_SPI_HZ;
        devCfg.mode           = 0;          // CPOL=0, CPHA=0 — APA102/SK9822
        devCfg.spics_io_num   = -1;         // no chip select
        devCfg.queue_size     = 1;
        devCfg.flags          = SPI_DEVICE_NO_DUMMY;

        err = spi_bus_add_device(state.host, &devCfg, &state.device);
        if (err != ESP_OK)
        {
            spi_bus_free(state.host);
            state.device = nullptr;
            if (errorMessage)
                *errorMessage = String("spi_bus_add_device failed: ") + esp_err_to_name(err);
            return false;
        }

        state.buffer = static_cast<uint8_t*>(heap_caps_malloc(requiredBytes, MALLOC_CAP_DMA | MALLOC_CAP_INTERNAL));
        if (!state.buffer)
        {
            spi_bus_remove_device(state.device);
            spi_bus_free(state.host);
            state.device = nullptr;
            if (errorMessage)
                *errorMessage = "APA102 DMA buffer allocation failed";
            return false;
        }

        state.bufferSize = requiredBytes;
        state.dataPin    = dataPin;
        state.clockPin   = clockPin;
        state.active     = true;
    }

    state.ledCount = ledCount;

    // Pre-fill the start frame (zeros) and end frame (0xFF padding); Show() only fills the pixel bytes.
    std::memset(state.buffer, 0x00, kStartFrameBytes);
    const size_t endOffset = kStartFrameBytes + kBytesPerPixel * ledCount;
    std::memset(state.buffer + endOffset, 0xFF, requiredBytes - endOffset);

    return true;
}

void APA102OutputManager::ReleaseChannel(size_t channelIndex)
{
    auto& state = _channels[channelIndex];

    if (state.device)
    {
        spi_bus_remove_device(state.device);
        state.device = nullptr;
        spi_bus_free(state.host);
    }

    if (state.buffer)
    {
        heap_caps_free(state.buffer);
        state.buffer = nullptr;
    }

    state.bufferSize = 0;
    state.dataPin    = -1;
    state.clockPin   = -1;
    state.ledCount   = 0;
    state.active     = false;
}

SuccessResultWithMessage APA102OutputManager::ApplyConfig(const DeviceConfig& config, const std::vector<std::shared_ptr<GFXBase>>& devices)
{
    std::lock_guard guard(WS281xGFX::TransportMutex());

    if (config.GetOutputDriver() != DeviceConfig::OutputDriver::APA102)
        return { false, "recompile needed" };

    const size_t requestedChannels = std::min(config.GetChannelCount(), devices.size());
    if (requestedChannels > kMaxChannels)
        return { false, String("APA102 hardware-SPI driver supports at most ") + kMaxChannels + " channels" };

    const size_t ledCount  = config.GetActiveLEDCount();
    const auto& dataPins   = config.GetAPA102DataPins();
    const auto& clockPins  = config.GetAPA102ClockPins();

    for (size_t i = 0; i < _channels.size(); ++i)
    {
        const bool shouldBeActive = i < requestedChannels;
        if (!shouldBeActive)
        {
            ReleaseChannel(i);
            continue;
        }

        auto& state = _channels[i];
        if (!state.active || state.dataPin != dataPins[i] || state.clockPin != clockPins[i] || state.ledCount != ledCount)
        {
            String errorMessage;
            if (!ConfigureChannel(i, dataPins[i], clockPins[i], ledCount, &errorMessage))
                return { false, errorMessage };
        }
    }

    _activeChannelCount = requestedChannels;
    _activeLEDCount     = ledCount;
    _colorOrder         = config.GetWS281xColorOrder();

    LogRuntimeAPA102Configuration(config, devices, "apply");
    return { true, "" };
}

//
// Show()
//
// Render the given pixel data to the LED strip(s) using the APA102 protocol over hardware SPI + DMA.
// Caller must already hold the transport mutex (or rely on the one we acquire here). The pre-built
// frame buffer for each channel already contains the start frame (zeros) and end frame (0xFF
// padding); we only fill the per-pixel bytes between them, then hand the whole buffer to the SPI
// master in a single transaction.
//
void APA102OutputManager::Show(const std::vector<std::shared_ptr<GFXBase>>& devices, uint16_t pixelsDrawn, uint8_t brightness, uint8_t fader)
{
    std::lock_guard guard(WS281xGFX::TransportMutex());

    if (_activeChannelCount == 0 || _activeLEDCount == 0)
        return;

    const size_t pixelsToShow = std::min(static_cast<size_t>(pixelsDrawn), _activeLEDCount);
    const auto showStartMicros = micros();

    for (size_t channelIndex = 0; channelIndex < _activeChannelCount && channelIndex < devices.size(); ++channelIndex)
    {
        auto& state = _channels[channelIndex];
        if (!state.active || !state.device || !state.buffer)
            continue;

        auto& device = devices[channelIndex];
        const size_t ledCount = std::min(state.ledCount, device->GetLEDCount());
        const auto indices = PixelFormatHelpers::IndicesFor(_colorOrder);

        uint8_t* p = state.buffer + kStartFrameBytes;
        for (size_t i = 0; i < ledCount; ++i)
        {
            CRGB color = (i < pixelsToShow) ? device->leds[i] : CRGB::Black;
            uint8_t wire[3] = {};
            wire[indices.rIdx] = PixelFormatHelpers::Scale(color.r, brightness, fader);
            wire[indices.gIdx] = PixelFormatHelpers::Scale(color.g, brightness, fader);
            wire[indices.bIdx] = PixelFormatHelpers::Scale(color.b, brightness, fader);

            p[0] = 0xE0 | kGlobalBrightness;
            p[1] = wire[0];
            p[2] = wire[1];
            p[3] = wire[2];
            p += kBytesPerPixel;
        }

        spi_transaction_t txn = {};
        txn.length    = state.bufferSize * 8;  // bits
        txn.tx_buffer = state.buffer;

        const esp_err_t err = spi_device_transmit(state.device, &txn);
        if (err != ESP_OK)
        {
            debugW("APA102 spi_device_transmit failed on channel %zu: %s", channelIndex, esp_err_to_name(err));
        }
    }

    const auto showElapsedMicros = micros() - showStartMicros;
    if (showElapsedMicros > 50000UL)
    {
        debugW("APA102 show slow: channels=%zu leds=%zu elapsed=%lu us",
               _activeChannelCount,
               _activeLEDCount,
               static_cast<unsigned long>(showElapsedMicros));
    }
}

#endif
