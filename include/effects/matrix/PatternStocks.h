#pragma once

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
#include <string.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <algorithm>
#include <chrono>
#include <thread>
#include <vector>
#include <map>
#include <mutex>
#include <sstream>

#include "formatsize.h"
#include "gfxfont.h"                // Adafruit GFX font structs
#include "systemcontainer.h"

extern const GFXfont Apple5x7 PROGMEM;
using namespace std;
using namespace std::chrono;
using namespace std::chrono_literals;

#define STOCKS_UPDATE_INTERVAL_SECONDS 6s
#define STOCKS_FETCH_INTERVAL_SECONDS  60s

#ifndef DEFAULT_STOCK_SERVER
#define DEFAULT_STOCK_SERVER           "davepl.dyndns.org:8888"
#endif
#define DEFAULT_TICKER_SYMBOLS         "AAPL,AMZN,TSLA,MSFT,SPCX,INTC"

// AnimatedText
//
// A class that draws text on the screen and animates it from one position to another

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

    // UpdatePos
    //
    // Updates the position of the text based on the elapsed time since the start of the animation

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

    // Draw
    //
    // Draws the text on the screen at the current position

    void Draw(GFXBase *g)
    {
        g->setFont(pfont);
        g->setTextColor(g->to16bit(color));
        g->setCursor(currentX, currentY);
        g->print(text);
    }
};

// StockPoint and StockData
//
// StockData and StockPoint are used to hold the data retrieved from the server.  A point is a single
// data point in the history of a stock, and StockData is the data for a single stock, including the
// current price and the history of points.

class StockPoint
{
public:
    uint32_t epoch = 0;
    float val;
};

class StockData
{
public:
    String symbol;
    String marketState;
    String currentPriceTime;
    String currentPriceTimeZone;
    String officialCloseDate;

    float currentPrice = 0.0f;
    float previousClose = 0.0f;
    float open = 0.0f;
    float high = 0.0f;
    float low = 0.0f;
    float close = 0.0f;
    float officialClose = 0.0f;
    float regularClose = 0.0f;
    float change = 0.0f;
    float changePercent = 0.0f;
    float volume = 0.0f;

    bool hasCurrentPrice = false;
    bool hasPreviousClose = false;
    bool hasOpen = false;
    bool hasHigh = false;
    bool hasLow = false;
    bool hasClose = false;
    bool hasOfficialClose = false;
    bool hasRegularClose = false;
    bool hasChange = false;
    bool hasChangePercent = false;
    bool hasVolume = false;

    vector<StockPoint> points;

    float DisplayPrice() const
    {
        return hasCurrentPrice ? currentPrice : close;
    }

    float DisplayChange() const
    {
        return change;
    }

    String to_string() const
    {
        return "Symbol: " + symbol +
               " State: " + marketState +
               " Current: " + DisplayPrice() +
               " Previous close: " + previousClose +
               " Open: " + open +
               " High: " + high +
               " Low: " + low +
               " Day close: " + close +
               " Official close: " + officialClose +
               " Change: " + DisplayChange() +
               " Change percent: " + changePercent +
               " Volume: " + volume +
               " History: " + points.size() + " points";
    }
};

// PatternStocks
//
// Retrieves stock quotes from private server and displays them

class PatternStocks : public EffectWithId<PatternStocks>
{
    AnimatedText textSymbol = AnimatedText("STOCK",  CRGB::White, &Apple5x7,  1.0f, MATRIX_WIDTH, 0,  0, 0);
    AnimatedText textPrice  = AnimatedText("PRICE",  CRGB::Grey,  &Apple5x7,  1.0f, MATRIX_WIDTH, 8,  0, 8);
    AnimatedText textChange = AnimatedText("CHANGE", CRGB::White, &Apple5x7,  1.0f, MATRIX_WIDTH, 16, 0, 16);
    AnimatedText textVolume = AnimatedText("VOLUME", CRGB::Grey,  &Apple5x7,  1.0f, MATRIX_WIDTH, 24, 0, 24);

private:
    // This requires a matching INIT_EFFECT_SETTING_SPECS() in effects.cpp or linker errors will ensue
    DECLARE_EFFECT_SETTING_SPECS(mySettingSpecs);

