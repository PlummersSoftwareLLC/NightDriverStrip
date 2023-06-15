//+--------------------------------------------------------------------------
//
// File:        drawing.cpp
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
//    Main draw loop and rendering code
//
// History:     May-11-2021         Davepl      Commented
//              Nov-02-2022         Davepl      Broke up into multiple functions
//
//---------------------------------------------------------------------------

#include <mutex>
#include <ArduinoOTA.h> // Over-the-air helper object so we can be flashed via WiFi
#include "globals.h"
#include "effects/matrix/spectrumeffects.h"

CRGB g_SinglePixel = CRGB::Blue;
CLEDController *g_ledSinglePixel;

// The g_buffer_mutex is a global mutex used to protect access while adding or removing frames
// from the led buffer.

extern std::mutex g_buffer_mutex;

#if USE_MATRIX
extern SmartMatrixHub75Calc<COLOR_DEPTH, LEDMatrixGFX::kMatrixWidth, LEDMatrixGFX::kMatrixHeight, LEDMatrixGFX::kPanelType, LEDMatrixGFX::kMatrixOptions> LEDMatrixGFX::matrix;
#endif

DRAM_ATTR std::unique_ptr<LEDBufferManager> g_aptrBufferManager[NUM_CHANNELS];
DRAM_ATTR std::unique_ptr<EffectManager<GFXBase>> g_ptrEffectManager = nullptr;

double volatile g_FreeDrawTime = 0.0;

extern uint32_t g_FPS;
extern AppTime g_AppTime;
extern bool g_bUpdateStarted;
extern float g_Brite;
extern uint32_t g_Watts;
extern const CRGBPalette16 vuPaletteGreen;

void ShowTM1814();

// DrawLoop
//
// Pull packets from the Wifi buffer if they've come due and draw them - if it-'s a few seconds without a WiFi frame,
// we will draw the local effect instead

DRAM_ATTR uint64_t g_usLastWifiDraw = 0;
DRAM_ATTR uint8_t g_Brightness = 255;
DRAM_ATTR uint8_t g_Fader = 255;

// MatrixPreDraw
//
// Gets the matrix ready for the effect or wifi to render into

void MatrixPreDraw()
{
#if USE_MATRIX
    // We treat the internal matrix buffer as our own little playground to draw in, but that assumes they're
    // both 24-bits RGB triplets.  Or at least the same size!

    static_assert(sizeof(CRGB) == sizeof(LEDMatrixGFX::SM_RGB), "Code assumes 24 bits in both places");

    EVERY_N_MILLIS(MILLIS_PER_FRAME)
    {

        #if SHOW_FPS_ON_MATRIX
            LEDMatrixGFX::backgroundLayer.setFont(gohufont11);
            // 3 is half char width at curret font size, 5 is half the height.
            string output = "FPS: " + std::to_string(g_FPS);
            LEDMatrixGFX::backgroundLayer.drawString(MATRIX_WIDTH / 2 - (3 * output.length()), MATRIX_HEIGHT / 2 - 5, rgb24(255, 255, 255), rgb24(0, 0, 0), output.c_str());
        #endif

        auto graphics = (*g_ptrEffectManager)[0];

        LEDMatrixGFX::matrix.setCalcRefreshRateDivider(MATRIX_CALC_DIVIDER);
        LEDMatrixGFX::matrix.setRefreshRate(MATRIX_REFRESH_RATE);

        auto pMatrix = std::static_pointer_cast<LEDMatrixGFX>(graphics);
        pMatrix->setLeds(LEDMatrixGFX::GetMatrixBackBuffer());

        // We set ouutselves to the lower of the fader value or the brightness value, 
        // so that we can fade between effects without having to change the brightness
        // setting. 

        pMatrix->SetBrightness(min(g_Brightness, g_Fader));

        if (g_ptrEffectManager->GetCurrentEffect()->ShouldShowTitle() && pMatrix->GetCaptionTransparency() > 0.00)
        {
            LEDMatrixGFX::titleLayer.setFont(font3x5);
            uint8_t brite = (uint8_t)(pMatrix->GetCaptionTransparency() * 255.0);
            LEDMatrixGFX::titleLayer.setBrightness(brite); // 255 would obscure it entirely
            debugV("Caption: %d", brite);

            rgb24 chromaKeyColor = rgb24(255, 0, 255);
            rgb24 shadowColor = rgb24(0, 0, 0);
            rgb24 titleColor = rgb24(255, 255, 255);

            LEDMatrixGFX::titleLayer.setChromaKeyColor(chromaKeyColor);
            LEDMatrixGFX::titleLayer.enableChromaKey(true);
            LEDMatrixGFX::titleLayer.setFont(font6x10);
            LEDMatrixGFX::titleLayer.fillScreen(chromaKeyColor);

            const size_t kCharWidth = 6;
            const size_t kCharHeight = 10;

            const auto caption = pMatrix->GetCaption();

            int y = MATRIX_HEIGHT - 2 - kCharHeight;
            int w = caption.length() * kCharWidth;
            int x = (MATRIX_WIDTH / 2) - (w / 2) + 1;

            auto szCaption = caption.c_str();
            LEDMatrixGFX::titleLayer.drawString(x - 1, y, shadowColor, szCaption);
            LEDMatrixGFX::titleLayer.drawString(x + 1, y, shadowColor, szCaption);
            LEDMatrixGFX::titleLayer.drawString(x, y - 1, shadowColor, szCaption);
            LEDMatrixGFX::titleLayer.drawString(x, y + 1, shadowColor, szCaption);
            LEDMatrixGFX::titleLayer.drawString(x, y, titleColor, szCaption);
        }
        else
        {
            LEDMatrixGFX::titleLayer.enableChromaKey(false);
            LEDMatrixGFX::titleLayer.setBrightness(0);
        }
    }
#endif
}

