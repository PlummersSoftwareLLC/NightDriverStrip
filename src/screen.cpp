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

#include <algorithm>
#include "globals.h"
#include "systemcontainer.h"
#include "soundanalyzer.h"

#if defined(TOGGLE_BUTTON_1) || defined(TOGGLE_BUTTON_2)
  #include "Bounce2.h"                            // For Bounce button class
#endif

#if USE_SCREEN

#if USE_TFTSPI
#include <TFT_eSPI.h>
#include <SPI.h>
#endif

#define SHOW_FPS    true                                // Indicates whether little lcd should show FPS

DRAM_ATTR std::mutex Screen::_screenMutex;              // The storage for the mutex of the screen class

// How many screen pages do we have
constexpr uint8_t g_InfoPageCount = std::clamp(NUM_INFO_PAGES, 1, 2);

// What page of screen we are showing
DRAM_ATTR uint8_t g_InfoPage = g_InfoPageCount - 1;      // Default to last page

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

    auto& display = g_ptrSystem->Display();

    if (bRedraw)
        display.fillScreen(bkgndColor);

    // Status line 1

    static const String szStatus("|/-\\");
    static int cStatus = 0;
    int c2 = cStatus % szStatus.length();
    char chStatus = szStatus[c2];
    cStatus++;

    //display.setFont();
    display.setTextSize(display.width() >= 240 ? 2 : 1);
    #if USE_OLED
        display.setTextColor(WHITE16, BLACK16);
    #else
        display.setTextColor(textColor, bkgndColor); // Second color is background color, giving us text overwrite
    #endif

    display.setCursor(xMargin, yMargin);
    display.println(str_sprintf("%s:%dx%d %c %dK", FLASH_VERSION_NAME, g_ptrSystem->Devices().size(), NUM_LEDS, chStatus, ESP.getFreeHeap() / 1024));

    // WiFi info line 2

    auto lineHeight = display.fontHeight();
    display.setCursor(xMargin + 0, yMargin + lineHeight);

    if (WiFi.isConnected() == false)
    {
        display.println("No Wifi");
    }
    else
    {
        const IPAddress address = WiFi.localIP();
        display.println(str_sprintf("%ddB:%d.%d.%d.%d",
                                    (int)labs(WiFi.RSSI()), // skip sign in first character
                                    address[0], address[1], address[2], address[3]));
    }

    // Buffer Status Line 3

    auto& bufferManager = g_ptrSystem->BufferManagers()[0];

    display.setCursor(xMargin + 0, yMargin + lineHeight * 4);
    display.println(str_sprintf("BUFR:%02d/%02d %dfps ",
                                bufferManager.Depth(),
                                bufferManager.BufferCount(),
                                g_Values.FPS));

    // Data Status Line 4

    display.setCursor(xMargin + 0, yMargin + lineHeight * 2);
    display.println(str_sprintf("DATA:%+06.2lf-%+06.2lf",
                                std::min(99.99, bufferManager.AgeOfOldestBuffer()),
                                std::min(99.99, bufferManager.AgeOfNewestBuffer())));

    // Clock info Line 5
    //
    // Get the current clock time in HH:MM:SS format

    time_t t;
    time(&t);
    struct tm *tmp = localtime(&t);
    char szTime[16];
    strftime(szTime, ARRAYSIZE(szTime), "%H:%M:%S", tmp);

    display.setCursor(xMargin + 0, yMargin + lineHeight * 3);
    display.println(str_sprintf("CLCK:%s %04.3lf",
                                g_Values.AppTime.CurrentTime() > 100000 ? szTime : "Unset",
                                g_Values.FreeDrawTime));

    // LED Power Info Line 6 - only if display tall enough

    if (display.height() >= lineHeight * 5 + lineHeight)
    {
        display.setCursor(xMargin + 0, yMargin + lineHeight * 5);
        display.println(str_sprintf("POWR:%3.0lf%% %4uW\n",
                                    g_Values.Brite,
                                    g_Values.Watts));
    }

    // PSRAM Info Line 7 - only if display tall enough

    if (display.height() >= lineHeight * 7)
    {
        auto& taskManager = g_ptrSystem->TaskManager();
        display.setCursor(xMargin + 0, yMargin + lineHeight * 6);
        display.println(str_sprintf("CPU: %3.0f%%, %3.0f%%  ", taskManager.GetCPUUsagePercent(0), taskManager.GetCPUUsagePercent(1)));
    }

    /* Old PSRAM code
    display.setCursor(xMargin + 0, yMargin + lineHeight * 7);
    display.println(str_sprintf("PRAM:%dK/%dK\n",
                                ESP.getFreePsram() / 1024,
                                ESP.getPsramSize() / 1024));
    */

    // Bar graph - across the bottom of the display showing buffer fill in a color, green/yellow/red
    //             that conveys the overall status

    if (display.height() >= lineHeight * 8)
    {
        int top = display.height() - lineHeight;
        int height = lineHeight - 3;
        int width = display.width() - xMargin * 2;
        float ratio = (float)bufferManager.Depth() / (float)bufferManager.BufferCount();
        ratio = std::min(1.0f, ratio);
        int filled = (width - 2) * ratio;

        // Color bar red/yellow/green depending on buffer fill

        uint16_t color = RED16;
        if (ratio > 0.25)
        {
            color = display.to16bit(CRGB::Orange);
            if (ratio > 0.5)
            {
                color = display.to16bit(CRGB::Green);
                if (ratio > 0.92)
                {
                    color = display.to16bit(CRGB::Orange);
                    if (ratio > 0.96)
                        color = display.to16bit(CRGB::Red);
                }
            }
        }

        #if USE_OLED
            display.fillRect(xMargin + 1, top, filled, height, WHITE16);
            display.fillRect(xMargin + filled, top, width - filled, height, BLACK16);
            display.drawRect(xMargin, top, width, height, WHITE16);
        #else
            display.fillRect(xMargin + 1, top + 1, filled, height - 2, color);
            display.fillRect(xMargin + filled, top + 1, width - filled, height - 2, bkgndColor);
            display.drawRect(xMargin, top, width, height, WHITE16);
        #endif
    }


