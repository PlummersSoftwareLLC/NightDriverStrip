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

#include "globals.h"

#if ENABLE_WIFI

#include <ArduinoOTA.h>             // Over-the-air helper object so we can be flashed via WiFi
#include <ESPmDNS.h>
#include <nvs.h>
#include <algorithm>

#include "debug_cli.h"
#include "effectmanager.h"
#include "ledviewer.h"              // For the LEDViewer task and object
#include "network.h"
#include "soundanalyzer.h"
#include "systemcontainer.h"

extern DRAM_ATTR std::mutex g_buffer_mutex;

static DRAM_ATTR WiFiUDP l_Udp;              // UDP object used for NNTP, etc


String urlEncode(const String& str)
{
    String encodedString = "";
    char c;
    char code0;
    char code1;
    for (int i = 0; i < str.length(); i++) {
        c = str.charAt(i);
        if (isalnum(c)) {
            encodedString += c;
        } else {
            code1 = (c & 0xf) + '0';
            if ((c & 0xf) > 9) {
                code1 = (c & 0xf) - 10 + 'A';
            }
            c = (c >> 4) & 0xf;
            code0 = c + '0';
            if (c > 9) {
                code0 = c - 10 + 'A';
            }
            encodedString += '%';
            encodedString += code0;
            encodedString += code1;
        }
    }
    return encodedString;
}

void DoStatsCommand()
{
    auto& bufferManager = g_ptrSystem->BufferManagers()[0];

    DebugCLI::cli_printf("%s:%zux%d %zuK", FLASH_VERSION_NAME, g_ptrSystem->Devices().size(), NUM_LEDS, (size_t)(ESP.getFreeHeap() / 1024));
    DebugCLI::cli_printf("%sdB:%s",String(WiFi.RSSI()).substring(1).c_str(), WiFi.isConnected() ? WiFi.localIP().toString().c_str() : "None");
    DebugCLI::cli_printf("BUFR:%02zu/%02zu [%lufps]", (size_t)bufferManager.Depth(), (size_t)bufferManager.BufferCount(), (unsigned long)g_Values.FPS);
    DebugCLI::cli_printf("DATA:%+04.2lf-%+04.2lf", bufferManager.AgeOfOldestBuffer(), bufferManager.AgeOfNewestBuffer());

    #if ENABLE_AUDIO
    DebugCLI::cli_printf("g_Analyzer._VU: %.2f, g_Analyzer._MinVU: %.2f, g_Analyzer.g_Analyzer._PeakVU: %.2f, g_Analyzer.gVURatio: %.2f", g_Analyzer.VU(), g_Analyzer.MinVU(), g_Analyzer.PeakVU(), g_Analyzer.VURatio());
    #endif

    #if INCOMING_WIFI_ENABLED
    DebugCLI::cli_printf("Socket Buffer _cbReceived: %zu", g_ptrSystem->SocketServer()._cbReceived);
    #endif
}

static const DebugCLI::command network_commands[] = {
    { "clock", "Refresh time from server", "Refreshing Time from Server",
        [](const DebugCLI::cli_argv&) { NTPTimeClient::UpdateClockFromWeb(&l_Udp); } },
    { "stats", "Display system statistics", "Displaying statistics",
        [](const DebugCLI::cli_argv&) { DoStatsCommand(); } },
    { "uptime", "Display system run duration", "Displaying system uptime",
        [](const DebugCLI::cli_argv&) { NTPTimeClient::ShowUptime(); } }
};

void InitNetworkCLI() {
    DebugCLI::RegisterCommands(network_commands, sizeof(network_commands)/sizeof(network_commands[0]));
}
#endif // ENABLE_WIFI

#if ENABLE_ESPNOW

// ESPNOW Support
//
// We accept ESPNOW commands to change effects and so on.  This is a simple structure that we'll receive over ESPNOW.
enum class ESPNowCommand : uint8_t
{
    ESPNOW_NEXTEFFECT = 1,
    ESPNOW_PREVEFFECT,
    ESPNOW_SETEFFECT,
    ESPNOW_INVALID = 255    // Followed by a uint32_t argument
};

// Message class
//
// Encapsulates an ESPNOW message, which is a command and an optional argument
class Message
{
public:
    constexpr Message(ESPNowCommand cmd, uint32_t argument)
        : cbSize(sizeof(Message)), command(cmd), arg1(argument)
    {
    }

