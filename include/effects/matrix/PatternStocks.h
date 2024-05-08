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
//    Retrieves stock quotes from private server and displays them.  The
//    Init function spins off a thread that fetches the stock data from the
//    private server, then we periodically update the stock data by checking
//    it's age and refreshing it if it's too old.
//
// History:     May-07-2024         davepl          Created
//
//---------------------------------------------------------------------------

#ifndef PatternStocks_H
#define PatternStocks_H

#include <Arduino.h>
#include <gfxfont.h>                // Adafruit GFX font structs
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

// We can only include the font header once, and Weather already does it, so we just extern it.  If
// the weather effect is not included in the build, we'll then have to include it here.

extern const uint8_t Apple5x7Bitmaps[] PROGMEM;

using namespace std;
using namespace std::chrono;
using namespace std::chrono_literals;

#define STOCKS_INTERVAL_SECONDS 6s

class AnimatedText
{
  private:
    int startX;
    int startY;
    int endX;
    int endY;
    int currentX;
    int currentY;
    String text;
    CRGB color;
    float animationTime;
    system_clock::time_point startTime;
    const GFXfont * pfont;

    
  public:
    AnimatedText(String text, CRGB color, const GFXfont * pfont, float animationTime, int startX, int startY, int endX, int endY)
    {
        startTime = system_clock::now();
        this->startX = startX;
        this->startY = startY;
        this->endX = endX;
        this->endY = endY;
        this->text = text;
        this->color = color;
        this->animationTime = animationTime;
        this->currentX = startX;
        this->currentY = startY;
        this->pfont = pfont;
    }

    void UpdatePos()
    {
        // Current time
        auto currentTime = system_clock::now();

        // Calculate elapsed time
        auto elapsedTime = duration_cast<duration<float>>(currentTime - startTime);

        // Calculate progress as a percentage
        float progress = std::min(elapsedTime.count() / animationTime, 1.0f);

        // Updated positions are based on progress from start to now
        currentX = startX + (endX - startX) * progress;
        currentY = startY + (endY - startY) * progress;
    }

    void Draw(GFXBase *g)
    {
        g->setFont(pfont);
        g->setTextColor(g->to16bit(color));
        g->setCursor(currentX, currentY);
        g->print(text);
    }
};

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

AnimatedText textSymbol = AnimatedText("STOCK",  CRGB::White, &Apple5x7, 1.0f, 0, 0,  MATRIX_WIDTH, 0);
AnimatedText textPrice  = AnimatedText("PRICE",  CRGB::Grey, &Apple5x7,  1.0f, 0, 8,  MATRIX_WIDTH, 8);
AnimatedText textChange = AnimatedText("CHANGE", CRGB::White, &Apple5x7, 1.0f, 0, 16, MATRIX_WIDTH, 16);
AnimatedText textVolume = AnimatedText("VOLUME", CRGB::Grey, &Apple5x7,  1.0f, 0, 24, MATRIX_WIDTH, 24);

class PatternStocks : public LEDStripEffect
{
private:
        
    int          iCurrentStock = 0;

    bool isUpdating = false;                // Flag to check if update is in progress
    system_clock::time_point lastUpdate;    // Time of last update
    std::map<String, StockData> stockData;  // map of stock symbols to quotes

    using StockDataCallback = function<void(const StockData&)>;

    HTTPClient http;

    void GetQuote(const String &symbol, StockDataCallback callback = nullptr)
    {
        String url   = String(cszQuoteServer);        
        String query = "/?ticker=" + symbol;
        int port     = 8888;

        http.begin(url, port, query);

        int httpCode = http.GET();
        if (httpCode == HTTP_CODE_OK) 
        {
            debugI("HTTP GET OK");
            String payload = http.getString(); // Get the response payload
            DynamicJsonDocument doc(4096);
            DeserializationError error = deserializeJson(doc, payload);
            debugV("JSON: %s", payload.c_str());
            
            if (!error)
            {
                StockData stockData;
                stockData.symbol    = doc["symbol"].as<String>();
                stockData.timestamp = system_clock::from_time_t(doc["timestamp"].as<time_t>());
                stockData.open      = doc["open"].as<float>();
                stockData.high      = doc["high"].as<float>();
                stockData.low       = doc["low"].as<float>();
                stockData.close     = doc["close"].as<float>();
                stockData.volume    = doc["volume"].as<float>();

                for (JsonVariant point : doc["points"].as<JsonArray>())
                {
                    StockPoint stockPoint;
                    stockPoint.dt  = system_clock::from_time_t(point["dt"].as<time_t>());
                    stockPoint.val = point["val"].as<float>();
                    stockData.points.push_back(stockPoint);
                }
                if (callback)
                    callback(stockData); // Successful retrieval and parsing
            }
            else
            {
                Serial.printf("Failed to parse JSON: %s\n", error.c_str());
                if (callback)
                    callback(StockData()); // Parsing error
            }
        }
        else
        {
            Serial.printf("[HTTP] GET failed, error: %s\n", http.errorToString(httpCode).c_str());
            if (callback)
                callback(StockData()); // HTTP request error
        }

        http.end(); // Close the connection properly
    }

