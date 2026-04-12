#pragma once


//+--------------------------------------------------------------------------
//
// File:        PatternWeather.h
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
//
// Description:
//
//   Gets the weather for a given zip code
//
// History:     Jun-25-202          Davepl          Adapted from own code
//              Nov-15-2023         mggates         Updated to use better weather icons
//
//---------------------------------------------------------------------------

#ifndef PatternWeather_H
#define PatternWeather_H

#include <Arduino.h>
#include <ArduinoJson.h>
#include <array>
#include <chrono>
#include <HTTPClient.h>
#include <map>
#include <string.h>
#include <thread>
#include <UrlEncode.h>

#include "effects.h"
#include "array_utils.h"
#include "systemcontainer.h"
#include "TJpg_Decoder.h"
#include "types.h"

// Use centralized Apple5x7 font across all targets
extern const GFXfont Apple5x7 PROGMEM;

using namespace std::chrono;
using namespace std::chrono_literals;

#define WEATHER_INTERVAL_SECONDS 600s
#define WEATHER_CHECK_WIFI_WAIT 5000

extern const uint8_t brokenclouds_start[]           asm("_binary_assets_bmp_brokenclouds_jpg_start");
extern const uint8_t brokenclouds_end[]             asm("_binary_assets_bmp_brokenclouds_jpg_end");
extern const uint8_t brokenclouds_night_start[]     asm("_binary_assets_bmp_brokencloudsnight_jpg_start");
extern const uint8_t brokenclouds_night_end[]       asm("_binary_assets_bmp_brokencloudsnight_jpg_end");
extern const uint8_t clearsky_start[]               asm("_binary_assets_bmp_clearsky_jpg_start");
extern const uint8_t clearsky_end[]                 asm("_binary_assets_bmp_clearsky_jpg_end");
extern const uint8_t clearsky_night_start[]         asm("_binary_assets_bmp_clearnight_jpg_start");
extern const uint8_t clearsky_night_end[]           asm("_binary_assets_bmp_clearnight_jpg_end");
extern const uint8_t fewclouds_start[]              asm("_binary_assets_bmp_fewclouds_jpg_start");
extern const uint8_t fewclouds_end[]                asm("_binary_assets_bmp_fewclouds_jpg_end");
extern const uint8_t fewclouds_night_start[]        asm("_binary_assets_bmp_fewcloudsnight_jpg_start");
extern const uint8_t fewclouds_night_end[]          asm("_binary_assets_bmp_fewcloudsnight_jpg_end");
extern const uint8_t mist_start[]                   asm("_binary_assets_bmp_mist_jpg_start");
extern const uint8_t mist_end[]                     asm("_binary_assets_bmp_mist_jpg_end");
extern const uint8_t mist_night_start[]             asm("_binary_assets_bmp_mistnight_jpg_start");
extern const uint8_t mist_night_end[]               asm("_binary_assets_bmp_mistnight_jpg_end");
extern const uint8_t rain_start[]                   asm("_binary_assets_bmp_rain_jpg_start");
extern const uint8_t rain_end[]                     asm("_binary_assets_bmp_rain_jpg_end");
extern const uint8_t rain_night_start[]             asm("_binary_assets_bmp_rainnight_jpg_start");
extern const uint8_t rain_night_end[]               asm("_binary_assets_bmp_rainnight_jpg_end");
extern const uint8_t scatteredclouds_start[]        asm("_binary_assets_bmp_scatteredclouds_jpg_start");
extern const uint8_t scatteredclouds_end[]          asm("_binary_assets_bmp_scatteredclouds_jpg_end");
extern const uint8_t scatteredclouds_night_start[]  asm("_binary_assets_bmp_scatteredcloudsnight_jpg_start");
extern const uint8_t scatteredclouds_night_end[]    asm("_binary_assets_bmp_scatteredcloudsnight_jpg_end");
extern const uint8_t showerrain_start[]             asm("_binary_assets_bmp_showerrain_jpg_start");
extern const uint8_t showerrain_end[]               asm("_binary_assets_bmp_showerrain_jpg_end");
extern const uint8_t showerrain_night_start[]       asm("_binary_assets_bmp_showerrainnight_jpg_start");
extern const uint8_t showerrain_night_end[]         asm("_binary_assets_bmp_showerrainnight_jpg_end");
extern const uint8_t snow_start[]                   asm("_binary_assets_bmp_snow_jpg_start");
extern const uint8_t snow_end[]                     asm("_binary_assets_bmp_snow_jpg_end");
extern const uint8_t snow_night_start[]             asm("_binary_assets_bmp_snownight_jpg_start");
extern const uint8_t snow_night_end[]               asm("_binary_assets_bmp_snownight_jpg_end");
extern const uint8_t thunderstorm_start[]           asm("_binary_assets_bmp_thunderstorm_jpg_start");
extern const uint8_t thunderstorm_end[]             asm("_binary_assets_bmp_thunderstorm_jpg_end");
extern const uint8_t thunderstorm_night_start[]     asm("_binary_assets_bmp_thunderstormnight_jpg_start");
extern const uint8_t thunderstorm_night_end[]       asm("_binary_assets_bmp_thunderstormnight_jpg_end");

