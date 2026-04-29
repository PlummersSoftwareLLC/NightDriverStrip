//+--------------------------------------------------------------------------
//
// File:        ws281xoutputmanager.cpp
//
// NightDriverStrip - (c) 2018 Plummer's Software LLC.  All Rights Reserved.
//
// ESP32 runtime WS281x output manager. This is the small indirection layer that
// lets us change pins/channel count/live LED count without changing effect code.
//
// The runtime transport intentionally uses the ESP-IDF RMT driver directly
// instead of FastLED's ESP32 transport. FastLED remains responsible for CRGB
// color handling in the effect layer, while NightDriver owns the mutable RMT
// channel/pin configuration required for live topology/output changes.
//
//---------------------------------------------------------------------------

#include "globals.h"

#if USE_WS281X

#if defined(CONFIG_RMT_SUPPRESS_DEPRECATE_WARN)
#undef CONFIG_RMT_SUPPRESS_DEPRECATE_WARN
#endif
#define CONFIG_RMT_SUPPRESS_DEPRECATE_WARN 1

#include "ws281xoutputmanager.h"

#include <algorithm>
#include <cstdint>

#include <esp_idf_version.h>
#include <driver/rmt.h>
#include <esp_err.h>
#include <esp_task_wdt.h>

// This runtime transport intentionally targets the ESP-IDF legacy RMT API.
// We depend on the Arduino/IDF compatibility layer because the sender uses
// the legacy translator-based TX path (`rmt_translator_init`,
// `rmt_write_sample`, `rmt_wait_tx_done`) rather than the newer IDF5 encoder
// API. If that shim is unavailable, this transport cannot be built.

#ifndef RMT_DEFAULT_CONFIG_TX
    #error "NightDriverStrip WS281x runtime transport requires the ESP-IDF legacy RMT API compatibility layer."
#endif

#include "gfxbase.h"
#include "ws281xgfx.h"

namespace
{
    static_assert(NUM_CHANNELS <= 8, "ESP32 RMT path supports up to 8 WS281x channels");

    // Legacy RMT uses a simple APB-clock divider model. With 80MHz / 2 we get
    // 25ns ticks, which is fine-grained enough to represent WS2812 timings with
    // integer durations.

    constexpr uint8_t kRmtClockDivider = 2; // 80MHz APB / 2 = 40MHz => 25ns ticks
    constexpr uint8_t kRmtMemoryBlocksPerChannel = 1;
    constexpr TickType_t kRmtWaitTimeout = pdMS_TO_TICKS(100);

    // FastLED already carries the WS2812 timing constants we want.  We reuse
    // those values here, but the actual transport and GPIO ownership remain in
    // NightDriver's runtime manager rather than in FastLED's controller layer.

    constexpr uint32_t kWs2812T0HighNs = FASTLED_WS2812_T1;
    constexpr uint32_t kWs2812T0LowNs = FASTLED_WS2812_T2 + FASTLED_WS2812_T3;
    constexpr uint32_t kWs2812T1HighNs = FASTLED_WS2812_T1 + FASTLED_WS2812_T2;
    constexpr uint32_t kWs2812T1LowNs = FASTLED_WS2812_T3;

    constexpr uint16_t NsToRmtTicks(uint32_t nanoseconds)
    {
        constexpr uint32_t kTickNs = 25;
        return static_cast<uint16_t>((nanoseconds + (kTickNs - 1)) / kTickNs);
    }

    constexpr rmt_item32_t MakeRmtItem(uint16_t highTicks, uint16_t lowTicks)
    {
        rmt_item32_t item{};
        item.level0 = 1;
        item.duration0 = highTicks;
        item.level1 = 0;
        item.duration1 = lowTicks;
        return item;
    }

    const DRAM_ATTR rmt_item32_t kBitZero = MakeRmtItem(NsToRmtTicks(kWs2812T0HighNs), NsToRmtTicks(kWs2812T0LowNs));
    const DRAM_ATTR rmt_item32_t kBitOne  = MakeRmtItem(NsToRmtTicks(kWs2812T1HighNs), NsToRmtTicks(kWs2812T1LowNs));

    // The translator is called by the legacy RMT driver as it needs more items.
    // It consumes raw GRB/RGB/etc. bytes and expands each bit into one timing
    // item, so the higher layers only need to provide packed color bytes.

    void IRAM_ATTR WS2812ByteTranslator(const void* src, rmt_item32_t* dest, size_t srcSize, size_t wantedNum, size_t* translatedSize, size_t* itemNum)
    {
        const auto* bytes = static_cast<const uint8_t*>(src);
        const size_t maxBytesByItems = wantedNum / 8;
        const size_t maxBytes = srcSize < maxBytesByItems ? srcSize : maxBytesByItems;

        size_t outputItems = 0;
        size_t consumedBytes = 0;
        while (consumedBytes < maxBytes)
        {
            const uint8_t value = bytes[consumedBytes];
            for (int bit = 7; bit >= 0; --bit)
                dest[outputItems++] = (value & (1U << bit)) ? kBitOne : kBitZero;
            ++consumedBytes;
        }

        *translatedSize = consumedBytes;
        *itemNum = outputItems;
    }

