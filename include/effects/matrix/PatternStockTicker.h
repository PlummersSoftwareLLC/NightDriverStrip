//+--------------------------------------------------------------------------
//
// File:        PatternStockTicker.h
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
//   Gets the Stock Data for a given set of comman seperated 
//   Stock Ticker Symbols
//
// History: Jun-04-2023     Gatesm      Adapted from Weather code
//          Aug-15-2030     Gatesm      Modified to support more than one stock symbol
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
#include <secrets.h>
#include <RemoteDebug.h>
#include <FastLED.h>
#include <ArduinoJson.h>
#include "globals.h"
#include "deviceconfig.h"
#include "jsonserializer.h"
#include <thread>
#include <map>
#include "effects.h"

// Default stock Ticker symbols for Apple, IBM, and Microsoft 
#define DEFAULT_STOCK_TICKERS       "AAPL,IBM,MSFT"


/**
 * @brief All the data about a specific Stock Ticker
 * Stored as a circular linked list so that the draw code
 * can easily move from one to the next
 * 
 */
class StockTicker
{
public:
    String _strSymbol;
    String _strCompanyName;
    String _strExchangeName;
    String _strCurrency;

    bool _isValid               = false;

    float  _marketCap           = 0.0f;
    float  _sharesOutstanding   = 0.0f;

    float  _currentPrice        = 0.0f;
    float  _change              = 0.0f;
    float  _percentChange       = 0.0f;
    float  _highPrice           = 0.0f;
    float  _lowPrice            = 0.0f;
    float  _openPrice           = 0.0f;
    float  _prevClosePrice      = 0.0f;
    long   _sampleTime          = 0l;
};

/**
 * @brief This class implements the stock ticker effect
 * it will show a repeating list of stock symbols,
 * their high and low values
 * 
 */
class PatternStockTicker : public LEDStripEffect
{

private:
    // Update stocks every 10 minutes, retry after 30 seconds on error, and check other things every 5 seconds
    static constexpr auto STOCK_CHECK_INTERVAL          = (10 * 60000);
    static constexpr auto STOCK_CHECK_ERROR_INTERVAL    = 30000;
    static constexpr auto STOCK_READER_INTERVAL         = 5000;
    static constexpr auto STOCK_DISPLAY_INTERVAL        = 30000;
    static constexpr auto STOCK_TOGGLE_DATA_INTERVAL    = 5000;
    static constexpr auto NO_STOCK_SELECTED             = -1; 

    // Keep the vector of stock symbol data in psram
    std::vector<StockTicker, psram_allocator<StockTicker>> _tickers = {};
    StockTicker *_currentTicker     = NULL;
    size_t _currentIndex            = NO_STOCK_SELECTED;
    bool   _stockChanged            = false;
    String _stockTickerList         = DEFAULT_STOCK_TICKERS;
    bool   _succeededBefore         = false;

    size_t _showDataSide            = 0;
    size_t _readerIndex             = std::numeric_limits<size_t>::max();
    unsigned long _msLastCheck      = 0;
    unsigned long _msLastDrawTime   = 0;
    unsigned long _msLastToggleTime = 0;

    static std::vector<SettingSpec, psram_allocator<SettingSpec>> mySettingSpecs;
    StockTicker _emptyTicker;

    /**
     * @brief Should this effect show its title?
     * The stock ticker is obviously stock data, 
     * and we don't want text overlaid on top of our text.
     * 
     * @return bool - false 
     */
    bool ShouldShowTitle() const
    {
        return false;
    }

    /**
     * @brief How many frames per second does this effect want?
     * 
     * @return size_t - 10 FPS
     */
    size_t DesiredFramesPerSecond() const override
    {
        return 30;
    }

    /**
     * @brief Does this effect requre double buffering support?
     * 
     * @return bool - true 
     */
    bool RequiresDoubleBuffering() const override
    {
        return true;
    }

