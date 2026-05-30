//+--------------------------------------------------------------------------
//
// File:        deviceconfig_validation.cpp
//
// NightDriverStrip - (c) 2018 Plummer's Software LLC.  All Rights Reserved.
//
// This file is part of deviceconfig.cpp; see that file header for additional context.
//
//---------------------------------------------------------------------------

#include "globals.h"

#include "deviceconfig.h"
#include "deviceconfig_internal.h"

DeviceConfig::ValidateResponse DeviceConfig::ValidateTopology(uint16_t width, uint16_t height, bool serpentine) const
{
    if (width == 0 || height == 0)
        return { false, "matrix dimensions must be greater than zero" };

    if (static_cast<size_t>(width) * height > GetCompiledLEDCount())
        return { false, DeviceConfigInternal::RecompileNeededMessage() };

    if (IsHub75Build())
    {
        if (width != GetCompiledMatrixWidth() || height != GetCompiledMatrixHeight())
            return { false, DeviceConfigInternal::RecompileNeededMessage() };

        if (serpentine != GetCompiledMatrixSerpentine())
            return { false, DeviceConfigInternal::RecompileNeededMessage() };
    }

    return { true, "" };
}

DeviceConfig::ValidateResponse DeviceConfig::ValidateOutputDriver(OutputDriver driver) const
{
    if (driver != GetCompiledOutputDriver())
        return { false, DeviceConfigInternal::RecompileNeededMessage() };

    return { true, "" };
}

DeviceConfig::ValidateResponse DeviceConfig::ValidateWS281xSettings(size_t channelCount, const std::array<int8_t, NUM_CHANNELS>& pins, WS281xColorOrder colorOrder) const
{
    if (channelCount == 0)
        return { false, "channel count must be greater than zero" };

    if (channelCount > GetCompiledChannelCount())
        return { false, DeviceConfigInternal::RecompileNeededMessage() };

    if (IsHub75Build())
    {
        if (channelCount != GetCompiledChannelCount())
            return { false, DeviceConfigInternal::RecompileNeededMessage() };

        if (pins != GetCompiledWS281xPins())
            return { false, DeviceConfigInternal::RecompileNeededMessage() };

        if (colorOrder != GetCompiledWS281xColorOrder())
            return { false, DeviceConfigInternal::RecompileNeededMessage() };
    }

    for (size_t i = 0; i < channelCount; ++i)
    {
        if (pins[i] < 0)
            return { false, "active channels require valid GPIO pins" };

        if (!GPIO_IS_VALID_OUTPUT_GPIO(static_cast<gpio_num_t>(pins[i])))
            return { false, "WS281x channel pins must be valid output GPIOs" };

        for (size_t j = i + 1; j < channelCount; ++j)
        {
            if (pins[i] == pins[j])
                return { false, "WS281x channel pins must be unique" };
        }
    }

    return { true, "" };
}

DeviceConfig::ValidateResponse DeviceConfig::ValidateRuntimeConfig(const RuntimeConfig& config) const
{
    auto [driverValid, driverMessage] = ValidateOutputDriver(config.outputs.driver);
    if (!driverValid)
        return { false, driverMessage };

    auto [topologyValid, topologyMessage] = ValidateTopology(config.topology.width, config.topology.height, config.topology.serpentine);
    if (!topologyValid)
        return { false, topologyMessage };

    auto [ws281xValid, ws281xMessage] = ValidateWS281xSettings(config.outputs.channelCount, config.outputs.outputPins, config.outputs.colorOrder);
    if (!ws281xValid)
        return { false, ws281xMessage };

    return { true, "" };
}

DeviceConfig::ValidateResponse DeviceConfig::SetRuntimeConfig(const RuntimeConfig& config, bool skipWrite)
{
    auto [isValid, validationMessage] = ValidateRuntimeConfig(config);
    if (!isValid)
        return { false, validationMessage };

    const bool changed =
        runtimeTopology.width != config.topology.width
        || runtimeTopology.height != config.topology.height
        || runtimeTopology.serpentine != config.topology.serpentine
        || runtimeOutputs.driver != config.outputs.driver
        || runtimeOutputs.channelCount != config.outputs.channelCount
        || runtimeOutputs.outputPins != config.outputs.outputPins
        || runtimeOutputs.colorOrder != config.outputs.colorOrder;

    runtimeTopology = config.topology;
    runtimeOutputs = config.outputs;

    if (!skipWrite)
        SaveToJSON();

    if (changed && !skipWrite)
        LogRuntimeConfig("runtime config changed");

    return { true, "" };
}