static constexpr auto pszDaysOfWeek = to_array( { "SUN", "MON", "TUE", "WED", "THU", "FRI", "SAT" } );

static std::map<const String, EmbeddedFile, std::less<const String>, psram_allocator<std::pair<const String, EmbeddedFile>>> weatherIcons =
{
    { "01d", EmbeddedFile(clearsky_start, clearsky_end) },
    { "02d", EmbeddedFile(fewclouds_start, fewclouds_end) },
    { "03d", EmbeddedFile(scatteredclouds_start, scatteredclouds_end) },
    { "04d", EmbeddedFile(brokenclouds_start, brokenclouds_end) },
    { "09d", EmbeddedFile(showerrain_start, showerrain_end) },
    { "10d", EmbeddedFile(rain_start, rain_end) },
    { "11d", EmbeddedFile(thunderstorm_start, thunderstorm_end) },
    { "13d", EmbeddedFile(snow_start, snow_end) },
    { "50d", EmbeddedFile(mist_start, mist_end) },
    { "01n", EmbeddedFile(clearsky_night_start, clearsky_night_end) },
    { "02n", EmbeddedFile(fewclouds_night_start, fewclouds_night_end) },
    { "03n", EmbeddedFile(scatteredclouds_night_start, scatteredclouds_night_end) },
    { "04n", EmbeddedFile(brokenclouds_night_start, brokenclouds_night_end) },
    { "09n", EmbeddedFile(showerrain_night_start, showerrain_night_end) },
    { "10n", EmbeddedFile(rain_night_start, rain_night_end) },
    { "11n", EmbeddedFile(thunderstorm_night_start, thunderstorm_night_end) },
    { "13n", EmbeddedFile(snow_night_start, snow_night_end) },
    { "50n", EmbeddedFile(mist_night_start, mist_night_end) }
};

/**
 * @brief This class implements the Weather Data effect
 *
 */
class PatternWeather : public EffectWithId<PatternWeather>
{
  private:

    String strLocationName    = "";
    String strLocation        = "";
    String strCountryCode     = "";
    String strLatitude        = "0.0";
    String strLongitude       = "0.0";
    int    dayOfWeek          = 0;
    String iconToday          = "";
    String iconTomorrow       = "";
    float  temperature        = 0.0f;
    float  highToday          = 0.0f;
    float  loToday            = 0.0f;
    float  highTomorrow       = 0.0f;
    float  loTomorrow         = 0.0f;

    bool   dataReady          = false;
    size_t readerIndex        = SIZE_MAX;
    system_clock::time_point latestUpdate = system_clock::from_time_t(0);

