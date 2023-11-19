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
//                                                  Fixed issue in tommorrow temperature selection
//
//---------------------------------------------------------------------------

#ifndef PatternWeather_H
#define PatternWeather_H

#include <Arduino.h>
#include <string.h>
#include <HTTPClient.h>
#include <UrlEncode.h>
#include <ledstripeffect.h>
#include <ledmatrixgfx.h>
#include <ArduinoJson.h>
#include "systemcontainer.h"
#include <FontGfx_apple5x7.h>
#include <thread>
#include <map>
#include "TJpg_Decoder.h"
#include "effects.h"
#include "types.h"

#define WEATHER_INTERVAL_SECONDS (10*60)
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

static const char * pszDaysOfWeek[] = { "SUN", "MON", "TUE", "WED", "THU", "FRI", "SAT" };

static std::map<int, EmbeddedFile, std::less<int>, psram_allocator<std::pair<int, EmbeddedFile>>> weatherIcons =
{
    { 1, EmbeddedFile(clearsky_start, clearsky_end) },
    { 2, EmbeddedFile(fewclouds_start, fewclouds_end) },
    { 3, EmbeddedFile(scatteredclouds_start, scatteredclouds_end) },
    { 4, EmbeddedFile(brokenclouds_start, brokenclouds_end) },
    { 9, EmbeddedFile(showerrain_start, showerrain_end) },
    { 10, EmbeddedFile(rain_start, rain_end) },
    { 11, EmbeddedFile(thunderstorm_start, thunderstorm_end) },
    { 13, EmbeddedFile(snow_start, snow_end) },
    { 50, EmbeddedFile(mist_start, mist_end) },
    { 101, EmbeddedFile(clearsky_night_start, clearsky_night_end) },
    { 102, EmbeddedFile(fewclouds_night_start, fewclouds_night_end) },
    { 103, EmbeddedFile(scatteredclouds_night_start, scatteredclouds_night_end) },
    { 104, EmbeddedFile(brokenclouds_night_start, brokenclouds_night_end) },
    { 109, EmbeddedFile(showerrain_night_start, showerrain_night_end) },
    { 110, EmbeddedFile(rain_night_start, rain_night_end) },
    { 111, EmbeddedFile(thunderstorm_night_start, thunderstorm_night_end) },
    { 113, EmbeddedFile(snow_night_start, snow_night_end) },
    { 150, EmbeddedFile(mist_night_start, mist_night_end) }
};

class PatternWeather : public LEDStripEffect
{

private:

    String strLocationName    = "";
    String strLocation        = "";
    String strCountryCode     = "";
    String strLatitude        = "0.0";
    String strLongitude       = "0.0";
    int    dayOfWeek          = 0;
    int    iconToday          = -1;
    int    iconTomorrow       = -1;
    float  temperature        = 0.0f;
    float  highToday          = 0.0f;
    float  loToday            = 0.0f;
    float  highTomorrow       = 0.0f;
    float  loTomorrow         = 0.0f;

    bool   dataReady          = false;
    bool   locationValid      = false;
    size_t readerIndex = std::numeric_limits<size_t>::max();
    time_t latestUpdate       = 0;


    /**
     * @brief Should the framework show the effect title?
     * 
     * The weather is obviously weather, and we don't want text overlaid on top of our text
     * 
     * @return false
     */
    virtual bool ShouldShowTitle() const
    {
        return false;
    }

    /**
     * @brief How many frames per second do we want?
     * 
     * @return size_t 
     */
    virtual size_t DesiredFramesPerSecond() const override
    {
        return 5;
    }

    /**
     * @brief Do we require double buffering?
     * 
     * @return false 
     */
    virtual bool RequiresDoubleBuffering() const override
    {
        return false;
    }

    /**
     * @brief Convert Kelvin to Farenheit
     * 
     * @param K Kelvin Temperature
     * @return float Farenheit Temperature
     */
    inline float KelvinToFarenheit(float K)
    {
        return (K - 273.15) * 9.0f/5.0f + 32;
    }

    /**
     * @brief Convert Kelvin to Celsius
     * 
     * @param K Kelvin Temperature
     * @return float Celsius Temperature
     */
    inline float KelvinToCelsius(float K)
    {
        return K - 273.15;
    }

    /**
     * @brief Based on user preference convert the
     * Kelvin temperature to the selected scale
     * 
     * @param K Kelvin Temperature
     * @return float Celsius or Farenheit Temperature
     */
    inline float KelvinToLocal(float K)
    {
        if (g_ptrSystem->DeviceConfig().UseCelsius())
            return KelvinToCelsius(K);
        else
            return KelvinToFarenheit(K);
    }