// WiFiDraw
//
// Draws from WiFi color data if available, returns pixels drawn this frame

uint16_t WiFiDraw()
{
    std::lock_guard<std::mutex> guard(g_buffer_mutex);

    uint16_t pixelsDrawn = 0;
    for (int iChannel = 0; iChannel < NUM_CHANNELS; iChannel++)
    {

        timeval tv;
        gettimeofday(&tv, nullptr);

        // Pull buffers out of the queue.

        if (false == g_aptrBufferManager[iChannel]->IsEmpty())
        {
            std::shared_ptr<LEDBuffer> pBuffer;
            if (NTPTimeClient::HasClockBeenSet() == false)
            {
                pBuffer = g_aptrBufferManager[iChannel]->GetOldestBuffer();
            }
            else
            {
                // Using a 'while' rather than an 'if' causes it to pull frames until it's caught up
                // written as 'while' it will pull frames until it gets one that is current.
                // Chew through ALL frames older than now, ignoring all but the last of them

                while (!g_aptrBufferManager[iChannel]->IsEmpty() && g_aptrBufferManager[iChannel]->PeekOldestBuffer()->IsBufferOlderThan(tv))
                    pBuffer = g_aptrBufferManager[iChannel]->GetOldestBuffer();
            }

            if (pBuffer)
            {
                g_usLastWifiDraw = micros();
                debugV("Calling LEDBuffer::Draw from wire with %d/%d pixels.", pixelsDrawn, NUM_LEDS);
                pBuffer->DrawBuffer();
                // In case we drew some pixels and then drew 0 due a failure, we want to return a positive
                // number of pixels drawn so the caller knows we did in fact render.
                pixelsDrawn += pBuffer->Length();
            }
        }
    }
    debugV("WifIDraw claims to have drawn %d pixels", pixelsDrawn);
    return pixelsDrawn;
}

// LocalDraw
//
// Draws from effects table rather than from WiFi data.  Returns the number of LEDs rendered.

uint16_t LocalDraw()
{
    if (nullptr == g_ptrEffectManager)
    {
        debugW("Drawing before g_pEffectManager is ready, so delaying...");
        delay(100);
        return 0;
    }
    else if (g_ptrEffectManager->EffectCount() > 0)
    {
        // If we've never drawn from wifi before, now would also be a good time to local draw
        if (g_usLastWifiDraw == 0 || (micros() - g_usLastWifiDraw > (TIME_BEFORE_LOCAL * MICROS_PER_SECOND)))
        {
            g_ptrEffectManager->Update(); // Draw the current built in effect

            #if SHOW_VU_METER
                static auto spectrum = std::static_pointer_cast<SpectrumAnalyzerEffect>(GetSpectrumAnalyzer(0));
                if (g_ptrEffectManager->IsVUVisible())
                    spectrum->DrawVUMeter(g_ptrEffectManager->g(), 0, g_Analyzer.MicMode() == PeakData::PCREMOTE ? & vuPaletteBlue : &vuPaletteGreen);
            #endif

            debugV("LocalDraw claims to have drawn %d pixels", NUM_LEDS);
            return NUM_LEDS;
        }
        else
        {
            debugV("Not drawing local effect because last wifi draw was %lf seconds ago.", (micros() - g_usLastWifiDraw) / (float)MICROS_PER_SECOND);
            // It's important to return 0 when you do not draw so that the caller knows we did not
            // render any pixels, and we can/should wait until the next frame.  Otherwise, the caller might
            // draw the strip needlessly, which can take significant time.
            return 0;
        }
    }
    debugV("Local draw not drawing");
    return 0;
}