    // Animation state
    uint32_t _animFrame = 0;
    int16_t  _scrollPos = 0;

    /**
     * @brief Should this effect show its title.
     * The weather is obviously weather, and we don't want text overlaid on top of our text
     *
     * @return bool - false No title rquired
     */
    virtual bool ShouldShowTitle() const
    {
        return false;
    }

    /**
     * @brief How many frames per second does this effect want
     *
     * @return size_t - 25 FPS
     */
    size_t DesiredFramesPerSecond() const override
    {
        return 25;
    }

    /**
     * @brief Does this effect requre double buffering support
     *
     * @return bool - false No double buffering needed
     */
    bool RequiresDoubleBuffering() const override
    {
        return false;
    }

    /**
     * @brief Convert Kelvin to Farenheit
     *
     * @param K  Temperature in Kelvin
     * @return float - Farenheit temperature
     */
    static inline float KelvinToFarenheit(float K)
    {
        return (K - 273.15) * 9.0f/5.0f + 32;
    }

    /**
     * @brief Convert Kelvin to Celsius
     *
     * @param K Temperature in Kelvin
     * @return float - Celsius temperature
     */
    static inline float KelvinToCelsius(float K)
    {
        return K - 273.15;
    }

    /**
     * @brief Convert Kelvin temperature to local units
     * based on the device configuration flag Use Celsius
     *
     * @param K Temperature in Kelvin
     * @return float - temperature in selected units
     */
    static inline float KelvinToLocal(float K)
    {
        if (g_ptrSystem->GetDeviceConfig().UseCelsius())
            return KelvinToCelsius(K);
        else
            return KelvinToFarenheit(K);
    }

    /**
     * @brief Update the latitude and longitude for the
     * selected city or zip code from the device configuration
     *
     * @return bool - true if the lat/log location is updated
     */
    bool updateCoordinates()
    {
        HTTPClient http;
        String url;

        if (!HasLocationChanged())
            return false;

        const String& configLocation = g_ptrSystem->GetDeviceConfig().GetLocation();
        const String& configCountryCode = g_ptrSystem->GetDeviceConfig().GetCountryCode();
        const bool configLocationIsZip = g_ptrSystem->GetDeviceConfig().IsLocationZip();

        if (configLocationIsZip)
            url = "http://api.openweathermap.org/geo/1.0/zip"
                "?zip=" + urlEncode(configLocation) + "," + urlEncode(configCountryCode) + "&appid=" + urlEncode(g_ptrSystem->GetDeviceConfig().GetOpenWeatherAPIKey());
        else
            url = "http://api.openweathermap.org/geo/1.0/direct"
                "?q=" + urlEncode(configLocation) + "," + urlEncode(configCountryCode) + "&limit=1&appid=" + urlEncode(g_ptrSystem->GetDeviceConfig().GetOpenWeatherAPIKey());

        http.begin(url);
        int httpResponseCode = http.GET();

        if (httpResponseCode <= 0)
        {
            debugE("Error fetching coordinates for location: %s", configLocation.c_str());
            http.end();
            return false;
        }

        auto doc = CreateJsonDocument();
        deserializeJson(doc, http.getString());
        JsonObject coordinates = configLocationIsZip ? doc.as<JsonObject>() : doc[0].as<JsonObject>();

        strLatitude = coordinates["lat"].as<String>();
        strLongitude = coordinates["lon"].as<String>();

        http.end();

        strLocation = configLocation;
        strCountryCode = configCountryCode;

        return true;
    }

