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
#include "esp_mac.h"
#include "types.h"

// Prototypes that should exist regardless of ENABLE_WIFI to keep callers clean
void GetMacAddressRaw(uint8_t *mac);
void InitNetworkCLI();
bool IsWiFiConnected();
void NetworkHandlingLoopEntry(void *);
bool SetSocketBlockingEnabled(int fd, bool blocking);


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

#include <Arduino.h>
#include <IPAddress.h>
#include <WString.h>

String GetMacAddress();
String GetMacAddressPretty();
bool GetWiFiHostByName(const char* hostname, IPAddress& ip);
String GetWiFiLocalIP();
int GetWiFiRSSI();
int GetWiFiStatus();
void SetWiFiModeSTA();
const char* WLtoString(int status);

#if ENABLE_WIFI
#include <atomic>
#include <functional>
#include <memory>
#include <utility>
#include <vector>

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