    constexpr Message()
        : cbSize(sizeof(Message)), command(ESPNowCommand::ESPNOW_INVALID), arg1(0)
    {
    }

    const uint8_t* data() const
    {
        return reinterpret_cast<const uint8_t*>(this);
    }

    constexpr size_t byte_size() const
    {
        return sizeof(Message);
    }

    uint8_t       cbSize;
    ESPNowCommand command;
    uint32_t      arg1;
} __attribute__((packed));

// onReceiveESPNOW
//
// Callback function for ESPNOW that is called when a data packet is received

void onReceiveESPNOW(const uint8_t *macAddr, const uint8_t *data, int dataLen)
{
    Message message;

    memcpy(&message, data, sizeof(message));
    debugI("ESPNOW Message received.");

    if (message.cbSize != sizeof(message))
    {
        debugE("ESPNOW Message received with wrong structure size: %u but should be %zu", message.cbSize, sizeof(message));
        return;
    }

    switch(message.command)
    {
        case ESPNowCommand::ESPNOW_NEXTEFFECT:
            debugI("ESPNOW Next effect");
            g_ptrSystem->EffectManager().NextEffect();
            break;

        case ESPNowCommand::ESPNOW_PREVEFFECT:
            debugI("ESPNOW Previous effect");
            g_ptrSystem->EffectManager().PreviousEffect();
            break;

        case ESPNowCommand::ESPNOW_SETEFFECT:
            debugI("ESPNOW Setting effect index to %u", message.arg1);
            g_ptrSystem->EffectManager().SetCurrentEffectIndex(message.arg1);
            break;

        default:
            debugE("ESPNOW Message received with unknown command: %d", (byte) message.command);
            break;
    }
}

#endif

// processRemoteDebugCmd
//
// Callback function that the debug library (which exposes a little console over telnet and serial) calls
// in order to allow us to add custom commands.  I've added a clock reset and stats command, for example.

