//+--------------------------------------------------------------------------
//
// File:        deviceconfig_internal.cpp
//
// NightDriverStrip - (c) 2018 Plummer's Software LLC.  All Rights Reserved.
//
// This file is part of deviceconfig.cpp; see that file header for additional context.
//
// Split scope: internal DeviceConfig helper routines and shared implementation details.
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

    #if USE_STRIP
    constexpr auto kCompiledWS281xColorOrder = ToRuntimeColorOrder(COLOR_ORDER);
    #else
    constexpr auto kCompiledWS281xColorOrder = DeviceConfig::WS281xColorOrder::GRB;
    #endif

    #if USE_STRIP
        #define ND_DATA_PIN(p)  static_cast<int8_t>(p)
    #else
        #define ND_DATA_PIN(p)  static_cast<int8_t>(-1)
    #endif
    #if USE_APA102
        #define ND_CLOCK_PIN(p) static_cast<int8_t>(p)
    #else
        #define ND_CLOCK_PIN(p) static_cast<int8_t>(-1)
    #endif

    constexpr std::array<int8_t, NUM_CHANNELS> kCompiledWS281xPins = {
        #if NUM_CHANNELS >= 1
            ND_DATA_PIN(LED_PIN0),
        #endif
        #if NUM_CHANNELS >= 2
            ND_DATA_PIN(LED_PIN1),
        #endif
        #if NUM_CHANNELS >= 3
            ND_DATA_PIN(LED_PIN2),
        #endif
        #if NUM_CHANNELS >= 4
            ND_DATA_PIN(LED_PIN3),
        #endif
        #if NUM_CHANNELS >= 5
            ND_DATA_PIN(LED_PIN4),
        #endif
        #if NUM_CHANNELS >= 6
            ND_DATA_PIN(LED_PIN5),
        #endif
        #if NUM_CHANNELS >= 7
            ND_DATA_PIN(LED_PIN6),
        #endif
        #if NUM_CHANNELS >= 8
            ND_DATA_PIN(LED_PIN7),
        #endif
    };

    constexpr std::array<int8_t, NUM_CHANNELS> kCompiledAPA102ClockPins = {
        #if NUM_CHANNELS >= 1
            ND_CLOCK_PIN(LED_CLOCK_PIN0),
        #endif
        #if NUM_CHANNELS >= 2
            ND_CLOCK_PIN(LED_CLOCK_PIN1),
        #endif
        #if NUM_CHANNELS >= 3
            ND_CLOCK_PIN(LED_CLOCK_PIN2),
        #endif
        #if NUM_CHANNELS >= 4
            ND_CLOCK_PIN(LED_CLOCK_PIN3),
        #endif
        #if NUM_CHANNELS >= 5
            ND_CLOCK_PIN(LED_CLOCK_PIN4),
        #endif
        #if NUM_CHANNELS >= 6
            ND_CLOCK_PIN(LED_CLOCK_PIN5),
        #endif
        #if NUM_CHANNELS >= 7
            ND_CLOCK_PIN(LED_CLOCK_PIN6),
        #endif
        #if NUM_CHANNELS >= 8
            ND_CLOCK_PIN(LED_CLOCK_PIN7),
        #endif
    };

    #undef ND_DATA_PIN
    #undef ND_CLOCK_PIN
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

    std::array<int8_t, NUM_CHANNELS> GetCompiledAPA102ClockPins()
    {
        return kCompiledAPA102ClockPins;
    }

    DeviceConfig::WS281xColorOrder GetCompiledWS281xColorOrder()
    {
        return kCompiledWS281xColorOrder;
    }
}