    /**
     * @brief Retrieve the 'static' information about the supplied
     * stock symbol
     * 
     * @param ticker Reference to a StockTicker object
     * @return bool - true if the symbol is found by the API
     */
    bool updateTickerCode(StockTicker &ticker)
    {
        HTTPClient http;
        String url;
        bool dataFound = false;

        url = "https://finnhub.io/api/v1/stock/profile2"
              "?symbol=" + urlEncode(ticker._strSymbol) + "&token=" + urlEncode(g_ptrSystem->DeviceConfig().GetStockTickerAPIKey());

        http.begin(url);
        int httpResponseCode = http.GET();

        if (httpResponseCode > 0)
        {
            /* Sample returned data
            {   "country":"US",
                "currency":"USD",
                "estimateCurrency":"USD",
                "exchange":"NASDAQ NMS - GLOBAL MARKET",
                "finnhubIndustry":"Retail",
                "ipo":"1997-05-15",
                "logo":"https://static2.finnhub.io/file/publicdatany/finnhubimage/stock_logo/AMZN.svg",
                "marketCapitalization":1221092.7955074026,
                "name":"Amazon.com Inc",
                "phone":"12062661000.0",
                "shareOutstanding":10260.4,
                "ticker":"AMZN",
                "weburl":"https://www.amazon.com/"}
            */

            AllocatedJsonDocument doc(1024);
            String headerData = http.getString();
            debugI("Stock Heder: %s", headerData.c_str());
            if (headerData.equals("{}")) 
            {
                ticker._strCompanyName    = "Bad Symbol";
                ticker._strExchangeName   = "";
                ticker._strCurrency       = "";
                ticker._isValid           = false;
                ticker._marketCap         = 0.0;
                ticker._sharesOutstanding = 0.0;

                debugW("Bad ticker symbol: '%s'", ticker._strSymbol.c_str());
            }
            else
            {
                deserializeJson(doc, headerData);
                JsonObject companyData =  doc.as<JsonObject>();
                dataFound = true;

                ticker._strSymbol         = companyData["ticker"].as<String>();
                ticker._strCompanyName    = companyData["name"].as<String>();
                ticker._strExchangeName   = companyData["exchange"].as<String>();
                ticker._strCurrency       = companyData["currency"].as<String>();
                ticker._marketCap         = companyData["marketCapitalization"];
                ticker._sharesOutstanding = companyData["shareOutstanding"];

                debugI("Got ticker header: sym %s Company %s, Exchange %s", 
                        ticker._strSymbol.c_str(), ticker._strCompanyName.c_str(), 
                        ticker._strExchangeName.c_str());
            }
        }
        else
        {
            debugE("Error (%d) fetching company data for ticker: %s", 
                    httpResponseCode, ticker._strSymbol.c_str());
        }


        http.end();
        return dataFound;
    }

    /**
     * @brief Get the price data for the supplied stock symbol
     * 
     * @param ticker Reference to a StockTicker object
     * @return bool - true if we successfully pulled data for the symbol
     */
    bool getStockData(StockTicker &ticker)
    {
        HTTPClient http;
        bool dataFound = false;

        String url = "https://finnhub.io/api/v1/quote"
            "?symbol=" + urlEncode(ticker._strSymbol)  + "&token=" + urlEncode(g_ptrSystem->DeviceConfig().GetStockTickerAPIKey());

        http.begin(url);
        int httpResponseCode = http.GET();
        
        if (httpResponseCode > 0)
        {
            /* Sample returned data
                {
                    "c":179.58,
                    "d":-1.37,
                    "dp":-0.7571,
                    "h":184.95,
                    "l":178.035,
                    "o":182.63,
                    "pc":180.95,
                    "t":1685995205
                }
            */
            
            AllocatedJsonDocument jsonDoc(256);
            String apiData = http.getString();
            debugI("Stock Data: %s", apiData.c_str());
            if (apiData.equals("{}")) 
            {
                ticker._isValid           = false;
                ticker._currentPrice      = 0.0;
                ticker._change            = 0.0;
                ticker._percentChange     = 0.0;
                ticker._highPrice         = 0.0;
                ticker._lowPrice          = 0.0;
                ticker._openPrice         = 0.0;
                ticker._prevClosePrice    = 0.0;
                ticker._sampleTime        = 0.0;
                ticker._sharesOutstanding = 0.0;

                debugW("Bad ticker symbol: '%s'", ticker._strSymbol.c_str());
            }
            else
            {
                deserializeJson(jsonDoc, apiData);
                JsonObject stockData =  jsonDoc.as<JsonObject>();

                // Once we have a non-zero current price the data is valid
                if (0 < stockData["c"])
                {
                    dataFound = true;

                    ticker._isValid           = true;
                    ticker._currentPrice      = stockData["c"];
                    ticker._change            = stockData["d"];
                    ticker._percentChange     = stockData["dp"];
                    ticker._highPrice         = stockData["h"];
                    ticker._lowPrice          = stockData["l"];
                    ticker._openPrice         = stockData["o"];
                    ticker._prevClosePrice    = stockData["pc"];
                    ticker._sampleTime        = stockData["t"];

                    debugI("Got ticker data: %s Now %f Lo %f, Hi %f, Change %f", 
                            ticker._strSymbol.c_str(), ticker._currentPrice, 
                            ticker._lowPrice, ticker._highPrice, ticker._change);
                }
            }
        }
        else
        {
            debugE("Error (%d) fetching Stock data for Ticker: %s", 
                    httpResponseCode, ticker._strSymbol.c_str());
        }

        http.end();
        return dataFound;
    }

