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

#include "globals.h"

extern DRAM_ATTR std::unique_ptr<EffectManager<GFXBase>> g_ptrEffectManager;

float g_Brite;
uint32_t g_Watts;

#if USE_SCREEN

#if USE_TFTSPI
#include <TFT_eSPI.h>
#include <SPI.h>
#endif

#if USE_SCREEN
std::unique_ptr<Screen> g_pDisplay;
#endif

//
// Externals - Mostly things that the screen will report or display for us
//

extern DRAM_ATTR std::unique_ptr<LEDBufferManager> g_aptrBufferManager[NUM_CHANNELS];

extern uint8_t g_Brightness;            // Global brightness from drawing.cpp
extern DRAM_ATTR bool g_bUpdateStarted; // Has an OTA update started?
extern uint8_t g_Brightness;            // Global brightness from drawing.cpp
extern DRAM_ATTR AppTime g_AppTime;     // For keeping track of frame timings
extern DRAM_ATTR uint32_t g_FPS;        // Our global framerate
extern DRAM_ATTR uint8_t giInfoPage;    // What page of screen we are showing
extern volatile double g_FreeDrawTime;  // Idle drawing time

DRAM_ATTR std::mutex Screen::_screenMutex; // The storage for the mutex of the screen class

bool g_ShowFPS = true; // Indicates whether little lcd should show FPS

// BasicInfoSummary
//
// THe page that shows Flash version, Wifi, clock, power, etc

