//+--------------------------------------------------------------------------
//
// File:        network.cpp
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
//    Network loop, remote contol, debug loop, etc.
//
// History:     May-11-2021         Davepl      Commented
//
//---------------------------------------------------------------------------

#include "globals.h"
#include "network.h"
#include "ledbuffer.h"
#include "spiffswebserver.h"
#include <mutex>
#include <ArduinoOTA.h>             // Over-the-air helper object so we can be flashed via WiFi
#include <WiFiClientSecure.h>
#include <HTTPClient.h>

#if USE_WIFI_MANAGER
#include <ESP_WiFiManager.h>
DRAM_ATTR ESP_WiFiManager g_WifiManager("NightDriverWiFi");
#endif

extern DRAM_ATTR std::unique_ptr<LEDBufferManager> g_apBufferManager[NUM_CHANNELS];
extern DRAM_ATTR CSPIFFSWebServer g_WebServer;

std::mutex g_buffer_mutex;

// processRemoteDebugCmd() 
//
// This is where we can add our own custom debugger commands

extern AppTime  g_AppTime;
extern double   g_BufferAgeOldest;
extern double   g_BufferAgeNewest;
extern uint32_t g_FPS;

extern volatile float gVURatioFade;
extern volatile float gVURatio;       // Current VU as a ratio to its recent min and max
extern volatile float gVU;            // Instantaneous read of VU value
extern volatile float gPeakVU;        // How high our peak VU scale is in live mode
extern volatile float gMinVU;   

// processRemoteDebugCmd
// 
// Callback function that the debug library (which exposes a little console over telnet and serial) calls
// in order to allow us to add custom commands.  I've added a clock reset and stats command, for example.

#if ENABLE_WIFI
    void processRemoteDebugCmd() 
    {
        String str = Debug.getLastCommand();
        if (str.equalsIgnoreCase("clock"))
        {
            debugI("Refreshing Time from Server...");

            digitalWrite(BUILTIN_LED_PIN, 1);
            NTPTimeClient::UpdateClockFromWeb(&g_Udp);
            digitalWrite(BUILTIN_LED_PIN, 0);
        }
        else if (str.equalsIgnoreCase("stats"))
        {
            debugI("Displaying statistics....");

            char szBuffer[256];
            snprintf(szBuffer, ARRAYSIZE(szBuffer), "%s:%dx%d %dK\n", FLASH_VERSION_NAME, NUM_CHANNELS, NUM_LEDS, ESP.getFreeHeap() / 1024);
            debugI("%s", szBuffer);


            snprintf(szBuffer, ARRAYSIZE(szBuffer), "%sdB:%s\n", 
                                                    String(WiFi.RSSI()).substring(1).c_str(), 
                                                    WiFi.isConnected() ? WiFi.localIP().toString().c_str() : "None");
            debugI("%s", szBuffer);

            snprintf(szBuffer, ARRAYSIZE(szBuffer), "BUFR:%02d/%02d [%dfps]\n", g_apBufferManager[0]->Depth(), g_apBufferManager[0]->BufferCount(), g_FPS);
            debugI("%s", szBuffer);

            snprintf(szBuffer, ARRAYSIZE(szBuffer), "DATA:%+04.2lf-%+04.2lf\n", g_BufferAgeOldest, g_BufferAgeNewest);
            debugI("%s", szBuffer);

            snprintf(szBuffer, ARRAYSIZE(szBuffer), "CLCK:%.2lf\n", g_AppTime.CurrentTime());
            debugI("%s", szBuffer);

            #if ENABLE_AUDIO
                snprintf(szBuffer, ARRAYSIZE(szBuffer), "gVU: %.2f, gMinVU: %.2f, gPeakVU: %.2f, gVURatio: %.2f", gVU, gMinVU, gPeakVU, gVURatio);
                debugI("%s", szBuffer);
            #endif

            #if INCOMING_WIFI_ENABLED
            snprintf(szBuffer, ARRAYSIZE(szBuffer), "Socket Buffer _cbReceived: %d", g_SocketServer._cbReceived);
            debugI("%s", szBuffer);
            #endif

            // Print out a buffer log with timestamps and deltas 
            
            for (size_t i = 0; i < g_apBufferManager[0]->Depth(); i++)
            {
                auto pBufferManager = g_apBufferManager[0].get();
                std::shared_ptr<LEDBuffer> pBuffer = (*pBufferManager)[i];
                double t = pBuffer->Seconds() + (double) pBuffer->MicroSeconds() / MICROS_PER_SECOND;
                snprintf(szBuffer, ARRAYSIZE(szBuffer), "Frame: %03d, Clock: %lf, Offset: %lf", i, t, g_AppTime.CurrentTime() - t);
                debugI("%s", szBuffer);
            }

        }
    }
#endif

// RemoteLoopEntry
//
// If enabled, this is the main thread loop for the remote control.  It is intialized and then
// called once every 20ms to pump its work queue and scan for new remote codes, etc.  If no
// remote is being used, this code and thread doesn't exist in the build.

#if ENABLE_REMOTE
extern RemoteControl g_RemoteControl;

