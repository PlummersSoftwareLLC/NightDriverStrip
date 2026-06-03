#pragma once

//+--------------------------------------------------------------------------
//
// File:        apa102outputmanager.h
//
// NightDriverStrip - (c) 2026 Plummer's Software LLC.  All Rights Reserved.
//
// Runtime APA102 output manager. Drives APA102/SK9822 strips through the
// ESP32 hardware SPI master with DMA. Each channel binds to its own SPI
// host (SPI2_HOST, then SPI3_HOST), so the manager supports at most two
// simultaneous APA102 channels; that matches the number of general-purpose
// SPI peripherals exposed by the ESP32 family.
//
//              2-Jun-2026         Created      Davepl
//---------------------------------------------------------------------------

#include "globals.h"

#if USE_APA102

#include <array>
#include <cstdint>
#include <memory>
#include <vector>

#include <driver/spi_master.h>

#include "deviceconfig.h"
#include "stripoutputmanager.h"

class GFXBase;

class APA102OutputManager : public IStripOutputManager
{
  public:
    // ESP32 / ESP32-S3 expose two general-purpose SPI hosts (SPI2/SPI3); one APA102 channel per host.
    static constexpr size_t kMaxChannels = 2;

  private:
    struct ChannelState
    {
        spi_host_device_t   host       = SPI2_HOST;
        spi_device_handle_t device     = nullptr;
        int8_t              dataPin    = -1;
        int8_t              clockPin   = -1;
        size_t              ledCount   = 0;
        uint8_t*            buffer     = nullptr;
        size_t              bufferSize = 0;
        bool                active     = false;
    };

    std::array<ChannelState, kMaxChannels> _channels{};
    size_t _activeChannelCount = 0;
    size_t _activeLEDCount = 0;
    DeviceConfig::WS281xColorOrder _colorOrder = DeviceConfig::GetCompiledWS281xColorOrder();

    bool ConfigureChannel(size_t channelIndex, int8_t dataPin, int8_t clockPin, size_t ledCount, String* errorMessage);
    void ReleaseChannel(size_t channelIndex);

  public:
    APA102OutputManager();
    ~APA102OutputManager() override;

    SuccessResultWithMessage ApplyConfig(const DeviceConfig& config, const std::vector<std::shared_ptr<GFXBase>>& devices) override;
    void Show(const std::vector<std::shared_ptr<GFXBase>>& devices, uint16_t pixelsDrawn, uint8_t brightness, uint8_t fader) override;
    void Reset() override;

    size_t GetActiveChannelCount() const override { return _activeChannelCount; }
    size_t GetActiveLEDCount() const override { return _activeLEDCount; }
};

#endif
