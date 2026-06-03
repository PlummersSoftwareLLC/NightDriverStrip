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
#include "stripoutputmanager.h"

class GFXBase;
class Transport;
class PixelFormat;

class WS281xOutputManager : public IStripOutputManager
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
    std::unique_ptr<Transport>    _transport;
    std::unique_ptr<PixelFormat>  _format;          // picked at construction by chip-type flag

    bool RecreateChannel(size_t channelIndex, int8_t pin, size_t ledCount, String* errorMessage);
    void ReleaseChannel(size_t channelIndex);

  public:
    WS281xOutputManager();
    ~WS281xOutputManager() override;

    bool ApplyConfig(const DeviceConfig& config, const std::vector<std::shared_ptr<GFXBase>>& devices, String* errorMessage = nullptr) override;
    void Show(const std::vector<std::shared_ptr<GFXBase>>& devices, uint16_t pixelsDrawn, uint8_t brightness, uint8_t fader) override;
    void Reset() override;

    size_t GetActiveChannelCount() const override { return _activeChannelCount; }
    size_t GetActiveLEDCount() const override { return _activeLEDCount; }
};

#endif