    template <uint8_t RIndex, uint8_t GIndex, uint8_t BIndex>
    void PackChannelPixels(uint8_t* output,
                           const CRGB* source,
                           size_t activeLedCount,
                           size_t pixelsToShow,
                           uint8_t brightness,
                           uint8_t fader)
    {
        for (size_t pixelIndex = 0; pixelIndex < activeLedCount; ++pixelIndex)
        {
            CRGB color = pixelIndex < pixelsToShow ? source[pixelIndex] : CRGB::Black;

            // Preserve the compiled-path semantics without first mutating the effect buffer:
            // scale by configured brightness, then apply the global master fader.
            nscale8x3_video(color.r, color.g, color.b, brightness);
            nscale8x3_video(color.r, color.g, color.b, fader);

            const size_t offset = pixelIndex * 3;
            output[offset + RIndex] = color.r;
            output[offset + GIndex] = color.g;
            output[offset + BIndex] = color.b;
        }
    }

    void PackChannelPixelsForColorOrder(uint8_t* output,
                                        const CRGB* source,
                                        size_t activeLedCount,
                                        size_t pixelsToShow,
                                        uint8_t brightness,
                                        uint8_t fader,
                                        DeviceConfig::WS281xColorOrder colorOrder)
    {
        switch (colorOrder)
        {
            case DeviceConfig::WS281xColorOrder::RGB:
                PackChannelPixels<0, 1, 2>(output, source, activeLedCount, pixelsToShow, brightness, fader);
                return;
            case DeviceConfig::WS281xColorOrder::RBG:
                PackChannelPixels<0, 2, 1>(output, source, activeLedCount, pixelsToShow, brightness, fader);
                return;
            case DeviceConfig::WS281xColorOrder::GRB:
                PackChannelPixels<1, 0, 2>(output, source, activeLedCount, pixelsToShow, brightness, fader);
                return;
            case DeviceConfig::WS281xColorOrder::GBR:
                PackChannelPixels<2, 0, 1>(output, source, activeLedCount, pixelsToShow, brightness, fader);
                return;
            case DeviceConfig::WS281xColorOrder::BRG:
                PackChannelPixels<1, 2, 0>(output, source, activeLedCount, pixelsToShow, brightness, fader);
                return;
            case DeviceConfig::WS281xColorOrder::BGR:
                PackChannelPixels<2, 1, 0>(output, source, activeLedCount, pixelsToShow, brightness, fader);
                return;
            default:
                PackChannelPixels<1, 0, 2>(output, source, activeLedCount, pixelsToShow, brightness, fader);
                return;
        }
    }

    void LogRuntimeWS281xConfiguration(const DeviceConfig& config, const std::vector<std::shared_ptr<GFXBase>>& devices, const char* reason)
    {
        debugI("WS281x config (%s): path=runtime driver=%s channels=%zu matrix=%ux%u serpentine=%d colorOrder=%s leds=%zu",
               reason ? reason : "update",
               config.GetRuntimeDriverName().c_str(),
               config.GetChannelCount(),
               static_cast<unsigned>(config.GetMatrixWidth()),
               static_cast<unsigned>(config.GetMatrixHeight()),
               config.IsMatrixSerpentine(),
               DeviceConfig::GetColorOrderName(config.GetWS281xColorOrder()).c_str(),
               config.GetActiveLEDCount());

        const auto& pins = config.GetWS281xPins();
        for (size_t channel = 0; channel < config.GetChannelCount() && channel < devices.size(); ++channel)
        {
            const auto& graphics = *devices[channel];
            debugI("WS281x channel %zu (runtime): pin=%d leds=%zu matrix=%ux%u serpentine=%d buffer=%p",
                   channel,
                   pins[channel],
                   graphics.GetLEDCount(),
                   static_cast<unsigned>(graphics.GetMatrixWidth()),
                   static_cast<unsigned>(graphics.GetMatrixHeight()),
                   graphics.IsSerpentine(),
                   graphics.leds);
        }
    }

    String FormatRmtError(const char* action, esp_err_t error)
    {
        return str_sprintf("%s failed (%s)", action, esp_err_to_name(error));
    }
}

WS281xOutputManager::~WS281xOutputManager()
{
    Reset();
}

