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
//    Network loop, remote control, debug loop, etc.
//
// History:     May-11-2021         Davepl      Commented
//
//---------------------------------------------------------------------------

#include <ArduinoOTA.h>             // Over-the-air helper object so we can be flashed via WiFi
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <ESPmDNS.h>
#include <nvs_flash.h>                   // Non-volatile storage access
#include <nvs.h>

#include "globals.h"
#include "ledviewer.h"                          // For the LEDViewer task and object
#include "network.h"
#include "systemcontainer.h"

extern DRAM_ATTR std::mutex g_buffer_mutex;

static DRAM_ATTR WiFiUDP l_Udp;              // UDP object used for NNTP, etc

// Static initializers
DRAM_ATTR bool NTPTimeClient::_bClockSet = false;                                   // Has our clock been set by SNTP?
DRAM_ATTR std::mutex NTPTimeClient::_clockMutex;                                    // Clock guard mutex for SNTP client

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
            debugA("Refreshing Time from Server...");
            NTPTimeClient::UpdateClockFromWeb(&l_Udp);
        }
        else if (str.equalsIgnoreCase("stats"))
        {
            auto& bufferManager = g_ptrSystem->BufferManagers()[0];

            debugA("Displaying statistics....");
            debugA("%s:%zux%d %uK", FLASH_VERSION_NAME, g_ptrSystem->Devices().size(), NUM_LEDS, ESP.getFreeHeap() / 1024);
            debugA("%sdB:%s",String(WiFi.RSSI()).substring(1).c_str(), WiFi.isConnected() ? WiFi.localIP().toString().c_str() : "None");
            debugA("BUFR:%02zu/%02zu [%dfps]", bufferManager.Depth(), bufferManager.BufferCount(), g_Values.FPS);
            debugA("DATA:%+04.2lf-%+04.2lf", bufferManager.AgeOfOldestBuffer(), bufferManager.AgeOfNewestBuffer());

            #if ENABLE_AUDIO
                debugA("g_Analyzer._VU: %.2f, g_Analyzer._MinVU: %.2f, g_Analyzer.g_Analyzer._PeakVU: %.2f, g_Analyzer.gVURatio: %.2f", g_Analyzer._VU, g_Analyzer._MinVU, g_Analyzer._PeakVU, g_Analyzer._VURatio);
            #endif

            #if INCOMING_WIFI_ENABLED
                debugA("Socket Buffer _cbReceived: %zu", g_ptrSystem->SocketServer()._cbReceived);
            #endif
        }
        else if (str.equalsIgnoreCase("clearsettings"))
        {
            debugA("Removing persisted settings....");
            g_ptrSystem->DeviceConfig().RemovePersisted();
            RemoveEffectManagerConfig();
        }
        else if (str.equalsIgnoreCase("uptime"))
        {
             NTPTimeClient::ShowUptime();
        }
        else
        {
            debugA("Unknown Command.  Extended Commands:");
            debugA("clock               Refresh time from server");
            debugA("stats               Display buffers, memory, etc");
            debugA("clearsettings       Reset persisted user settings");
            debugA("uptime              Show system uptime, reset reason");
        }
    }
#endif

// SetupOTA
//
// Set up the over-the-air programming info so that we can be flashed over WiFi