    size_t       readerIndex = SIZE_MAX;

    String       stockServer = DEFAULT_STOCK_SERVER;
    String       tickerSymbols = DEFAULT_TICKER_SYMBOLS;

    int          iCurrentStock = 0;
    size_t       lastCount     = SIZE_MAX;

    system_clock::time_point lastUpdate;    // Time of last update
    system_clock::time_point nextFetch = system_clock::now();  // Time of last quote pull

    std::map<String, StockData> stockData;  // map of stock symbols to quotes
    mutable std::mutex stockDataMutex;
    std::mutex quoteFetchMutex;

    using StockDataCallback = function<void(const StockData&)>;

    HTTPClient http;

    void AddRealtimeQuoteKeyHeader()
    {
        const char * realtimeQuoteKey = cszRealtimeQuoteKey;
        if (realtimeQuoteKey != nullptr && realtimeQuoteKey[0] != '\0')
            http.addHeader("X-Stockserver-Realtime-Key", realtimeQuoteKey);
    }

    void NormalizeStockServer()
    {
#if MESMERIZER
        if (stockServer == "davepl.dyndns.org:8888" || stockServer == "localhost:8888" || stockServer == "127.0.0.1:8888")
            stockServer = DEFAULT_STOCK_SERVER;
#endif
    }

    void DrawCompactQuoteDisplay(int screenHeight)
    {
        const int topLineHeight = screenHeight / 2;

        StockData currentStock;
        {
            std::lock_guard dataGuard(stockDataMutex);
            if (stockData.empty())
            {
                g().DrawTextInBand("STOCKS", 0, topLineHeight, CRGB::White);
                g().DrawTextInBand("--", topLineHeight, screenHeight - topLineHeight, CRGB::LightGrey);
                return;
            }

            const size_t stockIndex = std::min(static_cast<size_t>(std::max(0, iCurrentStock)), stockData.size() - 1);
            auto it = stockData.begin();
            std::advance(it, stockIndex);
            currentStock = it->second;
        }

        const float displayPrice = currentStock.DisplayPrice();
        const String priceText = displayPrice >= 10000 ? String(displayPrice, 0) : String(displayPrice, 2);
        const CRGB priceColor = currentStock.DisplayChange() >= 0.0f ? CRGB::LightGreen : CRGB::Red;

        g().DrawTextInBand(currentStock.symbol, 0, topLineHeight, CRGB::White);
        g().DrawTextInBand(priceText, topLineHeight, screenHeight - topLineHeight, priceColor);
    }

    static float JsonFloatOr(JsonVariantConst value, float fallback = 0.0f)
    {
        return value.is<float>() || value.is<double>() || value.is<int>() ? value.as<float>() : fallback;
    }

    static bool JsonHasNumber(JsonVariantConst value)
    {
        return value.is<float>() || value.is<double>() || value.is<int>() || value.is<unsigned int>();
    }

    static String JsonStringOr(JsonVariantConst value, const String& fallback = "")
    {
        return value.is<const char*>() ? value.as<String>() : fallback;
    }