    /**
     * @brief The hook called from the network thread to
     * Update the stock data 
     * 
     */
    void StockReader()
    {
        unsigned long msSinceLastCheck = millis() - _msLastCheck;

        /*
         * if the symbols have changed
         * or last check time is zero (first run)
         * or we have not had a succesfull data pull and last check interval is greater than the error interval
         * or last check interval is greater than the check interval
         */
        if (_stockChanged || !_msLastCheck
            || (!_succeededBefore && msSinceLastCheck > STOCK_CHECK_ERROR_INTERVAL)
            || msSinceLastCheck > STOCK_CHECK_INTERVAL)
        {
            // Track the check time so that we do not flood the net if we do not
            // have stocks to check or an API Key
            _msLastCheck = millis();
            UpdateStock();
        }
    }

    /**
     * @brief Drive the actual checking of Stock Data
     * 
     */
    void UpdateStock()
    {
        if (!WiFi.isConnected())
        {
            debugW("Skipping Stock update, waiting for WiFi...");
            return;
        }

        if (_tickers.empty())
        {
            debugW("No Stock Tickers selected, so skipping check...");
            return;
        }

        if (g_ptrSystem->DeviceConfig().GetStockTickerAPIKey().isEmpty())
        {
            debugW("No Stock API Key, so skipping check...");
            return;
        }
            
        if (_stockChanged)
            _succeededBefore = false;

        for (auto &ticker : _tickers)
        {
            bool doUpdateStock = true;

            if (_stockChanged)
                doUpdateStock = updateTickerCode(ticker);

            if (doUpdateStock)
                if (getStockData(ticker))
                    _succeededBefore = true;
        }
        _stockChanged = false;
    }

protected:
    static constexpr int _jsonSize = LEDStripEffect::_jsonSize + 192;

    /**
     * @brief 
     * 
     * @return bool - true if the settings were saved
     * @return false 
     */
    bool FillSettingSpecs() override
    {
        bool settingsSaved = false;

        // Save the parent class settings
        if (LEDStripEffect::FillSettingSpecs()) 
        {
            // Lazily load this class' SettingSpec instances if they haven't been already
            if (0 == mySettingSpecs.size())
            {
                mySettingSpecs.emplace_back(
                    NAME_OF(_stockTickerList),
                    "Stock symbols to show",
                    "The list of valid stock symbol to show, seperated by commas.  May be from any exchange.",
                    SettingSpec::SettingType::String
                );
            }

            // Add our SettingSpecs reference_wrappers to the base set provided by LEDStripEffect
            _settingSpecs.insert(_settingSpecs.end(), mySettingSpecs.begin(), mySettingSpecs.end());

            settingsSaved = true;
        }
        return settingsSaved;
    }

    /**
     * @brief Process the list of stock symbols 
     * and build the data structures in PSRAM to hold the data
     * 
     * @param newSymbols String, comma seperated
     * @return int - number of symbls found
     */
    int LoadNewTickerSymbols(String newSymbols)
    {
        int n = 0;

        if (!newSymbols.isEmpty())  
        {  
            int commaPosition = newSymbols.indexOf(',');  
            while (-1 != commaPosition)  
            {  
                n++;  
                addStockSymbol(newSymbols.substring(0, commaPosition));  
                newSymbols = newSymbols.substring(commaPosition + 1);  
                commaPosition = newSymbols.indexOf(',');  
            }  
            n++;  
            addStockSymbol(newSymbols);  

            _stockChanged = true;  
            _currentTicker = &_tickers[0];  
            _currentIndex = 0;  
        }  
        return n;  
    }  

