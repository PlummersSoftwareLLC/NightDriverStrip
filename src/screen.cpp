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
#include "soundanalyzer.h"
#include "systemcontainer.h"
// std::clamp
#include <algorithm>
#include "screen.h"

#if defined(TOGGLE_BUTTON_0) || defined(TOGGLE_BUTTON_1)
#include "Bounce2.h" // For Bounce button class
#endif

#if USE_SCREEN

#if USE_TFTSPI
#include <SPI.h>
#include <TFT_eSPI.h>
#endif

#define SHOW_FPS true // Indicates whether little lcd should show FPS

DRAM_ATTR std::mutex Screen::_screenMutex; // The storage for the mutex of the screen class

// What page of screen we are showing
DRAM_ATTR uint8_t g_iCurrentPage = 0; // Will initialize to last active page on first draw

// Default implementation for Page button handler (zero-based):
// Button 0 = NextEffect, Button 1 = Flip page

void Page::OnButtonPress(uint8_t buttonIndex)
{
    if (buttonIndex == 0)
    {
        // Default: advance to the next effect
        g_ptrSystem->EffectManager().NextEffect();
    }
    else if (buttonIndex == 1)
    {
        // Default: flip to the next page
        Screen::FlipToNextPage();
    }
}

// Page implementations and registry

class BasicInfoSummaryPage final : public Page
{
  public:
    std::string Name() const override { return "BasicInfoSummary"; }

    void OnButtonPress(uint8_t buttonIndex) override
    {
        Page::OnButtonPress(buttonIndex);
    }

    void Draw(Screen &display, bool bRedraw) override
    {
        // No border will be drawn so no inset margin needed on Heltec OLED
        #ifdef ARDUINO_HELTEC_WIFI_KIT_32
            const int xMargin = 0;
            const int yMargin = 0;
        #else
            const int xMargin = 8;
            const int yMargin = 5;
        #endif

        // Theme colors come from Screen implementation
        const uint16_t bkgndColor = display.GetBkgndColor();
        const uint16_t borderColor = display.GetBorderColor();
        const uint16_t textColor = display.GetTextColor();

        if (bRedraw)
            display.fillScreen(bkgndColor);

        // Status line 1
        static const String szStatus("|/-\\");
        static int cStatus = 0;
        char chStatus = szStatus[cStatus % szStatus.length()];
        cStatus++;

        if (display.width() > 480)
            display.setTextSize(3);
        else if (display.width() > 160)
            display.setTextSize(2);
        else
            display.setTextSize(1);

        // Second param is background for clean overwrite
        display.setTextColor(display.GetTextColor(), display.GetBkgndColor());
        display.setCursor(xMargin, yMargin);
        display.println(str_sprintf("%s:%dx%d %c %dK", FLASH_VERSION_NAME, g_ptrSystem->Devices().size(), NUM_LEDS,
                                    chStatus, ESP.getFreeHeap() / 1024));

        // WiFi info line 2
        auto lineHeight = display.fontHeight();
        display.setCursor(xMargin + 0, yMargin + lineHeight);
        if (!WiFi.isConnected())
            display.println("No Wifi");
        else
        {
            const IPAddress address = WiFi.localIP();
            display.println(str_sprintf("%ddB:%d.%d.%d.%d", (int)labs(WiFi.RSSI()), address[0], address[1], address[2], address[3]));
        }

        // Buffer Status Line 3
        auto &bufferManager = g_ptrSystem->BufferManagers()[0];
        display.setCursor(xMargin + 0, yMargin + lineHeight * 4);
        display.println(str_sprintf("BUFR:%02d/%02d %dfps ", bufferManager.Depth(), bufferManager.BufferCount(), g_Values.FPS));

        // Data Status Line 4
        display.setCursor(xMargin + 0, yMargin + lineHeight * 2);
        display.println(str_sprintf("DATA:%+06.2lf-%+06.2lf", std::min(99.99, bufferManager.AgeOfOldestBuffer()),
                                    std::min(99.99, bufferManager.AgeOfNewestBuffer())));

        // Clock info Line 5
        time_t t;
        time(&t);
        struct tm *tmp = localtime(&t);
        char szTime[16];
        strftime(szTime, std::size(szTime), "%H:%M:%S", tmp);
        display.setCursor(xMargin + 0, yMargin + lineHeight * 3);
        display.println(str_sprintf("CLCK:%s %04.3lf", g_Values.AppTime.CurrentTime() > 100000 ? szTime : "Unset",
                                    g_Values.FreeDrawTime));

        // LED Power Info Line 6 - only if display tall enough
        if (display.height() >= lineHeight * 5 + lineHeight)
        {
            display.setCursor(xMargin + 0, yMargin + lineHeight * 5);
            display.println(str_sprintf("POWR:%3.0lf%% %4uW\n", g_Values.Brite, g_Values.Watts));
        }

        // PSRAM/CPU Info Line 7 - only if display tall enough
        if (display.height() >= lineHeight * 7)
        {
            auto &taskManager = g_ptrSystem->TaskManager();
            display.setCursor(xMargin + 0, yMargin + lineHeight * 6);
            display.println(str_sprintf("CPU: %3.0f%%, %3.0f%%  ", taskManager.GetCPUUsagePercent(0), taskManager.GetCPUUsagePercent(1)));
        }

        // Bar graph at bottom
        if (display.height() >= lineHeight * 8)
        {
            int top = display.height() - lineHeight;
            int height = lineHeight - 3;
            int width = display.width() - xMargin * 2;
            float ratio = (float)bufferManager.Depth() / (float)bufferManager.BufferCount();
            ratio = std::min(1.0f, ratio);
            int filled = (width - 2) * ratio;

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

            if (display.IsMonochrome())
            {
                display.fillRect(xMargin + 1, top, filled, height, display.GetTextColor());
                display.fillRect(xMargin + filled, top, width - filled, height, display.GetBkgndColor());
                display.drawRect(xMargin, top, width, height, display.GetBorderColor());
            }
            else
            {
                display.fillRect(xMargin + 1, top + 1, filled, height - 2, color);
                display.fillRect(xMargin + filled, top + 1, width - filled, height - 2, bkgndColor);
                display.drawRect(xMargin, top, width, height, display.GetBorderColor());
            }
        }
    }
};

