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
//
//---------------------------------------------------------------------------

#include "globals.h"
#include "effects/effectmanager.h"
#include "ledbuffer.h"
#include "ntptimeclient.h"
#include "remotecontrol.h"
#include <mutex>
#include <ArduinoOTA.h>             // Over-the-air helper object so we can be flashed via WiFi
#include "ntptimeclient.h"

// The g_buffer_mutex is a global mutex used to protect access while adding or removing frames
// from the led buffer.  

extern std::mutex         g_buffer_mutex;

extern DRAM_ATTR unique_ptr<LEDBufferManager> g_apBufferManager[NUM_CHANNELS];
extern DRAM_ATTR unique_ptr<LEDMatrixGFX []>  g_aStrands;
extern uint32_t           g_FPS;
extern AppTime            g_AppTime;
extern unique_ptr<EffectManager> g_pEffectManager;
extern bool               g_bUpdateStarted;


void ShowTM1814();

// DrawPlaceholderDisplay
//
// When there's nothing to draw, fill the LEDs with colored dots so we know it's alive

void DrawPlaceholderDisplay()
{
    for (int iChannel = 0; iChannel < NUM_CHANNELS; iChannel++)
    {
        // Each channel is set to a different color, so red, orange, yellow, etc.

        CRGB color;
        if (iChannel == 0)
            color = CRGB::White;
        else
            color = color.setHue((iChannel - 1) * 36);                      

        g_aStrands[iChannel].fillScreen(CRGB::Black);

        static float iOffset = 0.0f;

        iOffset += 0.004f;
        if (iOffset >= 12)
            iOffset -= 12;

        // If we're a lamp, we don't want the colorful init, just go white.  The lamp
        // has few enough pixels we can light them all as well

        // bugbug Clean up this #if code

#if ATOMLIGHT
        color = CRGB::White;
        const int step  = 1;
#elif LEDSTRIP
        const int step = 12;            // On LED strips, we light every 12th

        // For the LED strips we set the color to something that shows the status of the WiFi and the clock.
        // Note that because the DrawLoop starts in parallel early on, you may not see any/all of these.

        if (WiFi.isConnected())
        {
            if (NTPTimeClient::HasClockBeenSet())
            {
                color = CRGB::Green;   // Green is ready to go
            }
            else
            {
                color = CRGB::Blue;   // Blue is WiFi connected but no clock (normal now that clock is streamed)
            }
        }
        else
        {
            color = CRGB::Red;          // No Wifi
        }

#elif STRAND 
        const int step = 3;             // Bulbs aren't very dense on a strand, so every third will do
#elif FANSET
        const int step = 2;             // Bulbs aren't very dense on a strand, so every third will do
#elif BROOKLYNROOM
        const int step = 3;             // BUGBUG move this to the #define section
#elif ATOMISTRING
        const int step = 1;
#else
        const int step = 1;
#endif
        for (float i = iOffset; i < g_aStrands[iChannel].GetLEDCount(); i += step)
            g_aStrands[iChannel].drawPixel(i, color);
    }
#if ATOMISTRING
    ShowTM1814();
#else
    FastLED.setBrightness(255);
    FastLED.show();
#endif
}

// DrawLoop
//
// Pull packets from the Wifi buffer if they've come due and draw them - if it's a few seconds without a WiFi frame,
// we will draw the local effect instead

DRAM_ATTR uint64_t g_msLastWifiDraw  = 0;
DRAM_ATTR double   g_BufferAgeOldest = 0;
DRAM_ATTR double   g_BufferAgeNewest = 0;

DRAM_ATTR byte     g_Brightness      = 255;
DRAM_ATTR byte     g_Fader           = 255;

// DrawLoopTaskEntry
// 
// Starting point for the draw code loop

