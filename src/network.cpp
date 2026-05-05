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
#include <esp_ota_ops.h>
#include <fcntl.h>

#if ENABLE_WIFI
    #include <algorithm>
    #include <ArduinoOTA.h>
    #include <ESPmDNS.h>
    #include <iterator>
    #include <nvs.h>
    #include <WiFi.h>
#elif ENABLE_ESPNOW
    #include <WiFi.h>
#endif

#include "colordata.h"
#include "deviceconfig.h"
#include "effectmanager.h"
#include "ledbuffer.h"
#include "nd_network.h"

#if ENABLE_REMOTE
    #include "remotecontrol.h"
#endif

#include "socketserver.h"
#include "systemcontainer.h"
#include "taskmgr.h"
#include "values.h"
#include "webserver.h"
#include "websocketserver.h"

#if ENABLE_WIFI
    #include "byte_utils.h"
    #include "console.h"
    #include "debug_cli.h"
    #include "ledviewer.h"
    #include "ntptimeclient.h"
    #include "soundanalyzer.h"

    #if USE_HUB75
        #include "hub75gfx.h"
    #endif
#endif

namespace nd_network
{
    // Private State
    static std::atomic<bool> l_bWifiDriverReady{false};

    // True while Improv owns WiFi during a provisioning attempt. Gates the
    // background reconnect loop so it doesn't race Improv on WiFi.begin /
    // WiFi.disconnect.
    static std::atomic<bool> l_ProvisioningActive{false};
    static std::atomic<int> l_LastWiFiDisconnectReason{0};

#if ENABLE_WIFI
    static DRAM_ATTR WiFiUDP l_Udp;

    // Writer function and flag combo
    struct NetworkReader::ReaderEntry
    {
        std::function<void()> reader;
        std::atomic_ulong     readInterval;
        std::atomic_ulong     lastReadMs;
        std::atomic_bool      flag = false;
        std::atomic_bool      canceled = false;

        ReaderEntry(std::function<void()> r, unsigned long interval)
            : reader(std::move(r)), lastReadMs(0), readInterval(interval) {}

        ReaderEntry(std::function<void()> r, unsigned long interval, unsigned long lastRead, bool f, bool c)
            : reader(std::move(r)), readInterval(interval), lastReadMs(lastRead), flag(f), canceled(c) {}
    };
#endif

    // Identity Implementations - eFuse sourced hardware identity

    // GetMacAddress
    //
    // Reads the hardware identity directly from the eFuses. This is stable
    // even without a WiFi radio chip and is available immediately at boot.
    String GetMacAddress(const char* separator)
    {
        uint8_t mac[6];
        esp_efuse_mac_get_default(mac);
        return str_sprintf("%02x%s%02x%s%02x%s%02x%s%02x%s%02x",
            mac[0], separator, mac[1], separator, mac[2], separator,
            mac[3], separator, mac[4], separator, mac[5]);
    }

    const char *WLtoString(int status)
    {
        switch (status)
        {
            case 255: return "WL_NO_SHIELD";
            case 0:   return "WL_IDLE_STATUS";
            case 1:   return "WL_NO_SSID_AVAIL";
            case 2:   return "WL_SCAN_COMPLETED";
            case 3:   return "WL_CONNECTED";
            case 4:   return "WL_CONNECT_FAILED";
            case 5:   return "WL_CONNECTION_LOST";
            case 6:   return "WL_DISCONNECTED";
            default:  return "WL_UNKNOWN_STATUS";
        }
    }

    // SetSocketBlockingEnabled
    //
    // In blocking mode, socket API calls wait until the operation is complete before returning control to the application.
    // In non-blocking mode, socket API calls return immediately. If an operation cannot be completed immediately, the function
    // returns an error (usually EWOULDBLOCK or EAGAIN).
    bool SetSocketBlockingEnabled(int fd, bool blocking)
    {
        if (fd < 0) return false;
        int flags = fcntl(fd, F_GETFL, 0);
        if (flags < 0) return false;
        if (blocking) flags &= ~O_NONBLOCK;
        else          flags |= O_NONBLOCK;
        return fcntl(fd, F_SETFL, flags) == 0;
    }

#if ENABLE_WIFI

