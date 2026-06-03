#pragma once

//+--------------------------------------------------------------------------
//
// File:        stripoutputmanager.h
//
// NightDriverStrip - (c) 2026 Plummer's Software LLC.  All Rights Reserved.
//
// Common interface implemented by every concrete strip output manager
// (WS281x, APA102, ...). SystemContainer owns one of these as a single
// runtime-swappable transport so the render path is free of #if ladders
// over the chosen driver.
//
//             2-Jun-2026         Created      Davepl
//
//---------------------------------------------------------------------------

#include "globals.h"

#if USE_STRIP

#include <cstdint>
#include <memory>
#include <vector>

#include <WString.h>

#include "types.h"

class GFXBase;
class DeviceConfig;

class IStripOutputManager
{
  public:
    virtual ~IStripOutputManager() = default;

    virtual SuccessResultWithMessage ApplyConfig(const DeviceConfig& config,
                                                 const std::vector<std::shared_ptr<GFXBase>>& devices) = 0;

    virtual void Show(const std::vector<std::shared_ptr<GFXBase>>& devices,
                      uint16_t pixelsDrawn,
                      uint8_t brightness,
                      uint8_t fader) = 0;

    virtual void Reset() = 0;

    virtual size_t GetActiveChannelCount() const = 0;
    virtual size_t GetActiveLEDCount() const = 0;
};

#endif