    /**
     * @brief Get the forcast for Tomorrow from the API
     *
     * Tommorow's expected high and low temperatures,
     * and an icon for tomorrow's weather forcast
     *
     * @param highTemp address to store the high temperature
     * @param lowTemp address to store the low temperature
     * @return bool - true if valid weather data retrieved
     */
    bool getTomorrowTemps(float& highTemp, float& lowTemp)
    {
        HTTPClient http;
        String url = "http://api.openweathermap.org/data/2.5/forecast"
            "?lat=" + strLatitude + "&lon=" + strLongitude + "&cnt=16&appid=" + urlEncode(g_ptrSystem->GetDeviceConfig().GetOpenWeatherAPIKey());
        http.begin(url);
        int httpResponseCode = http.GET();

        if (httpResponseCode > 0)
        {
            // Needs to be this large to process all the returned JSON
            auto doc = CreateJsonDocument();
            deserializeJson(doc, http.getString());
            JsonArray list = doc["list"];

            // Get tomorrow's date
            auto tomorrow = system_clock::now() + 24h;
            auto tomorrowTime = system_clock::to_time_t(tomorrow);
            auto tomorrowLocal = localtime(&tomorrowTime);
            char dateStr[11];
            strftime(dateStr, sizeof(dateStr), "%Y-%m-%d", tomorrowLocal);

            float dailyMinimum = 999.0;
            float dailyMaximum = 0.0;
            int slot = 0;

            iconTomorrow = "";

            // Look for the temperature data for tomorrow
            for (size_t i = 0; i < list.size(); ++i)
            {
                JsonObject entry = list[i];

                // convert the entry UTC to localtime
                time_t entryTime = entry["dt"];
                tm* entryLocal = localtime(&entryTime);
                char entryStr[11];
                strftime(entryStr, sizeof(entryStr), "%Y-%m-%d", entryLocal);

                // if it is tomorrow then figure out the min and max and get the icon
                if (strcmp(dateStr, entryStr) == 0)
                {
                    slot++;
                    JsonObject main = entry["main"];

                    // Identify the maximum of the 3 hour maximum temperature
                    float entryMaximum = main["temp_max"];
                    dailyMaximum = std::max(dailyMaximum, entryMaximum);

                    // Identify the minimum of the 3 hour mimimum temperatures
                    float entryMinimum = main["temp_min"];
                    if (entryMinimum > 0)
                        dailyMinimum = std::min(dailyMinimum, entryMinimum);

                    // Use the noon slot for the icon
                    if (slot == 4)
                        iconTomorrow = entry["weather"][0]["icon"].as<String>();
                }
            }

            highTemp        = KelvinToLocal(dailyMaximum);
            lowTemp         = KelvinToLocal(dailyMinimum);

            debugI("Got tomorrow's temps: Lo %d, Hi %d, Icon %s", (int)lowTemp, (int)highTemp, iconTomorrow.c_str());

            http.end();
            return true;
        }
        else
        {
            debugE("Error fetching forecast data for location: %s in country: %s", strLocation.c_str(), strCountryCode.c_str());
            http.end();
            return false;
        }
    }

    /**
     * @brief Get the Weather Data from the API
     *
     * Current temperature, expected high and low temperatures,
     * and an icon for the current weather
     *
     * @return bool - true if valid weather data retrieved
     */
    bool getWeatherData()
    {
        HTTPClient http;

        String url = "http://api.openweathermap.org/data/2.5/weather"
            "?lat=" + strLatitude + "&lon=" + strLongitude + "&appid=" + urlEncode(g_ptrSystem->GetDeviceConfig().GetOpenWeatherAPIKey());
        http.begin(url);
        int httpResponseCode = http.GET();
        if (httpResponseCode > 0)
        {
            iconToday = "";
            auto jsonDoc = CreateJsonDocument();
            deserializeJson(jsonDoc, http.getString());

            // Once we have a non-zero temp we can start displaying things
            if (0 < jsonDoc["main"]["temp"])
                dataReady = true;

            temperature = KelvinToLocal(jsonDoc["main"]["temp"]);
            highToday   = KelvinToLocal(jsonDoc["main"]["temp_max"]);
            loToday     = KelvinToLocal(jsonDoc["main"]["temp_min"]);

            iconToday = jsonDoc["weather"][0]["icon"].as<String>();
            debugI("Got today's temps: Now %d Lo %d, Hi %d, Icon %s", (int)temperature, (int)loToday, (int)highToday, iconToday.c_str());

            const char * pszName = jsonDoc["name"];
            if (pszName)
                strLocationName = pszName;

            http.end();
            return true;
        }
        else
        {
            debugE("Error fetching Weather data for location: %s in country: %s", strLocation.c_str(), strCountryCode.c_str());
            http.end();
            return false;
        }
    }