void BasicInfoSummary(bool bRedraw)
{
#ifdef ARDUINO_HELTEC_WIFI_KIT_32
    // No border will be drawn so no inset margin needed
    const int xMargin = 0;
    const int yMargin = 0;
#else
    const int xMargin = 8;
    const int yMargin = 5;
#endif

    // Blue Theme

    #if USE_OLED
        const uint16_t bkgndColor = BLACK16;
    #else
        const uint16_t bkgndColor = Screen::to16bit(CRGB::DarkBlue);
    #endif

    const uint16_t borderColor = Screen::to16bit(CRGB::Yellow);
    const uint16_t textColor = Screen::to16bit(CRGB::White);

    // Green Terminal Theme
    //
    // const uint16_t bkgndColor  = Screen::to16bit(CRGB::Black);
    // const uint16_t borderColor = Screen::to16bit(CRGB::Red);
    // const uint16_t textColor   = Screen::to16bit(CRGB(100, 255, 20));

    // bRedraw is set for full redraw, in which case we fill the screen

    if (bRedraw)
        g_pDisplay->fillScreen(bkgndColor);
        //g_pDisplay->fillRect(1, 1, g_pDisplay->height() - 2, g_pDisplay->width() - 2, bkgndColor);

    // Status line 1

    static const String szStatus("|/-\\");
    static int cStatus = 0;
    int c2 = cStatus % szStatus.length();
    char chStatus = szStatus[c2];
    cStatus++;

    //g_pDisplay->setFont();
    g_pDisplay->setTextSize(g_pDisplay->width() >= 240 ? 2 : 1);
    #if USE_OLED
        g_pDisplay->setTextColor(WHITE16, BLACK16);
    #else
        g_pDisplay->setTextColor(textColor, bkgndColor); // Second color is background color, giving us text overwrite
    #endif

    g_pDisplay->setCursor(xMargin, yMargin);
    g_pDisplay->println(str_sprintf("%s:%dx%d %c %dK", FLASH_VERSION_NAME, NUM_CHANNELS, NUM_LEDS, chStatus, ESP.getFreeHeap() / 1024));

    // WiFi info line 2

    auto lineHeight = g_pDisplay->fontHeight();
    g_pDisplay->setCursor(xMargin + 0, yMargin + lineHeight);

    if (WiFi.isConnected() == false)
    {
        g_pDisplay->println("No Wifi");
    }
    else
    {
        const IPAddress address = WiFi.localIP();
        g_pDisplay->println(str_sprintf("%ddB:%d.%d.%d.%d",
                                    (int)labs(WiFi.RSSI()), // skip sign in first character
                                    address[0], address[1], address[2], address[3]));
    }

    // Buffer Status Line 3

    g_pDisplay->setCursor(xMargin + 0, yMargin + lineHeight * 4);
    g_pDisplay->println(str_sprintf("BUFR:%02d/%02d %dfps ",
                                g_aptrBufferManager[0]->Depth(),
                                g_aptrBufferManager[0]->BufferCount(),
                                g_FPS));

    // Data Status Line 4

    g_pDisplay->setCursor(xMargin + 0, yMargin + lineHeight * 2);
    g_pDisplay->println(str_sprintf("DATA:%+06.2lf-%+06.2lf",
                                min(99.99f, g_aptrBufferManager[0]->AgeOfOldestBuffer()),
                                min(99.99f, g_aptrBufferManager[0]->AgeOfNewestBuffer())));

    // Clock info Line 5
    //
    // Get the current clock time in HH:MM:SS format

    time_t t;
    time(&t);
    struct tm *tmp = localtime(&t);
    char szTime[16];
    strftime(szTime, ARRAYSIZE(szTime), "%H:%M:%S", tmp);

    g_pDisplay->setCursor(xMargin + 0, yMargin + lineHeight * 3);
    g_pDisplay->println(str_sprintf("CLCK:%s %04.3lf",
                                g_AppTime.CurrentTime() > 100000 ? szTime : "Unset",
                                g_FreeDrawTime));

    // LED Power Info Line 6 - only if display tall enough

    if (g_pDisplay->height() >= lineHeight * 5 + lineHeight)
    {
        g_pDisplay->setCursor(xMargin + 0, yMargin + lineHeight * 5);
        g_pDisplay->println(str_sprintf("POWR:%3.0lf%% %4uW\n",
                                    g_Brite,
                                    g_Watts));
    }

    // PSRAM Info Line 7 - only if display tall enough

    if (g_pDisplay->height() >= lineHeight * 7)
    {
        g_pDisplay->setCursor(xMargin + 0, yMargin + lineHeight * 6);
        g_pDisplay->println(str_sprintf("PRAM:%dK/%dK\n",
                                    ESP.getFreePsram() / 1024,
                                    ESP.getPsramSize() / 1024));
    }

    // Bar graph - across the bottom of the display showing buffer fill in a color, green/yellow/red
    //             that conveys the overall status

    if (g_pDisplay->height() >= lineHeight * 8)
    {
        int top = g_pDisplay->height() - lineHeight;
        int height = lineHeight - 3;
        int width = g_pDisplay->width() - xMargin * 2;
        float ratio = (float)g_aptrBufferManager[0]->Depth() / (float)g_aptrBufferManager[0]->BufferCount();
        ratio = std::min(1.0f, ratio);
        int filled = (width - 2) * ratio;

        // Color bar red/yellow/green depending on buffer fill

        uint16_t color = RED16;
        if (ratio > 0.25)
        {
            color = g_pDisplay->to16bit(CRGB::Orange);
            if (ratio > 0.5)
            {
                color = g_pDisplay->to16bit(CRGB::Green);
                if (ratio > 0.92)
                {
                    color = g_pDisplay->to16bit(CRGB::Orange);
                    if (ratio > 0.96)
                        color = g_pDisplay->to16bit(CRGB::Red);
                }
            }
        }

        #if USE_OLED
            g_pDisplay->fillRect(xMargin + 1, top, filled, height, WHITE16);
            g_pDisplay->fillRect(xMargin + filled, top, width - filled, height, BLACK16);
            g_pDisplay->drawRect(xMargin, top, width, height, WHITE16);
        #else
            g_pDisplay->fillRect(xMargin + 1, top + 1, filled, height - 2, color);
            g_pDisplay->fillRect(xMargin + filled, top + 1, width - filled, height - 2, bkgndColor);
            g_pDisplay->drawRect(xMargin, top, width, height, WHITE16);
        #endif
    }

#ifndef ARDUINO_HELTEC_WIFI_KIT_32
    g_pDisplay->drawRect(0, 0, g_pDisplay->width(), g_pDisplay->height(), borderColor);
#endif
}

// CurrentEffectSummary
//
// Draws the current effect, number of effects, and an audio spectrum when AUDIO is on

