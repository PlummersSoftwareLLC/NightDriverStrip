//+--------------------------------------------------------------------------
//
// File:        screen.h
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
//    Generalizes drawing to various different screens.  For example,
//    drawing a line accepts a color in some cases, but not others, and
//    it depends on what display you are compiling for.  This is a bit
//    of an abstraction layer on those various devices.
//
// History:     Dec-10-2022         Davepl      Created
//
//---------------------------------------------------------------------------

#pragma once

#include <mutex>
#include <freefonts.h>
#include "gfxbase.h"

// A project with a screen will define one of these screen types (TFT, OLED, LCD, etc) and one object of the
// correct type will be created and assigned to g_pDisplay, which will have the appropriate type.

#if USE_OLED
extern U8G2_SSD1306_128X64_NONAME_F_HW_I2C *g_pDisplay;
#endif

#if USE_LCD
extern Adafruit_ILI9341 *g_pDisplay;
#endif

/*
#if USE_OLED
        g_pDisplay->clear();
        g_pDisplay->clearBuffer();                   // clear the internal memory
        g_pDisplay->setFont(u8g2_font_profont15_tf); // choose a suitable font
        g_pDisplay->setCursor(0, 10);
        g_pDisplay->println(strStatus);
        g_pDisplay->sendBuffer();
#elif USE_TFTSPI || USE_M5DISPLAY
#elif USE_LCD
        g_pDisplay->fillScreen(BLUE16);
        g_pDisplay->setFont(FM9);
        g_pDisplay->setTextColor(WHITE16);
        auto xh = 10;
        auto yh = 0;
        g_pDisplay->setCursor(xh, yh);
        g_pDisplay->print(strStatus);
#endif
*/
class Screen : public GFXBase
{
public:
    static DRAM_ATTR std::mutex _screenMutex;

    Screen(int w, int h) : GFXBase(w, h)
    {
    }

    // Define the drawable area for the spectrum to render into the status area

    const int TopMargin = 52;
    const int BottomMargin = 12;

    virtual void ScreenStatus(const String &strStatus) 
    {
        fillScreen(BLACK16);
        setFont();
        setTextSize(1);
        setTextColor(0xFBE0, BLACK16);
        auto xh = 10;
        auto yh = 0;
        setCursor(xh, yh);
        print(strStatus);
    }

    // fontHeight
    //
    // Returns the height of the current font

    virtual int fontHeight() 
    {
        int16_t x1, y1;
        uint16_t w, h;
        getTextBounds(String("W"), 0, 0, &x1, &y1, &w, &h);
        return h;
    }

    // textHeight
    // 
    // Returns the height of a string in screen pixels

    virtual int textHeight(const String & str)
    {
        int16_t x1, y1;
        uint16_t w, h;
        getTextBounds(str, 0, 0, &x1, &y1, &w, &h);
        return h;
    }

    // textWidth
    //
    // Returns the width of a string in screen pixels

    virtual int textWidth(const String & str)
    {
        int16_t x1, y1;
        uint16_t w, h;
        getTextBounds(str, 0, 0, &x1, &y1, &w, &h);
        return w;
    }
};

// Class specializations of the Screen class for various display types can implement hardware-specific versions of functions
// like fillRect.  They also do any required initial setup of the display.

#if USE_M5DISPLAY
    #include <M5Display.h>

    // M5Screen
    //
    // Screen class that supports the M5 devices

    class M5Screen : public Screen
    {
      private:

        M5Display m5display;

      public:

        M5Screen(int w, int h) : Screen(w, h)
        {
            M5.Lcd.fillScreen(BLUE16);
            M5.Lcd.setRotation(1);
            M5.Lcd.setTextDatum(L_BASELINE);
        }

        virtual void drawPixel(int16_t x, int16_t y, uint16_t color) override
        {
            M5.Lcd.drawPixel(x, y, color);
        }

        virtual void fillRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color) override
        {
            M5.Lcd.fillRect(x, y, w, h, color);
        }

        virtual void fillScreen(uint16_t color) override
        {
            M5.Lcd.fillScreen(color);
        }
    };
#endif

#if USE_TFTSPI
    #include <TFT_eSPI.h>
    #include <SPI.h>

    // TFTScreen
    //
    // Screen class that works with the TFT_eSPI library for devices such as the S3-TFT-Feather
    class TFTScreen : public Screen
    {
        TFT_eSPI tft;

    public:

        TFTScreen(int w, int h) : Screen(w, h)
        {
            tft.begin();
            
            pinMode(TFT_BL, OUTPUT);                // REVIEW begin() might do this for us
            digitalWrite(TFT_BL, 128);

            tft.setRotation(3);
            tft.fillScreen(TFT_GREEN);    
            tft.setTextDatum(L_BASELINE);   
        }   

        virtual void drawPixel(int16_t x, int16_t y, uint16_t color) override
        {
            tft.drawPixel(x, y, color);
        }

        virtual void fillRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color) override
        {
            tft.fillRect(x, y, w, h, color);
        }

        virtual void fillScreen(uint16_t color) override
        {
            tft.fillScreen(color);
        }
    };
#endif

extern std::unique_ptr<Screen> g_pDisplay;