    /**
     * @brief Add the supplied stock symbol to the vector of
     * stock ticker objects for processing
     * 
     * @param symbol The exchange ticker symbol for the stock to be added
     */
    void addStockSymbol(const String &symbol)
    {
        debugI("Creating ticker: %s", symbol.c_str());
        _emptyTicker._strSymbol = symbol;
        _tickers.push_back(_emptyTicker);
    }

    /**
     * @brief Get the Next Ticker object
     * 
     * @return StockTicker* - Pointer to the next Stock Ticker object
     */
    StockTicker *getNextTicker()
    {
        StockTicker *nextTicker = NULL;

        if (NO_STOCK_SELECTED != _currentIndex)
        {
            _currentIndex++;
            if (_currentIndex >= _tickers.size())
                _currentIndex = 0;
            nextTicker = &_tickers[_currentIndex];
        }

        return nextTicker;
    }

    /**
     * @brief Free up the Ticket objects in PSRAM
     * 
     */
    void cleanUpTickerData()
    {
        _tickers.clear();
        _currentTicker = NULL;
        _currentIndex = NO_STOCK_SELECTED;
    }

public:
    /**
     * @brief Construct a new Pattern Stock Ticker object
     * 
     */
    PatternStockTicker() : LEDStripEffect(EFFECT_MATRIX_STOCK_TICKER, "Stock")
    {
        LoadNewTickerSymbols(DEFAULT_STOCK_TICKERS);
    }

    /**
     * @brief Construct a new Pattern Stock Ticker object
     * 
     * @param jsonObject 
     */
    PatternStockTicker(const JsonObjectConst&  jsonObject) : LEDStripEffect(jsonObject)
    {
        String storedStockList = DEFAULT_STOCK_TICKERS;

        if (jsonObject.containsKey(PTY_STOCKTICKERS))
        {
            storedStockList = jsonObject[PTY_STOCKTICKERS].as<String>();
        }

        LoadNewTickerSymbols(storedStockList);
    }

    /**
     * @brief Destroy the Pattern Stock Ticker object
     * 
     */
    ~PatternStockTicker()
    {
        g_ptrSystem->NetworkReader().CancelReader(_readerIndex);
        cleanUpTickerData();
    }

    /**
     * @brief populate the jsonObject with our settings
     * 
     * @param jsonObject 
     * @return bool - true if the json object is 
     * @return false 
     */
    bool SerializeToJSON(JsonObject& jsonObject) override
    {
        StaticJsonDocument<_jsonSize> jsonDoc;

        JsonObject root = jsonDoc.to<JsonObject>();
        LEDStripEffect::SerializeToJSON(root);

        jsonDoc[PTY_STOCKTICKERS] = _stockTickerList;

        return jsonObject.set(jsonDoc.as<JsonObjectConst>());
    }

    /**
     * @brief Initialize the LED Strip Effect class and register the Network Reader task
     * 
     * @param gfx Base Graphics FX System
     * @return bool - true if effect is initialized
     */
    bool Init(std::vector<std::shared_ptr<GFXBase>>& gfx) override
    {
        if (!LEDStripEffect::Init(gfx))
            return false;

        _readerIndex = g_ptrSystem->NetworkReader().RegisterReader([this] { StockReader(); }, STOCK_READER_INTERVAL, true);

        return true;
    }

    /**
     * @brief Perform the actual drawing of the current stock ticker data
     * 
     */
    void Draw() override
    {
        unsigned long msSinceLastCheck  = millis() - _msLastDrawTime;
        unsigned long msSinceLastToggle = millis() - _msLastToggleTime;

        if (msSinceLastCheck >= STOCK_DISPLAY_INTERVAL)
        {
            _msLastDrawTime = millis();
            _msLastToggleTime = _msLastDrawTime;
            _currentTicker = getNextTicker();
            _showDataSide = 0;
            debugI("Now Drawing %s", _currentTicker->_strCompanyName.c_str());
        }
        else
        {
            if (msSinceLastToggle >= STOCK_TOGGLE_DATA_INTERVAL)
            {
                _msLastToggleTime = millis();
                _showDataSide = _showDataSide ^ 1;
            }
        }

        DrawTicker(_currentTicker, _showDataSide);
    }