void WS281xOutputManager::Reset()
{
    // Reset can race with the draw loop during live reconfiguration or teardown,
    // so it shares the same transport mutex as Show()/ApplyConfig().
    std::lock_guard<std::mutex> guard(WS281xGFX::TransportMutex());

    for (size_t i = 0; i < _channels.size(); ++i)
        ReleaseChannel(i);

    _activeChannelCount = 0;
    _activeLEDCount = 0;
    _colorOrder = DeviceConfig::GetCompiledWS281xColorOrder();
}

bool WS281xOutputManager::RecreateChannel(size_t channelIndex, int8_t pin, size_t ledCount, String* errorMessage)
{
    auto& state = _channels[channelIndex];
    const auto byteCount = ledCount * 3;

    // A channel recreate is the "hard" reconfigure path: tear down any existing
    // RMT binding, resize the packed byte buffer if LED count changed, then
    // install a fresh legacy-RMT TX channel on the new GPIO.

    if (state.installed)
        ReleaseChannel(channelIndex);

    if (!state.outputBytes || state.byteCount != byteCount)
    {
        auto nextOutputBytes = std::make_unique<uint8_t[]>(byteCount);
        if (!nextOutputBytes)
        {
            if (errorMessage)
                *errorMessage = "failed to allocate WS281x byte buffer";
            return false;
        }

        std::fill_n(nextOutputBytes.get(), byteCount, 0);
        state.outputBytes = std::move(nextOutputBytes);
        state.byteCount = byteCount;
    }

    // The IDF4-compatible helper seeds a TX config for the requested channel/GPIO.
    // We then override the pieces that matter for WS2812 timing and idle behavior.
    rmt_config_t config = RMT_DEFAULT_CONFIG_TX(static_cast<gpio_num_t>(pin), static_cast<rmt_channel_t>(channelIndex));
    config.clk_div = kRmtClockDivider;
    config.mem_block_num = kRmtMemoryBlocksPerChannel;
    config.tx_config.idle_output_en = true;
    config.tx_config.idle_level = RMT_IDLE_LEVEL_LOW;

    if (const auto error = rmt_config(&config); error != ESP_OK)
    {
        if (errorMessage)
            *errorMessage = FormatRmtError("rmt_config", error);
        return false;
    }

    if (const auto error = rmt_driver_install(static_cast<rmt_channel_t>(channelIndex), 0, ESP_INTR_FLAG_IRAM); error != ESP_OK)
    {
        if (errorMessage)
            *errorMessage = FormatRmtError("rmt_driver_install", error);
        return false;
    }

    if (const auto error = rmt_translator_init(static_cast<rmt_channel_t>(channelIndex), WS2812ByteTranslator); error != ESP_OK)
    {
        rmt_driver_uninstall(static_cast<rmt_channel_t>(channelIndex));
        if (errorMessage)
            *errorMessage = FormatRmtError("rmt_translator_init", error);
        return false;
    }

    state.pin = pin;
    state.ledCount = ledCount;
    state.installed = true;
    state.active = true;
    return true;
}

void WS281xOutputManager::ReleaseChannel(size_t channelIndex)
{
    auto& state = _channels[channelIndex];
    const auto channel = static_cast<rmt_channel_t>(channelIndex);

    // Wait for any in-flight frame to finish before uninstalling the driver so
    // a live pin/channel change does not pull the transport out from under Show().

    if (state.installed)
    {
        const auto waitError = rmt_wait_tx_done(channel, kRmtWaitTimeout);
        if (waitError == ESP_ERR_TIMEOUT)
        {
            debugW("rmt_wait_tx_done timed out during ReleaseChannel for channel=%zu pin=%d; forcing TX stop",
                   channelIndex,
                   state.pin);

            const auto stopError = rmt_tx_stop(channel);
            if (stopError != ESP_OK)
            {
                debugE("rmt_tx_stop failed during ReleaseChannel for channel=%zu pin=%d error=%s",
                       channelIndex,
                       state.pin,
                       esp_err_to_name(stopError));
            }
        }
        else if (waitError != ESP_OK)
        {
            debugE("rmt_wait_tx_done failed during ReleaseChannel for channel=%zu pin=%d error=%s",
                   channelIndex,
                   state.pin,
                   esp_err_to_name(waitError));
        }

        const auto uninstallError = rmt_driver_uninstall(channel);
        if (uninstallError != ESP_OK)
        {
            debugE("rmt_driver_uninstall failed during ReleaseChannel for channel=%zu pin=%d error=%s",
                   channelIndex,
                   state.pin,
                   esp_err_to_name(uninstallError));
        }
        state.installed = false;
    }

    if (state.pin >= 0)
    {
        pinMode(state.pin, OUTPUT);
        digitalWrite(state.pin, LOW);
    }

    state.pin = -1;
    state.ledCount = 0;
    state.active = false;
}

