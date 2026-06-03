//+--------------------------------------------------------------------------
//
// File:        deviceconfig_color.cpp
//
// NightDriverStrip - (c) 2018 Plummer's Software LLC.  All Rights Reserved.
//
// This file is part of deviceconfig.cpp; see that file header for additional context.
//
// Split scope: color-related DeviceConfig accessors and serialization helpers.
//---------------------------------------------------------------------------


#include "globals.h"

#include "deviceconfig.h"
#include "effectmanager.h"
#include "systemcontainer.h"

void DeviceConfig::SetColorSettings(const CRGB& newGlobalColor, const CRGB& newSecondColor)
{
    globalColor = newGlobalColor;
    secondColor = newSecondColor;
    applyGlobalColors = true;

    SaveToJSON();
}

void DeviceConfig::ApplyColorSettings(std::optional<CRGB> newGlobalColor, std::optional<CRGB> newSecondColor, bool clearGlobalColor, bool forceApplyGlobalColor)
{
    if (clearGlobalColor)
    {
        if (newGlobalColor.has_value())
            globalColor = newGlobalColor.value();
        if (newSecondColor.has_value())
            secondColor = newSecondColor.value();

        g_ptrSystem->GetEffectManager().ClearRemoteColor();

        applyGlobalColors = false;

        SaveToJSON();

        return;
    }

    CRGB finalGlobalColor = newGlobalColor.has_value() ? newGlobalColor.value() : globalColor;
    forceApplyGlobalColor = forceApplyGlobalColor || newGlobalColor.has_value();

    if (newSecondColor.has_value())
    {
        if (forceApplyGlobalColor)
        {
            applyGlobalColors = true;
            globalColor = finalGlobalColor;
        }

        secondColor = newSecondColor.value();

        g_ptrSystem->GetEffectManager().ApplyGlobalPaletteColors();

        SaveToJSON();
    }
    else if (forceApplyGlobalColor)
    {
        g_ptrSystem->GetEffectManager().ApplyGlobalColor(finalGlobalColor);
    }
}
