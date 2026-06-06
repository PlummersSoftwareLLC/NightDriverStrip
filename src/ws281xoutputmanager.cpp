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

#include "ws281xoutputmanager.h"

#include <algorithm>
#include <cstdint>

#include <esp_err.h>
#include <esp_heap_caps.h>
#include <esp_idf_version.h>
#include <esp_task_wdt.h>

// Two RMT backends, picked at compile time. On IDF 4.x (Arduino-ESP32 2.x)
// only the legacy driver/rmt.h API exists; on IDF 5.x driver_ng is the
// supported path. The legacy and driver_ng headers cannot be included in
// the same translation unit on IDF 5 because they use the name
// rmt_channel_t for two different types - the legacy as an enum, the new
// as `struct rmt_channel_t *` - so we include only the one we'll use.
#if ESP_IDF_VERSION_MAJOR >= 5
#include <driver/rmt_tx.h>
#else
#if defined(CONFIG_RMT_SUPPRESS_DEPRECATE_WARN)
#undef CONFIG_RMT_SUPPRESS_DEPRECATE_WARN
#endif
#define CONFIG_RMT_SUPPRESS_DEPRECATE_WARN 1
#include <driver/rmt.h>
#ifndef RMT_DEFAULT_CONFIG_TX
    #error "NightDriverStrip WS281x runtime transport requires the ESP-IDF legacy RMT API on IDF 4.x."
#endif
#endif

#include "gfxbase.h"
#include "pixelformat.h"
#include "ws281xgfx.h"

// Transport base class. Concrete subclasses (in the anonymous namespace below)
// wrap a single ESP-IDF RMT driver generation so the manager itself stays
// driver-agnostic. The base class lives at file scope so that the matching
// `class Transport;` forward declaration in the header (used by the
// `std::unique_ptr<Transport>` member) refers to the same type.
class Transport
{
public:
    virtual ~Transport() = default;
    // Configure (or reconfigure) a single TX channel for the given GPIO. Caller
    // guarantees the channel is not currently installed when this is invoked.
    // `byteCount` is the size of the outputBytes buffer for that channel
    // (always 3 * ledCount).
    virtual SuccessResultWithMessage ConfigureChannel(size_t channelIndex, gpio_num_t pin, size_t byteCount) = 0;

    // Tear down a previously installed channel. Idempotent: safe to call on
    // an already-released channel.
    virtual void ReleaseChannel(size_t channelIndex) = 0;

    // Queue a frame for transmission on this channel. Implementation is
    // responsible for any driver-specific error logging.
    virtual void TransmitChannel(size_t channelIndex, const uint8_t* bytes, size_t byteCount, int8_t pin, size_t activeLEDCount) = 0;

    // Block until the most recent frame on this channel has finished
    // transmitting (or the timeout elapses). Implementation does its own
    // error logging.
    virtual void WaitForChannel(size_t channelIndex, int8_t pin, size_t activeLEDCount) = 0;
};

namespace
{
    static_assert(NUM_CHANNELS <= 8, "ESP32 RMT path supports up to 8 WS281x channels");

    // Common timing
    //
    // FastLED already carries the WS2812 timing constants we want. We reuse
    // those values here, but the actual transport and GPIO ownership remain in
    // NightDriver's runtime manager rather than in FastLED's controller layer.

    constexpr uint32_t kWs2812T0HighNs = FASTLED_WS2812_T1;
    constexpr uint32_t kWs2812T0LowNs  = FASTLED_WS2812_T2 + FASTLED_WS2812_T3;
    constexpr uint32_t kWs2812T1HighNs = FASTLED_WS2812_T1 + FASTLED_WS2812_T2;
    constexpr uint32_t kWs2812T1LowNs  = FASTLED_WS2812_T3;

    // 25 ns ticks (40 MHz APB / 2 on legacy via kRmtClockDivider, or
    // resolution_hz=40MHz on driver_ng). The wait timeout is shared.
    constexpr TickType_t kRmtWaitTimeout = pdMS_TO_TICKS(100);