    // WiFi-Specific Implementations

    bool IsWiFiConnected() { return WiFi.isConnected(); }
    int  GetWiFiStatus()    { return (int)WiFi.status(); }
    int  GetWiFiRSSI()      { return WiFi.RSSI(); }
    void SetWiFiModeSTA()   { WiFi.mode(WIFI_STA); }

    String GetWiFiLocalIP() { return WiFi.localIP().toString(); }

    bool GetWiFiHostByName(const char *hostname, IPAddress &ip)
    {
        return WiFi.hostByName(hostname, ip) == 1;
    }

    #define WIFI_WAIT_BASE      4000    // Initial time to wait for WiFi to come up, in ms
    #define WIFI_WAIT_INCREASE  1000    // Increase of WiFi waiting time per cycle, in ms
    #define WIFI_WAIT_MAX       60000   // Maximum gap between retries, in ms

    #define WIFI_WAIT_INIT      (WIFI_WAIT_BASE - WIFI_WAIT_INCREASE)

    // ConnectToWiFi
    //
    // Try to connect to WiFi using the SSID and password passed as arguments
    WiFiConnectResult ConnectToWiFi(const String &ssid, const String &password)
    {
        return ConnectToWiFi(&ssid, &password);
    }

    // ConnectToWiFi
    //
    // Try to connect to WiFi using either the SSID and password pointed to by arguments, or the credentials
    // that were saved from an earlier call if no/nullptr arguments are passed.
    WiFiConnectResult ConnectToWiFi(const String *ssid, const String *password)
    {
        static bool bInitialized = false;
        if (!bInitialized)
        {
            WiFi.onEvent([](arduino_event_id_t event, arduino_event_info_t info) {
                if (event == ARDUINO_EVENT_WIFI_READY)
                    l_bWifiDriverReady = true;
                else if (event == ARDUINO_EVENT_WIFI_STA_DISCONNECTED)
                {
                    l_LastWiFiDisconnectReason.store(info.wifi_sta_disconnected.reason);
                    debugW("WiFi Disconnected. Reason: %d", info.wifi_sta_disconnected.reason);
                }
            });
            WiFi.mode(WIFI_STA);
            WiFi.setSleep(false);
            bInitialized = true;
        }

        static bool bPreviousConnection = false;
        static bool bReportedDisconnected = false;
        static unsigned long millisAtLastAttempt = 0;
        static unsigned long retryDelay = WIFI_WAIT_INIT;
        static String WiFi_ssid;
        static String WiFi_password;

        bool explicitCredentials = (ssid != nullptr && password != nullptr);

        // If credentials are explicitly supplied, always start a fresh connection attempt.
        // This matters for USB provisioning, where the same SSID may be submitted with a
        // corrected or intentionally repeated password.
        if (explicitCredentials)
        {
            WiFi_ssid = *ssid;
            WiFi_password = *password;
            retryDelay = WIFI_WAIT_INIT;
            debugI("WiFi credentials passed for SSID \"%s\"", WiFi_ssid.c_str());
        }
        else if (bPreviousConnection && WiFi.isConnected())
        {
            return WiFiConnectResult::Connected;
        }

        // (Re)connect if credentials were explicitly provided, or our last attempt was long enough ago
        if (explicitCredentials || millisAtLastAttempt == 0 || millis() - millisAtLastAttempt >= retryDelay)
        {
            millisAtLastAttempt = millis();
            retryDelay = std::min<unsigned long>(retryDelay + WIFI_WAIT_INCREASE, WIFI_WAIT_MAX);

            if (WiFi_ssid.length() == 0)
            {
                debugW("WiFi credentials not set, cannot connect.");
                return WiFiConnectResult::NoCredentials;
            }

            const String &hostname = g_ptrSystem->GetDeviceConfig().GetHostname();
            if (hostname.length() > 0)
                WiFi.setHostname(hostname.c_str());

            if (explicitCredentials || WiFi.status() == WL_CONNECT_FAILED)
            {
                // Explicit credentials mean Improv or startup asked for a new
                // connection attempt. Clear the driver's STA AP config so
                // WiFi.begin cannot short-circuit as "already connected" when
                // the SSID is unchanged.
                WiFi.disconnect(false, explicitCredentials);
                bPreviousConnection = false;
                bReportedDisconnected = false;

                const unsigned long disconnectStarted = millis();
                while (WiFi.isConnected() && millis() - disconnectStarted < 2000)
                    delay(20);

                if (WiFi.isConnected())
                    debugW("WiFi remained connected after disconnect request; starting the new attempt anyway.");
            }

            debugW("Connecting to Wifi SSID: \"%s\" - ESP32 Free Memory: %zu, PSRAM:%zu, PSRAM Free: %zu\n",
                   WiFi_ssid.c_str(), (size_t)ESP.getFreeHeap(), (size_t)ESP.getPsramSize(), (size_t)ESP.getFreePsram());

            WiFi.begin(WiFi_ssid.c_str(), WiFi_password.c_str());

            debugV("Done Wifi.begin, waiting for connection...");
        }

        if (WiFi.isConnected())
        {
            debugW("Connected to AP with BSSID: \"%s\", received IP: %s", WiFi.BSSIDstr().c_str(), WiFi.localIP().toString().c_str());
        }
        else
        // Additional services onwards are reliant on network so return if WiFi is not up (yet)
        {
            if (!bReportedDisconnected)
            {
                debugW("Not yet connected to WiFi, waiting...");
                bReportedDisconnected = true;
            }
            return WiFiConnectResult::Disconnected;
        }

        // If we were connected before, network-dependent services will have been started already
        if (bPreviousConnection)
            return WiFiConnectResult::Connected;

        bPreviousConnection = true;
        bReportedDisconnected = false;

        #if ENABLE_OTA
            SetupOTA(String(WiFi.getHostname()));
        #endif
        #if ENABLE_NTP
            static bool bUdpInitialized = false;
            if (!bUdpInitialized)
            {
                l_Udp.begin(1234);     // Use a fixed local port for NTP responses
                bUdpInitialized = true;
            }
            NTPTimeClient::UpdateClockFromWeb(&l_Udp);
        #endif
        #if ENABLE_WEBSERVER
            g_ptrSystem->GetWebServer().begin();
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
                    if (NTPTimeClient::UpdateClockFromWeb(&l_Udp))
                        lastUpdate = millis();
                }
            }
        }
    #endif

    // Non-volatile Storage for WiFi Credentials

    // GetWiFiConfigKey
    //
    // Creates a unique key for storing/retrieving WiFi credentials in NVS.
    // The key is a combination of a base key (like "WiFi_ssid") and the
    // credential source, ensuring different credential sets don't overwrite
    // each other.
    static inline String GetWiFiConfigKey(WifiCredSource source, const String &key)
    {
        return String(key + "_" + (int)source);
    }

    // ReadWiFiConfig
    //
    // Attempts to read the WiFi ssid and password from NVS storage strings.  The keys
    // for those name-value pairs are made from the variable names (WiFi_ssid, WiFi_Password)
    // directly.  Limited to 63 characters in both cases, which is the WPA2 ssid limit.
    bool ReadWiFiConfig(WifiCredSource source, String &WiFi_ssid, String &WiFi_password)
    {
        char szBuffer[64];
        nvs_handle_t nvsROHandle;
        if (nvs_open("storage", NVS_READONLY, &nvsROHandle) != ESP_OK) return false;

        size_t len = std::size(szBuffer);
        if (nvs_get_str(nvsROHandle, GetWiFiConfigKey(source, "WiFi_ssid").c_str(), szBuffer, &len) != ESP_OK)
        {
            nvs_close(nvsROHandle);
            return false;
        }
        WiFi_ssid = szBuffer;

        len = std::size(szBuffer);
        if (nvs_get_str(nvsROHandle, GetWiFiConfigKey(source, "WiFi_password").c_str(), szBuffer, &len) != ESP_OK)
        {
            nvs_close(nvsROHandle);
            return false;
        }
        WiFi_password = szBuffer;
        nvs_close(nvsROHandle);
        return true;
    }

    // WriteWiFiConfig
    //
    // Attempts to write the WiFi ssid and password to NVS storage strings.  The keys
    // for those name-value pairs are made from the variable names (WiFi_ssid, WiFi_Password)
    // directly.  It's not transactional, so it could conceivably succeed at writing
    // the ssid and not the password (but will still report failure).  Does not
    // enforce length limits on values given, so conceivable you could write longer
    // pairs than you could read, but they wouldn't work on WiFi anyway.
    bool WriteWiFiConfig(WifiCredSource source, const String &WiFi_ssid, const String &WiFi_password)
    {
        nvs_handle_t nvsRWHandle;
        if (nvs_open("storage", NVS_READWRITE, &nvsRWHandle) != ESP_OK) return false;

        bool success = (nvs_set_str(nvsRWHandle, GetWiFiConfigKey(source, "WiFi_ssid").c_str(), WiFi_ssid.c_str()) == ESP_OK) &&
                       (nvs_set_str(nvsRWHandle, GetWiFiConfigKey(source, "WiFi_password").c_str(), WiFi_password.c_str()) == ESP_OK);

        if (success) nvs_commit(nvsRWHandle);
        nvs_close(nvsRWHandle);
        return success;
    }

    bool IsProvisioningActive() { return l_ProvisioningActive.load(); }
    void SetProvisioningActive(bool active) { l_ProvisioningActive.store(active); }
    int  GetLastWiFiDisconnectReason() { return l_LastWiFiDisconnectReason.load(); }
    void ClearLastWiFiDisconnectReason() { l_LastWiFiDisconnectReason.store(0); }

    // ClearWiFiConfig
    //
    // Attempts to erase the WiFi ssid and password for a given source from NVS
    // storage. The keys for the name-value pairs are constructed based on the
    // source. This operation is not transactional; it's possible for one key
    // to be erased successfully while the other fails.
    bool ClearWiFiConfig(WifiCredSource source)
    {
        nvs_handle_t nvsRWHandle;
        if (nvs_open("storage", NVS_READWRITE, &nvsRWHandle) != ESP_OK) return false;

        // Don't allow shortcut operation here. erase BOTH keys, even
        // if first one errors.
        bool success = true;
        esp_err_t err = nvs_erase_key(nvsRWHandle, GetWiFiConfigKey(source, "WiFi_ssid").c_str());
        if (err != ESP_OK && err != ESP_ERR_NVS_NOT_FOUND) {
            success = false;
        }

        err = nvs_erase_key(nvsRWHandle, GetWiFiConfigKey(source, "WiFi_password").c_str());
        if (err != ESP_OK && err != ESP_ERR_NVS_NOT_FOUND) {
            success = false;
        }

        if (success) nvs_commit(nvsRWHandle);
        nvs_close(nvsRWHandle);
        return success;
    }

    // NetworkReader Implementation

    size_t NetworkReader::RegisterReader(const std::function<void()> &reader, unsigned long interval, bool flag)
    {
        auto entry = std::make_shared<ReaderEntry>(reader, interval);
        readers.push_back(entry);
        if (interval) entry->lastReadMs.store(millis());
        if (flag) FlagReader(readers.size() - 1);
        return readers.size() - 1;
    }

    void NetworkReader::FlagReader(size_t index)
    {
        if (index < readers.size())
        {
            readers[index]->flag.store(true);
            g_ptrSystem->GetTaskManager().NotifyNetworkThread();
        }
    }

    void NetworkReader::CancelReader(size_t index)
    {
        if (index < readers.size())
        {
            auto &entry = *readers[index];
            entry.canceled.store(true);
            entry.readInterval.store(0);
            entry.reader = nullptr;
        }
    }

    // NetworkHandlingLoopEntry
    //
    // Thead entry point for the Networking task
    // Pumps the various network loops and sets the time periodically, as well as reconnecting
    // to WiFi if the connection drops.  Also pumps the OTA (Over the air updates) loop.
    void NetworkHandlingLoopEntry(void *)
    {
        static unsigned long lastConnected = millis();
        static unsigned long lastWebSocketCleanup = 0;
        if (!MDNS.begin("esp32")) Serial.println("Error starting mDNS");

        TickType_t notifyWait = 0;
        for (;;)
        {
            ulTaskNotifyTake(pdTRUE, notifyWait);
            EVERY_N_SECONDS(1)
            {
                // While Improv is actively provisioning, it owns the WiFi
                // state machine. Skipping our reconnect retry here prevents
                // both code paths from concurrently calling WiFi.disconnect /
                // WiFi.begin and confusing the ESP-IDF driver.
                if (!l_ProvisioningActive.load())
                {
                    auto res = ConnectToWiFi();
                    if (res == WiFiConnectResult::Connected)
                    {
                        lastConnected = millis();
                        #if WEB_SOCKETS_ANY_ENABLED
                            // AsyncWebSocket client cleanup is useful, but doing it every second while
                            // frame preview traffic is active can churn otherwise healthy clients. Keep
                            // it periodic and infrequent so it only reaps genuinely stale connections.
                            if (millis() - lastWebSocketCleanup >= 15000)
                            {
                                lastWebSocketCleanup = millis();
                                g_ptrSystem->GetWebSocketServer().CleanupClients();
                            }
                        #endif
                    }
                    else
                    {
                        #if WAIT_FOR_WIFI
                            if (res != WiFiConnectResult::NoCredentials && (millis() - lastConnected) > WIFI_WAIT_MAX)
                            {
                                debugE("Rebooting due to no Wifi.");
                                delay(5000);
                                ESP.restart();
                            }
                        #endif
                    }
                }
            }

            if (!g_ptrSystem->HasNetworkReader() || !WiFi.isConnected())
            {
                notifyWait = pdMS_TO_TICKS(1000);
                continue;
            }

            auto &networkReader = g_ptrSystem->GetNetworkReader();
            unsigned long now = millis();
            unsigned long nextEventMs = 1000;

            for (auto &entryPtr : networkReader.readers)
            {
                auto &entry = *entryPtr;
                if (entry.canceled.load()) continue;

                unsigned long interval = entry.readInterval.load();
                if (interval)
                {
                    unsigned long target = entry.lastReadMs.load() + interval;
                    if (target <= now || (now < entry.lastReadMs.load()))
                        entry.flag.store(true);

                    unsigned long remaining = (target > now) ? (target - now) : 0;
                    if (remaining < nextEventMs) nextEventMs = remaining;
                }

                if (entry.flag.exchange(false))
                {
                    entry.reader();
                    entry.lastReadMs.store(millis());
                }
            }
            notifyWait = pdMS_TO_TICKS(nextEventMs);
        }
    }

    void DoStatsCommand(const DebugCLI::cli_argv &)
    {
        auto &bufferManager = g_ptrSystem->GetBufferManagers()[0];
        const auto& config = g_ptrSystem->GetDeviceConfig();

        DebugCLI::cli_printf("%s:%zux%zu %zuK %ddB:%s",
            FLASH_VERSION_NAME, g_ptrSystem->GetDevices().size(),
            config.GetActiveLEDCount(), (size_t)(ESP.getFreeHeap()/1024), abs(WiFi.RSSI()),
            WiFi.isConnected() ? WiFi.localIP().toString().c_str() : "None");
        DebugCLI::cli_printf("BUFR:%02zu/%02zu [%lufps]",
            (size_t)bufferManager.Depth(), (size_t)bufferManager.BufferCount(),
            (unsigned long)g_Values.FPS);
        DebugCLI::cli_printf("DATA:%+04.2f-%+04.2f",
            (float)bufferManager.AgeOfOldestBuffer(), (float)bufferManager.AgeOfNewestBuffer());

        #if ENABLE_AUDIO
        DebugCLI::cli_printf("g_Analyzer._VU: %.2f, g_Analyzer._MinVU: %.2f, g_Analyzer._PeakVU: %.2f, g_Analyzer.gVURatio: %.2f",
            g_Analyzer.VU(), g_Analyzer.MinVU(), g_Analyzer.PeakVU(), g_Analyzer.VURatio());
        #endif

        #if INCOMING_WIFI_ENABLED
        DebugCLI::cli_printf("Socket Buffer _cbReceived: %zu", g_ptrSystem->GetSocketServer()._cbReceived);
        #endif
    }

    void InitNetworkCLI()
    {
        static const DebugCLI::command cmds[] = {
            #if ENABLE_NTP
            { "clock", "Refresh time from server", "Refreshing Time from Server",
                [](const DebugCLI::cli_argv &) { NTPTimeClient::UpdateClockFromWeb(&l_Udp); }
            },
            #endif
            { "stats", "Display system statistics", "Displaying statistics",
                DoStatsCommand
            }
        };
        DebugCLI::RegisterCommands(cmds, std::size(cmds));
    }