void SetupOTA(const String & strHostname)
{
#if ENABLE_OTA
    ArduinoOTA.setRebootOnSuccess(true);

    if (strHostname.isEmpty())
        ArduinoOTA.setMdnsEnabled(false);
    else
        ArduinoOTA.setHostname(strHostname.c_str());

    ArduinoOTA
        .onStart([] {
            g_Values.UpdateStarted = true;

            String type;
            if (ArduinoOTA.getCommand() == U_FLASH)
                type = "sketch";
            else // U_SPIFFS
                type = "filesystem";

            debugI("Stopping IR remote");
            #if ENABLE_REMOTE
            g_ptrSystem->RemoteControl().end();
            #endif

            debugI("Start updating from OTA ");
            debugI("%s", type.c_str());
        })
        .onEnd([] {
            debugI("\nEnd OTA");
            g_Values.UpdateStarted = false;
        })
        .onProgress([](unsigned int progress, unsigned int total)
        {
            static uint last_time = millis();
            if (millis() - last_time > 1000)
            {
                last_time = millis();
                auto p = (progress / (total / 100));
                debugI("OTA Progress: %u%%\r", p);

                #if USE_HUB75
                    auto pMatrix = std::static_pointer_cast<LEDMatrixGFX>(g_ptrSystem->EffectManager().GetBaseGraphics());
                    pMatrix->SetCaption(str_sprintf("Update:%d%%", p), CAPTION_TIME);
                    pMatrix->setLeds(LEDMatrixGFX::GetMatrixBackBuffer());
                #endif
            }
            else
            {
                debugV("OTA Progress: %u%%\r", (progress / (total / 100)));
            }

        })
        .onError([](ota_error_t error)
        {
            g_Values.UpdateStarted = false;
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
            throw std::runtime_error("OTA Flash update failed.");
        });

    ArduinoOTA.begin();
#endif
}

// RemoteLoopEntry
//
// If enabled, this is the main thread loop for the remote control.  It is initialized and then
// called once every 20ms to pump its work queue and scan for new remote codes, etc.  If no
// remote is being used, this code and thread doesn't exist in the build.

#if ENABLE_REMOTE

void IRAM_ATTR RemoteLoopEntry(void *)
{
    //debugW(">> RemoteLoopEntry\n");

    auto& remoteControl = g_ptrSystem->RemoteControl();

    remoteControl.begin();
    while (true)
    {
        remoteControl.handle();
        delay(20);
    }
}
#endif

// ConnectToWiFi
//
// Connect to the pre-configured WiFi network.

#if ENABLE_WIFI

    #define WIFI_WAIT_BASE      4000    // Initial time to wait for WiFi to come up, in ms
    #define WIFI_WAIT_INCREASE  1000    // Increase of WiFi waiting time per cycle, in ms


    bool ConnectToWiFi(uint cRetries, bool waitForCredentials = false)
    {
        static bool bPreviousConnection = false;

        // Already connected, Go no further.
        if (WiFi.isConnected())
            return true;

        debugI("Setting host name to %s...%s", cszHostname,WLtoString(WiFi.status()));

        debugV("Wifi.disconnect");
        WiFi.disconnect();
        debugV("Wifi.mode");
        WiFi.mode(WIFI_STA);
        debugV("Wifi.begin");

        for (uint iPass = 0; iPass < cRetries; iPass++)
        {
            if (WiFi_ssid.length() == 0)
            {
                debugW("WiFi credentials not set, cannot connect.");

                if (waitForCredentials)
                {
                    debugW("Waiting for WiFi credentials to be set. Pass %u of %u...", iPass + 1, cRetries);
                    iPass = 0;
                }
                else
                {
                    break;
                }
            }
            else
            {
                debugW("Pass %u of %u: Connecting to Wifi SSID: \"%s\" - ESP32 Free Memory: %u, PSRAM:%u, PSRAM Free: %u\n",
                        iPass + 1, cRetries, WiFi_ssid.c_str(), ESP.getFreeHeap(), ESP.getPsramSize(), ESP.getFreePsram());

                WiFi.begin(WiFi_ssid.c_str(), WiFi_password.c_str());

                debugV("Done Wifi.begin, waiting for connection...");
            }

            // Give the module a few seconds to connect
            delay(WIFI_WAIT_BASE + iPass * WIFI_WAIT_INCREASE);

            if (WiFi.isConnected())
            {
                debugW("Connected to AP with BSSID: %s\n", WiFi.BSSIDstr().c_str());
                break;
            }
        }

        // Additional Services onwwards reliant on network so close if not up.
        if (!WiFi.isConnected())
        {
            debugW("Giving up on WiFi\n");
            return false;
        }

        debugW("Received IP: %s", WiFi.localIP().toString().c_str());

        // If we were connected before, network-dependent services will have been started already
        if (bPreviousConnection)
            return true;

        bPreviousConnection = true;

        #if INCOMING_WIFI_ENABLED
            auto& socketServer = g_ptrSystem->SocketServer();

            // Start listening for incoming data
            debugI("Starting/restarting Socket Server...");
            socketServer.release();
            if (false == socketServer.begin())
                throw std::runtime_error("Could not start socket server!");

            debugI("Socket server started.");
        #endif

        #if ENABLE_OTA
            debugI("Publishing OTA...");
            SetupOTA(String(cszHostname));
        #endif

        #if ENABLE_NTP
            debugI("Setting Clock...");
            NTPTimeClient::UpdateClockFromWeb(&l_Udp);
        #endif

        #if ENABLE_WEBSERVER
            debugI("Starting Web Server...");
            g_ptrSystem->WebServer().begin();
            debugI("Web Server begin called!");
        #endif

        return true;
    }

    #if ENABLE_NTP
        void UpdateNTPTime()
        {
            static unsigned long lastUpdate = 0;

            if (WiFi.isConnected())
            {
                // If we've already retrieved the time successfully, we'll only actually update every NTP_DELAY_SECONDS seconds
                if (!NTPTimeClient::HasClockBeenSet() || (millis() - lastUpdate) > ((NTP_DELAY_SECONDS) * 1000))
                {
                    debugV("Refreshing Time from Server...");
                    if (NTPTimeClient::UpdateClockFromWeb(&l_Udp))
                        lastUpdate = millis();
                }
            }
        }
    #endif