#if ENABLE_WIFI

    void processRemoteDebugCmd()
    {
        String str = Debug.getLastCommand();
        DebugCLI::RunCommand(str.c_str());

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
        .onStart([]()
        {
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
        .onEnd([]()
        {
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
                    auto pMatrix = std::static_pointer_cast<HUB75GFX>(g_ptrSystem->EffectManager().GetBaseGraphics()[0]);
                    pMatrix->SetCaption(str_sprintf("Update:%d%%", p), CAPTION_TIME);
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

#if ENABLE_WIFI

    #define WIFI_WAIT_BASE      4000    // Initial time to wait for WiFi to come up, in ms
    #define WIFI_WAIT_INCREASE  1000    // Increase of WiFi waiting time per cycle, in ms
    #define WIFI_WAIT_MAX       60000   // Maximum gap between retries, in ms

    #define WIFI_WAIT_INIT      (WIFI_WAIT_BASE - WIFI_WAIT_INCREASE)

    // ConnectToWiFi
    //
    // Try to connect to WiFi using the SSID and password passed as arguments
    WiFiConnectResult ConnectToWiFi(const String& ssid, const String& password)
    {
        return ConnectToWiFi(&ssid, &password);
    }

    // ConnectToWiFi
    //
    // Try to connect to WiFi using either the SSID and password pointed to by arguments, or the credentials
    // that were saved from an earlier call if no/nullptr arguments are passed.
    WiFiConnectResult ConnectToWiFi(const String* ssid = nullptr, const String* password = nullptr)
    {
        static bool bPreviousConnection = false;
        static unsigned long millisAtLastAttempt = 0;
        static unsigned long retryDelay = WIFI_WAIT_INIT;
        static String WiFi_ssid;
        static String WiFi_password;

        bool haveNewCredentials = (ssid != nullptr && password != nullptr && (WiFi_ssid != *ssid || WiFi_password != *password));

        // If we have new credentials then always reconnect using them
        if (haveNewCredentials)
        {
            WiFi_ssid = *ssid;
            WiFi_password = *password;
            retryDelay = WIFI_WAIT_INIT;
            debugI("WiFi credentials passed for SSID \"%s\"", WiFi_ssid.c_str());
        }
        // If we're already connected and services are running then go no further
        else if (bPreviousConnection && WiFi.isConnected())
        {
            return WiFiConnectResult::Connected;
        }

        // (Re)connect if credentials have changed, or our last attempt was long enough ago
        if (haveNewCredentials || millisAtLastAttempt == 0 || millis() - millisAtLastAttempt >= retryDelay)
        {
            millisAtLastAttempt = millis();
            retryDelay = std::min<unsigned long>(retryDelay + WIFI_WAIT_INCREASE, WIFI_WAIT_MAX);

            if (WiFi_ssid.length() == 0)
            {
                debugW("WiFi credentials not set, cannot connect.");
                return WiFiConnectResult::NoCredentials;
            }
            else
            {
                auto hostname = g_ptrSystem->DeviceConfig().GetHostname().c_str();

                if (hostname[0] == '\0')
                {
                    debugI("No hostname configured, so skipping setting it.");
                }
                else
                {
                    debugI("Setting host name to %s...", hostname);
                    WiFi.setHostname(hostname);
                }

                debugV("Wifi.disconnect");
                WiFi.disconnect();
                debugV("Wifi.mode");
                WiFi.mode(WIFI_STA);
                debugW("Connecting to Wifi SSID: \"%s\" - ESP32 Free Memory: %zu, PSRAM:%zu, PSRAM Free: %zu\n",
                       WiFi_ssid.c_str(), (size_t)ESP.getFreeHeap(), (size_t)ESP.getPsramSize(), (size_t)ESP.getFreePsram());

                WiFi.begin(WiFi_ssid.c_str(), WiFi_password.c_str());

                debugV("Done Wifi.begin, waiting for connection...");
            }
        }

        if (WiFi.isConnected())
        {
            debugW("Connected to AP with BSSID: \"%s\", received IP: %s", WiFi.BSSIDstr().c_str(), WiFi.localIP().toString().c_str());
        }
        else
        // Additional services onwards are reliant on network so return if WiFi is not up (yet)
        {
            debugW("Not yet connected to WiFi, waiting...");
            return WiFiConnectResult::Disconnected;
        }

        // If we were connected before, network-dependent services will have been started already
        if (bPreviousConnection)
            return WiFiConnectResult::Connected;

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
            SetupOTA(String(WiFi.getHostname()));
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

        return WiFiConnectResult::Connected;
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

                debugV("ProcessIncomingData -- Bands: %u, Length: %lu, Seconds: %llu, Micros: %llu ... ",
                       (unsigned int)numbands,
                       (unsigned long)length32,
                       seconds,
                       micros);

                // Data is transmitted as NUM_BANDS floats following the standard header
                const uint8_t* dataStart = payloadData.get() + STANDARD_DATA_HEADER_SIZE;
                const size_t availableFloats = (payloadLength > STANDARD_DATA_HEADER_SIZE)
                                                ? (payloadLength - STANDARD_DATA_HEADER_SIZE) / sizeof(float)
                                                : 0;
                const size_t copyCount = std::min<size_t>(NUM_BANDS, std::min<size_t>(numbands, availableFloats));

                PeakData peaks{}; // zero-initialized for any bands not provided
                if (copyCount > 0)
                {
                    const float* pFloats = reinterpret_cast<const float*>(dataStart);
                    std::copy_n(pFloats, copyCount, peaks.begin());
                }
                g_Analyzer.SetPeakDataFromRemote(peaks);
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

                debugV("ProcessIncomingData -- Channel: %u, Length: %lu, Seconds: %llu, Micros: %llu ... ",
                    (unsigned int)channel16,
                    (unsigned long)length32,
                    seconds,
                    micros);

                // The very old original implementation used channel numbers, not a mask, and only channel 0 was supported at that time, so if
                // we see a Channel 0 asked for, it must be very old, and we massage it into the mask for Channel0 instead
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

    #define MAX_PASSWORD_LEN 63

    // GetWiFiConfigKey
    //
    // Creates a unique key for storing/retrieving WiFi credentials in NVS.
    // The key is a combination of a base key (like "WiFi_ssid") and the
    // credential source, ensuring different credential sets don't overwrite
    // each other.

    inline String GetWiFiConfigKey(WifiCredSource source, const String& key)
    {
        return String(key + "_" + source);
    }

    // ReadWiFiConfig
    //
    // Attempts to read the WiFi ssid and password from NVS storage strings.  The keys
    // for those name-value pairs are made from the variable names (WiFi_ssid, WiFi_Password)
    // directly.  Limited to 63 characters in both cases, which is the WPA2 ssid limit.

    bool ReadWiFiConfig(WifiCredSource source, String& WiFi_ssid, String& WiFi_password)
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

            auto len = std::size(szBuffer);
            err = nvs_get_str(nvsROHandle, GetWiFiConfigKey(source, NAME_OF(WiFi_ssid)).c_str(), szBuffer, &len);
            if (ESP_OK != err)
            {
                debugE("Could not read WiFi_ssid for source %d from NVS", source);
                nvs_close(nvsROHandle);
                return false;
            }
            WiFi_ssid = szBuffer;

            len = std::size(szBuffer);
            err = nvs_get_str(nvsROHandle, GetWiFiConfigKey(source, NAME_OF(WiFi_password)).c_str(), szBuffer, &len);
            if (ESP_OK != err)
            {
                debugE("Could not read WiFi_password for SSID \"%s\" and source %d from NVS", WiFi_ssid.c_str(), source);
                nvs_close(nvsROHandle);
                return false;
            }
            WiFi_password = szBuffer;

            // Don't check in changes that would display the password in logs, etc.
            debugI("Retrieved SSID and Password for source %d from NVS: \"%s\", \"********\"", source, WiFi_ssid.c_str());

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

    bool WriteWiFiConfig(WifiCredSource source, const String& WiFi_ssid, const String& WiFi_password)
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

        err = nvs_set_str(nvsRWHandle, GetWiFiConfigKey(source, NAME_OF(WiFi_ssid)).c_str(), WiFi_ssid.c_str());
        if (ESP_OK != err)
        {
            debugW("Error (%s) storing ssid for source %d!\n", esp_err_to_name(err), source);
            success = false;
        }

        err = nvs_set_str(nvsRWHandle, GetWiFiConfigKey(source, NAME_OF(WiFi_password)).c_str(), WiFi_password.c_str());
        if (ESP_OK != err)
        {
            debugW("Error (%s) storing password for source %d!\n", esp_err_to_name(err), source);
            success = false;
        }

        if (success)
            // Do not check in code that displays the password in logs, etc.
            debugI("Stored SSID and Password for source %d to NVS: %s, *******", source, WiFi_ssid.c_str());

        nvs_commit(nvsRWHandle);
        nvs_close(nvsRWHandle);

        return success;
    }

    // ClearWiFiConfig
    //
    // Attempts to erase the WiFi ssid and password for a given source from NVS
    // storage. The keys for the name-value pairs are constructed based on the
    // source. This operation is not transactional; it's possible for one key
    // to be erased successfully while the other fails.

    bool ClearWiFiConfig(WifiCredSource source)
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

        err = nvs_erase_key(nvsRWHandle, GetWiFiConfigKey(source, NAME_OF(WiFi_ssid)).c_str());
        if (ESP_OK != err && err != ESP_ERR_NVS_NOT_FOUND)
        {
            debugW("Error (%s) erasing ssid for source %d!\n", esp_err_to_name(err), source);
            success = false;
        }

        err = nvs_erase_key(nvsRWHandle, GetWiFiConfigKey(source, NAME_OF(WiFi_password)).c_str());
        if (ESP_OK != err && err != ESP_ERR_NVS_NOT_FOUND)
        {
            debugW("Error (%s) erasing password for source %d!\n", esp_err_to_name(err), source);
            success = false;
        }

        if (success)
            debugI("Erased SSID and Password for source %d from NVS", source);

        nvs_commit(nvsRWHandle);
        nvs_close(nvsRWHandle);

        return success;
    }

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
        Debug.setCallBackProjectCmds(&processRemoteDebugCmd);   // Func called to handle any debug extensions we add

        while (!WiFi.isConnected())                             // Wait for wifi, no point otherwise
            delay(100);

        Debug.begin(WiFi.getHostname(), RemoteDebug::INFO);     // Initialize the WiFi debug server

        for (;;)                                                // Call Debug.handle() 20 times a second
        {
            Debug.handle();
            delay(MILLIS_PER_SECOND / 20);
        }
    }

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
    // The thread which serves requests for color data.

    void IRAM_ATTR ColorDataTaskEntry(void *)
    {
        LEDViewer _viewer(NetworkPort::ColorServer);
        int socket = -1;
        bool wsListenersPresent = false;
        BaseFrameEventListener frameEventListener;

        auto& effectManager = g_ptrSystem->EffectManager();
        #if COLORDATA_WEB_SOCKET_ENABLED
            auto& webSocketServer = g_ptrSystem->WebSocketServer();
        #endif

        effectManager.AddFrameEventListener(frameEventListener);

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

            auto leds = effectManager.g()->leds;

            if (frameEventListener.CheckAndClearNewFrameAvailable() && leds != nullptr)
            {
                if (socket >= 0)
                {
                    debugV("Sending color data packet");
                    // Potentially too large for the stack, so we allocate it on the heap instead
                    std::unique_ptr<ColorDataPacket> pPacket = std::make_unique<ColorDataPacket>();
                    pPacket->header = COLOR_DATA_PACKET_HEADER;
                    pPacket->width  = effectManager.g()->width();
                    pPacket->height = effectManager.g()->height();
                    memcpy(pPacket->colors, leds, sizeof(CRGB) * NUM_LEDS);

                    if (!_viewer.SendPacket(socket, pPacket.get(), sizeof(ColorDataPacket)))
                    {
                        // If anything goes wrong, we close the socket so it can accept new incoming attempts
                        debugW("Error on color data socket, so closing");
                        close(socket);
                        socket = -1;
                    }
                }

                #if COLORDATA_WEB_SOCKET_ENABLED
                    webSocketServer.SendColorData(leds, NUM_LEDS);
                #endif
            }

            #if COLORDATA_WEB_SOCKET_ENABLED
                wsListenersPresent = webSocketServer.HaveColorDataClients();
            #endif

            if (socket >= 0 || wsListenersPresent)
                delay(10);
            else
                delay(1000);
        }
    }
#endif // COLORDATA_SERVER_ENABLED

    // NetworkHandlingLoopEntry
    //
    // Thead entry point for the Networking task
    // Pumps the various network loops and sets the time periodically, as well as reconnecting
    // to WiFi if the connection drops.  Also pumps the OTA (Over the air updates) loop.

    void IRAM_ATTR NetworkHandlingLoopEntry(void *)
    {
        static unsigned long millisAtLastConnected = millis();

        //debugI(">> NetworkHandlingLoopEntry\n");
        if(!MDNS.begin("esp32")) {
            Serial.println("Error starting mDNS");
        }

        TickType_t notifyWait = 0;

        for (;;)
        {
            // Wait until we're woken up by a reader being flagged, or until we've reached the hold point
            ulTaskNotifyTake(pdTRUE, notifyWait);

            // Every second we check WiFi, and reconnect if we've lost the connection. If we are unable to restart
            // it for any reason, we reboot the chip in cases where its required, which we assume from WAIT_FOR_WIFI.

            EVERY_N_SECONDS(1)
            {
                auto connectResult = ConnectToWiFi();

                if (connectResult == WiFiConnectResult::Connected)
                {
                    millisAtLastConnected = millis();

                    #if WEB_SOCKETS_ANY_ENABLED
                        // It's recommended to clean up any stale web socket clients every second or so
                        g_ptrSystem->WebSocketServer().CleanupClients();
                    #endif
                }
                else
                {
                    debugV("Still waiting for WiFi to connect.");
                    #if WAIT_FOR_WIFI
                        // Reboot if we've been waiting for a connection for more than the maximum delay between
                        // connection retries and we _do_ have credentials
                        if (connectResult != WiFiConnectResult::NoCredentials && millis() - millisAtLastConnected > WIFI_WAIT_MAX)
                        {
                            debugE("Rebooting in 5 seconds due to no Wifi available.");
                            delay(5000);
                            throw new std::runtime_error("Rebooting due to no Wifi available.");
                        }
                    #endif
                }
            }

            // If the reader container isn't available yet or WiFi isn't up yet, we'll sleep for a second before we check again
            if (!g_ptrSystem->HasNetworkReader() || !WiFi.isConnected())
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

    size_t NetworkReader::RegisterReader(const std::function<void()>& reader, unsigned long interval, bool flag)
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
#else

    void InitNetworkCLI() {
    }

#endif // ENABLE_WIFI