void IRAM_ATTR DrawLoopTaskEntry(void *)
{
   
    debugI(">> DrawLoopTaskEntry\n");
    debugE("Entry Heap: %s", heap_caps_check_integrity_all(true) ? "PASS" : "FAIL");

    for (;;)
    {
        // Loop through each of the channels and see if they have a current frame that needs to be drawn
        
        uint cPixelsDrawnThisFrame = 0;
 
        if (WiFi.isConnected())
        {
            timeval tv;
            gettimeofday(&tv, nullptr);
            double dClockTime = tv.tv_sec + tv.tv_usec / (double) MICROS_PER_SECOND;
            
            std::lock_guard<std::mutex> guard(g_buffer_mutex);

            for (int iChannel = 0; iChannel < NUM_CHANNELS; iChannel++)
            {
                // Pull buffers out of the queue.  Changint the 'while' to an 'if' would cause it to draw every frame if it got behind, but when
                // written as 'while' it will pull frames until it gets one that is current.

                if (false == g_apBufferManager[iChannel]->IsEmpty())
                {
                    shared_ptr<LEDBuffer> pBuffer;
                    if (NTPTimeClient::HasClockBeenSet() == false)
                    {
                        pBuffer = g_apBufferManager[iChannel]->GetOldestBuffer();
                    }
                    else
                    {
                        // Chew through ALL frames older than now, ignoring them
                        while (!g_apBufferManager[iChannel]->IsEmpty() && g_apBufferManager[iChannel]->PeekOldestBuffer()->IsBufferOlderThan(tv))
                            pBuffer = g_apBufferManager[iChannel]->GetOldestBuffer();
                    }
                
                    if (pBuffer)
                    {
                        g_AppTime.NewFrame();
                        g_msLastWifiDraw = micros();  
                        cPixelsDrawnThisFrame = pBuffer->Length();
                        debugV("Calling LEDBuffer::Draw from wire with %d/%d pixels.", cPixelsDrawnThisFrame, NUM_LEDS);
                        pBuffer->DrawBuffer();
                    }
                }
                                
                if (false == g_apBufferManager[iChannel]->IsEmpty())
                {
                    auto pOldest = g_apBufferManager[iChannel]->PeekOldestBuffer();
                    auto pNewest = g_apBufferManager[iChannel]->PeekNewestBuffer();                    
                    g_BufferAgeNewest = (pNewest->Seconds() + pNewest->MicroSeconds() / (double) MICROS_PER_SECOND) - dClockTime;
                    g_BufferAgeOldest = -(pOldest->Seconds() + pOldest->MicroSeconds() / (double) MICROS_PER_SECOND) - dClockTime;
                }
                else
                {
                    g_BufferAgeNewest = g_BufferAgeOldest = 0;
                }
            }          
        }
        else
        {
            debugV("Not connected to WiFi so not checking for WiFi draw.");
        }

        // If we didn't draw now, and it's been a while since we did, and we have at least one local effect, then draw the local effect instead
        
        if (cPixelsDrawnThisFrame == 0)
        {
            if (nullptr == g_pEffectManager)
            {
                debugW("Drawing before g_pEffectManager is ready, so delaying...");
                delay(100);
            }
            else if (g_pEffectManager->EffectCount() > 0)
            {
                // If we've never drawn from wifi before, now would also be a good time to local draw
                if (g_msLastWifiDraw == 0 || (micros() - g_msLastWifiDraw > (TIME_BEFORE_LOCAL * MICROS_PER_SECOND)))  
                {
                    g_AppTime.NewFrame();       // Start a new frame, record the time, calc deltaTime, etc.
                    debugV("Calling EffectManager::Update to draw the built-in effect for %d pixels.", NUM_LEDS);
                    g_pEffectManager->Update(); // Draw the current built in effect
                    cPixelsDrawnThisFrame = NUM_LEDS;
                    debugV("Back from EffectManager::Update");

                }
                else
                {
                    debugV("Not drawing local effect because last wifi draw was %lf seconds ago.", (micros()-g_msLastWifiDraw) / (double) MICROS_PER_SECOND);
                }
            }
        }
        else
        {
            debugV("Already drew from WiFi so not drawing locally this frame.");
        }


        // If we've drawn anything from either source, we can now show it

        if (FastLED.count() == 0)
        {
            debugW("Draw loop is drawing before LEDs are ready, so delaying 100ms...");
            delay(100);
        }
        else
        {
            if (cPixelsDrawnThisFrame > 0)
            {
                debugV("Telling FastLED that we'll be drawing %d pixels\n", cPixelsDrawnThisFrame);
                for (int i  = 0; i < NUM_CHANNELS; i++)
                    FastLED[i].setLeds((*g_pEffectManager)[i]->GetLEDBuffer(), cPixelsDrawnThisFrame);

                //vTaskPrioritySet(g_taskDraw, DRAWING_PRIORITY_BOOST);
                #if ATOMISTRING
                    ShowTM1814();
                #else
                    int brite = calculate_max_brightness_for_power_mW(g_Brightness, POWER_LIMIT_MW);
                    debugV("Calling FastLED::show for %d/%d total at brightness of %d/255 on strip at %d FPS", 
                        cPixelsDrawnThisFrame, NUM_LEDS, brite, FastLED.getFPS());
                    FastLED.show((brite * g_Fader) / 256);
                    debugV("Back from FastLED::show");
                #endif

                g_FPS = FastLED.getFPS(); //     1.0/elapsed;    
                
                // If we draw, we delay so that anything else on our core, like the TFT, can get more CPU and update.

                delay(1);        
            }
            else
            {
                debugV("Draw loop ended without a draw.");
            }
        }

        // Once an OTA flash update has started, we don't want to hog the CPU or it goes quite slowly, 
        // so we'll pause to share the CPU a bit once the update has begun

        if (g_bUpdateStarted)
            delay(100);
        
        // If we didn't draw anything, we busy-wait so that we are continually checking the clock for an packet
        // whose time has come

        delay(1);
    }
}