#else

    // Stub WiFi Implementations (Enabled-independent API)

    bool IsWiFiConnected() { return false; }
    int  GetWiFiStatus()    { return 0; }
    int  GetWiFiRSSI()      { return 0; }
    void SetWiFiModeSTA()   {}

    String GetWiFiLocalIP() { return "0.0.0.0"; }

    bool GetWiFiHostByName(const char *, IPAddress &) { return false; }

    WiFiConnectResult ConnectToWiFi(const String &, const String &) { return WiFiConnectResult::NoCredentials; }
    WiFiConnectResult ConnectToWiFi(const String *, const String *) { return WiFiConnectResult::NoCredentials; }

    void UpdateNTPTime() {}
    bool ReadWiFiConfig(WifiCredSource, String &, String &) { return false; }
    bool WriteWiFiConfig(WifiCredSource, const String &, const String &) { return false; }
    bool ClearWiFiConfig(WifiCredSource) { return false; }

    bool IsProvisioningActive()            { return false; }
    void SetProvisioningActive(bool)       {}
    int  GetLastWiFiDisconnectReason()     { return 0; }
    void ClearLastWiFiDisconnectReason()   {}

    void NetworkHandlingLoopEntry(void *) { for (;;) delay(1000); }
    void InitNetworkCLI() {}