#ifndef ARDUINO_HELTEC_WIFI_KIT_32
    display.drawRect(0, 0, display.width(), display.height(), borderColor);
#endif
}

// CurrentEffectSummary
//
// Draws the current effect, number of effects, and an audio spectrum when AUDIO is on

void CurrentEffectSummary(bool bRedraw)
{
    auto& display = g_ptrSystem->Display();
    display.StartFrame();

    if (bRedraw)
        display.fillScreen(BLACK16);

    uint16_t backColor = Screen::to16bit(CRGB(0, 0, 64));

    // We only draw after a page flip or if anything has changed about the information that will be
    // shown in the page. This avoids flicker, but at the cost that we have to remember what we displayed
    // last time and check each time to see if its any different before drawing.

    static auto lasteffect = g_ptrSystem->EffectManager().GetCurrentEffectIndex();
    static auto sip = WiFi.localIP().toString();
    static auto lastFPS = g_Values.FPS;
    static auto lastFullDraw = 0;
    static auto lastAudio = 0;
    static auto lastSerial = 0;
    auto yh = 2; // Start at top of screen

    display.setTextSize(display.width() > 160 ? 2 : 1);
    const int topMargin = display.fontHeight() * 3 + 4;

    if (lastFullDraw == 0 || millis() - lastFullDraw > 1000)
    {
        lastFullDraw = millis();
        if (bRedraw != false ||
            lasteffect != g_ptrSystem->EffectManager().GetCurrentEffectIndex() ||
            sip != WiFi.localIP().toString())
        {
            display.fillRect(0, 0, display.width(), topMargin, backColor);
            display.fillRect(0, display.height() - display.BottomMargin, display.width(), display.BottomMargin, backColor);
            display.fillRect(0, topMargin - 1, display.width(), 1, BLUE16);
            display.fillRect(0, display.height() - display.BottomMargin + 1, display.width(), 1, BLUE16);

            lasteffect = g_ptrSystem->EffectManager().GetCurrentEffectIndex();
            sip = WiFi.localIP().toString();
            lastFPS = g_Values.FPS;

            //display.setFont();
            display.setTextColor(YELLOW16, backColor);
            String sEffect = String("Current Effect: ") +
                             String(g_ptrSystem->EffectManager().GetCurrentEffectIndex() + 1) +
                             String("/") +
                             String(g_ptrSystem->EffectManager().EffectCount());
            auto w = display.textWidth(sEffect);
            display.setCursor(display.width() / 2 - w / 2, yh);
            display.print(sEffect.c_str());
            yh += display.fontHeight();

            display.setTextColor(WHITE16, backColor);
            w = display.textWidth(g_ptrSystem->EffectManager().GetCurrentEffectName());
            display.setCursor(display.width() / 2 - w / 2, yh);
            display.print(g_ptrSystem->EffectManager().GetCurrentEffectName());
            yh += display.fontHeight();

            String sIP = WiFi.isConnected() ? WiFi.localIP().toString().c_str() : "No Wifi";
            display.setTextColor(YELLOW16, backColor);
            w = display.textWidth(sIP);
            display.setCursor(display.width() / 2 - w / 2, yh);
            yh += display.fontHeight();
            display.print(sIP);
        }

#if ENABLE_AUDIO
        if (SHOW_FPS && ((lastFPS != g_Values.FPS) || (lastAudio != g_Analyzer._AudioFPS) || (lastSerial != g_Analyzer._serialFPS)))
        {
            lastFPS = g_Values.FPS;
            lastSerial = g_Analyzer._serialFPS;
            lastAudio = g_Analyzer._AudioFPS;
            display.fillRect(0, display.height() - display.BottomMargin, display.width(), 1, BLUE16);
            display.setTextColor(YELLOW16, backColor);
            display.setTextSize(1);
            yh = display.height() - display.fontHeight();
            String strOut = str_sprintf(" LED: %2d  Aud: %2d Ser:%2d ", g_Values.FPS, g_Analyzer._AudioFPS, g_Analyzer._serialFPS);
            auto w = display.textWidth(strOut);
            display.setCursor(display.width() / 2 - w / 2, yh);
            display.print(strOut);
            yh += display.fontHeight();
        }
#endif
    }

#if ENABLE_AUDIO

    // Draw the VU Meter and Spectrum every time.  yScale is the number of vertical pixels that would represent
    // a single LED on the LED matrix.

    static unsigned long lastDraw = millis();

    int xHalf = display.width() / 2 - 1;   // xHalf is half the screen width
    float ySizeVU = display.height() / 16; // vu is 1/20th the screen height, height of each block
    int cPixels = 16;
    float xSize = xHalf / cPixels + 1;                          // xSize is count of pixels in each block
    int litBlocks = (g_Analyzer._VURatioFade / 2.0f) * cPixels; // litPixels is number that are lit

    for (int iPixel = 0; iPixel < cPixels; iPixel++) // For each pixel
    {
        uint16_t color16 = iPixel > litBlocks ? BLACK16 : display.to16bit(ColorFromPalette(vuPaletteGreen, iPixel * (256 / (cPixels))));
        display.fillRect(xHalf - iPixel * xSize, topMargin, xSize - 1, ySizeVU, color16);
        display.fillRect(xHalf + iPixel * xSize, topMargin, xSize - 1, ySizeVU, color16);
    }

    // Draw the spectrum analyzer bars

    int spectrumTop = topMargin + ySizeVU + 1; // Start at the bottom of the VU meter
    int bandHeight = display.height() - spectrumTop - display.BottomMargin;

    for (int iBand = 0; iBand < NUM_BANDS; iBand++)
    {
        CRGB bandColor = ColorFromPalette(RainbowColors_p, ((int)map(iBand, 0, NUM_BANDS, 0, 255) + 0) % 256);
        int bandWidth = display.width() / NUM_BANDS;
        auto color16 = display.to16bit(bandColor);
        auto topSection = bandHeight - bandHeight * g_Analyzer._peak2Decay[iBand];
        if (topSection > 0)
            display.fillRect(iBand * bandWidth, spectrumTop, bandWidth - 1, topSection, BLACK16);
        auto val = min(1.0f, g_Analyzer._peak2Decay[iBand]);
        assert(bandHeight * val <= bandHeight);
        display.fillRect(iBand * bandWidth, spectrumTop + topSection, bandWidth - 1, bandHeight - topSection, color16);
    }

    display.EndFrame();

    // Draw horizontal lines so the bars look like they are made of segments

//    for (int iLine = spectrumTop; iLine <= spectrumTop + bandHeight; iLine += display.height() / 25)
//        display.drawLine(0, iLine, display.width()-1, iLine, BLACK16);
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

    auto& display = g_ptrSystem->Display();

    std::lock_guard<std::mutex> guard(display._screenMutex);

    display.StartFrame();

    switch (g_InfoPage)
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

    display.EndFrame();

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
        static uint effectInterval;

        Button1.update();
        if (Button1.pressed())
        {
            std::lock_guard<std::mutex> guard(g_ptrSystem->Display()._screenMutex);

            // When the button is pressed advance to the next information page on the little display

            g_InfoPage = (g_InfoPage + 1) % g_InfoPageCount;

            auto& effectManager = g_ptrSystem->EffectManager();

            // We stop rotating the effects when we are on the debug info page, and resume when we are not
            if (g_InfoPage == 0)
            {
                effectInterval = effectManager.GetInterval();
                effectManager.SetInterval(0, true);
            }
            // Restore effect interval to the value we remembered, on the proviso that effect rotation is now indeed
            // paused. Otherwise, the user may have chosen a different effect interval while we weren't looking and
            // we don't want to mess with that.
            else if (effectManager.GetInterval() == 0)
                effectManager.SetInterval(effectInterval, true);

            bRedraw = true;
        }
#endif

#ifdef TOGGLE_BUTTON_2
        Button2.update();
        if (Button2.pressed())
        {
            debugI("Button 2 pressed on pin %d so advancing to next effect", TOGGLE_BUTTON_2);
            g_ptrSystem->EffectManager().NextEffect();
            bRedraw = true;
        }
#endif

        UpdateScreen(bRedraw);
        if (g_Values.UpdateStarted)
            delay(200);
        else
            delay(50);

        bRedraw = false;
    }
}

#endif