// Shared header/footer for effect pages
class TitlePage : public Page
{
  protected:
    // Cached content top calculated using the header text size to avoid shifting
    int _contentTop = 0;
    int ContentTop(Screen &display) const { return _contentTop > 0 ? _contentTop : (display.fontHeight() * 3 + 4); }

  public:
    std::string Name() const override
    {
        return "CurrentEffectSummary";
    }
    void OnButtonPress(uint8_t buttonIndex) override
    {
        if (buttonIndex == 1)
        {
            // On this page, use Button 1 to advance to the next effect
            debugI("Button 1 pressed so advancing to next effect");
            g_ptrSystem->EffectManager().NextEffect();
        }
        else
        {
            Page::OnButtonPress(buttonIndex);
        }
    }

    void Draw(Screen &display, bool bRedraw) override
    {
        if (bRedraw)
            display.fillScreen(BLACK16);

        uint16_t backColor = display.IsMonochrome() ? BLACK16 : Screen::to16bit(CRGB(0, 0, 64));

        static auto lasteffect = g_ptrSystem->EffectManager().GetCurrentEffectIndex();
        static auto sip = WiFi.localIP().toString();
        static auto lastFPS = g_Values.FPS;
        static auto lastFullDraw = 0;
        static auto lastAudio = 0;
        static auto lastSerial = 0;
        static auto lastScreen = millis();
        static String lastFooter = "";
        float screenFPS = 0;
        auto yh = 2; // Start at top of screen

        display.setTextSize(display.width() > 160 ? 2 : 1);
        const int topMargin = display.fontHeight() * 3 + 4;

        // Screen FPS (for display updates)
        screenFPS = (millis() - lastScreen) / 1000.0f;
        if (screenFPS != 0)
            screenFPS = 1.0f / screenFPS;
        lastScreen = millis();

        // Redraw header when needed
        if (bRedraw || lastFullDraw == 0 || millis() - lastFullDraw > 1000)
        {
            lastFullDraw = millis();
            const int currentEffect = g_ptrSystem->EffectManager().GetCurrentEffectIndex();
            const String currentIP = WiFi.localIP().toString();

            if (bRedraw || lasteffect != currentEffect || sip != currentIP)
            {
                display.fillRect(0, 0, display.width(), topMargin, backColor);
                display.fillRect(0, display.height() - display.BottomMargin, display.width(), display.BottomMargin, backColor);
                display.fillRect(0, topMargin - 1, display.width(), 1, BLUE16);
                display.fillRect(0, display.height() - display.BottomMargin + 1, display.width(), 1, BLUE16);

                lasteffect = g_ptrSystem->EffectManager().GetCurrentEffectIndex();
                sip = WiFi.localIP().toString();
                lastFPS = g_Values.FPS;

                display.setTextColor(display.GetBorderColor(), backColor);
                String sEffect = String("Effect: ") + String(g_ptrSystem->EffectManager().GetCurrentEffectIndex() + 1) +
                                 String("/") + String(g_ptrSystem->EffectManager().EffectCount());
                auto w = display.textWidth(sEffect);
                display.setCursor(display.width() / 2 - w / 2, yh);
                display.print(sEffect.c_str());
                yh += display.fontHeight();

                display.setTextColor(display.GetTextColor(), backColor);
                w = display.textWidth(g_ptrSystem->EffectManager().GetCurrentEffectName());
                display.setCursor(display.width() / 2 - w / 2, yh);
                display.print(g_ptrSystem->EffectManager().GetCurrentEffectName());
                yh += display.fontHeight();

                String sIP = WiFi.isConnected() ? currentIP.c_str() : "No Wifi";
                display.setTextColor(display.GetBorderColor(), backColor);
                w = display.textWidth(sIP);
                display.setCursor(display.width() / 2 - w / 2, yh);
                display.print(sIP);
            }

            // Footer line
            String footer = str_sprintf(" LED: %2d  Scr: %02d", g_Values.FPS, (int)screenFPS);
            #if ENABLE_AUDIO
                footer = str_sprintf(" LED: %2d  Aud: %2d Ser:%2d Scr: %02d", g_Values.FPS, g_Analyzer.AudioFPS(), g_Analyzer.SerialFPS(), (int)screenFPS);
            #endif
            
            if (footer != lastFooter)
            {
                lastFooter = footer;
                display.fillRect(0, display.height() - display.BottomMargin, display.width(), 1, BLUE16);
                display.setTextColor(display.GetBorderColor(), backColor);
                display.setTextSize(1);
                int yh = display.height() - display.fontHeight();
                auto w = display.textWidth(footer);
                display.setCursor(display.width() / 2 - w / 2, yh);
                display.print(footer);
            }
        }
    }
};

