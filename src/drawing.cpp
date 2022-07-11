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
#include "effectmanager.h"
#include "ledbuffer.h"
#include "ntptimeclient.h"
#include "remotecontrol.h"
#include <mutex>
#include <ArduinoOTA.h>             // Over-the-air helper object so we can be flashed via WiFi
#include "ntptimeclient.h"
#include "effects/matrix/spectrumeffects.h"

#ifdef USESTRIP
#include "ledstripgfx.h"
#endif

#ifdef USEMATRIX
#include "ledmatrixgfx.h"
#endif

// The g_buffer_mutex is a global mutex used to protect access while adding or removing frames
// from the led buffer.  

extern std::mutex         g_buffer_mutex;

DRAM_ATTR std::unique_ptr<LEDBufferManager> g_apBufferManager[NUM_CHANNELS];
DRAM_ATTR std::unique_ptr<EffectManager<GFXBase>> g_pEffectManager;

extern uint32_t           g_FPS;
extern AppTime            g_AppTime;
extern bool               g_bUpdateStarted;
extern double             g_Brite;
extern uint32_t           g_Watts; 
extern CRGBPalette256     vuPaletteGreen;

void ShowTM1814();

// DrawLoop
//
// Pull packets from the Wifi buffer if they've come due and draw them - if it's a few seconds without a WiFi frame,
// we will draw the local effect instead

DRAM_ATTR uint64_t g_msLastWifiDraw  = 0;
DRAM_ATTR double   g_BufferAgeOldest = 0;
DRAM_ATTR double   g_BufferAgeNewest = 0;

DRAM_ATTR uint8_t  g_Brightness      = 255;
DRAM_ATTR uint8_t  g_Fader           = 255;

// DrawLoopTaskEntry
// 
// Starting point for the draw code loop