void CurrentEffectSummary(bool bRedraw)
{
    if (bRedraw)
        g_pDisplay->fillScreen(BLACK16);

    uint16_t backColor = Screen::to16bit(CRGB(0, 0, 64));

    // We only draw after a page flip or if anything has changed about the information that will be
    // shown in the page. This avoids flicker, but at the cost that we have to remember what we displayed
    // last time and check each time to see if its any different before drawing.

    static auto lasteffect = g_ptrEffectManager->GetCurrentEffectIndex();
    static auto sip = WiFi.localIP().toString();
    static auto lastFPS = g_FPS;
    static auto lastFullDraw = 0;
    static auto lastAudio = 0;
    static auto lastSerial = 0;
    auto yh = 2; // Start at top of screen

    if (lastFullDraw == 0 || millis() - lastFullDraw > 1000)
    {
        lastFullDraw = millis();
        if (bRedraw != false ||
            lasteffect != g_ptrEffectManager->GetCurrentEffectIndex() ||
            sip != WiFi.localIP().toString())
        {
            g_pDisplay->fillRect(0, 0, g_pDisplay->width(), g_pDisplay->TopMargin, backColor);
            g_pDisplay->fillRect(0, g_pDisplay->height() - g_pDisplay->BottomMargin, g_pDisplay->width(), g_pDisplay->BottomMargin, backColor);
            g_pDisplay->fillRect(0, g_pDisplay->TopMargin - 1, g_pDisplay->width(), 1, BLUE16);
            g_pDisplay->fillRect(0, g_pDisplay->height() - g_pDisplay->BottomMargin + 1, g_pDisplay->width(), 1, BLUE16);

            lasteffect = g_ptrEffectManager->GetCurrentEffectIndex();
            sip = WiFi.localIP().toString();
            lastFPS = g_FPS;

            //g_pDisplay->setFont();
            g_pDisplay->setTextSize(2);
            g_pDisplay->setTextColor(YELLOW16, backColor);
            String sEffect = String("Current Effect: ") +
                             String(g_ptrEffectManager->GetCurrentEffectIndex() + 1) +
                             String("/") +
                             String(g_ptrEffectManager->EffectCount());
            auto w = g_pDisplay->textWidth(sEffect);
            g_pDisplay->setCursor(g_pDisplay->width() / 2 - w / 2, yh);
            g_pDisplay->print(sEffect.c_str());
            yh += g_pDisplay->fontHeight();
            // get effect name length and switch text size accordingly
            int effectnamelen = g_ptrEffectManager->GetCurrentEffectName().length();

            g_pDisplay->setTextColor(WHITE16, backColor);
            w = g_pDisplay->textWidth(g_ptrEffectManager->GetCurrentEffectName());
            g_pDisplay->setCursor(g_pDisplay->width() / 2 - w / 2, yh);
            g_pDisplay->print(g_ptrEffectManager->GetCurrentEffectName());
            yh += g_pDisplay->fontHeight();
            
            String sIP = WiFi.isConnected() ? WiFi.localIP().toString().c_str() : "No Wifi";
            g_pDisplay->setTextColor(YELLOW16, backColor);
            w = g_pDisplay->textWidth(sIP);
            g_pDisplay->setCursor(g_pDisplay->width() / 2 - w / 2, yh);
            yh += g_pDisplay->fontHeight();
            g_pDisplay->print(sIP);
        }

#if ENABLE_AUDIO
        if ((g_ShowFPS && ((lastFPS != g_FPS) || (lastAudio != g_Analyzer._AudioFPS) || (lastSerial != g_Analyzer._serialFPS))))
        {
            lastFPS = g_FPS;
            lastSerial = g_Analyzer._serialFPS;
            lastAudio = g_Analyzer._AudioFPS;
            g_pDisplay->fillRect(0, g_pDisplay->height() - g_pDisplay->BottomMargin, g_pDisplay->width(), 1, BLUE16);
            yh = g_pDisplay->height() - g_pDisplay->fontHeight() + 6;
            g_pDisplay->setTextColor(YELLOW16, backColor);
            g_pDisplay->setTextSize(1);
            String strOut = str_sprintf(" LED: %2d  Aud: %2d Ser:%2d ", g_FPS, g_Analyzer._AudioFPS, g_Analyzer._serialFPS);
            auto w = g_pDisplay->textWidth(strOut);
            g_pDisplay->setCursor(g_pDisplay->width() / 2 - w / 2, yh);
            g_pDisplay->print(strOut);
            yh += g_pDisplay->fontHeight();
        }
#endif
    }

#if ENABLE_AUDIO

    // Draw the VU Meter and Spectrum every time.  yScale is the number of vertical pixels that would represent
    // a single LED on the LED matrix.

    static unsigned long lastDraw = millis();

    int xHalf = g_pDisplay->width() / 2 - 1;   // xHalf is half the screen width
    float ySizeVU = g_pDisplay->height() / 16; // vu is 1/20th the screen height, height of each block
    int cPixels = 16;
    float xSize = xHalf / cPixels + 1;                          // xSize is count of pixels in each block
    int litBlocks = (g_Analyzer._VURatioFade / 2.0f) * cPixels; // litPixels is number that are lit

    for (int iPixel = 0; iPixel < cPixels; iPixel++) // For each pixel
    {
        uint16_t color16 = iPixel > litBlocks ? BLACK16 : g_pDisplay->to16bit(ColorFromPalette(vuPaletteGreen, iPixel * (256 / (cPixels))));
        g_pDisplay->fillRect(xHalf - iPixel * xSize, g_pDisplay->TopMargin, xSize - 1, ySizeVU, color16);
        g_pDisplay->fillRect(xHalf + iPixel * xSize, g_pDisplay->TopMargin, xSize - 1, ySizeVU, color16);
    }

    // Draw the spectrum analyzer bars

    int spectrumTop = g_pDisplay->TopMargin + ySizeVU + 1; // Start at the bottom of the VU meter
    int bandHeight = g_pDisplay->height() - spectrumTop - g_pDisplay->BottomMargin;

    for (int iBand = 0; iBand < NUM_BANDS; iBand++)
    {
        CRGB bandColor = ColorFromPalette(RainbowColors_p, ((int)map(iBand, 0, NUM_BANDS, 0, 255) + 0) % 256);
        int bandWidth = g_pDisplay->width() / NUM_BANDS;
        auto color16 = g_pDisplay->to16bit(bandColor);
        auto topSection = bandHeight - bandHeight * g_Analyzer.g_peak2Decay[iBand];
        if (topSection > 0)
            g_pDisplay->fillRect(iBand * bandWidth, spectrumTop, bandWidth - 1, topSection, BLACK16);
        auto val = min(1.0f, g_Analyzer.g_peak2Decay[iBand]);
        assert(bandHeight * val <= bandHeight);
        g_pDisplay->fillRect(iBand * bandWidth, spectrumTop + topSection, bandWidth - 1, bandHeight - topSection, color16);
    }

    // Draw horizontal lines so the bars look like they are made of segments

//    for (int iLine = spectrumTop; iLine <= spectrumTop + bandHeight; iLine += g_pDisplay->height() / 25)
//        g_pDisplay->drawLine(0, iLine, g_pDisplay->width()-1, iLine, BLACK16);
#endif
}