#endif // ENABLE_WIFI

} // namespace nd_network

// Global Support Helpers

#if ENABLE_ESPNOW
void onReceiveESPNOW(const uint8_t *macAddr, const uint8_t *data, int dataLen)
{
    struct Message { uint8_t cbSize; uint8_t command; uint32_t arg1; } __attribute__((packed));
    Message message;
    if (dataLen < sizeof(message)) return;
    memcpy(&message, data, sizeof(message));
    switch (message.command)
    {
        case 1: g_ptrSystem->GetEffectManager().NextEffect(); break;
        case 2: g_ptrSystem->GetEffectManager().PreviousEffect(); break;
        case 3: g_ptrSystem->GetEffectManager().SetCurrentEffectIndex(message.arg1); break;
    }
}
#endif

#if ENABLE_REMOTE
// RemoteLoopEntry
//
// If enabled, this is the main thread loop for the remote control.  It is initialized and then
// called once every 20ms to pump its work queue and scan for new remote codes, etc.  If no
// remote is being used, this code and thread doesn't exist in the build.
void IRAM_ATTR RemoteLoopEntry(void *)
{
    auto &rc = g_ptrSystem->GetRemoteControl();
    rc.begin();
    while (true) { rc.handle(); delay(20); }
}
#endif

String urlEncode(const String &str)
{
    String encoded = "";
    for (int i = 0; i < str.length(); i++)
    {
        char c = str.charAt(i);
        if (isalnum(c)) encoded += c;
        else {
            encoded += '%';
            encoded += str_sprintf("%02X", (uint8_t)c);
        }
    }
    return encoded;
}