#endif // ENABLE_WIFI

// ProcessIncomingData
//
// Code that actually handles whatever comes in on the socket.  Must be known good data
// as this code does not validate!  This is where the commands and pixel data are received
// from the server.

bool ProcessIncomingData(std::unique_ptr<uint8_t []> & payloadData, size_t payloadLength)
{
    #if !INCOMING_WIFI_ENABLED
        return false;
    #else

    uint16_t command16 = payloadData[1] << 8 | payloadData[0];

    debugV("payloadLength: %zu, command16: %d", payloadLength, command16);

    // The very old original implementation used channel numbers, not a mask, and only channel 0 was supported at that time, so if
    // we see a Channel 0 asked for, it must be very old, and we massage it into the mask for Channel0 instead

    switch (command16)
    {
        // WIFI_COMMAND_PEAKDATA has a header plus NUM_BANDS floats that will be used to set the audio peaks

        case WIFI_COMMAND_PEAKDATA:
        {
            #if ENABLE_AUDIO
                uint16_t numbands  = WORDFromMemory(&payloadData[2]);
                uint32_t length32  = DWORDFromMemory(&payloadData[4]);
                uint64_t seconds   = ULONGFromMemory(&payloadData[8]);
                uint64_t micros    = ULONGFromMemory(&payloadData[16]);

                debugV("ProcessIncomingData -- Bands: %u, Length: %u, Seconds: %llu, Micros: %llu ... ",
                    numbands,
                    length32,
                    seconds,
                    micros);

                PeakData peaks((double *)(payloadData.get() + STANDARD_DATA_HEADER_SIZE));
                peaks.ApplyScalars(PeakData::PCREMOTE);
                g_Analyzer.SetPeakData(peaks);
            #endif
            return true;
        }

        // WIFI_COMMAND_PIXELDATA64 has a header plus length32 CRGBs

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

            std::lock_guard<std::mutex> guard(g_buffer_mutex);

            for (int iChannel = 0, channelMask = 1; iChannel < g_ptrSystem->BufferManagers().size(); iChannel++, channelMask <<= 1)
            {
                if ((channelMask & channel16) != 0)
                {
                    debugV("Processing for Channel %d", iChannel);

                    bool bDone = false;
                    auto& bufferManager = g_ptrSystem->BufferManagers()[iChannel];

                    if (!bufferManager.IsEmpty())
                    {
                        auto pNewestBuffer = bufferManager.PeekNewestBuffer();
                        if (micros != 0 && pNewestBuffer->MicroSeconds() == micros && pNewestBuffer->Seconds() == seconds)
                        {
                            debugV("Updating existing buffer");
                            if (!pNewestBuffer->UpdateFromWire(payloadData, payloadLength))
                                return false;
                            bDone = true;
                        }
                    }
                    if (!bDone)
                    {
                        debugV("No match so adding new buffer");
                        auto pNewBuffer = bufferManager.GetNewBuffer();
                        if (!pNewBuffer->UpdateFromWire(payloadData, payloadLength))
                            return false;
                    }
                }
            }
            return true;
        }

        default:
        {
            debugV("ProcessIncomingData -- Unknown command: 0x%x", command16);
            return false;
        }
    }
    #endif
}

