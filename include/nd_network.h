#pragma once

//+--------------------------------------------------------------------------
//
// File:        nd_network.h
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
//    Network related functions and definitions
//
//---------------------------------------------------------------------------

#include "globals.h"
#include <atomic>
#include <functional>
#include <utility>
#include <vector>

#include <esp_arduino_version.h>
// Retire this test once Arduino3 fully lands.
#if ESP_ARDUINO_VERSION >= ESP_ARDUINO_VERSION_VAL(3, 0, 0)
   #include <Network.h> // For wl_status_t, etc.
#endif
#include <WiFi.h>
#include "esp_mac.h"
#include "types.h"

// NOTE: Do not include "socketserver.h" here. It pulls in "ledbuffer.h" -> "gfxbase.h",
// which uses debug macros defined by RemoteDebug. In Arduino v3, RemoteDebug includes
// WiFi.h, which includes Network.h; adding socketserver.h here creates a cycle where
// gfxbase.h is parsed before those macros exist. Keep socketserver.h in .cpp files.


    // For now, just a centralized location for the port numbers for our
    // various services. Someday these might be configurable.
    // This could be an enum class, but the static_cast<int> at the
    // callers is ugly.
    enum NetworkPort : int
    {
      ColorServer  = 12000,
      IncomingWiFi  = 49152,
      VICESocketServer = 25232,
      Telnet = 23,
      Webserver  = 80
    };

    // SetSocketBlockingEnabled
    //
    // In blocking mode, socket API calls wait until the operation is complete before returning control to the application.
    // In non-blocking mode, socket API calls return immediately. If an operation cannot be completed immediately, the function
    // returns an error (usually EWOULDBLOCK or EAGAIN).

    bool SetSocketBlockingEnabled(int fd, bool blocking);

#if ENABLE_WIFI
    enum class WiFiConnectResult
    {
      Connected,
      Disconnected,
      NoCredentials
    };

    enum WifiCredSource
    {
      ImprovCreds = 0,
      CompileTimeCreds = 1
    };

    WiFiConnectResult ConnectToWiFi(const String& ssid, const String& password);
    WiFiConnectResult ConnectToWiFi(const String* ssid, const String* password);
    void UpdateNTPTime();
    bool ReadWiFiConfig(WifiCredSource source, String& WiFi_ssid, String& WiFi_password);
    bool WriteWiFiConfig(WifiCredSource source, const String& WiFi_ssid, const String& WiFi_password);
    bool ClearWiFiConfig(WifiCredSource source);

    // Static Helpers
    //
    // Simple utility functions

    #define WL_NO_SHIELD        "WL_NO_SHIELD"
    #define WL_IDLE_STATUS      "WL_IDLE_STATUS"
    #define WL_NO_SSID_AVAIL    "WL_NO_SSID_AVAIL"
    #define WL_SCAN_COMPLETED   "WL_SCAN_COMPLETED"
    #define WL_CONNECTED        "WL_CONNECTED"
    #define WL_CONNECT_FAILED   "WL_CONNECT_FAILED"
    #define WL_CONNECTION_LOST  "WL_CONNECTION_LOST"
    #define WL_DISCONNECTED     "WL_DISCONNECTED"
    #define WL_UNKNOWN_STATUS   "WL_UNKNOWN_STATUS"

    const char* WLtoString(wl_status_t status);

    // get_mac_address_raw
    //
    // Reads the raw MAC

    void get_mac_address_raw(uint8_t *mac);

    // get_mac_address
    //
    // Returns a packed (non-pretty, without colons) version of the MAC id

    String get_mac_address();
    String get_mac_address_pretty();

    // SetSocketBlockingEnabled
    //
    // In blocking mode, socket API calls wait until the operation is complete before returning control to the application.
    // In non-blocking mode, socket API calls return immediately. If an operation cannot be completed immediately, the function
    // returns an error (usually EWOULDBLOCK or EAGAIN).

    bool SetSocketBlockingEnabled(int fd, bool blocking);

    // NetworkReader
    //
    // Allows functions to be registered that are called at regular intervals and/or on request, in the
    // background. As the name of the class implies, this is intended to be used to execute network
    // requests, like for effects that require data from RESTful APIs.

    class NetworkReader
    {
      // We allow the main network task entry point function to access private members
      friend void NetworkHandlingLoopEntry(void *);

    public:
      struct ReaderEntry;

    private:
      std::vector<std::shared_ptr<ReaderEntry>> readers;

    public:

      // Add a reader to the collection. Returns the index of the added reader, for use with FlagReader().
      //   Note that if an interval (in ms) is specified, the reader will run for the first time after
      //   the interval has passed, unless "true" is passed to the last parameter.
      size_t RegisterReader(const std::function<void()>& reader, unsigned long interval = 0, bool flag = false);

      // Flag a reader for invocation and wake up the task that calls them
      void FlagReader(size_t index);

      // Cancel a reader. After this, it will no longer be invoked.
      void CancelReader(size_t index);
  };
#endif

  void InitNetworkCLI();