    constexpr uint16_t NsToRmtTicks(uint32_t nanoseconds)
    {
        constexpr uint32_t kTickNs = 25;
        return static_cast<uint16_t>((nanoseconds + (kTickNs - 1)) / kTickNs);
    }

#if ESP_IDF_VERSION_MAJOR < 5
    // Legacy-only: clock divider model and per-bit rmt_item32_t entries
    // populated from ISR by the translator. driver_ng's bytes-encoder
    // has its own bit-symbol descriptors built inside DriverNgTransport
    // and doesn't need any of these.
    constexpr uint8_t kRmtClockDivider = 2;
    constexpr uint8_t kRmtMemoryBlocksPerChannel = 1;

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
#endif // ESP_IDF_VERSION_MAJOR < 5

    // Per-pixel packing moved into PixelFormat (see include/pixelformat.h).
    // Ws2812Format reproduces the previous PackChannelPixels behavior; new
    // chips (SK6812, SM16825, WS2805) add sibling format classes there.

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

#if ESP_IDF_VERSION_MAJOR < 5
    // Legacy IDF RMT API (driver/rmt.h). State-free: the channel index *is*
    // the rmt_channel_t value passed to every API call. Only present on
    // IDF 4 because legacy and driver_ng headers can't coexist in the same
    // translation unit on IDF 5 (rmt_channel_t name collision).
    class LegacyTransport : public ::Transport
    {
    public:
        SuccessResultWithMessage ConfigureChannel(size_t channelIndex, gpio_num_t pin, size_t /*byteCount*/) override
        {
            // The IDF4-compatible helper seeds a TX config for the requested
            // channel/GPIO. We then override the pieces that matter for WS2812
            // timing and idle behavior.
            rmt_config_t config = RMT_DEFAULT_CONFIG_TX(pin, static_cast<rmt_channel_t>(channelIndex));
            config.clk_div = kRmtClockDivider;
            config.mem_block_num = kRmtMemoryBlocksPerChannel;
            config.tx_config.idle_output_en = true;
            config.tx_config.idle_level = RMT_IDLE_LEVEL_LOW;

            if (const auto error = rmt_config(&config); error != ESP_OK)
                return { false, FormatRmtError("rmt_config", error) };

            if (const auto error = rmt_driver_install(static_cast<rmt_channel_t>(channelIndex), 0, ESP_INTR_FLAG_IRAM); error != ESP_OK)
                return { false, FormatRmtError("rmt_driver_install", error) };

            if (const auto error = rmt_translator_init(static_cast<rmt_channel_t>(channelIndex), WS2812ByteTranslator); error != ESP_OK)
            {
                rmt_driver_uninstall(static_cast<rmt_channel_t>(channelIndex));
                return { false, FormatRmtError("rmt_translator_init", error) };
            }

            return { true, "" };
        }

        void ReleaseChannel(size_t channelIndex) override
        {
            // Wait for any in-flight frame to finish before tearing down the
            // transport so a live pin/channel change does not pull the driver out
            // from under Show().
            const auto channel = static_cast<rmt_channel_t>(channelIndex);
            const auto waitError = rmt_wait_tx_done(channel, kRmtWaitTimeout);
            if (waitError == ESP_ERR_TIMEOUT)
            {
                debugW("rmt_wait_tx_done timed out during ReleaseChannel for channel=%zu; forcing TX stop",
                       channelIndex);

                const auto stopError = rmt_tx_stop(channel);
                if (stopError != ESP_OK)
                {
                    debugE("rmt_tx_stop failed during ReleaseChannel for channel=%zu error=%s",
                           channelIndex,
                           esp_err_to_name(stopError));
                }
            }
            else if (waitError != ESP_OK)
            {
                debugE("rmt_wait_tx_done failed during ReleaseChannel for channel=%zu error=%s",
                       channelIndex,
                       esp_err_to_name(waitError));
            }

            const auto uninstallError = rmt_driver_uninstall(channel);
            if (uninstallError != ESP_OK)
            {
                debugE("rmt_driver_uninstall failed during ReleaseChannel for channel=%zu error=%s",
                       channelIndex,
                       esp_err_to_name(uninstallError));
            }
        }

