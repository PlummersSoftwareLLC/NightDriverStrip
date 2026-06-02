//+--------------------------------------------------------------------------
//
// File:        apa102outputmanager.cpp
//
// NightDriverStrip - (c) 2026 Plummer's Software LLC.  All Rights Reserved.
//
// APA102/SK9822 runtime output manager. APA102 is clocked, so unlike WS281x it
// does not need RMT bit timing. We bit-bang data/clock GPIO pairs directly so
// multi-channel builds are not constrained by the number of ESP32 hardware SPI
// buses.
//
//---------------------------------------------------------------------------

#include "globals.h"

#if USE_APA102

#include "apa102outputmanager.h"

#include <algorithm>
#include <driver/gpio.h>
#include <soc/gpio_reg.h>
#include <soc/soc.h>

#include "gfxbase.h"
#include "pixelformat.h"
#include "ws281xgfx.h"

namespace
{
    #ifndef APA102_GLOBAL_BRIGHTNESS
        #define APA102_GLOBAL_BRIGHTNESS 31
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

    void SetGpioLevel(int8_t pin, bool high)
    {
        const uint32_t bit = 1UL << (pin & 31);
        if (pin < 32)
        {
            REG_WRITE(high ? GPIO_OUT_W1TS_REG : GPIO_OUT_W1TC_REG, bit);
            return;
        }

        #if defined(GPIO_OUT1_W1TS_REG) && defined(GPIO_OUT1_W1TC_REG)
            REG_WRITE(high ? GPIO_OUT1_W1TS_REG : GPIO_OUT1_W1TC_REG, bit);
        #else
            gpio_set_level(static_cast<gpio_num_t>(pin), high ? 1 : 0);
        #endif
    }

    void WriteBit(int8_t dataPin, int8_t clockPin, bool bit)
    {
        SetGpioLevel(dataPin, bit);
        SetGpioLevel(clockPin, true);
        SetGpioLevel(clockPin, false);
    }

    void WriteByte(int8_t dataPin, int8_t clockPin, uint8_t value)
    {
        for (int bit = 7; bit >= 0; --bit)
            WriteBit(dataPin, clockPin, (value & (1U << bit)) != 0);
    }

    void WriteStartFrame(int8_t dataPin, int8_t clockPin)
    {
        SetGpioLevel(dataPin, false);
        for (size_t i = 0; i < 4; ++i)
            WriteByte(dataPin, clockPin, 0x00);
    }

    void WriteEndFrame(int8_t dataPin, int8_t clockPin, size_t ledCount)
    {
        // APA102 needs at least ledCount / 2 extra clock pulses after the pixel
        // frames. Use a conservative byte count with a small fixed floor for
        // short strips.
        const size_t endBytes = std::max<size_t>(4, (ledCount + 15) / 16);
        SetGpioLevel(dataPin, true);
        for (size_t i = 0; i < endBytes; ++i)
            WriteByte(dataPin, clockPin, 0xFF);
    }

    void WritePixel(int8_t dataPin,
                    int8_t clockPin,
                    CRGB color,
                    uint8_t brightness,
                    uint8_t fader,
                    DeviceConfig::WS281xColorOrder colorOrder)
    {
        const auto indices = PixelFormatHelpers::IndicesFor(colorOrder);
        uint8_t wire[3] = {};
        wire[indices.rIdx] = PixelFormatHelpers::Scale(color.r, brightness, fader);
        wire[indices.gIdx] = PixelFormatHelpers::Scale(color.g, brightness, fader);
        wire[indices.bIdx] = PixelFormatHelpers::Scale(color.b, brightness, fader);

        WriteByte(dataPin, clockPin, 0xE0 | kGlobalBrightness);
        WriteByte(dataPin, clockPin, wire[0]);
        WriteByte(dataPin, clockPin, wire[1]);
        WriteByte(dataPin, clockPin, wire[2]);
    }

