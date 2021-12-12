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

class Screen
{
  public:

      static DRAM_ATTR std::mutex _screenMutex;   

      // Define the drawable area for the spectrum to render into the status area

      static const int TopMargin = 35;  
      static const int BottomMargin = 25;

    // ScreenStatus
    // 
    // Display a single string of text on the TFT screen, useful during boot for status, etc.

    static void IRAM_ATTR ScreenStatus(const char *pszStatus)
    {
    #if USE_OLED 
        g_u8g2.clear();
        g_u8g2.clearBuffer();                   // clear the internal memory
        g_u8g2.setFont(u8g2_font_profont15_tf); // choose a suitable font
        g_u8g2.setCursor(0, 10);
        g_u8g2.println(pszStatus);
        g_u8g2.sendBuffer();
    #elif USE_TFT
        M5.Lcd.fillScreen(BLACK16);
        M5.Lcd.setFreeFont(FF15);
        M5.Lcd.setTextColor(0xFBE0);
        auto xh = 10;
        auto yh = 0; 
        M5.Lcd.drawString(pszStatus, xh, yh);
    #elif USE_TFTSPI
        g_TFTSPI.fillScreen(TFT_BLACK);
        g_TFTSPI.setFreeFont(FF15);
        g_TFTSPI.setTextColor(0xFBE0);
        auto xh = 10;
        auto yh = 0; 
        g_TFTSPI.drawString(pszStatus, xh, yh);
    #elif USE_LCD
        g_pLCD->fillScreen(TFT_BLACK);
        g_pLCD->setFreeFont(FF15);
        g_pLCD->setTextColor(0xFBE0);
        auto xh = 10;
        auto yh = 0; 
        g_pLCD->drawString(pszStatus, xh, yh);
    #endif
    }

    static uint16_t screenWidth()
    {
        #if USE_TFT
            return M5.Lcd.width();
        #endif

        #if USE_TFTSPI
            return g_TFTSPI.width();
        #endif

        #if USE_LCD
            return g_pLCD->width();
        #endif

        #if USE_OLED
            return u8g2.width();
        #endif
    }

    static uint16_t fontHeight()
    {
        #if USE_TFT
            return M5.Lcd.fontHeight();
        #endif

        #if USE_TFTSPI
            return g_TFTSPI.fontHeight();
        #endif

        #if USE_LCD
            return g_pLCD->fontHeight();
        #endif

        #if USE_OLED
            return 12;
        #endif        
    }

    static uint16_t textWidth(const char * psz)
    {
        #if USE_TFT
            return M5.Lcd.textWidth(psz);
        #endif

        #if USE_TFTSPI
            return g_TFTSPI.textWidth(psz);
        #endif

        #if USE_LCD
            return g_pLCD.textWidth(psz;
        #endif

        #if USE_OLED
            return 9 * strlen(psz);
        #endif        
    }

    static uint16_t screenHeight()
    {
        #if USE_TFT
            return M5.Lcd.height();
        #endif

        #if USE_TFTSPI
            return g_TFTSPI.height();
        #endif

        #if USE_LCD
            return g_pLCD->height();
        #endif

        #if USE_OLED
            return u8g2.height();
        #endif
    }

    static void fillScreen(uint16_t color)
    {
        #if USE_TFT
            M5.Lcd.fillScreen(color);
        #endif

        #if USE_TFTSPI
            g_TFTSPI.fillScreen(color);
        #endif

        #if USE_LCD
            g_pLCD->fillScreen(color);
        #endif

        #if USE_OLED
            u8g2.clear();
        #endif
    }

    static void setTextColor(uint16_t foreground, uint16_t background)
    {
        #if USE_TFT
            M5.Lcd.setTextColor(foreground, background);
        #endif

        #if USE_TFTSPI
            g_TFTSPI.setTextColor(foreground, background);
        #endif

        #if USE_LCD
            M5.Kcd.setTextColor(foreground, background);
        #endif

        #if USE_OLED
            // NOP
        #endif
    }

    enum FONTSIZE { TINY, SMALL, MEDIUM, BIG };

    static void setTextSize(FONTSIZE size)
    {
        #if USE_TFT
            switch(size)
            {
                case BIG:
                    M5.Lcd.setTextFont(1);
                    M5.Lcd.setTextSize(3);
                    break;
                case MEDIUM:
                    M5.Lcd.setTextFont(1);
                    M5.Lcd.setTextSize(2);
                    break;
                case TINY:
                    M5.Lcd.setTextFont(1);
                    M5.Lcd.setTextSize(1);
                    break;                
                default:
                    M5.Lcd.setTextFont(screenWidth() > 160 ? 2 : 1);
                    M5.Lcd.setTextSize(1);
                    break;
            }
        #endif
        
        #if USE_TFTSPI
            switch(size)
            {
                case BIG:
                    g_TFTSPI.setTextFont(0);
                    g_TFTSPI.setTextSize(4);
                    break;
                case MEDIUM:
                    g_TFTSPI.setTextFont(0);
                    g_TFTSPI.setTextSize(3);
                    break;
                case TINY:
                    g_TFTSPI.setTextFont(0);
                    g_TFTSPI.setTextSize(1);
                    break;                
                default:
                    g_TFTSPI.setTextFont(0);
                    g_TFTSPI.setTextSize(2);
                    break;
            }
        #endif

        #if USE_LCD
            switch(size)
            {
                case BIG:
                    g_pLCD->setFreeFont(FF17);
                    break;
                case MEDIUM:
                    g_pLCD->setFreeFont(FF16);
                    break;
                case TINY:
                    g_pLCCD->setFreeFont(FF1);
                    break;                
                default:
                    g_pLCD->setFreeFont(FF15);
                    break;
            }
        #endif

        #if USE_OLED
               g_u8g2.setFont(u8g2_font_profont15_tf);  // OLED uses the same little font for everything
        #endif
    }

    static void setCursor(uint16_t x, uint16_t y)
    {
        #if USE_TFT
            M5.Lcd.setCursor(x, y);       // M5 baselines its text at the top
        #endif

        #if USE_TFTSPI
            g_TFTSPI.setCursor(x, y);
        #endif

        #if USE_LCD
            g_pLCD->setCursor(x, y);
        #endif

        #if USE_OLED
            u8g2.setCursor(x, y);
        #endif
    }

    static void println(const char * pszText)
    {
        #if USE_TFT
            M5.Lcd.println(pszText);
        #endif

        #if USE_TFTSPI
            g_TFTSPI.println(pszText);
        #endif

        #if USE_LCD
            g_pLCD.println(pszText);
        #endif

        #if USE_OLED
            u8g2.println(pszText);
        #endif
    }

    static void drawString(const char * pszText, uint16_t x, uint16_t y)
    {
        setCursor(x, y);
        println(pszText);
    }

    // drawString with no x component assumed you want it centered on the display

    static void drawString(const char * pszText, uint16_t y)
    {
        setCursor(screenWidth() / 2 - textWidth(pszText) / 2, y);
        println(pszText);
    }

    static void fillRect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color)
    {
        #if USE_TFT
            M5.Lcd.fillRect(x, y, w, h, color);
        #endif

        #if USE_TFTSPI
            g_TFTSPI.fillRect(x, y, w, h, color);
        #endif

        #if USE_LCD
            g_pLCD.fillRect(x, y, w, h, color);
        #endif

        #if USE_OLED
            throw runtime_error("OLED Cannot draw a rectangle yet!");
        #endif        
    }
};