        void TransmitChannel(size_t channelIndex, const uint8_t* bytes, size_t byteCount, int8_t pin, size_t activeLEDCount) override
        {
            const auto error = rmt_write_sample(static_cast<rmt_channel_t>(channelIndex), bytes, byteCount, false);
            if (error != ESP_OK)
            {
                debugE("rmt_write_sample failed for channel=%zu pin=%d leds=%zu error=%s",
                       channelIndex,
                       pin,
                       activeLEDCount,
                       esp_err_to_name(error));
            }
        }

        void WaitForChannel(size_t channelIndex, int8_t pin, size_t activeLEDCount) override
        {
            const auto error = rmt_wait_tx_done(static_cast<rmt_channel_t>(channelIndex), kRmtWaitTimeout);
            if (error != ESP_OK)
            {
                debugE("rmt_wait_tx_done failed for channel=%zu pin=%d leds=%zu error=%s",
                       channelIndex,
                       pin,
                       activeLEDCount,
                       esp_err_to_name(error));
            }
        }
    };
#endif // ESP_IDF_VERSION_MAJOR < 5

#if ESP_IDF_VERSION_MAJOR >= 5
    // driver_ng IDF RMT API (driver/rmt_tx.h). Holds parallel arrays of
    // channel and encoder handles, since that API issues opaque handles
    // rather than identifying channels by index.
    constexpr rmt_symbol_word_t MakeRmtSymbol(uint16_t highTicks, uint16_t lowTicks)
    {
        rmt_symbol_word_t symbol{};
        symbol.level0 = 1;
        symbol.duration0 = highTicks;
        symbol.level1 = 0;
        symbol.duration1 = lowTicks;
        return symbol;
    }

    class DriverNgTransport : public ::Transport
    {
        rmt_channel_handle_t _channels[NUM_CHANNELS] = {};
        rmt_encoder_handle_t _encoders[NUM_CHANNELS] = {};

    public:
        SuccessResultWithMessage ConfigureChannel(size_t channelIndex, gpio_num_t pin, size_t /*byteCount*/) override
        {
            rmt_tx_channel_config_t channelConfig = {};
            channelConfig.gpio_num = pin;
            channelConfig.clk_src = RMT_CLK_SRC_DEFAULT;
            // 40 MHz / 25 ns ticks - matches legacy clock divider 2 from APB 80 MHz.
            channelConfig.resolution_hz = 40 * 1000 * 1000;
            channelConfig.mem_block_symbols = 64;
            channelConfig.trans_queue_depth = 4;

            if (const auto error = rmt_new_tx_channel(&channelConfig, &_channels[channelIndex]); error != ESP_OK)
            {
                _channels[channelIndex] = nullptr;
                return { false, FormatRmtError("rmt_new_tx_channel", error) };
            }

            rmt_bytes_encoder_config_t encoderConfig = {};
            encoderConfig.bit0 = MakeRmtSymbol(NsToRmtTicks(kWs2812T0HighNs), NsToRmtTicks(kWs2812T0LowNs));
            encoderConfig.bit1 = MakeRmtSymbol(NsToRmtTicks(kWs2812T1HighNs), NsToRmtTicks(kWs2812T1LowNs));
            encoderConfig.flags.msb_first = 1;

            if (const auto error = rmt_new_bytes_encoder(&encoderConfig, &_encoders[channelIndex]); error != ESP_OK)
            {
                rmt_del_channel(_channels[channelIndex]);
                _channels[channelIndex] = nullptr;
                _encoders[channelIndex] = nullptr;
                return { false, FormatRmtError("rmt_new_bytes_encoder", error) };
            }

            if (const auto error = rmt_enable(_channels[channelIndex]); error != ESP_OK)
            {
                rmt_del_encoder(_encoders[channelIndex]);
                rmt_del_channel(_channels[channelIndex]);
                _channels[channelIndex] = nullptr;
                _encoders[channelIndex] = nullptr;
                return { false, FormatRmtError("rmt_enable", error) };
            }

            return { true, "" };
        }

