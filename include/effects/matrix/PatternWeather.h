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
// History:     Jun-25-202         Davepl      Adapted from own code
//
//---------------------------------------------------------------------------

#ifndef PatternWeather_H
#define PatternWeather_H

#include <Arduino.h>
#include <string.h>
#include <HTTPClient.h>
#include <ledstripeffect.h>
#include <ledmatrixgfx.h>
#include <secrets.h>
#include <RemoteDebug.h>
#include <FastLED.h>
#include <ArduinoJson.h>
#include "globals.h"
#include <FontGfx_apple5x7.h>

static const char * pszDaysOfWeek[] = { "SUN", "MON", "TUE", "WED", "THU", "FRI", "SAT" };

class PatternWeather : public LEDStripEffect
{
private:

    String strWeatherDescription = "";
    String strLocation           = "";
    int    dayOfWeek             = 0;
    float  temperature           = 0.0f;
    float  pressure              = 0.0f;
    float  highToday             = 0.0f;
    float  loToday               = 0.0f;
    float  highTomorrow          = 0.0f;
    float  loTomorrow            = 0.0f;

    // The weather is obviously weather, and we don't want text overlaid on top of our text

    virtual bool ShouldShowTitle() const
    {
        return false;
    }

    inline float KelvinToFarenheit(float K)
    {
        return (K - 273.15) * 9.0f/5.0f + 32;    
    }

    inline float KelvinToCelcius(float K)
    {
        return K * 9.0f/5.0f + 32;    
    }
    
    inline float KelvinToLocal(float K)
    {
        // Some future Canadian or Netherlander should add Celcius support as a preference, and that would replace
        // the "true" test that's here as a placeholder for now.

        if (true)
            return KelvinToFarenheit(K);
        else
            return KelvinToCelcius(K);
    }
    // getTomorrowTemps
    //
    // Request a forecast and then parse out the high and low temps for tomorrow

    String getTomorrowTemps(const String &zipCode, float& highTemp, float& lowTemp) 
    {
        HTTPClient http;
        String url = "http://api.openweathermap.org/data/2.5/forecast?zip=" + zipCode + ",us&appid=" + cszOpenWeatherAPIKey;
        http.begin(url);
        int httpResponseCode = http.GET();

        if (httpResponseCode > 0) 
        {
            String response = http.getString();
            DynamicJsonDocument doc(1024);
            deserializeJson(doc, response);

            // Get tomorrow's date
            time_t tomorrow = time(nullptr) + 86400;
            tm* tomorrowTime = localtime(&tomorrow);
            char dateStr[11];
            strftime(dateStr, sizeof(dateStr), "%Y-%m-%d", tomorrowTime);

            // Look for the temperature data for tomorrow
            JsonArray list = doc["list"];
            for (JsonObject entry : list) {
                String dt_txt = entry["dt_txt"].as<String>();
                if (dt_txt.startsWith(dateStr)) {
                    JsonObject main = entry["main"];
                    highTemp = KelvinToLocal(main["temp_max"]);
                    lowTemp  = KelvinToLocal(main["temp_min"]);
                    break;
                }
            }
            http.end();
            return response;

        }
        else {
            debugW("Error fetching forecast data for zip: %s", zipCode);
        }
        http.end();
        return "";
    }

    // getWeatherData
    //
    // Get the current temp and the high and low for today

    String getWeatherData(const String &zipCode)
    {
        HTTPClient http;
        String url = "http://api.openweathermap.org/data/2.5/weather?zip=" + zipCode + ",us&appid=" + cszOpenWeatherAPIKey;
        http.begin(url);
        int httpResponseCode = http.GET();
        if (httpResponseCode > 0)
        {
            String weatherData = http.getString();
            DynamicJsonDocument jsonDoc(1024);
            deserializeJson(jsonDoc, weatherData);
            temperature = KelvinToLocal(jsonDoc["main"]["temp"]);
            highToday   = KelvinToLocal(jsonDoc["main"]["temp_max"]);
            loToday     = KelvinToLocal(jsonDoc["main"]["temp_min"]);

            const char * pszDescription = jsonDoc["weather"][0]["description"];
            if (pszDescription)
                strWeatherDescription = pszDescription;

            const char * pszName = jsonDoc["name"];
            if (pszName)
                strLocation = pszName;

            http.end();
            return weatherData;
        }
        else
        {
            Serial.printf("Error fetching Weather data for zip: %s", zipCode);
            http.end();
            return "";
        }
    }

public:

