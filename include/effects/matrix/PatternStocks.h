//+--------------------------------------------------------------------------
//
// File:        PatternStocks.h
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
//    Retrieves stock quotes from private server and displays them
//
// History:     May-07-2024         davepl          Created
//
//---------------------------------------------------------------------------

#ifndef PatternStocks_H
#define PatternStocks_H

#include <Arduino.h>
#include <string.h>
#include <HTTPClient.h>
#include <UrlEncode.h>
#include <ledstripeffect.h>
#include <ledmatrixgfx.h>
#include <ArduinoJson.h>
#include "systemcontainer.h"
#include <chrono>
#include <thread>
#include <vector>
#include <map>
#include "TJpg_Decoder.h"
#include "effects.h"
#include "types.h"

using namespace std;
using namespace std::chrono;
using namespace std::chrono_literals;

#define STOCKS_INTERVAL_SECONDS 600s

// StockData and StockPoint are used to hold the data retrieved from the server

class StockPoint 
{
public:
    system_clock::time_point dt;
    float val;
};

class StockData 
{
public:
    String symbol;
    system_clock::time_point timestamp;
    float open = 0.0f;
    float high = 0.0f;
    float low = 0.0f;
    float close = 0.0f;
    float volume = 0.0f;
    vector<StockPoint> points;

    String to_string() const 
    {
        String oss;
        oss = "Symbol: " + symbol + 
               " Timestamp: " + timestamp.time_since_epoch().count() + 
               " Open: " + open + 
               " High: " + high + 
               " Low: " + low + 
               " Close: " + close + 
               " Volume: " + volume + 
               " History: " + points.size() + " points";
        return oss;
    }
};

// PatternStocks
//
// Retrieves stock quotes from private server and displays them

class PatternStocks : public LEDStripEffect
{
private:

    system_clock::time_point lastUpdate;

    using StockDataCallback = function<void(const StockData&)>;

    void GetQuote(const String &symbol, StockDataCallback callback)
    {
        HTTPClient http;
        String url = "http://localhost:8888/?ticker=" + symbol;
        http.begin(url);

        int httpCode = http.GET();
        if (httpCode == HTTP_CODE_OK) // Check is moved to here
        {
            String payload = http.getString(); // Get the response payload
            DynamicJsonDocument doc(4096);
            DeserializationError error = deserializeJson(doc, payload);
            if (!error)
            {
                StockData stockData;
                stockData.symbol = doc["symbol"].as<String>();
                stockData.timestamp = system_clock::from_time_t(doc["timestamp"].as<time_t>());
                stockData.open = doc["open"].as<float>();
                stockData.high = doc["high"].as<float>();
                stockData.low = doc["low"].as<float>();
                stockData.close = doc["close"].as<float>();
                stockData.volume = doc["volume"].as<float>();

                for (JsonVariant point : doc["points"].as<JsonArray>())
                {
                    StockPoint stockPoint;
                    stockPoint.dt = system_clock::from_time_t(point["dt"].as<time_t>());
                    stockPoint.val = point["val"].as<float>();
                    stockData.points.push_back(stockPoint);
                }

                callback(stockData); // Successful retrieval and parsing
            }
            else
            {
                Serial.printf("Failed to parse JSON: %s\n", error.c_str());
                callback(StockData()); // Parsing error
            }
        }
        else
        {
            Serial.printf("[HTTP] GET failed, error: %s\n", http.errorToString(httpCode).c_str());
            callback(StockData()); // HTTP request error
        }

        http.end(); // Close the connection properly
    }

    // Should this effect show its title.
    // The stocks effect does not show a title so it doesn't obscure our text display
    
    virtual bool ShouldShowTitle() const
    {
        return false;
    }

    size_t DesiredFramesPerSecond() const override
    {
        return 25;
    }

    bool RequiresDoubleBuffering() const override
    {
        return false;
    }

public:

    PatternStocks() : LEDStripEffect(EFFECT_MATRIX_STOCKS, "Stocks")
    {
    }

    PatternStocks(const JsonObjectConst&  jsonObject) : LEDStripEffect(jsonObject)
    {
    }

    ~PatternStocks()
    {
    }

    bool Init(std::vector<std::shared_ptr<GFXBase>>& gfx) override
    {
        if (!LEDStripEffect::Init(gfx))
            return false;

        // One-time init

        return true;
    }

    void Draw() override
    {
        g()->fillScreen(BLACK16);
        g()->fillRect(0, 0, MATRIX_WIDTH, 9, g()->to16bit(CRGB(0,0,128)));
        
        auto now = system_clock::now();
        if (now - lastUpdate >= STOCKS_INTERVAL_SECONDS) 
        {
            lastUpdate = now;
            debugI("Triggering thread to update stocks now...");
        }    
    }
};

#endif