// UpdateScreen
//
// Draws the OLED/LCD screen with the current stats on connection, buffer, drawing, etc.

void IRAM_ATTR UpdateScreen(bool bRedraw)
{

#if USE_SCREEN
    // If the display needs a refresh, we clear it here but we don't reset it yet so as to preserve
    // that info for drawing; it is set to false at the end of the function.

    // We don't want to be in the middle of drawing and have someone one another thread set the dirty
    // flag on us, so access to the flag is guarded by a mutex

    std::lock_guard<std::mutex> guard(g_pDisplay->_screenMutex);

    g_pDisplay->StartFrame();

    switch (giInfoPage)
    {
    case 0:
        BasicInfoSummary(bRedraw);
        break;

    case 1:
        CurrentEffectSummary(bRedraw);
        break;

    default:
        throw new std::runtime_error("Invalid info page in UpateScreen");
        break;
    }

    g_pDisplay->EndFrame();

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
    //debugI(">> ScreenUpdateLoopEntry\n");

    bool bRedraw = true;
    for (;;)
    {
        // bRedraw is set when the page changes so that it can get a full redraw.  It is also set initially as
        // nothing has been drawn for any page yet

#ifdef TOGGLE_BUTTON_1
        Button1.update();
        if (Button1.pressed())
        {
            std::lock_guard<std::mutex> guard(g_pDisplay->_screenMutex);

            // When the button is pressed advance to the next information page on the little display

            giInfoPage = (giInfoPage + 1) % NUM_INFO_PAGES;
            bRedraw = true;
        }
#endif

#ifdef TOGGLE_BUTTON_2
        Button2.update();
        if (Button2.pressed())
        {
            g_ptrEffectManager->NextEffect();
            bRedraw = true;
        }
#endif

        UpdateScreen(bRedraw);
        if (g_bUpdateStarted)
            delay(200);
        else
            delay(50);

        bRedraw = false;
    }
}

#endif
