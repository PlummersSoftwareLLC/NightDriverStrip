#pragma once

//+--------------------------------------------------------------------------
//
// File:        ws281xoutputmanager.h
//
// NightDriverStrip - (c) 2018 Plummer's Software LLC.  All Rights Reserved.
//
// Runtime WS281x output manager. Keeps FastLED for effect math and CRGB buffers,
// but moves GPIO/channel binding into a reconfigurable ESP32 output layer.
//
//---------------------------------------------------------------------------

#include "globals.h"

#if USE_WS281X

#include <array>
#include <cstdint>
#include <memory>
#include <vector>

#include "deviceconfig.h"

class GFXBase;

class WS281xOutputManager
{
    struct ChannelState
    {
        int8_t pin = -1;
        size_t ledCount = 0;
        size_t byteCount = 0;
        bool installed = false;
        bool active = false;
        std::unique_ptr<uint8_t[]> outputBytes;
    };

    std::array<ChannelState, NUM_CHANNELS> _channels{};
    size_t _activeChannelCount = 0;
    size_t _activeLEDCount = 0;
    DeviceConfig::WS281xColorOrder _colorOrder = DeviceConfig::GetCompiledWS281xColorOrder();

    bool RecreateChannel(size_t channelIndex, int8_t pin, size_t ledCount, String* errorMessage);
    void ReleaseChannel(size_t channelIndex);

  public:
    WS281xOutputManager() = default;
    ~WS281xOutputManager();

    bool ApplyConfig(const DeviceConfig& config, const std::vector<std::shared_ptr<GFXBase>>& devices, String* errorMessage = nullptr);
    void Show(const std::vector<std::shared_ptr<GFXBase>>& devices, uint16_t pixelsDrawn, uint8_t brightness, uint8_t fader);
    void Reset();

    size_t GetActiveChannelCount() const { return _activeChannelCount; }
    size_t GetActiveLEDCount() const { return _activeLEDCount; }
};

#endif
