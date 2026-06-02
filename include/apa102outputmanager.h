#pragma once

//+--------------------------------------------------------------------------
//
// File:        apa102outputmanager.h
//
// NightDriverStrip - (c) 2026 Plummer's Software LLC.  All Rights Reserved.
//
// Runtime APA102 output manager. Keeps the existing CRGB strip frame buffers
// and drives APA102/SK9822-style two-wire LEDs with one data and one clock GPIO
// per channel.
//
//              2-Jun-2026         Created      Davepl
//---------------------------------------------------------------------------

#include "globals.h"

#if USE_APA102

#include <array>
#include <cstdint>
#include <memory>
#include <vector>

#include "deviceconfig.h"
#include "stripoutputmanager.h"

class GFXBase;

class APA102OutputManager : public IStripOutputManager
{
    struct ChannelState
    {
        int8_t dataPin = -1;
        int8_t clockPin = -1;
        size_t ledCount = 0;
        bool active = false;
    };

    std::array<ChannelState, NUM_CHANNELS> _channels{};
    size_t _activeChannelCount = 0;
    size_t _activeLEDCount = 0;
    DeviceConfig::WS281xColorOrder _colorOrder = DeviceConfig::GetCompiledWS281xColorOrder();

    void ConfigureChannel(size_t channelIndex, int8_t dataPin, int8_t clockPin, size_t ledCount);
    void ReleaseChannel(size_t channelIndex);

  public:
    APA102OutputManager();
    ~APA102OutputManager() override;

    bool ApplyConfig(const DeviceConfig& config, const std::vector<std::shared_ptr<GFXBase>>& devices, String* errorMessage = nullptr) override;
    void Show(const std::vector<std::shared_ptr<GFXBase>>& devices, uint16_t pixelsDrawn, uint8_t brightness, uint8_t fader) override;
    void Reset() override;

    size_t GetActiveChannelCount() const override { return _activeChannelCount; }
    size_t GetActiveLEDCount() const override { return _activeLEDCount; }
};

#endif
