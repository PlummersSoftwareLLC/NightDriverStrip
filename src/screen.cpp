//+--------------------------------------------------------------------------
//
// File:        screen.cpp
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
//    Handles the TFT or OLED display that is optionally connected.  It
//    displays info like the IP address buffer depth and clock info, FPS, etc.
//
// History:     Jul-14-2021         Davepl      Moved out of main.cpp
//---------------------------------------------------------------------------

#include "globals.h"                            // CONFIG and global headers
#include "ledmatrixgfx.h"                       // Req'd for drawing types
#include "ledbuffer.h"                          // For g_apBufferManager type
#include "effects/effectmanager.h"              // So we can display cur effect
#include "Bounce2.h"
#include "freefonts.h"


#if USE_TFT
    #define TFT_ROTATION U8G2_R2
    U8G2_SSD1306_128X64_NONAME_F_HW_I2C g_TFT(TFT_ROTATION, /*reset*/ 16, /*clk*/ 15, /*data*/ 4);
#endif

//
// Externals - Moslty things that the screen will report or display for us
//

extern byte g_Brightness;                           // Global brightness from drawing.cpp
extern double g_BufferAgeOldest;                    // Age of oldest frame in WiFi buffer
extern double g_BufferAgeNewest;                    // Age of newest frame in WiFi buffer
extern DRAM_ATTR bool g_bUpdateStarted;             // Has an OTA update started?
extern byte g_Brightness;                           // Global brightness from drawing.cpp
extern DRAM_ATTR unique_ptr<LEDBufferManager> g_apBufferManager[NUM_CHANNELS];
extern DRAM_ATTR AppTime g_AppTime;                 // For keeping track of frame timings
extern DRAM_ATTR uint32_t g_FPS;                    // Our global framerate
extern volatile float gVU;
extern DRAM_ATTR uint8_t giInfoPage;                // What page of screen we are showing
extern DRAM_ATTR bool gbInfoPageDirty;              // Does display need to be erased?

bool g_ShowFPS = false;                             // Indicates whether little lcd should show FPS
#if ENABLE_AUDIO
extern volatile float DRAM_ATTR gVURatio;           // Current VU as a ratio to its recent min and max
#endif

// TFTStatus
// 
// Display a single string of text on the TFT screen, useful during boot for status, etc.

void IRAM_ATTR TFTStatus(const char *pszStatus)
{
#if USE_TFT 
    g_TFT.clear();
    g_TFT.clearBuffer();                   // clear the internal memory
    g_TFT.setFont(u8g2_font_profont15_tf); // choose a suitable font
    g_TFT.setCursor(0, 10);
    g_TFT.println(pszStatus);
    g_TFT.sendBuffer();
#endif
}

// UpdateTFT
//
// Draws the OLED/TFT screen with the current stats on connection, buffer, drawing, etc.