// Non-volatile Storage for WiFi Credentials

// ReadWiFiConfig
//
// Attempts to read the WiFi ssid and password from NVS storage strings.  The keys
// for those name-value pairs are made from the variable names (WiFi_ssid, WiFi_Password)
// directly.  Limited to 63 characters in both cases, which is the WPA2 ssid limit.

#define MAX_PASSWORD_LEN 63

bool ReadWiFiConfig()
{
    char szBuffer[MAX_PASSWORD_LEN+1];

    nvs_handle_t nvsROHandle;
    esp_err_t err = nvs_open("storage", NVS_READONLY, &nvsROHandle);
    if (err != ESP_OK)
    {
        debugW("Error (%s) opening NVS handle!\n", esp_err_to_name(err));
        return false;
    }
    else
    {
        // Read the SSID and Password from the NVS partition name/value keypair set

        auto len = ARRAYSIZE(szBuffer);
        err = nvs_get_str(nvsROHandle, NAME_OF(WiFi_ssid), szBuffer, &len);
        if (ESP_OK != err)
        {
            debugE("Could not read WiFi_ssid from NVS");
            nvs_close(nvsROHandle);
            return false;
        }
        WiFi_ssid = szBuffer;

        len = ARRAYSIZE(szBuffer);
        err = nvs_get_str(nvsROHandle, NAME_OF(WiFi_password), szBuffer, &len);
        if (ESP_OK != err)
        {
            debugE("Could not read WiFi_password for \"%s\" from NVS", WiFi_ssid.c_str());
            nvs_close(nvsROHandle);
            return false;
        }
        WiFi_password = szBuffer;

        // Don't check in changes that would display the password in logs, etc.
        debugW("Retrieved SSID and Password from NVS: \"%s\", \"********\"", WiFi_ssid.c_str());

        nvs_close(nvsROHandle);
        return true;
    }
}

// WriteWiFiConfig
//
// Attempts to write the WiFi ssid and password to NVS storage strings.  The keys
// for those name-value pairs are made from the variable names (WiFi_ssid, WiFi_Password)
// directly.  It's not transactional, so it could conceivably succeed at writing
// the ssid and not the password (but will still report failure).  Does not
// enforce length limits on values given, so conceivable you could write longer
// pairs than you could read, but they wouldn't work on WiFi anyway.

bool WriteWiFiConfig()
{
    nvs_handle_t nvsRWHandle;

    // The "storage" string must match NVS partition name in partition table

    esp_err_t err = nvs_open("storage", NVS_READWRITE, &nvsRWHandle);
    if (err != ESP_OK)
    {
        debugW("Error (%s) opening NVS handle!\n", esp_err_to_name(err));
        return false;
    }

    bool success = true;

    err = nvs_set_str(nvsRWHandle, NAME_OF(WiFi_ssid), WiFi_ssid.c_str());
    if (ESP_OK != err)
    {
        debugW("Error (%s) storing ssid!\n", esp_err_to_name(err));
        success = false;
    }

    err = nvs_set_str(nvsRWHandle, NAME_OF(WiFi_password), WiFi_password.c_str());
    if (ESP_OK != err)
    {
        debugW("Error (%s) storing password!\n", esp_err_to_name(err));
        success = false;
    }

    if (success)
        // Do not check in code that displays the password in logs, etc.
        debugW("Stored SSID and Password to NVS: %s, *******", WiFi_ssid.c_str());

    nvs_commit(nvsRWHandle);
    nvs_close(nvsRWHandle);

    return true;
}