void IRAM_ATTR RemoteLoopEntry(void *)
{
    debugI(">> RemoteLoopEntry\n");

    g_RemoteControl.begin();
    while (true)
    {
        g_RemoteControl.handle();
        delay(50);        
    }
}
#endif

// ConnectToWiFi
//
// Connect to the pre-configured WiFi network.  

#if ENABLE_WIFI

    #define WIFI_RETRIES 5

    bool ConnectToWiFi(uint cRetries)
    {
        // Already connected, Go no further.
        if (WiFi.isConnected())
        {
            return true;
        }
        
        debugI("Setting host name to %s...", cszHostname);

    #if USE_WIFI_MANAGER
        g_WifiManager.setDebugOutput(true);
        g_WifiManager.autoConnect("NightDriverWiFi");
    #else
        for (uint iPass = 0; iPass < cRetries; iPass++)
        {
            Serial.printf("Pass %u of %u: Connecting to Wifi SSID: %s - ESP32 Free Memory: %u, PSRAM:%u, PSRAM Free: %u\n",
                iPass, cRetries, cszSSID, ESP.getFreeHeap(), ESP.getPsramSize(), ESP.getFreePsram());

            //WiFi.disconnect();
            WiFi.begin(cszSSID, cszPassword);

            for (uint i = 0; i < WIFI_RETRIES; i++)
            {
                if (WiFi.isConnected())
                {
                    Serial.printf("Connected to AP with BSSID: %s\n", WiFi.BSSIDstr().c_str());
                    break;
                }
                else
                {
                    delay(1000);
                }
            }

            if (WiFi.isConnected()) {
                break;
            } 
        }
    #endif
        // Additional Services onwwards reliant on network so close if not up.
        if (false == WiFi.isConnected())
        {
            debugW("Giving up on WiFi\n");
            return false;
        }
        debugW("Received IP: %s", WiFi.localIP().toString().c_str());

        #if INCOMING_WIFI_ENABLED
            // Start listening for incoming data
            debugI("Starting/restarting Socket Server...");
            g_SocketServer.release();
            if (false == g_SocketServer.begin())
                throw runtime_error("Could not start socket server!");

            debugI("Socket server started.");
        #endif

        #if ENABLE_OTA
            debugI("Publishing OTA...");
            SetupOTA(cszHostname);
        #endif

        #if ENABLE_NTP
            debugI("Setting Clock...");
            NTPTimeClient::UpdateClockFromWeb(&g_Udp);
        #endif

        #if ENABLE_WEBSERVER
            debugI("Starting Web Server...");
            g_WebServer.begin();
            debugI("Web Server begin called!");
        #endif

        #if USEMATRIX
            //LEDStripEffect::mgraphics()->SetCaption(WiFi.localIP().toString().c_str(), 3000);
        #endif

        /*
        {
            WiFiClientSecure secClient;

            secClient.setInsecure();

            Serial.println("\nStarting secure connection to server...");
            uint32_t start = millis();
            int r = secClient.connect("google.com", 443, 5000);
            Serial.printf("Connection took: %lums\n", millis()-start);
            if(!r) 
            {
                Serial.println("Connection failed!");
            } 
            else 
            {
                Serial.println("Connected!  Sending GET!");
                secClient.println("GET https://www.google.com/search?q=tsla+stock+quote HTTP/1.0");
                secClient.println("Host: www.google.com");
                secClient.println("Connection: close");
                secClient.println();
                
                while (secClient.connected()) 
                {
                    String line = secClient.readStringUntil('\n');
                    secClient.printf("Data: %s", line.c_str());
                    if (line == "\r") {
                        Serial.println("headers received");
                        break;
                    }
                }
            }
            secClient.stop();
        }
        */

        return true;
    }
    
#endif

// SetupOTA
//
// Set up the over-the-air programming info so that we can be flashed over WiFi

void SetupOTA(const char *pszHostname)
{
#if ENABLE_OTA
    ArduinoOTA.setRebootOnSuccess(true);

    if (nullptr == pszHostname)
        ArduinoOTA.setMdnsEnabled(false);
    else
        ArduinoOTA.setHostname(pszHostname);

    ArduinoOTA
        .onStart([]() {
            g_bUpdateStarted = true;

            String type;
            if (ArduinoOTA.getCommand() == U_FLASH)
                type = "sketch";
            else // U_SPIFFS
                type = "filesystem";

            // NOTE: if updating SPIFFS this would be the place to unmount SPIFFS using SPIFFS.end()
            debugI("Stopping SPIFFS");
            #if ENABLE_WEBSEVER
            SPIFFS.end();
            #endif
            
            debugI("Stopping IR remote");
            #if ENABLE_REMOTE            
            g_RemoteControl.end();
            #endif        
            
            debugI("Start updating from OTA ");
            debugI("%s", type.c_str());
        })
        .onEnd([]() {
            debugI("\nEnd OTA");
        })
        .onProgress([](unsigned int progress, unsigned int total) 
        {
            static uint last_time = millis();
            if (millis() - last_time > 1000)
            {
                last_time = millis();
                debugI("Progress: %u%%\r", (progress / (total / 100)));
            }
            else
            {
                debugV("Progress: %u%%\r", (progress / (total / 100)));
            }
            
        })
        .onError([](ota_error_t error) {
            debugW("Error[%u]: ", error);
            if (error == OTA_AUTH_ERROR)
            {
                debugW("Auth Failed");
            }
            else if (error == OTA_BEGIN_ERROR)
            {
                debugW("Begin Failed");
            }
            else if (error == OTA_CONNECT_ERROR)
            {
                debugW("Connect Failed");
            }
            else if (error == OTA_RECEIVE_ERROR)
            {
                debugW("Receive Failed");
            }
            else if (error == OTA_END_ERROR)
            {
                debugW("End Failed");
            }
            throw runtime_error("OTA Flash update failed.");
        });

    ArduinoOTA.begin();
#endif
}