    /**
     * @brief Get the latitude, longitude and name of the requested location
     * Also sets the class global locationValid if the location was found
     * 
     * @return true location was found
     * @return false no change to location
     */
    bool updateCoordinates()
    {
        HTTPClient http;
        String url;

        if (!HasLocationChanged())
            return false;

        locationValid = false;

        const String& configLocation = g_ptrSystem->DeviceConfig().GetLocation();
        const String& configCountryCode = g_ptrSystem->DeviceConfig().GetCountryCode();
        const bool configLocationIsZip = g_ptrSystem->DeviceConfig().IsLocationZip();

        if (configLocationIsZip)
            url = "http://api.openweathermap.org/geo/1.0/zip"
                "?zip=" + urlEncode(configLocation) + "," + urlEncode(configCountryCode) + "&appid=" + urlEncode(g_ptrSystem->DeviceConfig().GetOpenWeatherAPIKey());
        else
            url = "http://api.openweathermap.org/geo/1.0/direct"
                "?q=" + urlEncode(configLocation) + "," + urlEncode(configCountryCode) + "&limit=1&appid=" + urlEncode(g_ptrSystem->DeviceConfig().GetOpenWeatherAPIKey());

        http.begin(url);
        int httpResponseCode = http.GET();

        if (httpResponseCode > 0)
        {
            AllocatedJsonDocument doc(256);
            String jsonloc = http.getString();
            DeserializationError de = deserializeJson(doc, jsonloc);
            if (!de)
            {
                JsonObject coordinates = configLocationIsZip ? doc.as<JsonObject>() : doc[0].as<JsonObject>();
            
                strLatitude = coordinates["lat"].as<String>();
                strLongitude = coordinates["lon"].as<String>();

                debugI("location lat: %s, lon: %s", strLatitude.c_str(), strLongitude.c_str());

                strLocation = configLocation;
                strCountryCode = configCountryCode;
                locationValid = true;
            }
            else
            {
                debugW("Bad location JSON: %s", de.c_str());
            }

        }
        else
        {
            debugW("Error fetching coordinates for location: %s", configLocation.c_str());
            return false;
        }

        http.end();
        return locationValid;
    }

    /**
     * @brief Get the Tomorrow Temperatures
     * 
     * Request a forecast and then parse out the high and low temps for tomorrow
     * along with the weather icon for mid-day
     * 
     * @param highTemp
     * @param lowTemp
     * @param icon 
     * @return true Successful data retrieval
     * @return false Failed data retrieval
     */
    bool getTomorrowTemps(float& highTemp, float& lowTemp, int& icon)
    {
        HTTPClient http;
        String url = "http://api.openweathermap.org/data/2.5/forecast"
            "?lat=" + strLatitude + "&lon=" + strLongitude + "&cnt=18&appid=" + urlEncode(g_ptrSystem->DeviceConfig().GetOpenWeatherAPIKey());
        debugI("tomorrowURL: %s", url.c_str());
        http.begin(url);
        int httpResponseCode = http.GET();

        if (httpResponseCode > 0)
        {
            AllocatedJsonDocument doc(9216);
            String tomorrowJson = http.getString();
            DeserializationError de = deserializeJson(doc, tomorrowJson);
            String count = doc["cnt"];
            debugI("Return count %s DE code %s", count.c_str(), de.c_str());
            JsonArray list = doc["list"];

            // Get tomorrow's date 
            time_t tomorrow = time(nullptr) + 86400; 
            tm* tomorrowTime = localtime(&tomorrow); 
            char dateStr[11]; 
            strftime(dateStr, sizeof(dateStr), "%Y-%m-%d", tomorrowTime); 

            float localMin = 999.0;
            float localMax = 0.0;
 
            // Look for the temperature data for tomorrow  - needs more work
            int slot = 0;
            for (size_t i = 0; i < list.size(); i++) 
            {
                JsonObject entry = list[i];

                // convert the entry UTC to localtime
                // if it is tomorrow then figure out the min and max and get the icon
                time_t entry_time = entry["dt"];
                tm* entryLocal = localtime(&entry_time);
                char entryStr[11];
                strftime(entryStr, sizeof(entryStr), "%Y-%m-%d", entryLocal);

                if (strcmp(dateStr, entryStr) == 0) 
                {
                    JsonObject main = entry["main"];
                    if ((main["temp_max"] > 0) && (main["temp_max"] > localMax))
                        localMax = main["temp_max"];
                    if ((main["temp_min"] > 0) && (main["temp_min"] < localMin))
                        localMin = main["temp_min"];

                    slot++;
                    if (slot == 4)
                    {
                        String iconIdTomorrow = entry["weather"][0]["icon"];
                        icon = iconIdTomorrow.toInt(); // + (iconIdTomorrow.endsWith("d") ? 0 : 100); // always a day icon}
                    }
                }
            }
            highTemp        = KelvinToLocal(localMax);
            lowTemp         = KelvinToLocal(localMin);

            debugI("Got tomorrow's temps: Lo %d, Hi %d, Icon %d", (int)lowTemp, (int)highTemp, (int)iconTomorrow);

            http.end();
            return true;
        }
        else
        {
            debugW("Error fetching forecast data for location: %s in country: %s", strLocation.c_str(), strCountryCode.c_str());
            http.end();
            return false;
        }
    }

