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
//    Generalizes drawing to various different screens
//
// History:     Dec-10-2022         Davepl      Created
//
//---------------------------------------------------------------------------

#pragma once

#include "globals.h"
#include "ledmatrixgfx.h"
#include "freefonts.h"
#include <mutex>

// A project with a screen will define one of these screen types (TFT, OLED, LCD, etc) and one object of the
// correct type will be created and assigned to g_pDisplay, which will have the appropriate type.

#if USE_OLED
    extern U8G2_SSD1306_128X64_NONAME_F_HW_I2C * g_pDisplay;
#endif

#if USE_LCD
    extern Adafruit_ILI9341 * g_pDisplay;
#endif

#if USE_TFT
    extern M5Display * g_pDisplay;
#endif

#if USE_TFTSPI
    #include <TFT_eSPI.h>
    #include <SPI.h>
    extern TFT_eSPI * g_pDisplay;
#endif

class Screen
{
  public:

    static DRAM_ATTR std::mutex _screenMutex;   

    // Define the drawable area for the spectrum to render into the status area

#if M5STICKCPLUS
    static const int TopMargin = 37;  
#else
    static const int TopMargin = 21;  
#endif


    static const int BottomMargin = 22;

    static inline uint16_t to16bit(const CRGB rgb) // Convert CRGB -> 16 bit 5:6:5
    {
      return ((rgb.r / 8) << 11) | ((rgb.g / 4) << 5) | (rgb.b / 8);
    }

    // ScreenStatus
    // 
    // Display a single string of text on the TFT screen, useful during boot for status, etc.

    static void IRAM_ATTR ScreenStatus(const char *pszStatus)
    {
    #if USE_OLED 
        g_pDisplay->clear();
        g_pDisplay->clearBuffer();                   // clear the internal memory
        g_pDisplay->setFont(u8g2_font_profont15_tf); // choose a suitable font
        g_pDisplay->setCursor(0, 10);
        g_pDisplay->println(pszStatus);
        g_pDisplay->sendBuffer();
    #elif USE_TFT
        g_pDisplay->fillScreen(BLACK16);
        g_pDisplay->setFreeFont(FF15);
        g_pDisplay->setTextColor(0xFBE0);
        auto xh = 10;
        auto yh = 0; 
        g_pDisplay->drawString(pszStatus, xh, yh);
    #elif USE_TFTSPI
        g_pDisplay->fillScreen(TFT_BLACK);
        g_pDisplay->setFreeFont(FF15);
        g_pDisplay->setTextColor(0xFBE0);
        auto xh = 10;
        auto yh = 0; 
        g_pDisplay->drawString(pszStatus, xh, yh);
    #elif USE_LCD
        g_pDisplay->fillScreen(TFT_BLACK);
        g_pDisplay->setFreeFont(FF15);
        g_pDisplay->setTextColor(0xFBE0);
        auto xh = 10;
        auto yh = 0; 
        g_pDisplay->drawString(pszStatus, xh, yh);
    #endif
    }

    static uint16_t screenWidth()
    {
        #if USE_OLED
            return g_pDisplay->getDisplayWidth();
        #elif USE_SCREEN
            return g_pDisplay->width();
        #else
            return 1;
        #endif
    }

    static uint16_t fontHeight()
    {
        #if USE_OLED
            return 12;
        #elif USE_SCREEN
            return g_pDisplay->fontHeight();
        #else
            return 1;
        #endif
    }

    static uint16_t textWidth(const char * psz)
    {
        #if USE_OLED
            return g_pDisplay->getStrWidth(psz);
        #elif USE_SCREEN
            return g_pDisplay->textWidth(psz);
        #else 
            return 1;
        #endif

    }

    static uint16_t screenHeight()
    {
        #if USE_OLED
            return g_pDisplay->getDisplayHeight();
        #elif USE_SCREEN
            return g_pDisplay->height();
        #else
            return 1;
        #endif
    }

    static void fillScreen(uint16_t color)
    {
        #if USE_OLED
            g_pDisplay->clear();
        #elif USE_SCREEN
            g_pDisplay->fillScreen(color);
        #endif

    }

    static void setTextColor(uint16_t foreground, uint16_t background)
    {
        #if USE_OLED
            // NOP
        #elif USE_SCREEN
            g_pDisplay->setTextColor(foreground, background);
        #endif

    }

    enum FONTSIZE { TINY, SMALL, MEDIUM, BIG };

    static void setTextSize(FONTSIZE size)
    {
        #if USE_TFT
            switch(size)
            {
                case BIG:
                    g_pDisplay->setTextFont(1);
                    g_pDisplay->setTextSize(3);
                    break;
                case MEDIUM:
                    g_pDisplay->setTextFont(1);
                    g_pDisplay->setTextSize(2);
                    break;
                case TINY:
                    g_pDisplay->setTextFont(1);
                    g_pDisplay->setTextSize(1);
                    break;                
                default:
                    g_pDisplay->setTextFont(screenWidth() > 160 ? 2 : 1);
                    g_pDisplay->setTextSize(1);
                    break;
            }
        #endif
        
        #if USE_TFTSPI
            switch(size)
            {
                case BIG:
                    g_pDisplay->setTextFont(0);
                    g_pDisplay->setTextSize(4);
                    break;
                case MEDIUM:
                    g_pDisplay->setTextFont(0);
                    g_pDisplay->setTextSize(3);
                    break;
                case TINY:
                    g_pDisplay->setTextFont(0);
                    g_pDisplay->setTextSize(1);
                    break;                
                default:
                    g_pDisplay->setTextFont(0);
                    g_pDisplay->setTextSize(2);
                    break;
            }
        #endif

        #if USE_LCD
            switch(size)
            {
                case BIG:
                    g_pDisplay->setFreeFont(FF17);
                    break;
                case MEDIUM:
                    g_pDisplay->setFreeFont(FF16);
                    break;
                case TINY:
                    g_pLCCD->setFreeFont(FF1);
                    break;                
                default:
                    g_pDisplay->setFreeFont(FF15);
                    break;
            }
        #endif

        #if USE_OLED
               g_pDisplay->setFont(u8g2_font_profont15_tf);  // OLED uses the same little font for everything
        #endif
    }

    static void setCursor(uint16_t x, uint16_t y)
    {
        #if USE_OLED
            g_pDisplay->setCursor(x, y + fontHeight() - 1);
        #elif USE_SCREEN
            g_pDisplay->setCursor(x, y);       // M5 baselines its text at the top
        #endif
    }

    static void println(const char * pszText)
    {
        #if USE_SCREEN
            g_pDisplay->println(pszText);
        #endif
    }

    static void drawString(const char * pszText, uint16_t x, uint16_t y)
    {
        #if USE_SCREEN
        setCursor(x, y);
        println(pszText);
        #endif
    }

    // drawString with no x component assumed you want it centered on the display

    static void drawString(const char * pszText, uint16_t y)
    {
        #if USE_SCREEN
        setCursor(screenWidth() / 2 - textWidth(pszText) / 2, y);
        println(pszText);
        #endif
    }

    static void fillRect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color)
    {
        #if USE_OLED
            if (color != BLACK16)
                g_pDisplay->setDrawColor(color);
                g_pDisplay->drawBox(x, y, w, h);
        #elif USE_SCREEN
            g_pDisplay->fillRect(x, y, w, h, color);
        #endif
    }
};