// ProcessIncomingData
//
// Code that actually handles whatever comes in on the socket.  Must be known good data
// as this code does not validate!  This is where the commands and pixel data are received
// from the server.

bool ProcessIncomingData(uint8_t *payloadData, size_t payloadLength)
{
    #if !INCOMING_WIFI_ENABLED
        return false;
    #else

    uint16_t command16 = payloadData[1] << 8 | payloadData[0];

    debugV("payloadLength: %u, command16: %d", payloadLength, command16);

    // The very old original implementation used channel numbers, not a mask, and only channel 0 was supported at that time, so if
    // we see a Channel 0 asked for, it must be very old, and we massage it into the mask for Channel0 instead

    switch (command16)
    {
        case WIFI_COMMAND_PIXELDATA64:
        {
            uint16_t channel16 = WORDFromMemory(&payloadData[2]);
            uint32_t length32  = DWORDFromMemory(&payloadData[4]);
            uint64_t seconds   = ULONGFromMemory(&payloadData[8]);
            uint64_t micros    = ULONGFromMemory(&payloadData[16]);


            debugV("ProcessIncomingData -- Channel: %u, Length: %u, Seconds: %llu, Micros: %llu ... ", 
                   channel16, 
                   length32, 
                   seconds, 
                   micros);

            // Another option here would be to draw on all channels (0xff) instead of just one (0x01) if 0 is specified
            
            if (channel16 == 0)
                channel16 = 1;

            // Go through the channel mask to see which bits are set in the channel16 specifier, and send the data to each and every
            // channel that matches the mask.  So if the send channel 7, that means the lowest 3 channels will be set.

            lock_guard<mutex> guard(g_buffer_mutex);

            //if (!heap_caps_check_integrity_all(true))
            //    debugW("### Corrupt heap detected in WIFI_COMMAND_PIXELDATA64");

            for (int iChannel = 0, channelMask = 1; iChannel < NUM_CHANNELS; iChannel++, channelMask <<= 1)
            {
                if ((channelMask & channel16) != 0)
                {
                    debugV("Processing for Channel %d", iChannel);
                    
                    bool bDone = false;
                    if (!g_apBufferManager[iChannel]->IsEmpty())
                    {
                        auto pNewestBuffer = g_apBufferManager[iChannel]->PeekNewestBuffer();
                        if (micros != 0 && pNewestBuffer->MicroSeconds() == micros && pNewestBuffer->Seconds() == seconds)
                        {
                            debugV("Updating existing buffer");
                            if (!pNewestBuffer->UpdateFromWire(payloadData, payloadLength))
                                return false;
                            g_apBufferManager[iChannel]->UpdateOldestAndNewest();
                            bDone = true;
                        }
                    }
                    if (!bDone)
                    {
                        debugV("No match so adding new buffer");
                        auto pNewBuffer = g_apBufferManager[iChannel]->GetNewBuffer();
                        if (!pNewBuffer->UpdateFromWire(payloadData, payloadLength))
                            return false;
                        g_apBufferManager[iChannel]->UpdateOldestAndNewest();
                    }
                }
            }
            return true;
        }

        case WIFI_COMMAND_VU:
        {
            debugW("Deprecated WIFI_COMMAND_VU received.");
            uint32_t vu32 = payloadData[5] << 24 | payloadData[4] << 16 | payloadData[3] << 8 | payloadData[2];
            debugV("Incoming VU: %u", vu32);
            return true;
        }

        // WIFI_COMMAND_CLOCK
        //
        // Allows the server to send its current timeofday clock; if it's newer than our clock, we update ours

        case WIFI_COMMAND_CLOCK:
        {
            debugW("Deprecated WIFI_COMMAND_CLOCK received.");
            if (payloadLength != WIFI_COMMAND_CLOCK_SIZE)
            {
                debugW("Incorrect packet size for clock command received.  Expected %d and got %d\n", WIFI_COMMAND_CLOCK_SIZE, payloadLength);
                return false;
            }

            uint16_t channel16    = payloadData[3]  << 8  | payloadData[2];

            if (channel16 != 0)
            {
                debugW("Nonzero channel for clock not currently supported, but received: %d\n", channel16);
            }

            return true;
        }

        default:
        {
            return false;
        }
    }
    #endif
}