class CurrentEffectSummaryPage final : public TitlePage
{
  public:
    std::string Name() const override { return "CurrentEffectSummary"; }

    void OnButtonPress(uint8_t buttonIndex) override
    {
        if (buttonIndex == 0)
        {
            debugI("Button 1 pressed so advancing to next effect");
            g_ptrSystem->EffectManager().NextEffect();
        }
        else
        {
            Page::OnButtonPress(buttonIndex);
        }
    }

    void Draw(Screen &display, bool bRedraw) override
    {
        // Draw shared header/footer
        TitlePage::Draw(display, bRedraw);

        // Add spectrum analyzer content
        #if ENABLE_AUDIO
        {
            const int topMargin = ContentTop(display);

            // Draw VU
            const int xHalf = display.width() / 2 - 1;
            const float ySizeVU = display.height() / 16; // height of each block
            const int cPixels = 16;
            const float xSize = xHalf / (float)cPixels + 1;
            const int litBlocks = (g_Analyzer.VURatioFade() / 2.0f) * cPixels;
            for (int iPixel = 0; iPixel < cPixels; iPixel++)
            {
                uint16_t color16 = iPixel > litBlocks ? BLACK16 : display.to16bit(ColorFromPalette(vuPaletteGreen, iPixel * (256 / (cPixels))));
                display.fillRect(xHalf - iPixel * xSize, topMargin, xSize - 1, ySizeVU, color16);
                display.fillRect(xHalf + iPixel * xSize, topMargin, xSize - 1, ySizeVU, color16);
            }

            // Spectrum bands
            const int spectrumTop = topMargin + ySizeVU + 1;
            const int bandHeight = display.height() - spectrumTop - display.BottomMargin;
            for (int iBand = 0; iBand < NUM_BANDS; iBand++)
            {
                CRGB bandColor = ColorFromPalette(RainbowColors_p, ((int)map(iBand, 0, NUM_BANDS, 0, 255)) % 256);
                int bandWidth = display.width() / NUM_BANDS;
                auto color16 = display.to16bit(bandColor);
                auto topSection = bandHeight - bandHeight * g_Analyzer.Peak2Decay(iBand);
                if (topSection > 0)
                    display.fillRect(iBand * bandWidth, spectrumTop, bandWidth - 1, topSection, BLACK16);
                auto val = min(1.0f, g_Analyzer.Peak2Decay(iBand));
                assert(bandHeight * val <= bandHeight);
                display.fillRect(iBand * bandWidth, spectrumTop + topSection, bandWidth - 1, bandHeight - topSection, color16);
            }
        }
        #endif // ENABLE_AUDIO
    }
};