bool WS281xOutputManager::ApplyConfig(const DeviceConfig& config, const std::vector<std::shared_ptr<GFXBase>>& devices, String* errorMessage)
{
    // ApplyConfig and Show share the transport mutex so GPIO/channel changes are
    // atomic with respect to the render thread's transmit path.
    std::lock_guard<std::mutex> guard(WS281xGFX::TransportMutex());

    if (config.GetOutputDriver() != DeviceConfig::OutputDriver::WS281x)
    {
        if (errorMessage)
            *errorMessage = "recompile needed";
        return false;
    }

    const size_t channelCount = std::min(config.GetChannelCount(), devices.size());
    const size_t ledCount = config.GetActiveLEDCount();
    const auto& pins = config.GetWS281xPins();

    // Walk the full compile-time channel array every apply:
    // - active entries are recreated only if pin/length/install state changed
    // - inactive entries are explicitly released so old GPIO bindings disappear

    for (size_t i = 0; i < _channels.size(); ++i)
    {
        const bool shouldBeActive = i < channelCount;
        if (!shouldBeActive)
        {
            ReleaseChannel(i);
            continue;
        }

        auto& state = _channels[i];
        if (!state.active || !state.installed || state.pin != pins[i] || state.ledCount != ledCount)
        {
            if (!RecreateChannel(i, pins[i], ledCount, errorMessage))
                return false;
        }
    }

    _activeChannelCount = channelCount;
    _activeLEDCount = ledCount;
    _colorOrder = config.GetWS281xColorOrder();

    if (errorMessage)
        *errorMessage = "";

    LogRuntimeWS281xConfiguration(config, devices, "apply");
    return true;
}

void WS281xOutputManager::Show(const std::vector<std::shared_ptr<GFXBase>>& devices, uint16_t pixelsDrawn, uint8_t brightness, uint8_t fader)
{
    // The same mutex used by ApplyConfig() keeps live transport mutations from
    // colliding with the draw loop while it is filling buffers or transmitting.

    std::lock_guard<std::mutex> guard(WS281xGFX::TransportMutex());

    if (_activeChannelCount == 0 || _activeLEDCount == 0)
        return;

    const size_t pixelsToShow = std::min(static_cast<size_t>(pixelsDrawn), _activeLEDCount);

    // First build packed output bytes for every active channel.  The GFX layer
    // owns CRGB frame buffers; the runtime transport owns these temporary-once-
    // per-channel packed bytes that match the selected color order.

    for (size_t channelIndex = 0; channelIndex < _activeChannelCount && channelIndex < devices.size(); ++channelIndex)
    {
        auto& state = _channels[channelIndex];
        if (!state.active || !state.installed || !state.outputBytes)
            continue;

        const auto& device = devices[channelIndex];
        auto* output = state.outputBytes.get();
        PackChannelPixelsForColorOrder(output, device->leds, _activeLEDCount, pixelsToShow, brightness, fader, _colorOrder);
    }

    const auto showStartMicros = micros();

    // Queue every active channel first, then wait for completion in a second
    // pass. This keeps all strips in the same frame as closely aligned as the
    // legacy RMT API allows.

    for (size_t channelIndex = 0; channelIndex < _activeChannelCount && channelIndex < devices.size(); ++channelIndex)
    {
        auto& state = _channels[channelIndex];
        if (!state.active || !state.installed || !state.outputBytes)
            continue;

        const auto error = rmt_write_sample(static_cast<rmt_channel_t>(channelIndex), state.outputBytes.get(), state.byteCount, false);
        if (error != ESP_OK)
        {
            debugE("rmt_write_sample failed for channel=%zu pin=%d leds=%zu error=%s",
                   channelIndex,
                   state.pin,
                   _activeLEDCount,
                   esp_err_to_name(error));
        }
    }

    // The transmit wait is also where live reconfiguration pressure tends to
    // show up first, so failures here are logged separately from the queue step.

    for (size_t channelIndex = 0; channelIndex < _activeChannelCount && channelIndex < devices.size(); ++channelIndex)
    {
        auto& state = _channels[channelIndex];
        if (!state.active || !state.installed)
            continue;

        const auto error = rmt_wait_tx_done(static_cast<rmt_channel_t>(channelIndex), kRmtWaitTimeout);
        if (error != ESP_OK)
        {
            debugE("rmt_wait_tx_done failed for channel=%zu pin=%d leds=%zu error=%s",
                   channelIndex,
                   state.pin,
                   _activeLEDCount,
                   esp_err_to_name(error));
        }
    }

    const auto showElapsedMicros = micros() - showStartMicros;
    if (showElapsedMicros > 50000UL)
    {
        debugW("WS281x runtime show slow: channels=%zu leds=%zu elapsed=%lu us",
               _activeChannelCount,
               _activeLEDCount,
               static_cast<unsigned long>(showElapsedMicros));
    }
}

#endif