    // GetAllQuotes
    //
    // Retrieves all quotes for the given list of symbols, provided as a comma-separated string
    // Calls getQuote for each symbol in the list, and saves it in the stockData map

    void GetAllQuotes(const String& symbols, StockDataCallback callback = nullptr)
    {
        // Lambda to split the symbols string by comma, because somehow this is cooler than strtok() was in 1989
        // and I want to flex, but also because strtok() is not thread-safe and we're using threads here.  So there.
        // Also, I'm using std::string here because std::getline() is easier to use with std::string than with String.

        auto split = [](const String& s, char delimiter) -> std::vector<String> 
        {
            std::vector<String> tokens;
            std::istringstream tokenStream(s.c_str());
            std::string token; // Change to std::string to work with std::getline

            while (std::getline(tokenStream, token, delimiter)) {
                tokens.push_back(String(token.c_str())); // Convert std::string to String when pushing back
            }
            return tokens;
        };

        // Use the lambda to split the symbols string in a vector of strings

        std::vector<String> symbolList = split(symbols, ',');

        // And now for each symbol in the list, call GetQuote and save the data in the stockData map

        for (const String& symbol : symbolList)
        {
            GetQuote(symbol, [this, callback](const StockData& stockDataReceived) 
            {
                if (!stockDataReceived.symbol.isEmpty())    // Check if the stock data is not empty
                    this->stockData[stockDataReceived.symbol] = stockDataReceived;
                if (callback)
                    callback(stockDataReceived);            // Optionally, call the callback for each symbol's data
            });
        }
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

    static void FetchQuotesTask(void *pvParameters) 
    {
        auto *instance = static_cast<PatternStocks*>(pvParameters); // Cast the void pointer back to the correct type

        debugI("Background task started to update stocks...");
        instance->isUpdating = true;
        instance->GetAllQuotes(String(cszStockList), [instance](const StockData& stockDataReceived) 
        {
            if (!stockDataReceived.symbol.isEmpty())
                debugI("Received stock data for %s", stockDataReceived.symbol.c_str());
            else
                debugI("Failed to retrieve stock data");
        });
        instance->isUpdating = false;

        vTaskDelete(NULL); // Cleanly delete the task once done
    }

    void BackgroundFetchQuotes()
    {
        // Use a C++ thread to run GetAllQuotes without blocking the main thread because we're cool like that,
        // and Init cannot be used because it's called from the main thread

        if (!isUpdating)
        {
            isUpdating = true;
            xTaskCreate(
                    FetchQuotesTask,          // Task function
                    "FetchQuotes",            // Name of the task (for debugging purposes)
                    8192,                     // Stack size in words
                    this,                     // Pass the `this` pointer as the task parameter
                    1,                        // Priority of the task
                    NULL                      // Task handle (not needed unless you want to reference the task later)
                );
        }
    }

    bool Init(std::vector<std::shared_ptr<GFXBase>>& gfx) override
    {
        if (!LEDStripEffect::Init(gfx))
            return false;

        lastUpdate = system_clock::now();

        return true;
    }

    void StartQuoteDisplay(StockData data)
    {
        // Display the stock data
        debugI("Displaying stock data for %s", data.symbol.c_str());

        textSymbol = AnimatedText(data.symbol, CRGB::White, &Apple5x7, 1.0f, 64, 0, 0, 0);
        textPrice  = AnimatedText(String(data.close, 2), CRGB::LightGrey, &Apple5x7, 1.0f, 64, 0, 32, 0);
        textChange = AnimatedText(String(data.close - data.open, 2), data.close >= data.open ? CRGB::Green : CRGB::Red, &Apple5x7, 1.0f, MATRIX_WIDTH, 7, 32, 7);
        textVolume = AnimatedText(String(data.volume, 0), CRGB::LightGrey, &Apple5x7, 1.0f, -MATRIX_WIDTH, 15, 0, 15);
    }

    void UpdateQuoteDisplay()
    {
        textSymbol.UpdatePos();
        textPrice.UpdatePos();
        textChange.UpdatePos();
        textVolume.UpdatePos();

        textSymbol.Draw(g().get());
        textPrice.Draw(g().get());
        textChange.Draw(g().get());
        textVolume.Draw(g().get());
    }

    void Draw() override
    {
        g()->fillScreen(BLACK16);
        g()->fillRect(0, 0, MATRIX_WIDTH, 9, g()->to16bit(CRGB(0,0,128)));
        
        if (WiFi.isConnected())
        {
            BackgroundFetchQuotes();                
        }

        auto now = system_clock::now();
        if (now - lastUpdate >= STOCKS_INTERVAL_SECONDS) 
        {
            lastUpdate = now;
            if (!stockData.empty())
            {
                int index = iCurrentStock;
                iCurrentStock = (iCurrentStock + 1) % stockData.size();
                auto it = stockData.begin();
                std::advance(it, iCurrentStock);
                StartQuoteDisplay(it->second);
            }
        }    
        UpdateQuoteDisplay();
    }
};

#endif