        void ReleaseChannel(size_t channelIndex) override
        {
            if (_channels[channelIndex])
            {
                const auto waitError = rmt_tx_wait_all_done(_channels[channelIndex], 100);
                if (waitError == ESP_ERR_TIMEOUT)
                {
                    debugW("rmt_tx_wait_all_done timed out during ReleaseChannel for channel=%zu",
                           channelIndex);
                }
                else if (waitError != ESP_OK)
                {
                    debugE("rmt_tx_wait_all_done failed during ReleaseChannel for channel=%zu error=%s",
                           channelIndex,
                           esp_err_to_name(waitError));
                }

                if (const auto error = rmt_disable(_channels[channelIndex]); error != ESP_OK)
                {
                    debugE("rmt_disable failed during ReleaseChannel for channel=%zu error=%s",
                           channelIndex,
                           esp_err_to_name(error));
                }
            }

            if (_encoders[channelIndex])
            {
                if (const auto error = rmt_del_encoder(_encoders[channelIndex]); error != ESP_OK)
                {
                    debugE("rmt_del_encoder failed during ReleaseChannel for channel=%zu error=%s",
                           channelIndex,
                           esp_err_to_name(error));
                }
                _encoders[channelIndex] = nullptr;
            }

            if (_channels[channelIndex])
            {
                if (const auto error = rmt_del_channel(_channels[channelIndex]); error != ESP_OK)
                {
                    debugE("rmt_del_channel failed during ReleaseChannel for channel=%zu error=%s",
                           channelIndex,
                           esp_err_to_name(error));
                }
                _channels[channelIndex] = nullptr;
            }
        }

        void TransmitChannel(size_t channelIndex, const uint8_t* bytes, size_t byteCount, int8_t pin, size_t activeLEDCount) override
        {
            rmt_transmit_config_t txConfig = {};
            txConfig.loop_count = 0;
            const auto error = rmt_transmit(_channels[channelIndex], _encoders[channelIndex], bytes, byteCount, &txConfig);
            if (error != ESP_OK)
            {
                debugE("rmt_transmit failed for channel=%zu pin=%d leds=%zu error=%s",
                       channelIndex,
                       pin,
                       activeLEDCount,
                       esp_err_to_name(error));
            }
        }

        void WaitForChannel(size_t channelIndex, int8_t pin, size_t activeLEDCount) override
        {
            const auto error = rmt_tx_wait_all_done(_channels[channelIndex], 100); // 100ms timeout
            if (error != ESP_OK)
            {
                debugE("rmt_tx_wait_all_done failed for channel=%zu pin=%d leds=%zu error=%s",
                       channelIndex,
                       pin,
                       activeLEDCount,
                       esp_err_to_name(error));
            }
        }
    };
#endif // ESP_IDF_VERSION_MAJOR >= 5

    std::unique_ptr<::Transport> CreateTransport()
    {
#if ESP_IDF_VERSION_MAJOR >= 5
        return std::make_unique<DriverNgTransport>();
#else
        return std::make_unique<LegacyTransport>();
#endif
    }
}

namespace
{
    // Pick the PixelFormat that matches the LED chip on the wire. Currently
    // selected by compile-time flag - eventually this could become a
    // DeviceConfig runtime knob. SK6812 (4-channel RGBW) is the second chip
    // we support; the WS2812 path is the default.
    std::unique_ptr<PixelFormat> CreatePixelFormat()
    {
#if defined(USE_SK6812) && USE_SK6812
        return std::make_unique<Sk6812Format>();
#else
        return std::make_unique<Ws2812Format>();
#endif
    }
}

