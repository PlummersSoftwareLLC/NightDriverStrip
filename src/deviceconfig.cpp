//+--------------------------------------------------------------------------
//
// File:        deviceconfig.cpp
//
// NightDriverStrip - (c) 2018 Plummer's Software LLC.  All Rights Reserved.
//
// This file is part of the NightDriver software project.
//
//    NightDriver is free software: you can redistribute it and/or modify
//    it under the terms of the GNU General Public License as published by
//    the Free Software Foundation, either version 3 of the License, or
//    (at your option) any later version.
//
//    NightDriver is distributed in the hope that it will be useful,
//    but WITHOUT ANY WARRANTY; without even the implied warranty of
//    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//    GNU General Public License for more details.
//
//    You should have received a copy of the GNU General Public License
//    along with Nightdriver.  It is normally found in copying.txt
//    If not, see <https://www.gnu.org/licenses/>.
//
// Description:
//
//    Implementation of DeviceConfig class methods
//
// History:     Apr-18-2023         Rbergen      Created
//
//---------------------------------------------------------------------------

#include <HTTPClient.h>
#include <UrlEncode.h>
#include "globals.h"
#include "systemcontainer.h"

extern const char timezones_start[] asm("_binary_config_timezones_json_start");

DRAM_ATTR size_t g_DeviceConfigJSONBufferSize = 0;

void DeviceConfig::SaveToJSON()
{
    g_ptrSystem->JSONWriter().FlagWriter(writerIndex);
}

DeviceConfig::DeviceConfig()
{
    writerIndex = g_ptrSystem->JSONWriter().RegisterWriter(
        [this] { assert(SaveToJSONFile(DEVICE_CONFIG_FILE, g_DeviceConfigJSONBufferSize, *this)); }
    );

    std::unique_ptr<AllocatedJsonDocument> pJsonDoc(nullptr);

    if (LoadJSONFile(DEVICE_CONFIG_FILE, g_DeviceConfigJSONBufferSize, pJsonDoc))
    {
        debugI("Loading DeviceConfig from JSON");

        DeserializeFromJSON(pJsonDoc->as<JsonObjectConst>(), true);
        pJsonDoc->clear();
    }
    else
    {
        debugW("DeviceConfig could not be loaded from JSON, using defaults");

        SetTimeZone(timeZone, true);

        SaveToJSON();
    }
}

// The timezone JSON file used by this logic is generated using tools/gen-tz-json.py
bool DeviceConfig::SetTimeZone(const String& newTimeZone, bool skipWrite)
{
    String quotedTZ = "\n\"" + newTimeZone + '"';

    const char *start = strstr(timezones_start, quotedTZ.c_str());

    // If we can't find the new timezone as a timezone name, assume it's a literal value
    if (start == NULL)
        setenv("TZ", newTimeZone.c_str(), 1);
    // We received a timezone name, so we extract and use its timezone value
    else
    {
        start += quotedTZ.length();
        start = strchr(start, '"');
        if (start == NULL)      // Can't actually happen unless timezone file is malformed
            return false;

        start++;
        const char *end = strchr(start, '"');
        if (end == NULL)        // Can't actually happen unless timezone file is malformed
            return false;

        size_t length = end - start;

        std::unique_ptr<char[]> value = make_unique_psram_array<char>(length + 1);
        strncpy(value.get(), start, length);
        value[length] = 0;

        setenv("TZ", value.get(), 1);
    }

    tzset();

    timeZone = newTimeZone;
    if (!skipWrite)
        SaveToJSON();

    return true;
}

DeviceConfig::ValidateResponse DeviceConfig::ValidateOpenWeatherAPIKey(const String &newOpenWeatherAPIKey)
{
    HTTPClient http;

    String url = "http://api.openweathermap.org/data/2.5/weather?lat=0&lon=0&appid=" + urlEncode(newOpenWeatherAPIKey);

    http.begin(url);

    switch (http.GET())
    {
        case HTTP_CODE_OK:
        {
            http.end();
            return { true, "" };
        }

        case HTTP_CODE_UNAUTHORIZED:
        {
            AllocatedJsonDocument jsonDoc(_jsonSize);
            deserializeJson(jsonDoc, http.getString());

            String message = "";
            if (jsonDoc.containsKey("message"))
                message = jsonDoc["message"].as<String>();

            http.end();
            return { false, message };
        }

        // Anything else
        default:
        {
            http.end();
            return { false, "Unable to validate" };
        }
    }
}

void DeviceConfig::SetColorSettings(const CRGB& newGlobalColor, const CRGB& newSecondColor)
{
    globalColor = newGlobalColor;
    secondColor = newSecondColor;
    applyGlobalColors = true;

    SaveToJSON();
}

// This function contains the logic for dealing with the various color-related settings we have.
// The logic effectively mimics the behavior of pressing a color button on the IR remote control when (only) the
// global color is set or (re)applied, but also allows the secondary global palette color to be specified directly.
// The code in this function figures out how to prioritize and combine the values of (optional) settings; the actual
// logic for applying the correct color(s) and palette is contained in a number of EffectManager member functions.
void DeviceConfig::ApplyColorSettings(std::optional<CRGB> newGlobalColor, std::optional<CRGB> newSecondColor, bool clearGlobalColor, bool forceApplyGlobalColor)
{
    // If we're asked to clear the global color, we'll remember any colors we were passed, but won't do anything with them
    if (clearGlobalColor)
    {
        if (newGlobalColor.has_value())
            globalColor = newGlobalColor.value();
        if (newSecondColor.has_value())
            secondColor = newSecondColor.value();

        g_ptrSystem->EffectManager().ClearRemoteColor();

        applyGlobalColors = false;

        SaveToJSON();

        return;
    }

    CRGB finalGlobalColor = newGlobalColor.has_value() ? newGlobalColor.value() : globalColor;
    forceApplyGlobalColor = forceApplyGlobalColor || newGlobalColor.has_value();

    // If we were given a second color, set it and the global one if necessary. Then have EffectManager do its thing...
    if (newSecondColor.has_value())
    {
        if (forceApplyGlobalColor)
        {
            applyGlobalColors = true;
            globalColor = finalGlobalColor;
        }

        secondColor = newSecondColor.value();

        g_ptrSystem->EffectManager().ApplyGlobalPaletteColors();

        SaveToJSON();
    }
    // ...otherwise, apply the "set global color" logic if we were asked to do so
    else if (forceApplyGlobalColor)
        g_ptrSystem->EffectManager().ApplyGlobalColor(finalGlobalColor);
}