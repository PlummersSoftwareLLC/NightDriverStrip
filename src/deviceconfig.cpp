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
#include "deviceconfig.h"

DRAM_ATTR std::unique_ptr<DeviceConfig> g_ptrDeviceConfig;
extern const char timezones_start[] asm("_binary_config_timezones_json_start");

DRAM_ATTR size_t g_DeviceConfigJSONBufferSize = 0;

void DeviceConfig::SaveToJSON()
{
    g_ptrJSONWriter->FlagWriter(writerIndex);
}

DeviceConfig::DeviceConfig()
{
    writerIndex = g_ptrJSONWriter->RegisterWriter(
        [this]() { SaveToJSONFile(DEVICE_CONFIG_FILE, g_DeviceConfigJSONBufferSize, *this); }
    );

    std::unique_ptr<AllocatedJsonDocument> pJsonDoc(nullptr);

    if (LoadJSONFile(DEVICE_CONFIG_FILE, g_DeviceConfigJSONBufferSize, pJsonDoc))
    {
        debugI("Loading DeviceConfig from JSON");

        DeserializeFromJSON(pJsonDoc->as<JsonObjectConst>(), true);
    }
    else
    {
        debugW("DeviceConfig could not be loaded from JSON, using defaults");

        // Set default for additional settings in this code
        location = cszLocation;
        locationIsZip = bLocationIsZip;
        countryCode = cszCountryCode;
        openWeatherApiKey = cszOpenWeatherAPIKey;
        use24HourClock = false;
        useCelsius = false;
        ntpServer = DEFAULT_NTP_SERVER;
        rememberCurrentEffect = false;

        SetTimeZone(cszTimeZone, true);

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

        std::unique_ptr<char[]> value = std::make_unique<char[]>(length + 1);
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
            AllocatedJsonDocument jsonDoc(1024);
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

/* Commented out because NVS seems to run out of space way too soon

void DeviceConfig::WriteToNVS(const String& name, const String& value)
{
    nvs_handle_t nvsRWHandle;

    // The "storage" string must match NVS partition name in partition table

    esp_err_t err = nvs_open("deviceconfig", NVS_READWRITE, &nvsRWHandle);
    if (err != ESP_OK)
    {
        debugW("Error (%s) opening NVS handle!\n", esp_err_to_name(err));
        return;
    }

    err = nvs_set_str(nvsRWHandle, name.c_str(), value.c_str());
    if (ESP_OK != err)
        debugW("Error (%s) storing %s!\n", esp_err_to_name(err), name.c_str());

    nvs_commit(nvsRWHandle);
    nvs_close(nvsRWHandle);
}

void DeviceConfig::WriteToNVS(const String& name, bool value)
{
    nvs_handle_t nvsRWHandle;

    // The "storage" string must match NVS partition name in partition table

    esp_err_t err = nvs_open("deviceconfig", NVS_READWRITE, &nvsRWHandle);
    if (err != ESP_OK)
    {
        debugW("Error (%s) opening NVS handle!\n", esp_err_to_name(err));
        return;
    }

    err = nvs_set_u8(nvsRWHandle, name.c_str(), value);
    if (ESP_OK != err)
        debugW("Error (%s) storing %s!\n", esp_err_to_name(err), name.c_str());

    nvs_commit(nvsRWHandle);
    nvs_close(nvsRWHandle);
}

DeviceConfig::DeviceConfig()
{
    char szBuffer[256];

    nvs_handle_t nvsROHandle;
    esp_err_t err = nvs_open("deviceconfig", NVS_READONLY, &nvsROHandle);
    if (err != ESP_OK)
    {
        debugW("Error (%s) opening NVS handle!\n", esp_err_to_name(err));

        SetLocation(cszLocation);
        SetLocationIsZip(bLocationIsZip);
        SetCountryCode(cszCountryCode);
        SetOpenWeatherAPIKey(cszOpenWeatherAPIKey);
        SetTimeZone(cszTimeZone);
        Set24HourClock(false);

        return;
    }

    auto len = ARRAYSIZE(szBuffer);
    err = nvs_get_str(nvsROHandle, NAME_OF(location), szBuffer, &len);
    if (ESP_OK != err)
    {
        debugE("Could not read location from NVS: %s", esp_err_to_name(err));
        SetLocation(cszLocation);
    }
    else
        SetLocation(szBuffer, true);

    uint8_t locationIsZip;
    err = nvs_get_u8(nvsROHandle, NAME_OF(locationIsZip), &locationIsZip);
    if (ESP_OK != err)
    {
        debugE("Coud not read locationIsZip from NVS: %s", esp_err_to_name(err));
        SetLocationIsZip(bLocationIsZip);
    }
    else
        SetLocationIsZip(locationIsZip, true);

    err = nvs_get_str(nvsROHandle, NAME_OF(countryCode), szBuffer, &len);
    if (ESP_OK != err)
    {
        debugE("Could not read countryCode from NVS: %s", esp_err_to_name(err));
        SetCountryCode(cszCountryCode);
    }
    else
        SetCountryCode(szBuffer, true);

    err = nvs_get_str(nvsROHandle, "_OWAK", szBuffer, &len);
    if (ESP_OK != err)
    {
        debugE("Could not read _OWAK from NVS: %s", esp_err_to_name(err));
        SetOpenWeatherAPIKey(cszOpenWeatherAPIKey);
    }
    else
        SetOpenWeatherAPIKey(szBuffer, true);

    err = nvs_get_str(nvsROHandle, NAME_OF(timeZone), szBuffer, &len);
    if (ESP_OK != err)
    {
        debugE("Could not read timeZone from NVS: %s", esp_err_to_name(err));
        SetTimeZone(cszTimeZone);
    }
    else
        SetTimeZone(szBuffer, true);

    uint8_t use24HourClock;
    err = nvs_get_u8(nvsROHandle, NAME_OF(use24HourClock), &use24HourClock);
    if (ESP_OK != err)
    {
        debugE("Could not read use24HourClock from NVS: %s", esp_err_to_name(err));
        SetLocationIsZip(false);
    }
    else
        SetLocationIsZip(use24HourClock, true);


    nvs_close(nvsROHandle);
}
*/

/* Commented out, because the web interface can also retrieve and parse the timezone file itself

void DeviceConfig::GetTimeZones(std::vector<std::unique_ptr<char[]>>& timeZones)
{
    const char *timeZoneStart = timezones_start;
    const char *timeZoneEnd;
    size_t timeZoneLength;
    std::unique_ptr<char[]> timeZone;
    timeZones.clear();

    while(true)
    {
        timeZoneStart = strstr(timeZoneStart, "\n\"");
        if (timeZoneStart == NULL)
            break;

        timeZoneStart += 2;
        timeZoneEnd = strchr(timeZoneStart, '"');
        if (timeZoneEnd == NULL)    // Can't actually happen unless timezone file is malformed
            break;

        timeZoneLength = timeZoneEnd - timeZoneStart;
        timeZone = std::make_unique<char[]>(timeZoneLength + 1);
        strncpy(timeZone.get(), timeZoneStart, timeZoneLength);
        timeZone[timeZoneLength] = 0;

        timeZones.push_back(timeZone);
    }
}
*/