    /**
     * @brief Hook called from the Network Reader Thread
     * This drives the collection of the weather data.
     *
     */
    void UpdateWeather()
    {
        while (!nd_network::IsWiFiConnected())
        {
            debugW("Delaying Weather update, waiting for WiFi...");
            vTaskDelay(pdMS_TO_TICKS(WEATHER_CHECK_WIFI_WAIT));
        }

        // Only try to update if we have an API Key
        if (!g_ptrSystem->GetDeviceConfig().GetOpenWeatherAPIKey().isEmpty())
        {
            updateCoordinates();

            if (getWeatherData())
                getTomorrowTemps(highTomorrow, loTomorrow);
        }
    }

    /**
     * @brief Determine if the device configuration has changed the
     * location selection
     *
     * @return bool - true if the location has changed
     */
    bool HasLocationChanged()
    {
        bool locationChanged = g_ptrSystem->GetDeviceConfig().GetLocation() != strLocation;
        bool countryChanged = g_ptrSystem->GetDeviceConfig().GetCountryCode() != strCountryCode;

        return locationChanged || countryChanged;
    }

    // Map a temperature to a hue-based color: cold=blue → mild=green → warm=orange → hot=red
    CRGB tempToColor(float temp) const
    {
        bool c = g_ptrSystem->GetDeviceConfig().UseCelsius();
        if (temp < (c ?  0.0f : 32.0f)) return CRGB( 80,  80, 255);  // freezing   – blue
        if (temp < (c ? 10.0f : 50.0f)) return CRGB(  0, 180, 255);  // cold       – cyan
        if (temp < (c ? 18.0f : 65.0f)) return CRGB(  0, 230, 120);  // mild       – green
        if (temp < (c ? 27.0f : 80.0f)) return CRGB(255, 220,   0);  // warm       – yellow
        if (temp < (c ? 35.0f : 95.0f)) return CRGB(255, 120,   0);  // hot        – orange
        return                                  CRGB(255,  30,   0);  // very hot   – red
    }