    /**
     * @brief Draw the specified ticker data on the panel
     * 
     * @param ticker Pointer to the StockTicker object to draw
     * @param dataOffset Determines which data set is shown 0 = hi/low, 1 = open/close
     */
    void DrawTicker(StockTicker *ticker, int dataOffset) 
    {
        const int fontHeight = 7;
        const int fontWidth  = 5;
        const int xHalf      = MATRIX_WIDTH / 2 - 1;

        g()->fillScreen(BLACK16);
        g()->fillRect(0, 0, MATRIX_WIDTH, MATRIX_HEIGHT, g()->to16bit(CRGB(0,0,128)));
        g()->setFont(&Apple5x7);
 
        // Print the Company name

        int x = 0;
        int y = fontHeight + 1;
        g()->setCursor(x, y);
        g()->setTextColor(WHITE16);

        if (NULL == ticker) {
            // Tell the user there is no stocks selected and bail
            g()->setTextColor(YELLOW16);
            g()->print("No Stocks");
            return;
        }

        if (g_ptrSystem->DeviceConfig().GetStockTickerAPIKey().isEmpty())
        {
            // Tell the user there is no API Key and bail
            g()->setTextColor(RED16);
            g()->print("No API Key");
            return;
        }

        // Display the stock symbol

        String showCompany = ticker->_strSymbol;
        showCompany.toUpperCase();
        g()->print(showCompany.substring(0, (MATRIX_WIDTH - fontWidth)/fontWidth));

        // Display the Stock Price 
        // set the color based on the direction of the last change

        if (ticker->_isValid)
        {
            String strPrice(ticker->_currentPrice, 3);
            x = 1;
            y += fontHeight + 1;

            g()->setCursor(x, y);
            if (ticker->_change > 0.0) {
                g()->setTextColor(GREEN16);
            } else if (ticker->_change < 0.0) {
                g()->setTextColor(RED16);
            } else {
                g()->setTextColor(WHITE16);
            }
            g()->print("Pr: ");
            g()->print(strPrice);                                                                                                                                                                                                                                                                                                                                   g()->print(strPrice);
        }

        // Draw the separator lines

        y+=1;

        g()->drawLine(0, y, MATRIX_WIDTH-1, y, CRGB(0,0,128));

        // Draw the price data in lighter white

        if (ticker->_isValid)
        {
            g()->setTextColor(g()->to16bit(CRGB(192,192,192)));
            if (0 == dataOffset)
            {
                // Draw current high and low price

                String strHi( ticker->_highPrice, 3);
                String strLo( ticker->_lowPrice, 3);

                x = 2;
                y += fontHeight;

                g()->setCursor(x,y);
                g()->print("Hi: ");
                g()->print(strHi);

                x = 2;
                y+= fontHeight;

                g()->setCursor(x,y);
                g()->print("Lo: ");
                g()->print(strLo);
            }
            else
            {
                // Draw Open and Close price
                
                String strOpen = String(ticker->_openPrice, 3);
                String strClose = String(ticker->_prevClosePrice, 3);

                x = 2;
                y += fontHeight;

                g()->setCursor(x,y);
                g()->print("Op: ");
                g()->print(strOpen);

                x = 2;
                y+= fontHeight;

                g()->setCursor(x,y);
                g()->print("Cl: ");
                g()->print(strClose);
            }
        }
    }

    /**
     * @brief Update the JSON Object with our current setting values
     * 
     * @param jsonObject Reference to the settings JSON object
     * @return bool - true if json object is populated
     */
    bool SerializeSettingsToJSON(JsonObject& jsonObject) override
    {
        StaticJsonDocument<_jsonSize> jsonDoc;
        auto rootObject = jsonDoc.to<JsonObject>();

        LEDStripEffect::SerializeSettingsToJSON(jsonObject);

        jsonDoc[NAME_OF(_stockTickerList)] = _stockTickerList;

        assert(!jsonDoc.overflowed());
        
        return jsonObject.set(jsonDoc.as<JsonObjectConst>());
    }

    /**
     * @brief Set the Setting for this object
     * 
     * @param name Name of setting
     * @param value Value of setting
     * @return true if setting name processed
     * @return false if setting name unrecognized
     */
    bool SetSetting(const String& name, const String& value) override
    {
        if (name == NAME_OF(_stockTickerList) && _stockTickerList != value)
            _stockChanged = true;

        RETURN_IF_SET(name, NAME_OF(_stockTickerList), _stockTickerList, value);

        return LEDStripEffect::SetSetting(name, value);
    }
};

#endif