    static StockData ParseQuoteV2(JsonDocument& doc)
    {
        StockData stockData;

        // Compact HUB75 profile.
        if (doc["v"].as<int>() == 2)
        {
            stockData.symbol               = JsonStringOr(doc["s"]);
            stockData.marketState          = JsonStringOr(doc["st"]);
            stockData.currentPrice         = JsonFloatOr(doc["p"]);
            stockData.hasCurrentPrice      = JsonHasNumber(doc["p"]);
            stockData.previousClose        = JsonFloatOr(doc["pc"]);
            stockData.hasPreviousClose     = JsonHasNumber(doc["pc"]);
            stockData.officialClose        = JsonFloatOr(doc["oc"]);
            stockData.hasOfficialClose     = JsonHasNumber(doc["oc"]);
            stockData.close                = stockData.officialClose;
            stockData.hasClose             = stockData.hasOfficialClose;
            stockData.officialCloseDate    = JsonStringOr(doc["ocd"]);
            stockData.hasRegularClose      = JsonHasNumber(doc["rc"]);
            stockData.regularClose         = JsonFloatOr(doc["rc"]);
            stockData.change               = JsonFloatOr(doc["chg"]);
            stockData.hasChange            = JsonHasNumber(doc["chg"]);
            stockData.changePercent        = JsonFloatOr(doc["chgp"]);
            stockData.hasChangePercent     = JsonHasNumber(doc["chgp"]);
            stockData.currentPriceTimeZone = JsonStringOr(doc["tz"]);

            for (JsonArray point : doc["h"].as<JsonArray>())
            {
                if (point.size() < 2)
                    continue;

                StockPoint stockPoint;
                stockPoint.epoch = point[0].as<uint32_t>();
                stockPoint.val   = JsonFloatOr(point[1]);
                stockData.points.push_back(stockPoint);
            }

            return stockData;
        }

        // Full v2 profile.
        if (doc["api_version"].as<int>() == 2)
        {
            stockData.symbol               = JsonStringOr(doc["symbol"]);
            stockData.marketState          = JsonStringOr(doc["market_state"]);
            stockData.currentPrice         = JsonFloatOr(doc["current_price"]);
            stockData.hasCurrentPrice      = JsonHasNumber(doc["current_price"]);
            stockData.currentPriceTime     = JsonStringOr(doc["current_price_time"]);
            stockData.currentPriceTimeZone = JsonStringOr(doc["current_price_time_zone"]);
            stockData.previousClose        = JsonFloatOr(doc["previous_close"]);
            stockData.hasPreviousClose     = JsonHasNumber(doc["previous_close"]);
            stockData.officialClose        = JsonFloatOr(doc["official_close"]);
            stockData.hasOfficialClose     = JsonHasNumber(doc["official_close"]);
            stockData.officialCloseDate    = JsonStringOr(doc["official_close_date"]);
            stockData.hasRegularClose      = JsonHasNumber(doc["regular_close"]);
            stockData.regularClose         = JsonFloatOr(doc["regular_close"]);
            stockData.change               = JsonFloatOr(doc["change"]);
            stockData.hasChange            = JsonHasNumber(doc["change"]);
            stockData.changePercent        = JsonFloatOr(doc["change_percent"]);
            stockData.hasChangePercent     = JsonHasNumber(doc["change_percent"]);

            JsonObjectConst day = doc["day"].as<JsonObjectConst>();
            stockData.open   = JsonFloatOr(day["open"]);
            stockData.hasOpen = JsonHasNumber(day["open"]);
            stockData.high   = JsonFloatOr(day["high"]);
            stockData.hasHigh = JsonHasNumber(day["high"]);
            stockData.low    = JsonFloatOr(day["low"]);
            stockData.hasLow = JsonHasNumber(day["low"]);
            stockData.close  = JsonFloatOr(day["close"], stockData.officialClose);
            stockData.hasClose = JsonHasNumber(day["close"]) || stockData.hasOfficialClose;
            stockData.volume = JsonFloatOr(day["volume"]);
            stockData.hasVolume = JsonHasNumber(day["volume"]);

            JsonArrayConst points = doc["history"]["points"].as<JsonArrayConst>();
            for (JsonObjectConst point : points)
            {
                StockPoint stockPoint;
                stockPoint.epoch = point["epoch"].as<uint32_t>();
                stockPoint.val   = JsonFloatOr(point["price"]);
                stockData.points.push_back(stockPoint);
            }
        }

        return stockData;
    }