class EffectSimulatorPage final : public TitlePage
{
  public:
    std::string Name() const override { return "CurrentEffect"; }

    void Draw(Screen &display, bool bRedraw) override
    {
        // Draw shared header/footer first
        TitlePage::Draw(display, bRedraw);

        // Determine content area between header and footer
        const int headerTop = ContentTop(display);
        const int contentTop = headerTop;
        const int contentHeight = display.height() - display.BottomMargin - contentTop;
        const int contentWidth = display.width();

        // Matrix dimensions
        const int mw = MATRIX_WIDTH;
        const int mh = MATRIX_HEIGHT;
        if (mw <= 0 || mh <= 0 || contentWidth <= 0 || contentHeight <= 0)
            return;

        // Max integer scale that fits both dimensions
        int sx = contentWidth / mw;
        int sy = contentHeight / mh;
        int scale = std::min(sx, sy);
        if (scale <= 0)
            scale = 1; // No fractional downscale; ensure at least 1px per LED

        // Center the image in the content area
        const int drawWidth = mw * scale;
        const int drawHeight = mh * scale;
        const int xOffset = (contentWidth - drawWidth) / 2;
        const int yOffset = contentTop + (contentHeight - drawHeight) / 2;

        // Optional: clear content area on full redraw to avoid ghosting around edges
        if (bRedraw)
            display.fillRect(0, contentTop, contentWidth, contentHeight, BLACK16);

        // Fetch current graphics buffer
        auto &effectManager = g_ptrSystem->EffectManager();
        auto gfx = effectManager.g();
        if (!gfx || gfx->leds == nullptr)
            return;

        // Blit: draw each LED as a scale x scale rectangle
        for (int y = 0; y < mh; ++y)
        {
            for (int x = 0; x < mw; ++x)
            {
                // Use getPixel to respect XY mapping
                CRGB c = gfx->getPixel(x, y);
                uint16_t c16 = display.to16bit(c);
                int px = xOffset + x * scale;
                int py = yOffset + y * scale;
                // Draw filled rect; subtract 1 to create a fine 1px grid line when scale > 1
                if (scale > 1)
                    display.fillRect(px, py, scale - 1, scale - 1, c16);
                else
                    display.drawPixel(px, py, c16);
            }
        }
    }
};

// Screen private helpers (declared in screen.h)
std::vector<std::unique_ptr<Page>> &Screen::Pages()
{
    static std::vector<std::unique_ptr<Page>> pages;
    if (pages.empty())
    {
        pages.emplace_back(std::make_unique<BasicInfoSummaryPage>());
        pages.emplace_back(std::make_unique<CurrentEffectSummaryPage>());
        pages.emplace_back(std::make_unique<EffectSimulatorPage>());
    }
    return pages;
}

int Screen::ActivePageCount()
{
    auto &pages = Pages();
    int available = (int)pages.size();
    int limit = std::clamp(NUM_INFO_PAGES, 1, available);
    return limit;
}
// End of page implementations and helpers