    // Draw a weather-condition animation overlaid on one icon panel.
    // startX/Y: top-left corner of the panel; w/h: panel dimensions.
    void drawWeatherAnimation(int16_t startX, int16_t startY, int16_t w, int16_t h, const String& icon)
    {
        if (icon.length() < 2) return;

        const String cond    = icon.substring(0, 2);
        const int16_t endX   = startX + w;
        const int16_t endY   = startY + h;
        const bool    isNight = (icon.length() > 2 && icon[2] == 'n');

        auto addPixel = [&](int16_t x, int16_t y, CRGB color)
        {
            if (x >= startX && x < endX && y >= startY && y < endY && g()->isValidPixel((uint)x, (uint)y))
                g()->leds[XY(x, y)] += color;
        };

        if (cond == "09" || cond == "10")   // Rain / shower rain
        {
            for (int i = 0; i < 6; i++)
            {
                int16_t rx = startX + (i * 5 + 2) % w;
                int16_t ry = startY + (int16_t)((_animFrame * 2 + i * 5) % h);
                addPixel(rx,     ry,     CRGB( 0,  60, 160));
                addPixel(rx,     ry - 1, CRGB( 0,  20,  60));  // short trail
            }
        }
        else if (cond == "13")              // Snow
        {
            for (int i = 0; i < 4; i++)
            {
                int16_t fx = startX + (int16_t)((i * 8 + 1 + (_animFrame / 6 + i) % 3) % w);
                int16_t fy = startY + (int16_t)((_animFrame + i * 7) % h);
                addPixel(fx, fy, CRGB(160, 160, 255));
            }
        }
        else if (cond == "11")              // Thunderstorm: rain + periodic lightning bolt
        {
            // Rain streaks
            for (int i = 0; i < 5; i++)
            {
                int16_t rx = startX + (i * 6 + 1) % w;
                int16_t ry = startY + (int16_t)((_animFrame * 2 + i * 4) % h);
                addPixel(rx, ry, CRGB(0, 25, 80));
            }
            // Lightning bolt for 4 frames every 80 frames (~3.2 s at 25 fps)
            uint32_t phase = _animFrame % 80;
            if (phase < 4)
            {
                uint8_t bright = (phase < 2) ? 255 : 180;
                int16_t bx = startX + w / 2;
                for (int16_t by = startY + 1; by < startY + 9 && by < endY; by++)
                {
                    int16_t px = bx + ((by & 1) ? 1 : -1);
                    if (g()->isValidPixel((uint)px, (uint)by))
                        g()->leds[XY(px, by)] = CRGB(bright, bright, (uint8_t)(bright * 4 / 5));
                }
            }
        }
        else if (cond == "01")              // Clear sky
        {
            if (isNight)
            {
                // Twinkling stars in the upper portion of the icon
                for (int i = 0; i < 4; i++)
                {
                    uint8_t twinkle = beatsin8(18 + i * 11, 0, 150, (uint16_t)(i * 64));
                    int16_t sx = startX + (i * 9 + 4) % w;
                    int16_t sy = startY + (i * 4 + 1) % (h / 2);
                    addPixel(sx, sy, CRGB(twinkle, twinkle, twinkle));
                }
            }
            else
            {
                // Pulsing warm golden shimmer near the sun centre
                uint8_t glow = beatsin8(10, 0, 90);
                int16_t cx = startX + w / 2;
                int16_t cy = startY + h / 3;
                addPixel(cx,     cy, CRGB(glow, glow / 2, 0));
                addPixel(cx + 1, cy, CRGB(glow / 2, glow / 4, 0));
            }
        }
        else if (cond == "50")              // Mist / fog: slow drifting horizontal bands
        {
            for (int row = 0; row < 3; row++)
            {
                int16_t fy = startY + 3 + row * (h / 4) + (int16_t)((_animFrame / 8) % (h / 4));
                if (fy < startY || fy >= endY) continue;
                int16_t shift = (int16_t)((_animFrame / 4 + row * 7) % w);
                for (int16_t fx = 0; fx < w; fx++)
                    addPixel(startX + (fx + shift) % w, fy, CRGB(30, 35, 40));
            }
        }
    }

public:

    /**
     * @brief Construct a new Pattern Weather object
     *
     */
    PatternWeather() : EffectWithId<PatternWeather>("Weather") {}

    /**
     * @brief Construct a new Pattern Weather object
     *
     * @param jsonObject Configuration JSON Object
     */
    PatternWeather(const JsonObjectConst&  jsonObject) : EffectWithId<PatternWeather>(jsonObject) {}

    /**
     * @brief Destroy the Pattern Weather object
     * Cancel the Network Reader thread for this index
     */
    ~PatternWeather()
    {
        g_ptrSystem->GetNetworkReader().CancelReader(readerIndex);
    }

    /**
     * @brief
     *
     * @param gfx Graphic Base engine
     * @return bool - true if successfully initialized
     */
    bool Init(std::vector<std::shared_ptr<GFXBase>>& gfx) override
    {
        if (!LEDStripEffect::Init(gfx))
            return false;

        // Register a Network Reader task with no interval.  Will manually flag in Draw()
        readerIndex = g_ptrSystem->GetNetworkReader().RegisterReader([this] { UpdateWeather(); });

        return true;
    }

