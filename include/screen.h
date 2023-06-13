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
//#include <freefonts.h>
#include "gfxbase.h"

class Screen : public GFXBase
{
public:
    static DRAM_ATTR std::mutex _screenMutex;

    Screen(int w, int h) : GFXBase(w, h)
    {
    }

    // Some devices, like the OLE, require that you send the whole buffer at once, but others do not.  The default implenetation is to do nothing.

    virtual void StartFrame()
    {
    }

    virtual void EndFrame()
    {
    }

    // Define the drawable area for the spectrum to render into the status area

    const int TopMargin = 52;
    const int BottomMargin = 12;

    virtual void ScreenStatus(const String &strStatus) 
    {
        fillScreen(BLACK16);
        //setFont();
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

    // M5Screen
    //
    // Display code for the M5 based TFT displays on the M5 Stick, Stick C Plus, and Stack

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
            M5.Lcd.fillScreen(GREEN16);
            M5.Lcd.setRotation(1);
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

#if USE_OLED && !USE_SSD1306

    // OLEDScreen
    //
    // Display code for the blue OLED display on the Heltect Wifi Kit 32 Original

    #include <U8g2lib.h>                // Library for monochrome displays
    #include <gfxfont.h>                // Adafruit GFX font structs
    #include <Adafruit_GFX.h>           // GFX wrapper so we can draw on screen

    class OLEDScreen : public Screen
    {
        U8G2_SSD1306_128X64_NONAME_F_HW_I2C oled;

    public:

        OLEDScreen(int w, int h) : Screen(w, h), oled(U8G2_R2, /*reset*/ 16, /*clk*/ 15, /*data*/ 4)
        {
            oled.begin();
            oled.clear();
        }   

        virtual void StartFrame() override
        {
            oled.clearBuffer();
        }

        virtual void EndFrame() override
        {
            oled.sendBuffer();
        }
        virtual void drawPixel(int16_t x, int16_t y, uint16_t color) override
        {
            oled.setDrawColor(color == BLACK16 ? 0  : 1);
            oled.drawPixel(x, y);
        }

        virtual void fillScreen(uint16_t color) override
        {
            oled.clearDisplay();
        }
    };
#endif

#if USE_SSD1306

    // SSD1306Screen
    //
    // Display code for the SSD1306 display on the Heltect Wifi Kit 32 V3

    #include <U8g2lib.h>                // Library for monochrome displays
    #include <gfxfont.h>                // Adafruit GFX font structs
    #include <Adafruit_GFX.h>           // GFX wrapper so we can draw on screen
    #include <heltec.h>                // Display 

    class SSD1306Screen : public Screen
    {
    public:

        SSD1306Screen(int w, int h) : Screen(w, h)
        {
            Heltec.begin(true /*DisplayEnable Enable*/, true /*LoRa Disable*/, false /*Serial Enable*/);
            Heltec.display->screenRotate(ANGLE_180_DEGREE);
        }   

        virtual void StartFrame() override
        {
        }

        virtual void EndFrame() override
        {
            Heltec.display->display();
        }

        virtual void drawPixel(int16_t x, int16_t y, uint16_t color) override
        {
            if (color == BLACK16)
                Heltec.display->clearPixel(x,y);
            else
                Heltec.display->setPixel(x,y);
        }

        virtual void fillScreen(uint16_t color) override
        {
            Heltec.display->clear();
        }
    };
#endif

#if USE_LCD

    #include <Adafruit_ILI9341.h>

    // LCDScreen
    //
    // Screen class that works with the WROVER devkit board
    
    class LCDScreen : public Screen
    {
        SPIClass hspi;
        std::unique_ptr<Adafruit_ILI9341> pLCD;

    public:

        LCDScreen(int w, int h) : Screen(w, h), hspi(HSPI)
        {
            hspi.begin(TFT_SCK, TFT_MISO, TFT_MOSI, -1);

            #ifndef TFT_BL
            #define TFT_BL 5 // LED back-light
            #endif
            pinMode(TFT_BL, OUTPUT); //initialize BL

            pLCD = std::make_unique<Adafruit_ILI9341>(&hspi, TFT_DC, TFT_CS, TFT_RST);
            pLCD->begin();
            pLCD->setRotation(1);
            pLCD->fillScreen(GREEN16);
        }   

        virtual void drawPixel(int16_t x, int16_t y, uint16_t color) override
        {
            pLCD->writePixel(x, y, color);
        }

        virtual void fillRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color) override
        {
            pLCD->fillRect(x, y, w, h, color);
        }

    };
#endif

#if USE_SCREEN
extern std::unique_ptr<Screen> g_pDisplay;
#endif

