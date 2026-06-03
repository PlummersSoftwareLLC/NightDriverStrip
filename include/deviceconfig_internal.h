//+--------------------------------------------------------------------------
//
// File:        deviceconfig_internal.h
//
// NightDriverStrip - (c) 2018 Plummer's Software LLC.  All Rights Reserved.
//
//---------------------------------------------------------------------------

#pragma once

#include <array>

#include "deviceconfig.h"

namespace DeviceConfigInternal
{
    const char* RecompileNeededMessage();
    std::array<int8_t, NUM_CHANNELS> GetCompiledWS281xPins();
    std::array<int8_t, NUM_CHANNELS> GetCompiledAPA102ClockPins();
    DeviceConfig::WS281xColorOrder GetCompiledWS281xColorOrder();
}