    void GetQuote(const String &symbol, StockDataCallback callback = nullptr)
    {
        http.begin("http://" + stockServer + "/?ticker=" + symbol + "&v=2&points=64");
        AddRealtimeQuoteKeyHeader();

        int httpCode = http.GET();
        if (httpCode == HTTP_CODE_OK)
        {
            debugD("HTTP GET OK");
            String payload = http.getString(); // Get the response payload
            auto doc = CreateJsonDocument();
            DeserializationError error = deserializeJson(doc, payload);
            debugV("JSON: %s", payload.c_str());

            if (!error)
            {
                StockData stockData = ParseQuoteV2(doc);
                if (callback)
                    callback(stockData); // Successful retrieval and parsing
            }
            else
            {
                debugE("Failed to parse JSON: %s\n", error.c_str());
                if (callback)
                    callback(StockData()); // Parsing error
            }
        }
        else
        {
            debugE("[HTTP] GET failed, error: %s\n", http.errorToString(httpCode).c_str());
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
        std::lock_guard fetchGuard(quoteFetchMutex);

        // Lambda to split the symbols string by comma, because somehow this is cooler than strtok() was in 1989
        // and I want to flex, but also because strtok() is not thread-safe and we're using threads here.  So there.
        // Also, I'm using std::string here because std::getline() is easier to use with std::string than with String.

        auto split = [](const String& s, char delimiter) -> std::vector<String>
        {
            std::vector<String> tokens;
            std::istringstream tokenStream(s.c_str());
            std::string token; // Change to std::string to work with std::getline

            while (std::getline(tokenStream, token, delimiter)) {
                String symbol(token.c_str()); // Convert std::string to String when pushing back
                symbol.trim();
                if (!symbol.isEmpty())
                    tokens.push_back(symbol);
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
                {
                    std::lock_guard dataGuard(stockDataMutex);
                    this->stockData[stockDataReceived.symbol] = stockDataReceived;
                }
                if (callback)
                    callback(stockDataReceived);            // Optionally, call the callback for each symbol's data
            });
        }
    }