    /**
     * @brief Get the current temp and weather icon along with the high and low for today
     * 
     * @return true - Successful Data Retrieval
     * @return false - Failed data retrieval
     */
    bool getWeatherData()
    {
        HTTPClient http;

        String url = "http://api.openweathermap.org/data/2.5/weather"
            "?lat=" + strLatitude + "&lon=" + strLongitude + "&appid=" + urlEncode(g_ptrSystem->DeviceConfig().GetOpenWeatherAPIKey());
        http.begin(url);
        int httpResponseCode = http.GET();
        if (httpResponseCode > 0)
        {
            iconToday = -1;
            AllocatedJsonDocument jsonDoc(1024);
            String todayJson = http.getString();
            DeserializationError de = deserializeJson(jsonDoc, todayJson);
            debugI("DE code %s", de.c_str());

            // Once we have a non-zero temp we can start displaying things
            if (0 < jsonDoc["main"]["temp"])
                dataReady = true;

            temperature = KelvinToLocal(jsonDoc["main"]["temp"]);
            highToday   = KelvinToLocal(jsonDoc["main"]["temp_max"]);
            loToday     = KelvinToLocal(jsonDoc["main"]["temp_min"]);

            String iconIndex = jsonDoc["weather"][0]["icon"];
            debugI("icon %s", iconIndex.c_str());
            iconToday = iconIndex.toInt() + (iconIndex.endsWith("d") ? 0 : 100);
            debugI("Got today's temps: Now %d Lo %d, Hi %d, Icon %d", (int)temperature, (int)loToday, (int)highToday, (int)iconToday);

            const char * pszName = jsonDoc["name"];
            if (pszName)
                strLocationName = pszName;

            http.end();
            return true;
        }
        else
        {
            debugW("Error fetching Weather data for location: %s in country: %s", strLocation.c_str(), strCountryCode.c_str());
            http.end();
            return false;
        }
    }

    /**
     * @brief Entry point for network handler to retrive the current weather data
     * 
     */
    void UpdateWeather()
    {
        while(!WiFi.isConnected())
        {
            debugI("Delaying Weather update, waiting for WiFi...");
            vTaskDelay(pdMS_TO_TICKS(WEATHER_CHECK_WIFI_WAIT));
        }

        if (g_ptrSystem->DeviceConfig().GetOpenWeatherAPIKey().isEmpty())
        {
            debugW("No API Key Present!");
            return;
        }


        updateCoordinates();

        if (locationValid)
        {
            if (getWeatherData())
            {
                debugW("Got today's weather");
                if (getTomorrowTemps(highTomorrow, loTomorrow, iconTomorrow))
                {
                    debugI("Got tomorrow's weather");
                }
                else
                {
                    debugW("Failed to get tomorrow's weather");
                }
            }
            else
            {
                debugW("Failed to get today's weather");
            }
        }
    }

    /**
     * @brief Have the preferences for the location changed
     * 
     * @return true 
     * @return false 
     */
    bool HasLocationChanged()
    {
        String configLocation = g_ptrSystem->DeviceConfig().GetLocation();
        String configCountryCode =g_ptrSystem->DeviceConfig().GetCountryCode();

        return strLocation != configLocation || strCountryCode != configCountryCode;
    }

public:

    PatternWeather() : LEDStripEffect(EFFECT_MATRIX_WEATHER, "Weather")
    {
    }

    PatternWeather(const JsonObjectConst&  jsonObject) : LEDStripEffect(jsonObject)
    {
    }

    ~PatternWeather()
    {
        g_ptrSystem->NetworkReader().CancelReader(readerIndex);
    }

    bool Init(std::vector<std::shared_ptr<GFXBase>>& gfx) override
    {
        if (!LEDStripEffect::Init(gfx))
            return false;

        readerIndex = g_ptrSystem->NetworkReader().RegisterReader([this] { UpdateWeather(); });

        return true;
    }