// Out-of-line so the unique_ptr<Transport> default-deleter sees the full
// Transport definition above, and the unique_ptr<PixelFormat> deleter sees
// the full PixelFormat hierarchy from pixelformat.h.
WS281xOutputManager::WS281xOutputManager()
    : _transport(CreateTransport()), _format(CreatePixelFormat())
{
}

WS281xOutputManager::~WS281xOutputManager()
{
    Reset();
}

void WS281xOutputManager::Reset()
{
    // Reset can race with the draw loop during live reconfiguration or teardown,
    // so it shares the same transport mutex as Show()/ApplyConfig().
    std::lock_guard guard(WS281xGFX::TransportMutex());

    for (size_t i = 0; i < _channels.size(); ++i)
        ReleaseChannel(i);

    _activeChannelCount = 0;
    _activeLEDCount = 0;
    _colorOrder = DeviceConfig::GetCompiledWS281xColorOrder();
}

SuccessResultWithMessage WS281xOutputManager::RecreateChannel(size_t channelIndex, int8_t pin, size_t ledCount)
{
    auto& state = _channels[channelIndex];
    // BytesPerPixel comes from the chip-specific PixelFormat (3 for WS2812,
    // 4 for SK6812, 5 for SM16825/WS2805).
    const auto byteCount = ledCount * _format->BytesPerPixel();

    // A channel recreate is the "hard" reconfigure path: tear down any existing
    // RMT binding, resize the packed byte buffer if LED count changed, then
    // install a fresh RMT TX channel on the new GPIO.

    if (state.installed)
        ReleaseChannel(channelIndex);

    if (!state.outputBytes || state.byteCount != byteCount)
    {
        // The legacy driver DMAs from this buffer (so it MUST live in
        // DMA-capable internal RAM) and driver_ng's non-DMA mode does fine
        // with the same allocation. With the PSRAM-by-default policy in
        // main.cpp, plain std::make_unique<uint8_t[]>(byteCount) lands
        // buffers above the threshold in PSRAM, which fails on the legacy
        // path with:
        //   "rmt: Using buffer allocated from psram"  -> ESP_ERR_INVALID_ARG
        // heap_caps_malloc with DMA+INTERNAL pins it correctly for both
        // drivers, so we use the same allocator either way.
        auto* mem = static_cast<uint8_t*>(heap_caps_malloc(byteCount,
                            MALLOC_CAP_DMA | MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT));
        if (!mem)
            return { false, "failed to allocate DMA-capable WS281x byte buffer" };

        std::fill_n(mem, byteCount, 0);
        // unique_ptr<uint8_t[]> default deleter calls free(), which is the
        // correct deallocator for heap_caps_malloc'd memory on ESP-IDF.
        state.outputBytes.reset(mem);
        state.byteCount = byteCount;
    }

    auto [channelConfigured, channelConfigureError] = _transport->ConfigureChannel(channelIndex, static_cast<gpio_num_t>(pin), byteCount);
    if (!channelConfigured)
        return { false, channelConfigureError };

    state.pin = pin;
    state.ledCount = ledCount;
    state.installed = true;
    state.active = true;
    return { true, "" };
}

void WS281xOutputManager::ReleaseChannel(size_t channelIndex)
{
    auto& state = _channels[channelIndex];

    // Wait for any in-flight frame to finish before tearing down the
    // transport so a live pin/channel change does not pull the driver out
    // from under Show().

    if (state.installed)
    {
        _transport->ReleaseChannel(channelIndex);
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

SuccessResultWithMessage WS281xOutputManager::ApplyConfig(const DeviceConfig& config, const std::vector<std::shared_ptr<GFXBase>>& devices)
{
    // ApplyConfig and Show share the transport mutex so GPIO/channel changes are
    // atomic with respect to the render thread's transmit path.
    std::lock_guard guard(WS281xGFX::TransportMutex());

    if (config.GetOutputDriver() != DeviceConfig::OutputDriver::WS281x)
        return { false, "recompile needed" };

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
            auto [channelRecreated, recreateError] = RecreateChannel(i, pins[i], ledCount);
            if (!channelRecreated)
                return { false, recreateError };
        }
    }

    _activeChannelCount = channelCount;
    _activeLEDCount = ledCount;
    _colorOrder = config.GetWS281xColorOrder();

    LogRuntimeWS281xConfiguration(config, devices, "apply");
    return { true, "" };
}