#if ENABLE_WIFI

    // DebugLoopTaskEntry
    //
    // Entry point for the Debug task, pumps the Debug handler

    void IRAM_ATTR DebugLoopTaskEntry(void *)
    {
        //debugI(">> DebugLoopTaskEntry\n");

    // Initialize RemoteDebug

        debugV("Starting RemoteDebug server...\n");

        Debug.setResetCmdEnabled(true);                         // Enable the reset command
        Debug.showProfiler(false);                              // Profiler (Good to measure times, to optimize codes)
        Debug.showColors(false);                                // Colors
        Debug.setCallBackProjectCmds(&processRemoteDebugCmd);   // Func called to handle any debug externsions we add

        while (!WiFi.isConnected())                             // Wait for wifi, no point otherwise
            delay(100);

        Debug.begin(cszHostname, RemoteDebug::INFO);            // Initialize the WiFi debug server

        for (;;)                                                // Call Debug.handle() 20 times a second
        {
            EVERY_N_MILLIS(50)
            {
                Debug.handle();
            }

            delay(10);
        }
    }
#endif

#if INCOMING_WIFI_ENABLED

    // SocketServerTaskEntry
    //
    // Repeatedly calls the code to open up a socket and receive new connections

    void IRAM_ATTR SocketServerTaskEntry(void *)
    {
        for (;;)
        {
            if (WiFi.isConnected())
            {
                auto& socketServer = g_ptrSystem->SocketServer();

                socketServer.release();
                socketServer.begin();
                socketServer.ProcessIncomingConnectionsLoop();
                debugW("Socket connection closed.  Retrying...\n");
            }
            delay(500);
        }
    }
#endif

#if COLORDATA_SERVER_ENABLED
    // ColorDataTaskEntry
    //
    // The thread which serves requests for color data on port 49153
    void IRAM_ATTR ColorDataTaskEntry(void *)
    {
        LEDViewer _viewer(12000);
        int socket = -1;

        for(;;)
        {
            while (!WiFi.isConnected())
                delay(250);

            if (!_viewer.begin())
            {
                debugE("Unable to start color data server!");
                delay(1000);
                continue;
            }
            else
            {
                debugW("Started color data server!");
                break;
            }
        }

        for (;;)
        {
            if (socket < 0)
                socket = _viewer.CheckForConnection();

            while (socket >= 0)
            {
                auto& effectManager = g_ptrSystem->EffectManager();

                if (effectManager.IsNewFrameAvailable())
                {
                    effectManager.SetNewFrameAvailable(false);

                    debugV("Sending color data packet");
                    // Potentially too large for the stack, so we allocate it on the heap instead
                    std::unique_ptr<ColorDataPacket> pPacket = std::make_unique<ColorDataPacket>();
                    pPacket->header = COLOR_DATA_PACKET_HEADER;
                    pPacket->width  = effectManager.g()->width();
                    pPacket->height = effectManager.g()->height();
                    if (effectManager.g()->leds != nullptr)
                    {
                        memcpy(pPacket->colors, effectManager.g()->leds, sizeof(CRGB) * NUM_LEDS);

                        if (!_viewer.SendPacket(socket, pPacket.get(), sizeof(ColorDataPacket)))
                        {
                            // If anything goes wrong, we close the socket so it can accept new incoming attempts
                            debugW("Error on color data socket, so closing");
                            close(socket);
                            socket = -1;
                            break;
                        }
                    }
                }
                delay(10);
            }
            delay(1000);
        }
    }
#endif // COLORDATA_SERVER_ENABLED