    void Draw() override
    {
        const int fontHeight = 7;
        const int fontWidth  = 5;
        const int xHalf      = MATRIX_WIDTH / 2 - 1;

        g()->fillScreen(BLACK16);
        g()->fillRect(0, 0, MATRIX_WIDTH, 9, g()->to16bit(CRGB(0,0,128)));

        g()->setFont(&Apple5x7);

        time_t now;
        time(&now);

        auto secondsSinceLastUpdate = now - latestUpdate;

        // If location and/or country have changed, trigger an update regardless of timer, but
        // not more than once every half a minute
        if (secondsSinceLastUpdate >= WEATHER_INTERVAL_SECONDS || (HasLocationChanged() && secondsSinceLastUpdate >= 30))
        {
            latestUpdate = now;

            debugW("Triggering thread to check weather now...");
            // Trigger the weather reader.
            g_ptrSystem->NetworkReader().FlagReader(readerIndex);
        }

        if (iconToday != -1)
        {
            // Draw the graphics
            auto iconEntry = weatherIcons.find((iconToday));
            if (iconEntry != weatherIcons.end())
            {
                auto icon = iconEntry->second;
                JRESULT res = TJpgDec.drawJpg(0, 10, icon.contents, icon.length);
                if (JDR_OK != res)        // Draw the image
                    debugW("Could not display today icon %d, %d", iconToday, res);
            }
            else
            {
                debugW("Could not find today icon %d", iconToday);
            }
        }

        if (iconTomorrow != -1)
        {

            auto iconEntry = weatherIcons.find((iconTomorrow));
            if (iconEntry != weatherIcons.end())
            {
                auto icon = iconEntry->second;
                if (JDR_OK != TJpgDec.drawJpg(xHalf+1, 10, icon.contents, icon.length))        // Draw the image
                    debugW("Could not display tomorrow icon %d", iconTomorrow);
            }
            else
            {
                debugW("Could not find tomorrow icon %d", iconToday);
            }
        }

        // Print the town/city name

        int x = 0;
        int y = fontHeight + 1;
        g()->setCursor(x, y);
        g()->setTextColor(WHITE16);
        String showLocation = strLocation;
        showLocation.toUpperCase();
        if (g_ptrSystem->DeviceConfig().GetOpenWeatherAPIKey().isEmpty())
        {
            g()->print("No API Key");
            return;
        }
        else
            g()->print((strLocationName.isEmpty() ? showLocation : strLocationName).substring(0, (MATRIX_WIDTH - 2 * fontWidth)/fontWidth));

        // Display the temperature, right-justified

        if (dataReady)
        {
            String strTemp((int)temperature);
            x = MATRIX_WIDTH - fontWidth * strTemp.length();
            g()->setCursor(x, y);
            g()->setTextColor(g()->to16bit(CRGB(192,192,192)));
            g()->print(strTemp);
        }

        // Draw the separator lines

        y+=1;

        g()->drawLine(0, y, MATRIX_WIDTH-1, y, CRGB(0,0,128));
        g()->drawLine(xHalf, y, xHalf, MATRIX_HEIGHT-1, CRGB(0,0,128));
        y+=2 + fontHeight;

        // Figure out which day of the week it is

        time_t today = time(nullptr);
        const tm * todayTime = localtime(&today);
        const char * pszToday = pszDaysOfWeek[todayTime->tm_wday];
        const char * pszTomorrow = pszDaysOfWeek[ (todayTime->tm_wday + 1) % 7 ];

        // Draw the day of the week and tomorrow's day as well

        g()->setTextColor(WHITE16);
        g()->setCursor(0, MATRIX_HEIGHT);
        g()->print(pszToday);
        g()->setCursor(xHalf+2, MATRIX_HEIGHT);
        g()->print(pszTomorrow);

        // Draw the temperature in lighter white

        if (dataReady)
        {
            g()->setTextColor(g()->to16bit(CRGB(192,192,192)));
            String strHi((int) highToday);
            String strLo((int) loToday);

            // Draw today's HI and LO temperatures

            x = xHalf - fontWidth * strHi.length();
            y = MATRIX_HEIGHT - fontHeight;
            g()->setCursor(x,y);
            g()->print(strHi);
            x = xHalf - fontWidth * strLo.length();
            y+= fontHeight;
            g()->setCursor(x,y);
            g()->print(strLo);

            // Draw tomorrow's HI and LO temperatures

            strHi = String((int)highTomorrow);
            strLo = String((int)loTomorrow);
            x = MATRIX_WIDTH - fontWidth * strHi.length();
            y = MATRIX_HEIGHT - fontHeight;
            g()->setCursor(x,y);
            g()->print(strHi);
            x = MATRIX_WIDTH - fontWidth * strLo.length();
            y+= fontHeight;
            g()->setCursor(x,y);
            g()->print(strLo);
        }
    }
};

#endif