void IRAM_ATTR UpdateScreen()
{
#if USE_TFT
        char szBuffer[256];
        static const char szStatus[] = "|/-\\";
        static int cStatus = 0;
        int c2 = cStatus % strlen(szStatus);
        char chStatus = szStatus[ c2 ];
        cStatus++;

        g_TFT.clearBuffer(); 
        snprintf(szBuffer, ARRAYSIZE(szBuffer), "%s:%dx%d %c %dK", FLASH_VERSION_NAME, NUM_CHANNELS, STRAND_LEDS, chStatus, ESP.getFreeHeap() / 1024);
        g_TFT.drawStr(0, 13, szBuffer); // write something to the internal memory

        if (WiFi.isConnected() == false)
        {
            snprintf(szBuffer, ARRAYSIZE(szBuffer), "No Wifi Connection");
        }
        else
        {
            const IPAddress address = WiFi.localIP();
            snprintf(szBuffer, ARRAYSIZE(szBuffer), "%ddB:%d.%d.%d.%d", 
                                                    (int)labs(WiFi.RSSI()),                            // skip sign in first character
                                                    address[0], address[1], address[2], address[3]);
        }
        g_TFT.drawStr(0, 25, szBuffer); // write something to the internal memory

        snprintf(szBuffer, ARRAYSIZE(szBuffer), "BUFR:%02d/%02d [%dfps]", g_apBufferManager[0]->Depth(), g_apBufferManager[0]->BufferCount(), g_FPS);
        g_TFT.drawStr(0, 61, szBuffer); // write something to the internal memory

        snprintf(szBuffer, ARRAYSIZE(szBuffer), "DATA:%+04.2lf-%+04.2lf", g_BufferAgeOldest, g_BufferAgeNewest);
        g_TFT.drawStr(0, 37, szBuffer); // write something to the internal memory

        snprintf(szBuffer, ARRAYSIZE(szBuffer), "CLCK:%.2lf", g_AppTime.CurrentTime());
        g_TFT.drawStr(0, 49, szBuffer); // write something to the internal memory

        g_TFT.sendBuffer();

        if (false == heap_caps_check_integrity_all(true))
        {
            debugE("TFT Heap FAILED checks!");
        }

#elif USE_OLED

        if (giInfoPage == 1)
        {
            // We only draw after a page flip or if anything has changed about the information that will be
            // shown in the page. This avoids flicker, but at the cost that we have to remember what we dispalyed
            // last time and check each time to see if its any different before drawing.

            static auto lasteffect = g_pEffectManager->GetCurrentEffectIndex();
            static auto sip        = WiFi.localIP().toString();
            static auto lastFPS    = g_FPS;

            if (gbInfoPageDirty == true                                      || 
                lasteffect      != g_pEffectManager->GetCurrentEffectIndex() || 
                sip             != WiFi.localIP().toString()                 || 
                (g_ShowFPS && (lastFPS != g_FPS))
            )
            {
                gbInfoPageDirty = false;
                lasteffect      = g_pEffectManager->GetCurrentEffectIndex();
                sip             = WiFi.localIP().toString();
                lastFPS         = g_FPS;

                M5.Lcd.fillScreen(BLACK16);

                M5.Lcd.setFreeFont(FF17);
                M5.Lcd.setTextColor(0xFBE0);
                auto xh = M5.Lcd.width() / 2;
                auto yh = M5.Lcd.fontHeight();

                M5.Lcd.setTextDatum(C_BASELINE);
                string sEffect = to_string("Current Effect: ") + 
                                 to_string(g_pEffectManager->GetCurrentEffectIndex() + 1) + 
                                 to_string(" / ") + 
                                 to_string(g_pEffectManager->EffectCount());

                M5.Lcd.drawString(sEffect.c_str(), xh, yh += M5.Lcd.fontHeight());      // -4 is purely aesthetic alignment
                
                M5.Lcd.setFreeFont(FF18);
                M5.Lcd.setTextColor(WHITE16);
                M5.Lcd.drawString(g_pEffectManager->GetCurrentEffectName(), xh, yh += M5.Lcd.fontHeight() - 4);   // -4 is just aesthetic alignment

                if (WiFi.isConnected())
                {
                    String sIP = WiFi.localIP().toString().c_str();
                    M5.Lcd.setFreeFont(FF17);
                    M5.Lcd.setTextColor(YELLOW16);
                    M5.Lcd.drawString(sIP.c_str(), xh, yh += M5.Lcd.fontHeight());
                }
                if (g_ShowFPS)
                {
                    string sFPS = "FPS: " + to_string(g_FPS);
                    M5.Lcd.setFreeFont(FF1);
                    M5.Lcd.drawString(sFPS.c_str(), xh, yh += M5.Lcd.fontHeight());
                }

                M5.Lcd.setFreeFont(FF9);
                M5.Lcd.setTextColor(GREEN16);
                M5.Lcd.drawString("NightDriverLED.com", xh, M5.Lcd.height() - M5.Lcd.fontHeight());

                // Reset text color to white (no way to fetch original and save it that I could see)
                M5.Lcd.setTextColor(WHITE16);
                
            }
        }
        else if (giInfoPage == 0)
        {
            static uint lastFullDraw = 0;
            char szBuffer[256];
            if (millis() - lastFullDraw > 100)
            {
                lastFullDraw = millis();
                // clear the internal memory
                #ifdef POWER_LIMIT_MW
                int brite = calculate_max_brightness_for_power_mW(g_Brightness, POWER_LIMIT_MW);
                #else
                int brite = 255;
                #endif

                snprintf(szBuffer, ARRAYSIZE(szBuffer), "%s:%dx%d %dK  %03dB", FLASH_VERSION_NAME, NUM_CHANNELS, STRAND_LEDS, ESP.getFreeHeap() / 1024, brite);
                M5.Lcd.drawString(szBuffer, 0, 13); // write something to the internal memory

                if (WiFi.isConnected() == false)
                {
                    snprintf(szBuffer, ARRAYSIZE(szBuffer), g_pEffectManager ? g_pEffectManager->GetCurrentEffectName() : "N/C");
                }
                else
                {
                    snprintf(szBuffer, ARRAYSIZE(szBuffer), "%sdB:%s", 
                                                            String(WiFi.RSSI()).substring(1).c_str(), 
                                                            WiFi.localIP().toString().c_str());
                }
                M5.Lcd.drawString(szBuffer, 0, 25); // write something to the internal memory

                #if ENABLE_AUDIO
                snprintf(szBuffer, ARRAYSIZE(szBuffer), "BUFR:%d/%d [%dfps]  %.2lf", g_apBufferManager[0]->Depth(), g_apBufferManager[0]->BufferCount(), g_FPS, gVURatio);
                M5.Lcd.drawString(szBuffer, 0, 61); // write something to the internal memory
                #endif
            }

            snprintf(szBuffer, ARRAYSIZE(szBuffer), "DATA:%+04.2lf-%+04.2lf", g_BufferAgeOldest, g_BufferAgeNewest);
            M5.Lcd.drawString(szBuffer, 0, 37); // write something to the internal memory

            snprintf(szBuffer, ARRAYSIZE(szBuffer), "CLCK:%.2lf", g_AppTime.CurrentTime());
            M5.Lcd.drawString(szBuffer, 0, 49); // write something to the internal memory

            #if ENABLE_AUDIO
                const int barHeight = 10;
                int barPos = M5.Lcd.width() * (gVURatio - 1.0);
                int barY = M5.Lcd.height() - barHeight;

                if (barPos < 6)
                    barPos = 6;

                M5.Lcd.fillRect(barPos, barY, M5.Lcd.width()-barPos, barHeight, BLACK16);
                M5.Lcd.fillRect(0, barY, barPos, barHeight, WHITE16);
            #endif
        }
        else 
        {

        }
#endif
}


// ScreenUpdateLoopEntry
//
// Displays statistics on the Heltec's built in TFT board.  If you are using a different board, you would simply get rid of
// this or modify it to fit a screen you do have.  You could also try serial output, as it's on a low-pri thread it shouldn't
// disturb the primary cores, but I haven't tried it myself.

#if TOGGLE_BUTTON
extern Bounce2::Button sideButton;
#endif


void IRAM_ATTR ScreenUpdateLoopEntry(void *)
{
    debugI(">> ScreenUpdateLoopEntry\n");
    debugI("ScreenUpdateLoop started\n");

    #if USE_TFT
      g_TFT.setDisplayRotation(TFT_ROTATION);
      g_TFT.setFont(u8g2_font_profont15_tf); // choose a suitable font
      g_TFT.clear();
    #endif

    for (;;)
    {
        UpdateScreen();
        delay(g_bUpdateStarted ? 200 : 10);

#ifdef TOGGLE_BUTTON
        sideButton.update();
        if (sideButton.pressed())
        {
            // When the button is pressed advance to the next information page on the little display

            giInfoPage = (giInfoPage + 1) % NUM_INFO_PAGES;
            gbInfoPageDirty = true;
        }        
#endif        
    }
}