    /**
     * @brief Draw the weather display with animated header, color-coded temperatures,
     * condition-specific animations (rain, snow, lightning, etc.), and scrolling city name.
     */
    void Draw() override
    {
        _animFrame++;

        const int fontHeight = 7;
        const int fontWidth  = 5;
        const int xHalf      = MATRIX_WIDTH / 2 - 1;   // = 31

        // Trigger weather fetch when interval elapses or location changes
        auto now = system_clock::now();
        auto secondsSinceLastUpdate = now - latestUpdate;
        if (secondsSinceLastUpdate >= WEATHER_INTERVAL_SECONDS || (HasLocationChanged() && secondsSinceLastUpdate >= 30s))
        {
            latestUpdate = now;
            debugI("Triggering thread to check weather now...");
            g_ptrSystem->GetNetworkReader().FlagReader(readerIndex);
        }

        // ── Clear ────────────────────────────────────────────────────────────
        g()->fillScreen(BLACK16);

        // ── Animated gradient header bar (rows 0–8) ──────────────────────────
        // Slow blue-to-indigo gradient that breathes in brightness
        uint8_t pulse = beatsin8(12, 55, 130);
        for (int x = 0; x < MATRIX_WIDTH; x++)
        {
            uint8_t t  = (uint8_t)((x * 255) / (MATRIX_WIDTH - 1));
            uint8_t bv = lerp8by8(pulse, pulse / 2, t);    // blue: bright left → dim right
            uint8_t rv = lerp8by8(0,     pulse / 5, t);    // slight red tint on right → indigo
            uint8_t gv = lerp8by8(0,     pulse / 9, t);
            CRGB hc = CRGB(rv, gv, bv);
            for (int y = 0; y <= 8; y++)
                g()->leds[XY(x, y)] = hc;
        }

        g()->setFont(&Apple5x7);

        // ── City name (truncated or scrolling) ───────────────────────────────
        bool noKey = g_ptrSystem->GetDeviceConfig().GetOpenWeatherAPIKey().isEmpty();
        if (noKey)
        {
            g()->setTextColor(g()->to16bit(CRGB(255, 80, 0)));
            g()->setCursor(1, fontHeight);
            g()->print("NO API KEY");
        }
        else
        {
            // Reserve right-side pixels for current temperature (up to "−20" = 3 chars)
            const int tempReserve = 4 * fontWidth + 2;                 // 22 px
            const int maxCityChars = (MATRIX_WIDTH - tempReserve - 2) / fontWidth;  // ≈ 8

            String name = strLocationName.isEmpty() ? strLocation : strLocationName;
            name.toUpperCase();

            g()->setTextColor(WHITE16);
            g()->setCursor(1, fontHeight);

            if ((int)name.length() <= maxCityChars)
            {
                g()->print(name);
            }
            else
            {
                // Advance scroll by one character every 12 frames (~2× per second)
                if (_animFrame % 12 == 0)
                    _scrollPos = (_scrollPos + 1) % ((int)name.length() + 4);
                String padded = name + "    ";           // trailing gap before loop-back
                String sub = "";
                int pLen = (int)padded.length();
                for (int i = 0; i < maxCityChars; i++)
                    sub += padded[(_scrollPos + i) % pLen];
                g()->print(sub);
            }

            // ── Current temperature — right-justified, color-coded ────────────
            if (dataReady)
            {
                String strTemp = String((int)temperature);
                int tx = MATRIX_WIDTH - fontWidth * (int)strTemp.length() - 1;
                g()->setCursor(tx, fontHeight);
                g()->setTextColor(g()->to16bit(tempToColor(temperature)));
                g()->print(strTemp);
            }
        }

        // ── Gradient separator line (row 9) ──────────────────────────────────
        // Uses the built-in gradient line helper (col1 = left, col2 = right)
        g()->drawLineF(0, 9, MATRIX_WIDTH - 1, 9, CRGB(0, 20, 200), CRGB(0, 60, 100));

        // ── Weather icons ─────────────────────────────────────────────────────
        auto iconEntry = weatherIcons.find(iconToday);
        if (iconEntry != weatherIcons.end())
        {
            auto& icon = iconEntry->second;
            if (JDR_OK != TJpgDec.drawJpg(0, 10, icon.contents, icon.length))
                debugW("Could not display icon %s", iconToday.c_str());
        }

        iconEntry = weatherIcons.find(iconTomorrow);
        if (iconEntry != weatherIcons.end())
        {
            auto& icon = iconEntry->second;
            if (JDR_OK != TJpgDec.drawJpg(xHalf + 1, 10, icon.contents, icon.length))
                debugW("Could not display icon %s", iconTomorrow.c_str());
        }

        // ── Condition-specific animations overlaid on icons ───────────────────
        if (dataReady)
        {
            const int iconH = MATRIX_HEIGHT - 10;
            drawWeatherAnimation(0,         10, xHalf,                  iconH, iconToday);
            drawWeatherAnimation(xHalf + 1, 10, MATRIX_WIDTH - xHalf - 1, iconH, iconTomorrow);
        }
        else if (!noKey)
        {
            // Loading: three pulsing dots centred on the lower half
            int dotY  = 20;
            int dotX0 = MATRIX_WIDTH / 2 - 4;
            for (int d = 0; d < 3; d++)
            {
                uint8_t db = beatsin8(8, 40, 200, (uint16_t)(d * 85));
                int dx = dotX0 + d * 4;
                if (g()->isValidPixel((uint)dx, (uint)dotY))
                    g()->leds[XY(dx, dotY)] = CRGB(0, db / 2, db);
            }
        }

        // ── Animated vertical divider ─────────────────────────────────────────
        for (int y = 9; y < MATRIX_HEIGHT; y++)
        {
            uint8_t bright = beatsin8(18, 70, 190, (uint16_t)(y * 18));
            g()->leds[XY(xHalf, y)] = CRGB(0, bright / 10, bright / 2);
        }

        // ── Day-of-week labels ────────────────────────────────────────────────
        time_t today = time(nullptr);
        const tm* todayTime = localtime(&today);
        const char* pszToday    = pszDaysOfWeek[todayTime->tm_wday];
        const char* pszTomorrow = pszDaysOfWeek[(todayTime->tm_wday + 1) % 7];

        g()->setTextColor(WHITE16);
        g()->setCursor(1, MATRIX_HEIGHT);
        g()->print(pszToday);
        g()->setCursor(xHalf + 2, MATRIX_HEIGHT);
        g()->print(pszTomorrow);

        // ── Hi / Lo temperatures — warm orange for Hi, cool blue for Lo ───────
        if (dataReady)
        {
            const int yHi = MATRIX_HEIGHT - fontHeight;
            const int yLo = MATRIX_HEIGHT;              // baseline at bottom edge

            String strHiToday  = String((int)highToday);
            String strLoToday  = String((int)loToday);
            String strHiTomor  = String((int)highTomorrow);
            String strLoTomor  = String((int)loTomorrow);

            // Today
            g()->setTextColor(g()->to16bit(CRGB(255, 130,   0)));
            g()->setCursor(xHalf - fontWidth * (int)strHiToday.length(), yHi);
            g()->print(strHiToday);

            g()->setTextColor(g()->to16bit(CRGB( 80, 160, 255)));
            g()->setCursor(xHalf - fontWidth * (int)strLoToday.length(), yLo);
            g()->print(strLoToday);

            // Tomorrow
            g()->setTextColor(g()->to16bit(CRGB(255, 130,   0)));
            g()->setCursor(MATRIX_WIDTH - fontWidth * (int)strHiTomor.length() - 1, yHi);
            g()->print(strHiTomor);

            g()->setTextColor(g()->to16bit(CRGB( 80, 160, 255)));
            g()->setCursor(MATRIX_WIDTH - fontWidth * (int)strLoTomor.length() - 1, yLo);
            g()->print(strLoTomor);
        }
    }
};

#endif