void IRAM_ATTR DrawLoopTaskEntry(void *)
{
    CRGBPalette256 greenPalette(vu_gpGreen);
    
    debugI(">> DrawLoopTaskEntry\n");

    // Initialize our graphics and the first effect

    GFXBase * graphics = (GFXBase *)(*g_pEffectManager)[0].get();
    graphics->Setup();

    #if USEMATRIX
        // We don't need color correction on the chromakey'd title layer
        LEDMatrixGFX::titleLayer.enableColorCorrection(false);

        // Starting the effect might need to draw, so we need to set the leds up before doing so
        LEDMatrixGFX * pMatrix = (LEDMatrixGFX *)(*g_pEffectManager)[0].get();
        pMatrix->setLeds(LEDMatrixGFX::GetMatrixBackBuffer());
        auto spectrum = GetSpectrumAnalyzer(0);
    #endif
    g_pEffectManager->StartEffect();
    
    // Run the draw loop

    for (;;)
    {
        static uint64_t lastFrame = millis();
        g_FPS = FPS(lastFrame, millis());
        lastFrame = millis();        
        
        // Loop through each of the channels and see if they have a current frame that needs to be drawn
        
        uint cPixelsDrawnThisFrame = 0;
 
        #if USEMATRIX
            // We treat the internal matrix buffer as our own little playground to draw in, but that assumes they're
            // both 24-bits RGB triplets.  Or at least the same size!

            static_assert( sizeof(CRGB) == sizeof(LEDMatrixGFX::SM_RGB), "Code assumes 24 bits in both places" );

            LEDMatrixGFX::MatrixSwapBuffers();
            LEDMatrixGFX * pMatrix = (LEDMatrixGFX *)(*g_pEffectManager)[0].get();
            pMatrix->setLeds(LEDMatrixGFX::GetMatrixBackBuffer());

            LEDMatrixGFX::titleLayer.setFont(font3x5);
            if (pMatrix->GetCaptionTransparency() > 0) 
            {
                rgb24 chromaKeyColor = rgb24(255,0,255);
                rgb24 shadowColor = rgb24(0,0,0);
                rgb24 titleColor = rgb24(255,255,255);
                
                LEDMatrixGFX::titleLayer.setChromaKeyColor(chromaKeyColor);
                LEDMatrixGFX::titleLayer.enableChromaKey(true);

                LEDMatrixGFX::titleLayer.setFont(font5x7);
                LEDMatrixGFX::titleLayer.fillScreen(chromaKeyColor);

                const size_t kCharWidth = 5;
                const size_t kCharHeight = 7;

                int y = MATRIX_HEIGHT - 2 - kCharHeight;

                int w = strlen(pMatrix->GetCaption()) * kCharWidth;
                int x = (MATRIX_WIDTH / 2) - (w / 2); 

                LEDMatrixGFX::titleLayer.drawString(x-1, y,   shadowColor, pMatrix->GetCaption());
                LEDMatrixGFX::titleLayer.drawString(x+1, y,   shadowColor, pMatrix->GetCaption());
                LEDMatrixGFX::titleLayer.drawString(x,   y-1, shadowColor, pMatrix->GetCaption());
                LEDMatrixGFX::titleLayer.drawString(x,   y+1, shadowColor, pMatrix->GetCaption());
                LEDMatrixGFX::titleLayer.drawString(x,   y,   titleColor,  pMatrix->GetCaption());
                
                LEDMatrixGFX::titleLayer.setBrightness(pMatrix->GetCaptionTransparency() * 240);                // 255 would obscure it entirely
            }
            else 
            {
                LEDMatrixGFX::titleLayer.enableChromaKey(false);
                LEDMatrixGFX::titleLayer.setBrightness(0);
            }   
 
        #endif

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
                    std::shared_ptr<LEDBuffer> pBuffer;
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
                            g_BufferAgeOldest = (pOldest->Seconds() + pOldest->MicroSeconds() / (double) MICROS_PER_SECOND) - dClockTime;
                    debugV("Clock: %+04.2lf, Oldest: %+04.2lf, Newest: %+04.2lf", dClockTime, g_BufferAgeOldest, g_BufferAgeNewest);
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
                    g_pEffectManager->Update(); // Draw the current built in effect
                    cPixelsDrawnThisFrame = NUM_LEDS;

                    #if USEMATRIX
                        auto * pGraphics = (*g_pEffectManager)[0].get();
                        if (g_pEffectManager->IsVUVisible())
                            ((SpectrumAnalyzerEffect *)spectrum.get())->DrawVUMeter(pGraphics, 0, &vuPaletteGreen);
                    #endif

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

#if USESTRIP
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
                {
                    LEDStripGFX * pStrip = (LEDStripGFX *)(*g_pEffectManager)[i].get();
                    FastLED[i].setLeds(pStrip->GetLEDBuffer(), cPixelsDrawnThisFrame);
                }

                //vTaskPrioritySet(g_taskDraw, DRAWING_PRIORITY_BOOST);
                int brite = calculate_max_brightness_for_power_mW(g_Brightness, POWER_LIMIT_MW);
                debugV("Calling FastLED::show for %d/%d total at brightness of %d/255 on strip at %d FPS", 
                    cPixelsDrawnThisFrame, NUM_LEDS, brite, FastLED.getFPS());
                FastLED.show((brite * g_Fader) / 256);
                debugV("Back from FastLED::show");

                g_FPS = FastLED.getFPS(); //     1.0/elapsed;    
                g_Brite = 255.0 * 100.0 / calculate_max_brightness_for_power_mW(g_Brightness, POWER_LIMIT_MW);
                g_Watts = calculate_unscaled_power_mW( ((LEDStripGFX *)(*g_pEffectManager)[0].get())->GetLEDBuffer(), cPixelsDrawnThisFrame )/ 1000;

                
                // If we draw, we delay at least a bit so that anything else on our core, like the TFT, can get more CPU and update.

                delay(1);        
            }
            else
            {
                debugV("Draw loop ended without a draw.");
            }
        }

        #ifdef ONBOARD_LED_R
            ledcWrite(1, graphics->leds[0].r); // write red component to channel 1, etc.
            ledcWrite(2, graphics->leds[0].g);
            ledcWrite(3, graphics->leds[0].b);
            ledcWrite(4, (graphics->leds[0].r + graphics->leds[0].g + graphics->leds[0].b) / 3);
        #endif

#endif

#if USEMATRIX
            LEDMatrixGFX::PresentFrame();
#endif

        // Once an OTA flash update has started, we don't want to hog the CPU or it goes quite slowly, 
        // so we'll pause to share the CPU a bit once the update has begun

        if (g_bUpdateStarted)
            delay(100);
        
        // If we didn't draw anything, we near-busy-wait so that we are continually checking the clock for an packet
        // whose time has come

        delay(1);
    }
}