// SetupOTA
//
// Set up the over-the-air programming info so that we can be flashed over WiFi
void SetupOTA(const String &strHostname)
{
#if ENABLE_OTA
    ArduinoOTA.setRebootOnSuccess(true);
    if (!strHostname.isEmpty())
        ArduinoOTA.setHostname(strHostname.c_str());
    else
        ArduinoOTA.setMdnsEnabled(false);

    ArduinoOTA.onStart([]() {
        g_Values.UpdateStarted = true;

        debugI("Start updating from OTA %s",
            ArduinoOTA.getCommand() == U_FLASH ? "sketch" : "filesystem");

        #if ENABLE_REMOTE
            g_ptrSystem->GetRemoteControl().end();
        #endif
    }).onEnd([]() {
        debugI("\nEnd OTA");
        g_Values.UpdateStarted = false;
    }).onProgress([](unsigned int progress, unsigned int total) {
        static uint last_time = millis();
        if (millis() - last_time > 1000)
        {
            last_time = millis();
            auto p = (progress / (total / 100));
            debugI("OTA Progress: %u%%\r", p);

            #if USE_HUB75
                auto pMatrix = std::static_pointer_cast<HUB75GFX>(g_ptrSystem->GetEffectManager().GetBaseGraphics()[0]);
                pMatrix->SetCaption(str_sprintf("Update:%d%%", p), CAPTION_TIME);
            #endif
        }
        else
        {
            debugV("OTA Progress: %u%%\r", (progress / (total / 100)));
        }
    }).onError([](ota_error_t error) {
        g_Values.UpdateStarted = false;
        debugE("OTA Error [%u]: ", error);

        if (error == OTA_AUTH_ERROR) debugW("Auth Failed");
        else if (error == OTA_BEGIN_ERROR) debugW("Begin Failed");
        else if (error == OTA_CONNECT_ERROR) debugW("Connect Failed");
        else if (error == OTA_RECEIVE_ERROR) debugW("Receive Failed");
        else if (error == OTA_END_ERROR) debugW("End Failed");
        throw std::runtime_error("OTA Flash update failed.");
    });
    ArduinoOTA.begin();
#endif
}