void WS281xOutputManager::Show(const std::vector<std::shared_ptr<GFXBase>>& devices, uint16_t pixelsDrawn, uint8_t brightness, uint8_t fader)
{
    // The same mutex used by ApplyConfig() keeps live transport mutations from
    // colliding with the draw loop while it is filling buffers or transmitting.

    std::lock_guard guard(WS281xGFX::TransportMutex());

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

        // Delegate to the chip-specific format. Passes the optional whites
        // plane (nullptr for plain WS2812 builds; populated by setPixelCCT /
        // setPixelWhite calls on SK6812+ builds). cctKelvin, ambient white,
        // and white-extract ratio defaults are baked in here pending the
        // DeviceConfig knobs landing in a follow-up commit. Each can be
        // overridden at build time via a -D in the env's build_src_flags.

        #ifndef NIGHTDRIVER_DEFAULT_CCT_KELVIN
            #define NIGHTDRIVER_DEFAULT_CCT_KELVIN 4000
        #endif
        #ifndef NIGHTDRIVER_DEFAULT_AMBIENT_CW
            #define NIGHTDRIVER_DEFAULT_AMBIENT_CW 0
        #endif
        #ifndef NIGHTDRIVER_DEFAULT_AMBIENT_WW
            #define NIGHTDRIVER_DEFAULT_AMBIENT_WW 0
        #endif

        // SK6812_WHITE_EXTRACT_RATIO: 0..255, fraction of shared-portion
        // white pulled into the dedicated W LED

        #ifndef SK6812_WHITE_EXTRACT_RATIO
            #define SK6812_WHITE_EXTRACT_RATIO 128
        #endif

        constexpr uint16_t kDefaultCctKelvin   = NIGHTDRIVER_DEFAULT_CCT_KELVIN;
        constexpr uint8_t  kDefaultAmbientCw   = NIGHTDRIVER_DEFAULT_AMBIENT_CW;
        constexpr uint8_t  kDefaultAmbientWw   = NIGHTDRIVER_DEFAULT_AMBIENT_WW;
        constexpr uint8_t  kDefaultExtractRatio = SK6812_WHITE_EXTRACT_RATIO;

        _format->Pack(output,
                      device->leds,
                      device->whites,                 // may be nullptr
                      _activeLEDCount,
                      pixelsToShow,
                      brightness, fader,
                      _colorOrder,
                      kDefaultCctKelvin,
                      kDefaultAmbientCw,
                      kDefaultAmbientWw,
                      kDefaultExtractRatio);
    }

    const auto showStartMicros = micros();

    // Queue every active channel first, then wait for completion in a second
    // pass. This keeps all strips in the same frame as closely aligned as the
    // RMT API allows.

    for (size_t channelIndex = 0; channelIndex < _activeChannelCount && channelIndex < devices.size(); ++channelIndex)
    {
        auto& state = _channels[channelIndex];
        if (!state.active || !state.installed || !state.outputBytes)
            continue;

        _transport->TransmitChannel(channelIndex, state.outputBytes.get(), state.byteCount, state.pin, _activeLEDCount);
    }

    // The transmit wait is also where live reconfiguration pressure tends to
    // show up first, so failures here are logged separately from the queue step.

    for (size_t channelIndex = 0; channelIndex < _activeChannelCount && channelIndex < devices.size(); ++channelIndex)
    {
        auto& state = _channels[channelIndex];
        if (!state.active || !state.installed)
            continue;

        _transport->WaitForChannel(channelIndex, state.pin, _activeLEDCount);
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