    void FetchQuotes()
    {
        GetAllQuotes(tickerSymbols, [](const StockData& stockDataReceived)
        {
            if (stockDataReceived.symbol.isEmpty())
                debugI("Failed to retrieve stock data");
        });
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

  protected:

    // Create our SettingSpec instances if needed, and return (a pointer to) them
    EffectSettingSpecs* FillSettingSpecs() override
    {
        // Lazily load this class' SettingSpec instances if they haven't been already
        if (mySettingSpecs.size() == 0)
        {
            mySettingSpecs.push_back(SettingSpec::Validate(SettingSpec{
                .Name         = NAME_OF(stockServer),
                .FriendlyName = "Stock server location",
                .Description  = "The host and port of the service that provides stock data.",
                .Type         = SettingSpec::SettingType::String
            }));
            mySettingSpecs.push_back(SettingSpec::Validate(SettingSpec{
                .Name         = NAME_OF(tickerSymbols),
                .FriendlyName = "Ticker symbols",
                .Description  = "Comma-separated list of ticker symbols for which to retrieve and display stock data.",
                .Type         = SettingSpec::SettingType::String
            }));
        }

        return &mySettingSpecs;
    }

public:

    PatternStocks() : EffectWithId<PatternStocks>("Stocks") {}

    PatternStocks(const JsonObjectConst&  jsonObject) : EffectWithId<PatternStocks>(jsonObject)
    {
        if (jsonObject["sds"].is<String>())
            stockServer = jsonObject["sds"].as<String>();
        if (jsonObject["tsl"].is<String>())
            tickerSymbols = jsonObject["tsl"].as<String>();

        NormalizeStockServer();
    }

    ~PatternStocks()
    {
        g_ptrSystem->GetNetworkReader().CancelReader(readerIndex);
    }

    bool SerializeToJSON(JsonObject& jsonObject) override
    {
        auto jsonDoc = CreateJsonDocument();

        JsonObject root = jsonDoc.to<JsonObject>();
        LEDStripEffect::SerializeToJSON(root);

        jsonDoc["sds"] = stockServer;
        jsonDoc["tsl"] = tickerSymbols;

        return SetIfNotOverflowed(jsonDoc, jsonObject, __PRETTY_FUNCTION__);
    }


    bool Init(std::vector<std::shared_ptr<GFXBase>>& gfx) override
    {
        if (!LEDStripEffect::Init(gfx))
            return false;

        NormalizeStockServer();

        // Register a Network Reader task with no interval.  Will manually flag in Draw()
        readerIndex = g_ptrSystem->GetNetworkReader().RegisterReader([this] { FetchQuotes(); });

        // Fire off the stock data reader for an initial download of stock data
        lastUpdate = system_clock::now();
        g_ptrSystem->GetNetworkReader().FlagReader(readerIndex);

        return true;
    }

    void RefreshQuotes(const std::function<void(const StockData&)>& callback = nullptr)
    {
        GetAllQuotes(tickerSymbols, callback);
    }

    const String& StockServer() const
    {
        return stockServer;
    }

    // StartQuoteDisplay
    //
    // Given a StockData, sets the current display to show the stock data

    void StartQuoteDisplay(StockData data)
    {
        // Display the stock data
        debugD("Displaying stock data for %s", data.symbol.c_str());

        const float displayPrice = data.DisplayPrice();

        auto pricetext = displayPrice >= 10000 ? String(displayPrice, 0) : String(displayPrice, 2);
        auto pricelen  = pricetext.length();
        constexpr auto textwidth = 5;

        auto changetext = String(data.DisplayChange(), 2);
        auto changelen  = changetext.length();

        constexpr auto formatVolumeLargerThan = 1000000;
        auto voltext = data.hasVolume ? formatSize(data.volume, formatVolumeLargerThan, 0) : String("--");
        auto vollen  = voltext.length();

        textSymbol = AnimatedText(data.symbol, CRGB::White, &Apple5x7, 0.50f, -MATRIX_WIDTH, 8, 0, 8);
        textPrice  = AnimatedText(pricetext, CRGB::White, &Apple5x7, 0.75f, -MATRIX_WIDTH, 8, MATRIX_WIDTH - pricelen * textwidth, 8);
        textChange = AnimatedText(changetext, data.DisplayChange() >= 0.0f ? CRGB::LightGreen : CRGB::Red, &Apple5x7, 1.0f, -MATRIX_WIDTH, 15, MATRIX_WIDTH - changelen * textwidth, 15);
        textVolume = AnimatedText(voltext, CRGB::LightGrey, &Apple5x7, 1.0f, -MATRIX_WIDTH * 2, 22, MATRIX_WIDTH - vollen * textwidth, 22);
    }


    // UpdateQuoteDisplay
    //
    // Updates the position of the text and draws it on the screen, then draws
    // the up/down graph of the stock

    void UpdateQuoteDisplay()
    {
        auto& graphics = g();

        textSymbol.UpdatePos();
        textPrice.UpdatePos();
        textChange.UpdatePos();
        textVolume.UpdatePos();

        textSymbol.Draw(&graphics);
        textPrice.Draw(&graphics);
        textChange.Draw(&graphics);
        textVolume.Draw(&graphics);

        StockData currentStock;
        {
            std::lock_guard dataGuard(stockDataMutex);
            if (stockData.empty())
                return;

            if (iCurrentStock >= static_cast<int>(stockData.size()))
                iCurrentStock = 0;

            auto it = stockData.begin();
            std::advance(it, iCurrentStock);
            currentStock = it->second;
        }

        // Draw the stock history graph

        int y = 24;
        int h = MATRIX_HEIGHT - y;
        const size_t pointsToDraw = std::min(static_cast<size_t>(MATRIX_WIDTH), currentStock.points.size());

        if (pointsToDraw > 0)
        {
            // We have the high and low data in the stock, but let's not trust it and calculate it ourselves
            // If this works, Davepl wrote it.  If not, Robert made me do it!

            const size_t firstPointIndex = currentStock.points.size() - pointsToDraw;
            auto firstPoint = currentStock.points.begin() + firstPointIndex;
            auto lastPoint = currentStock.points.end();

            auto [minpoint, maxpoint] =
                std::minmax_element(firstPoint, lastPoint, [](const StockPoint& a, const StockPoint& b)
                {
                        return a.val < b.val;
                });

            // We're comparing against the previous day's close, so make sure we include that in the range
            float min = std::min(minpoint->val, currentStock.previousClose);
            float max = std::max(maxpoint->val, currentStock.previousClose);
            float range = max - min;

            if (range > 0.0f)
            {
                float scale = range > 0.0f ? h / range : 0.0f;
                float breakeven = currentStock.previousClose;
                float breakevenY = y + h - (breakeven - min) * scale;

                const int graphStartX = MATRIX_WIDTH - static_cast<int>(pointsToDraw);

                for (size_t i = 0; i < pointsToDraw - 1; i++)
                {
                    const StockPoint& p0 = currentStock.points[firstPointIndex + i];
                    const StockPoint& p1 = currentStock.points[firstPointIndex + i + 1];

                    float x0 = graphStartX + static_cast<int>(i);
                    float y0 = y + h - (p0.val - min) * scale;
                    float x1 = x0 + 1;
                    float y1 = y + h - (p1.val - min) * scale;

                    // Now draw from bottom up to breakeven in red, and from breakeven to top in green

                    if (p0.val < breakeven)
                        graphics.drawLine(x0, breakevenY, x1, y1, CRGB::Red);
                    else
                        graphics.drawLine(x0, y0, x1, breakevenY, CRGB::Green);
                }
            }
        }
    }

    // Draw
    //
    // Draws the stock display made up of four animated text flyers (price, symbol, change, volume)
    // and the stock history graph

    void Draw() override
    {
        auto& graphics = g();
        const int screenWidth = static_cast<int>(graphics.GetMatrixWidth());
        const int screenHeight = static_cast<int>(graphics.GetMatrixHeight());

        graphics.fillScreen(BLACK16);
        graphics.setFont(&Apple5x7);
        graphics.setTextWrap(false);

        // Periodically refetch the stock data from the server

        if (nd_network::IsWiFiConnected())
        {
            if (system_clock::now() >= nextFetch)
            {
                nextFetch = system_clock::now() + STOCKS_FETCH_INTERVAL_SECONDS;
                // Trigger the stock data reader.
                g_ptrSystem->GetNetworkReader().FlagReader(readerIndex);
            }
        }

        // Rotate the display through the available stock data

        auto now = system_clock::now();

        // We move on to next stock if the interval has passed, or we have less stock data available than before
        StockData nextStock;
        bool hasNextStock = false;
        {
            std::lock_guard dataGuard(stockDataMutex);
            const size_t stockCount = stockData.size();
            auto showNextStock = now - lastUpdate >= STOCKS_UPDATE_INTERVAL_SECONDS || stockCount < lastCount;

            // Only do something if we should and have stock data to show
            if (showNextStock && stockCount > 0)
            {
                lastUpdate = now;
                lastCount = stockCount;
                iCurrentStock = (iCurrentStock + 1) % stockCount;

                auto it = stockData.begin();
                std::advance(it, iCurrentStock);
                nextStock = it->second;
                hasNextStock = true;
            }
        }

        if (hasNextStock)
            StartQuoteDisplay(nextStock);

        // Paint Frame

        if (screenHeight < 32)
        {
            DrawCompactQuoteDisplay(screenHeight);
            return;
        }

        graphics.fillRect(0, 0, screenWidth, 9, graphics.to16bit(CRGB(0,0,128)));
        UpdateQuoteDisplay();
    }

    // Extension override to serialize our settings on top of those from LEDStripEffect
    bool SerializeSettingsToJSON(JsonObject& jsonObject) override
    {
        auto jsonDoc = CreateJsonDocument();

        JsonObject root = jsonDoc.to<JsonObject>();
        LEDStripEffect::SerializeSettingsToJSON(root);

        jsonDoc[NAME_OF(stockServer)] = stockServer;
        jsonDoc[NAME_OF(tickerSymbols)] = tickerSymbols;

        return SetIfNotOverflowed(jsonDoc, jsonObject, __PRETTY_FUNCTION__);
    }

    // Extension override to accept our settings on top of those known by LEDStripEffect
    bool SetSetting(const String& name, const String& value) override
    {
        if (FieldAccess::AssignIfSelected(name, NAME_OF(stockServer), stockServer, value))
            return true;

        // If we receive a new list of stock ticker symbols then forget what stock data we
        // have and trigger a reload.
        if (FieldAccess::AssignIfSelected(name, NAME_OF(tickerSymbols), tickerSymbols, value))
        {
            {
                std::lock_guard dataGuard(stockDataMutex);
                iCurrentStock = 0;
                stockData.clear();
                lastCount = SIZE_MAX;
            }
            g_ptrSystem->GetNetworkReader().FlagReader(readerIndex);
            return true;
        }

        return LEDStripEffect::SetSetting(name, value);
    }

};

#endif
