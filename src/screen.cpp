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
#include "colordata.h"
#if ENABLE_AUDIO
#include "soundanalyzer.h"
#endif
#include <mutex>

double g_Brite;
uint32_t g_Watts;  

#if USE_OLED
    #define SCREEN_ROTATION U8G2_R2
    U8G2_SSD1306_128X64_NONAME_F_HW_I2C * g_pDisplay = new U8G2_SSD1306_128X64_NONAME_F_HW_I2C(SCREEN_ROTATION, /*reset*/ 16, /*clk*/ 15, /*data*/ 4);
#endif

extern DRAM_ATTR shared_ptr<LEDMatrixGFX> g_pStrands[NUM_CHANNELS];

#if USE_LCD
    Adafruit_ILI9341 * g_pDisplay;
#endif

#if USE_TFT
    M5Display * g_pDisplay;
#endif

#if USE_TFTSPI
    #include <TFT_eSPI.h>
    #include <SPI.h>
    TFT_eSPI * g_pDisplay = new TFT_eSPI();
#endif

//
// Externals - Mostly things that the screen will report or display for us
//

extern DRAM_ATTR unique_ptr<LEDBufferManager> g_apBufferManager[NUM_CHANNELS];

extern byte g_Brightness;                           // Global brightness from drawing.cpp
extern double g_BufferAgeOldest;                    // Age of oldest frame in WiFi buffer
extern double g_BufferAgeNewest;                    // Age of newest frame in WiFi buffer
extern DRAM_ATTR bool g_bUpdateStarted;             // Has an OTA update started?
extern byte g_Brightness;                           // Global brightness from drawing.cpp
extern DRAM_ATTR AppTime g_AppTime;                 // For keeping track of frame timings
extern DRAM_ATTR uint32_t g_FPS;                    // Our global framerate
extern volatile float gVU;
extern DRAM_ATTR uint8_t giInfoPage;                // What page of screen we are showing
extern DRAM_ATTR bool gbInfoPageDirty;              // Does display need to be erased?

DRAM_ATTR std::mutex Screen::_screenMutex;          // The storage for the mutex of the screen class

bool g_ShowFPS = true;                              // Indicates whether little lcd should show FPS
#if ENABLE_AUDIO
extern volatile float DRAM_ATTR gVURatio;           // Current VU as a ratio to its recent min and max
#endif

// UpdateScreen
//
// Draws the OLED/LCD screen with the current stats on connection, buffer, drawing, etc.