#if ENABLE_WIFI

    // NetworkHandlingLoopEntry
    //
    // Thead entry point for the Networking task
    // Pumps the various network loops and sets the time periodically, as well as reconnecting
    // to WiFi if the connection drops.  Also pumps the OTA (Over the air updates) loop.

    void IRAM_ATTR NetworkHandlingLoopEntry(void *)
    {
        //debugI(">> NetworkHandlingLoopEntry\n");
        if(!MDNS.begin("esp32")) {
            Serial.println("Error starting mDNS");
        }

        TickType_t notifyWait = 0;

        for (;;)
        {
            // Wait until we're woken up by a reader being flagged, or until we've reached the hold point
            ulTaskNotifyTake(pdTRUE, notifyWait);

            /* Every few seconds we check WiFi, and reconnect if we've lost the connection.  If we are unable to restart
                it for any reason, we reboot the chip in cases where its required, which we assume from WAIT_FOR_WIFI */

            EVERY_N_SECONDS(1)
            {
                if (WiFi.isConnected() == false && ConnectToWiFi(5) == false)
                {
                    debugE("Cannot Connect to Wifi!");
                    #if WAIT_FOR_WIFI
                        debugE("Rebooting in 5 seconds due to no Wifi available.");
                        delay(5000);
                        throw new std::runtime_error("Rebooting due to no Wifi available.");
                    #endif
                }
            }

            // If the reader container isn't available yet, we'll sleep for a second before we check again
            if (!g_ptrSystem->HasNetworkReader())
            {
                notifyWait = pdMS_TO_TICKS(1000);
                continue;
            }

            auto& networkReader = g_ptrSystem->NetworkReader();
            unsigned long now = millis();

            // Flag entries of which the read interval has passed
            for (auto& entry : networkReader.readers)
            {
                if (entry.canceled.load())
                    continue;

                auto interval = entry.readInterval.load();
                unsigned long targetMs = entry.lastReadMs.load() + interval;

                // The last check captures cases where millis() returns bogus data; if the delta between now and lastReadMs is greater
                //   than the interval then something's up with our timekeeping, so we trigger the reader just to be sure
                if (interval && (targetMs <= now || (std::max(now, targetMs) - std::min(now, targetMs)) > interval))
                    entry.flag.store(true);

                // Unset flag before we do the actual read. This makes that we don't miss another flag raise if it happens while reading
                if (entry.flag.exchange(false))
                {
                    entry.reader();
                    entry.lastReadMs.store(millis());
                }
            }

            // We wake up at least once every second
            unsigned long holdMs = 1000;
            now = millis();

            // Calculate how long we can sleep. This is determined by the reader that is closest to its interval passing.
            for (auto& entry : networkReader.readers)
            {
                if (entry.canceled.load())
                    continue;

                auto interval = entry.readInterval.load();
                auto lastReadMs = entry.lastReadMs.load();

                if (!interval)
                    continue;

                // If one of the reader intervals passed then we're up for another read cycle right away, so we can stop looking further
                if (lastReadMs + interval <= now)
                {
                    holdMs = 0;
                    break;
                }
                else
                {
                    unsigned long entryHoldMs = std::min(interval, interval - (now - lastReadMs));
                    if (entryHoldMs < holdMs)
                        holdMs = entryHoldMs;
                }
            }

            notifyWait = pdMS_TO_TICKS(holdMs);
        }
    }

    size_t NetworkReader::RegisterReader(std::function<void()> reader, unsigned long interval, bool flag)
    {
        // Add the reader with its flag unset
        auto& readerEntry = readers.emplace_back(reader, interval);

        // If an interval is specified, start the interval timer now.
        if (interval)
            readerEntry.lastReadMs.store(millis());

        size_t index = readers.size() - 1;

        if (flag)
            FlagReader(index);

        return index;
    }

    void NetworkReader::FlagReader(size_t index)
    {
        // Check if we received a valid reader index
        if (index >= readers.size())
            return;

        readers[index].flag.store(true);

        g_ptrSystem->TaskManager().NotifyNetworkThread();
    }

    void NetworkReader::CancelReader(size_t index)
    {
        // Check if we received a valid reader index
        if (index >= readers.size())
            return;

        auto& entry = readers[index];
        entry.canceled.store(true);
        entry.readInterval.store(0);
        entry.reader = nullptr;
    }

#endif // ENABLE_WIFI