// ShowStrip
//
// ShowStrip sends the data to the LED strip.  If it's fewer than the size of the strip, we only send that many.

void ShowStrip(uint16_t numToShow)
{
    // If we've drawn anything from either source, we can now show it

    if (FastLED.count() == 0)
    {
        debugW("Draw loop is drawing before LEDs are ready, so delaying 100ms...");
        delay(100);
    }
    else
    {
        if (numToShow > 0)
        {
            debugV("Telling FastLED that we'll be drawing %d pixels\n", numToShow);

            for (int i = 0; i < NUM_CHANNELS; i++)
                FastLED[i].setLeds((*g_ptrEffectManager)[i]->leds, numToShow);

            FastLED.show(g_Fader);

            g_FPS = FastLED.getFPS();
            g_Brite = 100.0 * calculate_max_brightness_for_power_mW(g_Brightness, POWER_LIMIT_MW) / 255;
            g_Watts = calculate_unscaled_power_mW((*g_ptrEffectManager)[0]->leds, numToShow) / 1000; // 1000 for mw->W
        }
        else
        {
            debugV("Draw loop ended without a draw.");
        }
    }
}

// CalcDelayUntilNextFrame
//
// Returns the amount of time to wait patiently until it's time to draw the next frame, up to one second max

int CalcDelayUntilNextFrame(double frameStartTime, uint16_t localPixelsDrawn, uint16_t wifiPixelsDrawn)
{
    // Delay enough to slow down to the desired framerate

#if MILLIS_PER_FRAME == 0

    if (localPixelsDrawn > 0)
    {
        const double minimumFrameTime = 1.0 / g_ptrEffectManager->GetCurrentEffect()->DesiredFramesPerSecond();
        double elapsed = g_AppTime.CurrentTime() - frameStartTime;
        if (elapsed < minimumFrameTime)
        {
            g_FreeDrawTime = std::clamp(minimumFrameTime - elapsed, 0.0, 1.0);
            return (g_FreeDrawTime * MILLIS_PER_SECOND);
        }
    }
    else if (wifiPixelsDrawn > 0)
    {
        // Sleep up to 1/25th second, depending on how far away the next frame we need to service is

        double t = 0.04;
        for (int iChannel = 0; iChannel < NUM_CHANNELS; iChannel++)
        {
            auto pOldest = g_aptrBufferManager[iChannel]->PeekOldestBuffer();
            if (pOldest)
                t = std::min(t, (pOldest->Seconds() + pOldest->MicroSeconds() / (double) MICROS_PER_SECOND) - g_AppTime.CurrentTime());
        }

        g_FreeDrawTime = t;
        if (g_FreeDrawTime > 0.0)
            return (g_FreeDrawTime * MILLIS_PER_SECOND);
        else
            g_FreeDrawTime = 0.0;
    }
    else
    {
        debugV("Nothing drawn this pass because neither wifi nor local rendered a frame");
        // Nothing drawn this pass - check back soon
        g_FreeDrawTime = .001;
    }

    return 1;
#endif
}

// ShowOnboardLED
//
// If the board has an onboard LED, this will update it to show some activity from the draw

void ShowOnboardRGBLED()
{
    // Some boards have onboard PWM RGB LEDs, so if defined, we color them here.  If we're doing audio,
    // the color maps to the sound level.  If no audio, it shows the middle LED color from the strip.

    #if ONBOARD_LED_R
        #if ENABLE_AUDIO
            CRGB c = ColorFromPalette(HeatColors_p, g_Analyzer._VURatioFade / 2.0 * 255);
            ledcWrite(1, 255 - c.r); // write red component to channel 1, etc.
            ledcWrite(2, 255 - c.g);
            ledcWrite(3, 255 - c.b);
        #else
            int iLed = NUM_LEDS / 2;
            ledcWrite(1, 255 - graphics->leds[iLed].r); // write red component to channel 1, etc.
            ledcWrite(2, 255 - graphics->leds[iLed].g);
            ledcWrite(3, 255 - graphics->leds[iLed].b);
        #endif
    #endif
}

