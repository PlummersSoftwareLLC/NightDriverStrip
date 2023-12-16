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
#include "systemcontainer.h"

static DRAM_ATTR CRGB l_SinglePixel = CRGB::Blue;
static DRAM_ATTR uint64_t l_usLastWifiDraw = 0;

// The g_buffer_mutex is a global mutex used to protect access while adding or removing frames
// from the led buffer.

extern DRAM_ATTR std::mutex g_buffer_mutex;

extern const CRGBPalette16 vuPaletteGreen;

std::shared_ptr<LEDStripEffect> GetSpectrumAnalyzer(CRGB color);    // Defined in effectmanager.cpp

// WiFiDraw
//
// Draws from WiFi color data if available, returns pixels drawn this frame

uint16_t WiFiDraw()
{
    std::lock_guard<std::mutex> guard(g_buffer_mutex);

    uint16_t pixelsDrawn = 0;
    for (auto& bufferManager : g_ptrSystem->BufferManagers())
    {

        timeval tv;
        gettimeofday(&tv, nullptr);

        // Pull buffers out of the queue.

        if (false == bufferManager.IsEmpty())
        {
            std::shared_ptr<LEDBuffer> pBuffer;
            if (NTPTimeClient::HasClockBeenSet() == false)
            {
                pBuffer = bufferManager.GetOldestBuffer();
            }
            else
            {
                // Using a 'while' rather than an 'if' causes it to pull frames until it's caught up
                // written as 'while' it will pull frames until it gets one that is current.
                // Chew through ALL frames older than now, ignoring all but the last of them

                while (!bufferManager.IsEmpty() && bufferManager.PeekOldestBuffer()->IsBufferOlderThan(tv))
                    pBuffer = bufferManager.GetOldestBuffer();
            }

            if (pBuffer)
            {
                l_usLastWifiDraw = micros();
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
    if (!g_ptrSystem->HasEffectManager())
    {
        debugW("Drawing before EffectManager is ready, so delaying...");
        delay(100);
        return 0;
    }
    else
    {
        auto& effectManager = g_ptrSystem->EffectManager();

        if (effectManager.EffectCount() > 0)
        {
            // If we've never drawn from wifi before, now would also be a good time to local draw
            if (l_usLastWifiDraw == 0 || (micros() - l_usLastWifiDraw > (TIME_BEFORE_LOCAL * MICROS_PER_SECOND)))
            {
                effectManager.Update(); // Draw the current built in effect

                #if SHOW_VU_METER
                    static auto spectrum = std::static_pointer_cast<SpectrumAnalyzerEffect>(GetSpectrumAnalyzer(0));
                    if (effectManager.IsVUVisible())
                        spectrum->DrawVUMeter(effectManager.g(), 0, g_Analyzer.MicMode() == PeakData::PCREMOTE ? & vuPaletteBlue : &vuPaletteGreen);
                #endif

                debugV("LocalDraw claims to have drawn %d pixels", NUM_LEDS);
                return NUM_LEDS;
            }
            else
            {
                debugV("Not drawing local effect because last wifi draw was %lf seconds ago.", (micros() - l_usLastWifiDraw) / (float)MICROS_PER_SECOND);
                // It's important to return 0 when you do not draw so that the caller knows we did not
                // render any pixels, and we can/should wait until the next frame.  Otherwise the caller might
                // draw the strip needlessly, which can take significant time.
                return 0;
            }
        }
    }

    debugV("Local draw not drawing");
    return 0;
}

// CalcDelayUntilNextFrame
//
// Returns the amount of time to wait patiently until it's time to draw the next frame, up to one second max

int CalcDelayUntilNextFrame(double frameStartTime, uint16_t localPixelsDrawn, uint16_t wifiPixelsDrawn)
{
    constexpr auto kMinDelay = 0.001;

    // Delay enough to slow down to the desired framerate

#if MILLIS_PER_FRAME == 0

    if (localPixelsDrawn > 0)
    {
        const double minimumFrameTime = 1.0 / g_ptrSystem->EffectManager().GetCurrentEffect().DesiredFramesPerSecond();
        double elapsed = g_Values.AppTime.CurrentTime() - frameStartTime;
        if (elapsed < minimumFrameTime)
            g_Values.FreeDrawTime = std::clamp(minimumFrameTime - elapsed, 0.0, 1.0);
    }
    else if (wifiPixelsDrawn > 0)
    {
        // Look through all the channels to see how far away the next wifi frame is times for.  We can then delay
        // up to the minimum value found across all buffer managers.

        double t = std::numeric_limits<double>::max();
        bool bFoundFrame = false;

        for (auto& bufferManager : g_ptrSystem->BufferManagers())
        {
            auto pOldest = bufferManager.PeekOldestBuffer();
            if (pOldest)
            {
                t = std::min(t, pOldest->TimeTillDue());
                bFoundFrame = true;
            }
        }
        g_Values.FreeDrawTime = bFoundFrame ? std::clamp(t, 0.0, 1.0) : kMinDelay;
    }
    else
    {
        debugV("Nothing drawn this pass because neither wifi nor local rendered a frame");
        // Nothing drawn this pass - check back soon
        g_Values.FreeDrawTime = kMinDelay;
    }

    return g_Values.FreeDrawTime * MILLIS_PER_SECOND;
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
        FastLED.addLeds<WS2812B, ONBOARD_PIXEL_DATA, ONBOARD_PIXEL_ORDER>(&l_SinglePixel, 1);
        pinMode(ONBOARD_PIXEL_POWER, OUTPUT);
        digitalWrite(ONBOARD_PIXEL_POWER, HIGH);
    #endif
}

void ShowOnboardPixel()
{
    // Some boards have onboard PWM RGB LEDs, so if defined, we color them here.  If we're doing audio,
    // the color maps to the sound level.  If no audio, it shows the middle LED color from the strip.

    #ifdef ONBOARD_PIXEL_POWER
        l_SinglePixel = FastLED[0].leds()[0];
    #endif
}

// DrawLoopTaskEntry
//
// Main draw loop entry point

void IRAM_ATTR DrawLoopTaskEntry(void *)
{
    debugW(">> DrawLoopTaskEntry\n");

    // If this board has an onboard RGB pixel, set it up now

    PrepareOnboardPixel();

    // Start the effect

    g_ptrSystem->EffectManager().StartEffect();

    // Run the draw loop

    debugW("Entering main draw loop!");

    for (;;)
    {
        g_Values.AppTime.NewFrame();

        uint16_t localPixelsDrawn   = 0;
        uint16_t wifiPixelsDrawn    = 0;
        double frameStartTime       = g_Values.AppTime.FrameStartTime();

        auto graphics = g_ptrSystem->EffectManager().GetBaseGraphics();

        graphics->PrepareFrame();

        if (WiFi.isConnected())
            wifiPixelsDrawn = WiFiDraw();

        // If we didn't draw now, and it's been a while since we did, and we have at least one local effect, then draw the local effect instead

        if (wifiPixelsDrawn == 0)
            localPixelsDrawn = LocalDraw();

        // If we drew any pixels by any method, we'll call that a frame and track it for FPS purposes.  We also notify the
        // color data thread that a new frame is available and can be transmitted to clients

        if (wifiPixelsDrawn + localPixelsDrawn > 0)
        {
            // If the module has onboard LEDs, we support a couple of different types, and we set it to be the same as whatever
            // is on LED #0 of Channel #0.

            ShowOnboardPixel();
            ShowOnboardRGBLED();

            g_Values.FPS = FastLED.getFPS();
            g_ptrSystem->EffectManager().SetNewFrameAvailable(true);
        }

        graphics->PostProcessFrame(localPixelsDrawn, wifiPixelsDrawn);

        // Delay at least 2ms and not more than 1s until next frame is due

        constexpr auto minimumDelay = 5;
        delay( std::max(minimumDelay, CalcDelayUntilNextFrame(frameStartTime, localPixelsDrawn, wifiPixelsDrawn) ));

        // Once an OTA flash update has started, we don't want to hog the CPU or it goes quite slowly,
        // so we'll slow down to share the CPU a bit once the update has begun

        if (g_Values.UpdateStarted)
            delay(500);
    }
}
