//+--------------------------------------------------------------------------
//
// File:        deviceconfig_internal.cpp
//
// NightDriverStrip - (c) 2018 Plummer's Software LLC.  All Rights Reserved.
//
// This file is part of deviceconfig.cpp; see that file header for additional context.
//
//---------------------------------------------------------------------------

#include "globals.h"

#include "deviceconfig_internal.h"

namespace
{
    constexpr const char* kRecompileNeededMessage = "recompile needed";

    // Maps the compile-time COLOR_ORDER macro to DeviceConfig runtime enum values.
    constexpr DeviceConfig::WS281xColorOrder ToRuntimeColorOrder(EOrder order)
    {
        switch (order)
        {
            case EOrder::RGB: return DeviceConfig::WS281xColorOrder::RGB;
            case EOrder::RBG: return DeviceConfig::WS281xColorOrder::RBG;
            case EOrder::GRB: return DeviceConfig::WS281xColorOrder::GRB;
            case EOrder::GBR: return DeviceConfig::WS281xColorOrder::GBR;
            case EOrder::BRG: return DeviceConfig::WS281xColorOrder::BRG;
            case EOrder::BGR: return DeviceConfig::WS281xColorOrder::BGR;
            default:          return DeviceConfig::WS281xColorOrder::GRB;
        }
    }

    #if USE_WS281X
    constexpr auto kCompiledWS281xColorOrder = ToRuntimeColorOrder(COLOR_ORDER);
    #else
    constexpr auto kCompiledWS281xColorOrder = DeviceConfig::WS281xColorOrder::GRB;
    #endif

    constexpr std::array<int8_t, NUM_CHANNELS> kCompiledWS281xPins = {
        #if NUM_CHANNELS >= 1
        LED_PIN0,
        #endif
        #if NUM_CHANNELS >= 2
        LED_PIN1,
        #endif
        #if NUM_CHANNELS >= 3
        LED_PIN2,
        #endif
        #if NUM_CHANNELS >= 4
        LED_PIN3,
        #endif
        #if NUM_CHANNELS >= 5
        LED_PIN4,
        #endif
        #if NUM_CHANNELS >= 6
        LED_PIN5,
        #endif
        #if NUM_CHANNELS >= 7
        LED_PIN6,
        #endif
        #if NUM_CHANNELS >= 8
        LED_PIN7,
        #endif
    };
}

namespace DeviceConfigInternal
{
    const char* RecompileNeededMessage()
    {
        return kRecompileNeededMessage;
    }

    std::array<int8_t, NUM_CHANNELS> GetCompiledWS281xPins()
    {
        return kCompiledWS281xPins;
    }

    DeviceConfig::WS281xColorOrder GetCompiledWS281xColorOrder()
    {
        return kCompiledWS281xColorOrder;
    }
}