// FlipToNextPage
void Screen::FlipToNextPage()
{
    std::lock_guard<std::mutex> guard(_screenMutex);

    // Advance to the next page
    const int activeCount = ActivePageCount();
    g_iCurrentPage = (g_iCurrentPage + 1) % std::max(1, activeCount);
}

// Old free functions replaced by Screen methods below

// Screen::Update
// Draws the OLED/LCD screen with the current stats on connection, buffer, drawing, etc.
void IRAM_ATTR Screen::Update(bool bRedraw)
{
    #if USE_SCREEN
        std::lock_guard<std::mutex> guard(_screenMutex);

        // Initialize default page to last active page on first draw
        static bool s_initialized = false;
        const int activeCount = ActivePageCount();
        if (!s_initialized)
        {
            g_iCurrentPage = activeCount > 0 ? activeCount - 1 : 0;
            s_initialized = true;
            bRedraw = true;
        }

        // Ensure index in range
        if (g_iCurrentPage >= activeCount)
            g_iCurrentPage = std::max(0, activeCount - 1);

        StartFrame();
        auto &pages = Pages();
        pages[g_iCurrentPage]->Draw(*this, bRedraw);
        EndFrame();
    #endif
}

// Screen::RunUpdateLoop
// Displays statistics on the Heltec's built in OLED board.  If you are using a different board, you would simply get
// rid of this or modify it to fit a screen you do have.  You could also try serial output, as it's on a low-pri thread
// it shouldn't disturb the primary cores.

void IRAM_ATTR Screen::RunUpdateLoop()
{
    // debugI(">> ScreenUpdateLoopEntry\n");

    bool bRedraw = true;
    // Lazy init of buttons when loop starts (after hardware/defines are known)
    static bool s_buttonsInited = false;
    if (!s_buttonsInited)
    {
        #ifdef TOGGLE_BUTTON_0
            _button0.attach(TOGGLE_BUTTON_0, INPUT_PULLUP);
            _button0.interval(1);
            _button0.setPressedState(LOW);
        #endif
        #ifdef TOGGLE_BUTTON_1
            _button1.attach(TOGGLE_BUTTON_1, INPUT_PULLUP);
            _button1.interval(1);
            _button1.setPressedState(LOW);
        #endif
        s_buttonsInited = true;
    }
    for (;;)
    {
        // bRedraw is set when the page changes so that it can get a full redraw.  It is also set initially as
        // nothing has been drawn for any page yet

        #if USE_M5
            // Use M5Unified button abstraction when available
            M5.update();
            if (M5.BtnA.wasPressed())
            {
                auto &pages = Pages();
                pages[g_iCurrentPage]->OnButtonPress(0); // BtnA -> button 0 (flip page by default)
                bRedraw = true;
            }
            if (M5.BtnB.wasPressed())
            {
                auto &pages = Pages();
                pages[g_iCurrentPage]->OnButtonPress(1); // BtnB -> button 1 (next effect by default)
                bRedraw = true;
            }
        #endif

    #ifdef TOGGLE_BUTTON_0
            _button0.update();
            if (_button0.pressed())
            {
                auto &pages = Pages();
                pages[g_iCurrentPage]->OnButtonPress(0);
                bRedraw = true;
            }
        #endif

    #ifdef TOGGLE_BUTTON_1
            _button1.update();
            if (_button1.pressed())
            {
                auto &pages = Pages();
                pages[g_iCurrentPage]->OnButtonPress(1);
                bRedraw = true;
            }
        #endif

        this->Update(bRedraw);
        if (g_Values.UpdateStarted)
        {
            delay(200);
        }
        else
        {
            #if AMOLED_S3
                lv_task_handler();
            #endif
            delay(1);
        }
        bRedraw = false;
    }
}

// Thin wrapper to preserve the existing FreeRTOS task entry point name while
// delegating to the new Screen::RunUpdateLoop() method.
void IRAM_ATTR ScreenUpdateLoopEntry(void *)
{
    // Ensure the system and display are available
    if (g_ptrSystem)
    {
        auto &display = g_ptrSystem->Display();
        display.RunUpdateLoop();
    }
}

#endif