// PrepareOnboardPixel
//
// Do any setup required for the onboard pixel, if we have one

void PrepareOnboardPixel()
{
#ifdef ONBOARD_PIXEL_POWER
    g_ledSinglePixel = &FastLED.addLeds<WS2812B, ONBOARD_PIXEL_DATA, ONBOARD_PIXEL_ORDER>(&g_SinglePixel, 1);
    pinMode(ONBOARD_PIXEL_POWER, OUTPUT);
    digitalWrite(ONBOARD_PIXEL_POWER, HIGH);
#endif
}

void ShowOnboardPixel()
{
    // Some boards have onboard PWM RGB LEDs, so if defined, we color them here.  If we're doing audio,
    // the color maps to the sound level.  If no audio, it shows the middle LED color from the strip.

#ifdef ONBOARD_PIXEL_POWER
    g_SinglePixel = FastLED[0].leds()[0];
#endif
}

// DrawLoopTaskEntry
//
// Main draw loop entry point

void IRAM_ATTR DrawLoopTaskEntry(void *)
{

    //debugW(">> DrawLoopTaskEntry\n");

    // Initialize our graphics and the first effect

    PrepareOnboardPixel();

#if USE_MATRIX
    // We don't need color correction on the title layer, but we want it on the main background

    LEDMatrixGFX::titleLayer.enableColorCorrection(false);
    LEDMatrixGFX::backgroundLayer.enableColorCorrection(true);

    // Starting the effect might need to draw, so we need to set the leds up before doing so
    auto pMatrix = std::static_pointer_cast<LEDMatrixGFX>(g_ptrEffectManager->g());
    pMatrix->setLeds(LEDMatrixGFX::GetMatrixBackBuffer());
    auto spectrum = GetSpectrumAnalyzer(0);
#endif
    g_ptrEffectManager->StartEffect();

    // Run the draw loop

    debugW("Entering main draw loop!");

    for (;;)
    {
        g_AppTime.NewFrame();
        // Loop through each of the channels and see if they have a current frame that needs to be drawn

        uint16_t localPixelsDrawn   = 0;
        uint16_t wifiPixelsDrawn    = 0;
        double frameStartTime       = g_AppTime.FrameStartTime();

        #if USE_MATRIX
            MatrixPreDraw();
        #endif

        if (WiFi.isConnected())
            wifiPixelsDrawn = WiFiDraw();

        // If we didn't draw now, and it's been a while since we did, and we have at least one local effect, then draw the local effect instead

        if (wifiPixelsDrawn == 0)
            localPixelsDrawn = LocalDraw();

        #if USE_MATRIX
            if (wifiPixelsDrawn + localPixelsDrawn > 0)
            {
                LEDMatrixGFX::MatrixSwapBuffers(g_ptrEffectManager->GetCurrentEffect()->RequiresDoubleBuffering(), pMatrix->GetCaptionTransparency() > 0);
                FastLED.countFPS();
                g_FPS = FastLED.getFPS();
            }
        #endif
        #if USESTRIP
            if (wifiPixelsDrawn)
                ShowStrip(wifiPixelsDrawn);
            else if (localPixelsDrawn)
                ShowStrip(localPixelsDrawn);
        #endif

        if (wifiPixelsDrawn + localPixelsDrawn > 0)
            g_ptrEffectManager->SetNewFrameAvailable(true);

        // If the module has onboard LEDs, we support a couple of different types, and we set it to be the same as whatever
        // is on LED #0 of Channel #0.

        ShowOnboardPixel();
        ShowOnboardRGBLED();

        // Delay at least 1ms and not more than 1s until next frame is due
        
        delay( CalcDelayUntilNextFrame(frameStartTime, localPixelsDrawn, wifiPixelsDrawn) );

        // Once an OTA flash update has started, we don't want to hog the CPU or it goes quite slowly,
        // so we'll slow down to share the CPU a bit once the update has begun

        if (g_bUpdateStarted)
            delay(100);
    }
}