void ConfirmUpdate()
{
#if ENABLE_OTA
    esp_ota_mark_app_valid_cancel_rollback();
#endif
}

#if ENABLE_WIFI
// ProcessIncomingData
//
// Code that actually handles whatever comes in on the socket.  Must be known good data
// as this code does not validate!  This is where the commands and pixel data are received
// from the server.
bool ProcessIncomingData(std::unique_ptr<uint8_t[]> &payloadData, size_t payloadLength)
{
    #if !INCOMING_WIFI_ENABLED
        return false;
    #else
        uint16_t command16 = payloadData[1] << 8 | payloadData[0];

        debugV("payloadLength: %zu, command16: %d", payloadLength, command16);

        switch (command16)
        {
            // WIFI_COMMAND_PEAKDATA has a header plus NUM_BANDS floats that
            // will be used to set the audio peaks
            case WIFI_COMMAND_PEAKDATA:
            {
                #if ENABLE_AUDIO
                    uint16_t numbands  = WORDFromMemory(&payloadData[2]);
                    uint32_t length32  = DWORDFromMemory(&payloadData[4]);
                    uint64_t seconds   = ULONGFromMemory(&payloadData[8]);
                    uint64_t micros    = ULONGFromMemory(&payloadData[16]);

                    debugV("ProcessIncomingData -- Bands: %u, Length: %lu, Seconds: %llu, Micros: %llu ... ",
                           (unsigned int)numbands, (unsigned long)length32, seconds, micros);

                    // Data is transmitted as NUM_BANDS floats following the standard header
                    const uint8_t* dataStart = payloadData.get() + STANDARD_DATA_HEADER_SIZE;
                    const size_t availableFloats = (payloadLength > STANDARD_DATA_HEADER_SIZE)
                                                    ? (payloadLength - STANDARD_DATA_HEADER_SIZE) / sizeof(float)
                                                    : 0;
                    const size_t copyCount = std::min<size_t>(NUM_BANDS, std::min<size_t>(numbands, availableFloats));

                    PeakData peaks{}; // zero-initialized for any bands not provided
                    if (copyCount > 0)
                    {
                        auto pFloats = reinterpret_cast<const float *>(dataStart);
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
                       (unsigned int)channel16, (unsigned long)length32, seconds, micros);

                // The very old original implementation used channel numbers, not a mask, and only channel 0 was supported at that time, so if
                // we see a Channel 0 asked for, it must be very old, and we massage it into the mask for Channel0 instead
                // Another option here would be to draw on all channels (0xff) instead of just one (0x01) if 0 is specified
                if (channel16 == 0)
                    channel16 = 1;

                // Go through the channel mask to see which bits are set in the channel16 specifier, and send the data to each and every
                // channel that matches the mask.  So if the send channel 7, that means the lowest 3 channels will be set.
                std::lock_guard<std::mutex> guard(g_buffer_mutex);

                for (int iChannel = 0, channelMask = 1; iChannel < g_ptrSystem->GetBufferManagers().size(); iChannel++, channelMask <<= 1)
                {
                    if ((channelMask & channel16) != 0)
                    {
                        debugV("Processing for Channel %d", iChannel);

                        bool bDone = false;
                        auto &bufferManager = g_ptrSystem->GetBufferManagers()[iChannel];

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
            auto &socketServer = g_ptrSystem->GetSocketServer();

            socketServer.release();
            if (socketServer.begin())
            {
                socketServer.ProcessIncomingConnectionsLoop();
                debugV("Socket connection closed.  Retrying...");
            }
            else
            {
                debugE("Failed to start socket server, retrying in 5 seconds...");
                delay(5000);
            }
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
    constexpr uint32_t kPreviewMaxFps          = 30;
    constexpr uint32_t kPreviewFrameIntervalMs = 1000 / kPreviewMaxFps;

    LEDViewer _viewer(NetworkPort::ColorServer);
    int       socket = -1;
    bool      wsListenersPresent = false;
    BaseFrameEventListener frameEventListener;
    uint32_t lastPreviewSendMs = 0;
    std::unique_ptr<ColorDataPacket> previewPacket = std::make_unique<ColorDataPacket>();

    auto &effectManager = g_ptrSystem->GetEffectManager();
#if COLORDATA_WEB_SOCKET_ENABLED
    auto &webSocketServer = g_ptrSystem->GetWebSocketServer();
#endif

    effectManager.AddFrameEventListener(frameEventListener);

    for (;;)
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

        auto& graphics = effectManager.g();
        auto leds = graphics.leds;
        const auto activeLEDCount = graphics.GetLEDCount();

#if COLORDATA_WEB_SOCKET_ENABLED
        wsListenersPresent = webSocketServer.HaveColorDataClients();
#else
        wsListenersPresent = false;
#endif

        if (frameEventListener.CheckAndClearNewFrameAvailable() && leds != nullptr)
        {
            const auto previewActive = (socket >= 0) || wsListenersPresent;
            const auto now = millis();
            if (previewActive && now - lastPreviewSendMs >= kPreviewFrameIntervalMs)
            {
                lastPreviewSendMs = now;

                previewPacket->header = COLOR_DATA_PACKET_HEADER;
                previewPacket->width  = graphics.GetMatrixWidth();
                previewPacket->height = graphics.GetMatrixHeight();
                memcpy(previewPacket->colors, leds, sizeof(CRGB) * activeLEDCount);

                // Prefer the websocket preview transport used by the local UI. The legacy raw TCP
                // preview socket remains available for external tools, but we do not drive both
                // transports simultaneously for the same frame.
#if COLORDATA_WEB_SOCKET_ENABLED
                if (wsListenersPresent)
                {
                    webSocketServer.SendColorData(leds, activeLEDCount);
                }
                else
#endif
                if (socket >= 0)
                {
                    debugV("Sending color data packet");
                    const auto packetSize =
                        sizeof(previewPacket->header) +
                        sizeof(previewPacket->width) +
                        sizeof(previewPacket->height) +
                        sizeof(CRGB) * activeLEDCount;
                    const auto sendResult = _viewer.SendPacket(socket, previewPacket.get(), packetSize);
                    if (sendResult == LEDViewer::SendResult::Failed)
                    {
                        debugW("Error on color data socket, so closing");
                        close(socket);
                        socket = -1;
                    }
                }
            }
        }

        if (socket >= 0 || wsListenersPresent)
            delay(10);
        else
            delay(1000);
    }
}
#endif // COLORDATA_SERVER_ENABLED
#endif // ENABLE_WIFI