    PatternWeather() : LEDStripEffect("Weather")
    {
    }

    virtual void Draw()
    {
        const int fontHeight = 7;
        const int fontWidth  = 5;
        const int xHalf = MATRIX_WIDTH / 2 - 1;
        bool bShouldCheckWeather = false;

        graphics()->fillScreen(CRGB(0, 0, 0));
        graphics()->fillRect(0, 0, MATRIX_WIDTH-1, 9, graphics()->to16bit(CRGB(0,0,128)));        

        graphics()->setFont(&Apple5x7);

        if (WiFi.isConnected())
        {
            if (strLocation.isEmpty())
            {
                EVERY_N_SECONDS_I(timingObj, 0)
                {
                    String weatherData = getWeatherData(cszZipCode);
                    if (!strLocation.isEmpty())
                    {
                        getTomorrowTemps(cszZipCode, highTomorrow, loTomorrow);
                        // Looks like success, so recheck the weather in 10 minutes
                        timingObj.setPeriod(60 * 10);
                    }
                    else
                    {   
                        // On a failure, check back soon
                        timingObj.setPeriod(20);                
                    }
                }
            }
        }

        int x = 0;
        int y = fontHeight + 1;
        graphics()->setCursor(x, y);
        graphics()->setTextColor(WHITE16);
        strLocation.toUpperCase();
        graphics()->print(strLocation.isEmpty() ? String(cszZipCode) : strLocation);

        String strTemp((int)temperature);
        x = MATRIX_WIDTH - fontWidth * strTemp.length();
        graphics()->setCursor(x, y);
        graphics()->setTextColor(graphics()->to16bit(CRGB(192,192,192)));
        graphics()->print(strTemp);

        y+=1;

        graphics()->drawLine(0, y, MATRIX_WIDTH-1, y, CRGB(0,0,128));
        graphics()->drawLine(xHalf, y, xHalf, MATRIX_HEIGHT-1, CRGB(0,0,128));
        y+=2 + fontHeight;

        // Figure out which day of the week it is

        time_t today = time(nullptr);
        tm * todayTime = localtime(&today);
        const char * pszToday = pszDaysOfWeek[todayTime->tm_wday];
        const char * pszTomorrow = pszDaysOfWeek[ (todayTime->tm_wday + 1) % 7 ];

        graphics()->setTextColor(WHITE16);
        graphics()->setCursor(0, MATRIX_HEIGHT);
        graphics()->print(pszToday);

        graphics()->setCursor(xHalf+2, MATRIX_HEIGHT);
        graphics()->print(pszTomorrow);

        graphics()->setTextColor(graphics()->to16bit(CRGB(192,192,192)));
        
        String strHi((int) highToday);
        String strLo((int) loToday);
        
        x = xHalf - fontWidth * strHi.length();
        y = MATRIX_HEIGHT - fontHeight;
        graphics()->setCursor(x,y);
        graphics()->print(strHi);
        x = xHalf - fontWidth * strLo.length();
        y+= fontHeight;
        graphics()->setCursor(x,y);
        graphics()->print(strLo);

        strHi = String((int)highTomorrow);
        strLo = String((int)loTomorrow);

        x = MATRIX_WIDTH - fontWidth * strHi.length();
        y = MATRIX_HEIGHT - fontHeight;
        graphics()->setCursor(x,y);
        graphics()->print(strHi);
        x = MATRIX_WIDTH - fontWidth * strLo.length();
        y+= fontHeight;
        graphics()->setCursor(x,y);
        graphics()->print(strLo);

    }
};

#endif