    void LogRuntimeAPA102Configuration(const DeviceConfig& config, const std::vector<std::shared_ptr<GFXBase>>& devices, const char* reason)
    {
        debugI("APA102 config (%s): path=bitbang driver=%s channels=%zu matrix=%ux%u serpentine=%d colorOrder=%s leds=%zu",
               reason ? reason : "update",
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
            debugI("APA102 channel %zu: data=%d clock=%d leds=%zu matrix=%ux%u serpentine=%d buffer=%p",
                   channel,
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

void APA102OutputManager::ConfigureChannel(size_t channelIndex, int8_t dataPin, int8_t clockPin, size_t ledCount)
{
    auto& state = _channels[channelIndex];

    if (state.active && (state.dataPin != dataPin || state.clockPin != clockPin))
        ReleaseChannel(channelIndex);

    pinMode(dataPin, OUTPUT);
    pinMode(clockPin, OUTPUT);
    SetGpioLevel(dataPin, false);
    SetGpioLevel(clockPin, false);

    state.dataPin = dataPin;
    state.clockPin = clockPin;
    state.ledCount = ledCount;
    state.active = true;
}

void APA102OutputManager::ReleaseChannel(size_t channelIndex)
{
    auto& state = _channels[channelIndex];

    if (state.dataPin >= 0)
    {
        pinMode(state.dataPin, OUTPUT);
        SetGpioLevel(state.dataPin, false);
    }

    if (state.clockPin >= 0)
    {
        pinMode(state.clockPin, OUTPUT);
        SetGpioLevel(state.clockPin, false);
    }

    state.dataPin = -1;
    state.clockPin = -1;
    state.ledCount = 0;
    state.active = false;
}

bool APA102OutputManager::ApplyConfig(const DeviceConfig& config, const std::vector<std::shared_ptr<GFXBase>>& devices, String* errorMessage)
{
    std::lock_guard guard(WS281xGFX::TransportMutex());

    if (config.GetOutputDriver() != DeviceConfig::OutputDriver::APA102)
    {
        if (errorMessage)
            *errorMessage = "recompile needed";
        return false;
    }

    const size_t channelCount = std::min(config.GetChannelCount(), devices.size());
    const size_t ledCount = config.GetActiveLEDCount();
    const auto& dataPins = config.GetAPA102DataPins();
    const auto& clockPins = config.GetAPA102ClockPins();

    for (size_t i = 0; i < _channels.size(); ++i)
    {
        const bool shouldBeActive = i < channelCount;
        if (!shouldBeActive)
        {
            ReleaseChannel(i);
            continue;
        }

        auto& state = _channels[i];
        if (!state.active || state.dataPin != dataPins[i] || state.clockPin != clockPins[i] || state.ledCount != ledCount)
            ConfigureChannel(i, dataPins[i], clockPins[i], ledCount);
    }

    _activeChannelCount = channelCount;
    _activeLEDCount = ledCount;
    _colorOrder = config.GetWS281xColorOrder();

    if (errorMessage)
        *errorMessage = "";

    LogRuntimeAPA102Configuration(config, devices, "apply");
    return true;
}

// 
// Show() 
//
// Render the given pixel data to the LED strip(s) using the APA102 protocol. This method is called on the critical path of rendering frames, 
// so it is optimized for performance. It assumes that the caller has already acquired the transport mutex and that the channel/LED counts 
// have been validated. The method iterates over each active channel/device, sends the APA102 start frame, then sends pixel data for each LED 
// (up to pixelsDrawn), applying brightness and fader scaling, and finally sends the end frame. If rendering takes an unusually long time, it 
// logs a warning with details about the configuration and elapsed time.
//
// This method is on the critical path of rendering frames to the LED strip, so it is carefully optimized for performance. It assumes that 
// the caller has already acquired the transport mutex and that the channel/LED counts have been validated.

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
        if (!state.active)
            continue;

        auto& device = devices[channelIndex];
        const size_t ledCount = std::min(_activeLEDCount, device->GetLEDCount());

        WriteStartFrame(state.dataPin, state.clockPin);

        for (size_t i = 0; i < ledCount; ++i)
        {
            CRGB color = (i < pixelsToShow) ? device->leds[i] : CRGB::Black;
            WritePixel(state.dataPin, state.clockPin, color, brightness, fader, _colorOrder);
        }

        WriteEndFrame(state.dataPin, state.clockPin, ledCount);
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