void IRAM_ATTR UpdateScreen()
{
    // If the display needs a refresh, we clear it here but we don't reset it yet so as to preserve
    // that info for drawing; it is set to false at the end of the function.

    // We don't want to be in the middle of drawing and have someone one another thread set the dirty
    // flag on us, so access to the flag is guarded by a mutex

	std::lock_guard<std::mutex> guard(Screen::_screenMutex);

    #if USE_OLED
        g_pDisplay->clearBuffer(); 
    #endif

    if (giInfoPage == 0)
    {
        if (gbInfoPageDirty)
            Screen::fillScreen(BLUE16);

        char szBuffer[256];
        static const char szStatus[] = "|/-\\";
        static int cStatus = 0;
        int c2 = cStatus % strlen(szStatus);
        char chStatus = szStatus[ c2 ];
        cStatus++;

        Screen::setTextColor(WHITE16, BLUE16);    // Second color is background color, giving us text overwrite
        Screen::setTextSize(Screen::SMALL);

        // snprintf(szBuffer, ARRAYSIZE(szBuffer), "%s:%dx%d %c %dK ", FLASH_VERSION_NAME, NUM_CHANNELS, STRAND_LEDS, chStatus, ESP.getFreeHeap() / 1024);

        auto w = calculate_unscaled_power_mW( g_pStrands[0]->GetLEDBuffer(), g_pStrands[0]->GetLEDCount() )/ 1000;

        snprintf(szBuffer, ARRAYSIZE(szBuffer), "%s:%dx%d %c %dW ", FLASH_VERSION_NAME, NUM_CHANNELS, STRAND_LEDS, chStatus, w);
        Screen::setCursor(0, 0);
        Screen::println(szBuffer);

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
        auto lineHeight = Screen::fontHeight();
        Screen::setCursor(0, lineHeight);
        Screen::println(szBuffer);

        snprintf(szBuffer, ARRAYSIZE(szBuffer), "BUFR:%02d/%02d [%dfps]", g_apBufferManager[0]->Depth(), g_apBufferManager[0]->BufferCount(), g_FPS);
        Screen::setCursor(0, lineHeight * 4);
        Screen::println(szBuffer);


        snprintf(szBuffer, ARRAYSIZE(szBuffer), "DATA:%+04.2lf-%+04.2lf", g_BufferAgeOldest, g_BufferAgeNewest);
        Screen::setCursor(0, lineHeight * 2);
        Screen::println(szBuffer);

        snprintf(szBuffer, ARRAYSIZE(szBuffer), "CLCK:%.2lf", g_AppTime.CurrentTime());
        Screen::setCursor(0, lineHeight * 3);
        Screen::println(szBuffer);

        if (Screen::screenHeight() >= lineHeight * 5 + Screen::fontHeight())
        {
            snprintf(szBuffer, ARRAYSIZE(szBuffer), "PRAM:%d Brite:%0.2f%% ", 
                ESP.getFreePsram(),
                255.0f * 100 / calculate_max_brightness_for_power_mW(g_Brightness, POWER_LIMIT_MW));

            Screen::setCursor(0, lineHeight * 5);
            Screen::println(szBuffer);
        }

        // We clear the screen but we don't reset it - we reset it after actually
        // having replaced all the info on the screen (ie: drawing)

        gbInfoPageDirty = false;
    }
    else if (giInfoPage == 1)
    {
        if (gbInfoPageDirty)
            Screen::fillScreen(BLACK16);

        uint16_t backColor = Screen::to16bit(CRGB(0, 0, 64));

        // We only draw after a page flip or if anything has changed about the information that will be
        // shown in the page. This avoids flicker, but at the cost that we have to remember what we displayed
        // last time and check each time to see if its any different before drawing.

        static auto lasteffect = g_pEffectManager->GetCurrentEffectIndex();
        static auto sip        = WiFi.localIP().toString();
        static auto lastFPS    = g_FPS;

        if (gbInfoPageDirty != false                                     ||
            lasteffect      != g_pEffectManager->GetCurrentEffectIndex() || 
            sip             != WiFi.localIP().toString()                 || 
            (g_ShowFPS && (lastFPS != g_FPS))
        )
        {
            Screen::fillRect(0, 0, Screen::screenWidth(), Screen::TopMargin, backColor);
            Screen::fillRect(0, Screen::screenHeight() - Screen::BottomMargin, Screen::screenWidth(), Screen::BottomMargin, backColor);
            Screen::fillRect(0, Screen::TopMargin-1, Screen::screenWidth(), 1, BLUE16);
            Screen::fillRect(0, Screen::screenHeight() - Screen::BottomMargin, Screen::screenWidth(), 1, BLUE16);


            lasteffect      = g_pEffectManager->GetCurrentEffectIndex();
            sip             = WiFi.localIP().toString();
            lastFPS         = g_FPS;

            Screen::setTextSize(Screen::SMALL);
            Screen::setTextColor(YELLOW16, backColor);
            auto yh = 1;                        // Start at top of screen

            string sEffect = to_string("Current Effect: ") + 
                             to_string(g_pEffectManager->GetCurrentEffectIndex() + 1) + 
                             to_string("/") + 
                             to_string(g_pEffectManager->EffectCount());
            Screen::drawString(sEffect.c_str(),yh);     
            
            yh += Screen::fontHeight();
			// get effect name length and switch text size accordingly
            int effectnamelen = strlen(g_pEffectManager->GetCurrentEffectName());
            Screen::setTextSize((Screen::screenWidth() > 160) && (effectnamelen <= 18) ? Screen::MEDIUM : Screen::SMALL);
            Screen::setTextColor(WHITE16, backColor);
            Screen::drawString(g_pEffectManager->GetCurrentEffectName(), yh);  

            Screen::setTextSize(Screen::TINY);
            yh = Screen::screenHeight() - Screen::fontHeight() * 3 + 4; 
                
            String sIP = WiFi.isConnected() ? WiFi.localIP().toString().c_str() : "No Wifi";
            sIP += " - NightDriverLED.com";
            Screen::setTextColor(YELLOW16, backColor);
            Screen::drawString(sIP.c_str(), yh);
            yh += Screen::fontHeight();

            #if ENABLE_AUDIO
            if (g_ShowFPS)
            {
                char szBuffer[64];
                snprintf(szBuffer, sizeof(szBuffer), "LED FPS: %2d  Audio FPS: %2d ", g_FPS, g_AudioFPS);
                Screen::drawString(szBuffer, yh);
                yh += Screen::fontHeight();
            }
            #endif

            string s = "NightDriverLED.com";
            Screen::setTextColor(GREEN16, backColor);
            Screen::drawString(s.c_str(), Screen::screenHeight());

            // Reset text color to white (no way to fetch original and save it that I could see)
            Screen::setTextColor(WHITE16, backColor);
        }
        gbInfoPageDirty = false;

        #if ENABLE_AUDIO
            
            // Draw the VU Meter and Spectrum every time.  yScale is the number of vertical pixels that would represent
            // a single LED on the LED matrix.  

            float yScale = (Screen::screenHeight() - Screen::TopMargin - Screen::BottomMargin) / (float) MATRIX_HEIGHT;

            int xHalf   = MATRIX_WIDTH/2-1;
            int cPixels = gVURatio / 2.0 * xHalf; 
            cPixels = min(cPixels, xHalf);

            for (int iPixel = 0; iPixel < xHalf; iPixel++)
            {
                for (int yVU = 0; yVU < 2; yVU++)
                {
                    int xHalfM5 = Screen::screenWidth() / 2;
                    float xScale = Screen::screenWidth()/ (float) MATRIX_WIDTH;
                    uint16_t color16 = iPixel > cPixels ? BLACK16 : Screen::to16bit(ColorFromPalette(vuPaletteGreen, iPixel * (256/(xHalf))));
                    Screen::fillRect(xHalfM5-(iPixel-1)*xScale, Screen::TopMargin + yVU*yScale, xScale-1, yScale, color16);
                    Screen::fillRect(xHalfM5+iPixel*xScale,     Screen::TopMargin + yVU*yScale, xScale-1, yScale, color16);
                }
            }    

            // Draw the spectrum

            int spectrumTop = Screen::TopMargin + yScale;                // Start at the bottom of the VU meter
            for (int iBand = 0; iBand < NUM_BANDS; iBand++)
            {
                CRGB bandColor = ColorFromPalette(RainbowColors_p, (::map(iBand, 0, NUM_BANDS, 0, 255) + 0) % 256);
                auto dy = Screen::screenHeight() - spectrumTop - Screen::BottomMargin;

                int bandWidth = Screen::screenWidth() / NUM_BANDS;
                int bandHeight = max(3.0f, dy - yScale - 2);
                auto color16 = Screen::to16bit(bandColor);
                auto topSection = bandHeight - bandHeight * g_peak2Decay[iBand] - yScale;
                if (topSection > 0)
                    Screen::fillRect(iBand * bandWidth, spectrumTop + yScale, bandWidth - 1, topSection, BLACK16);
                Screen::fillRect(iBand * bandWidth, spectrumTop + dy - bandHeight * g_peak2Decay[iBand], bandWidth - 1, bandHeight * g_peak2Decay[iBand], color16);
            }
        #endif
    }
    else if (giInfoPage == 3)
    {   
        if (gbInfoPageDirty)
            Screen::fillScreen(BLACK16);

        // It always gets cleared, and that's all we need, so we just set the flag to clean again
        Screen::setTextSize(Screen::SMALL);
        Screen::setTextColor(WHITE16, BLACK16);

        static uint lastFullDraw = 0;
        char szBuffer[256];
        const int lineHeight = Screen::fontHeight() + 2;

        if (millis() - lastFullDraw > 100)
        {
            lastFullDraw = millis();

            #ifdef POWER_LIMIT_MW
            double brite = 255.0 * 100.0 / calculate_max_brightness_for_power_mW(g_Brightness, POWER_LIMIT_MW);
            #else
            int brite = 100;
            #endif

            snprintf(szBuffer, ARRAYSIZE(szBuffer), "%s:%dx%d %dK %03dB", FLASH_VERSION_NAME, NUM_CHANNELS, STRAND_LEDS, ESP.getFreeHeap() / 1024, (int)brite);
            Screen::drawString(szBuffer, 0, 0); // write something to the internal memory

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
            Screen::drawString(szBuffer, 0, lineHeight * 1); // write something to the internal memory

            #if ENABLE_AUDIO
            snprintf(szBuffer, ARRAYSIZE(szBuffer), "BUFR:%d/%d[%dfps]%.2lf", g_apBufferManager[0]->Depth(), g_apBufferManager[0]->BufferCount(), g_FPS, gVURatio);
            Screen::drawString(szBuffer, 0, lineHeight * 4); // write something to the internal memory
            #endif
        }

        snprintf(szBuffer, ARRAYSIZE(szBuffer), "DATA:%+04.2lf-%+04.2lf", g_BufferAgeOldest, g_BufferAgeNewest);
        Screen::drawString(szBuffer, 0, lineHeight * 2); // write something to the internal memory

        snprintf(szBuffer, ARRAYSIZE(szBuffer), "CLCK:%.2lf", g_AppTime.CurrentTime());
        Screen::drawString(szBuffer, 0, lineHeight * 3); // write something to the internal memory

        #if ENABLE_AUDIO
            const int barHeight = 10;
            int barPos = Screen::screenWidth() * (gVURatio - 1.0);
            int barY = Screen::screenHeight() - barHeight;

            if (barPos < 5)
                barPos = 5;

            Screen::fillRect(barPos, barY, Screen::screenWidth()-barPos, barHeight, BLACK16);
            Screen::fillRect(0, barY, barPos, barHeight, RED16);
        #endif

        gbInfoPageDirty = false;    
    }
    else if (giInfoPage == 2)
    {
        if (gbInfoPageDirty)
        {
            Screen::fillScreen(BLACK16);

            // This page is largely rendered by the Spectrum effect which draws to us during its render
            Screen::setTextSize(Screen::TINY);
            Screen::setTextColor(GREEN16, BLACK16);

            string s = "NightDriverLED.com";
            Screen::setTextColor(GREEN16, BLACK16);
            auto xh = Screen::screenWidth() / 2 - (s.length() * Screen::fontHeight() * 3 / 4) / 2;
            Screen::drawString(s.c_str(), xh, Screen::screenHeight() - Screen::fontHeight() * 2 - 2);

            s = "Visit Dave's Garage on YouTube!";
            Screen::setTextColor(YELLOW16, BLACK16);
            xh = Screen::screenWidth() / 2 - (s.length() * Screen::fontHeight() * 3 / 4) / 2;
            Screen::drawString(s.c_str(), xh, Screen::screenHeight() - Screen::fontHeight() * 1);
            gbInfoPageDirty = false;
        }
    }
    #if USE_OLED
      g_pDisplay->sendBuffer();
    #endif
}


// ScreenUpdateLoopEntry
//
// Displays statistics on the Heltec's built in OLED board.  If you are using a different board, you would simply get rid of
// this or modify it to fit a screen you do have.  You could also try serial output, as it's on a low-pri thread it shouldn't
// disturb the primary cores, but I haven't tried it myself.

#ifdef TOGGLE_BUTTON_1
extern Bounce2::Button Button1;
#endif

#ifdef TOGGLE_BUTTON_2
extern Bounce2::Button Button2;
#endif

void IRAM_ATTR ScreenUpdateLoopEntry(void *)
{
    debugI(">> ScreenUpdateLoopEntry\n");
    debugI("ScreenUpdateLoop started\n");

    #if USE_OLED
      g_pDisplay->setDisplayRotation(SCREEN_ROTATION);
      g_pDisplay->setFont(u8g2_font_profont15_tf); // choose a suitable font
      g_pDisplay->clear();
    #endif

    for (;;)
    {
#ifdef TOGGLE_BUTTON_1
        Button1.update();
        if (Button1.pressed())
        {
    		std::lock_guard<std::mutex> guard(Screen::_screenMutex);

            // When the button is pressed advance to the next information page on the little display

            giInfoPage = (giInfoPage + 1) % NUM_INFO_PAGES;
            gbInfoPageDirty = true;
        }     
#endif        

#ifdef TOGGLE_BUTTON_2
        Button2.update();
        if (Button2.pressed())
        {
            g_pEffectManager->NextEffect();
        }     
#endif        

        UpdateScreen();
        delay(g_bUpdateStarted ? 200 : 10);

    }
}
