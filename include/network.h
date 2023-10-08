//+--------------------------------------------------------------------------
//
// File:        network.h
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
//    Network related functions and definitons
//
//---------------------------------------------------------------------------
#pragma once

#include "types.h"

#if INCOMING_WIFI_ENABLED
    #include "socketserver.h"
#endif

    extern DRAM_ATTR String WiFi_password;
    extern DRAM_ATTR String WiFi_ssid;
#if ENABLE_WIFI
    void processRemoteDebugCmd();

    bool ConnectToWiFi(uint cRetries, bool waitForCredentials);
    void UpdateNTPTime();
    void SetupOTA(const String & strHostname);
    bool ReadWiFiConfig();
    bool WriteWiFiConfig();
    extern DRAM_ATTR String WiFi_password;
    extern DRAM_ATTR String WiFi_ssid;

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

    inline static const char* WLtoString(wl_status_t status)
    {
      switch (status) {
        case 255: return WL_NO_SHIELD;
        case 0: return   WL_IDLE_STATUS;
        case 1: return   WL_NO_SSID_AVAIL;
        case 2: return   WL_SCAN_COMPLETED;
        case 3: return   WL_CONNECTED;
        case 4: return   WL_CONNECT_FAILED;
        case 5: return   WL_CONNECTION_LOST;
        case 6: return   WL_DISCONNECTED;
        default: return  WL_UNKNOWN_STATUS;
      }
    }

    // get_mac_address_raw
    //
    // Reads the raw MAC

    inline void get_mac_address_raw(uint8_t *mac)
    {
        esp_efuse_mac_get_default(mac);
    }

    // get_mac_address
    //
    // Returns a packed (non-pretty, without colons) version of the MAC id

    inline String get_mac_address()
    {
      uint8_t mac[6];
      WiFi.macAddress(mac);
      return str_sprintf("%02x%02x%02x%02x%02x%02x", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    }

    // get_mac_address_pretty()
    //
    // Returns a packed (non-pretty, without colons) version of the MAC id

    inline String get_mac_address_pretty()
    {
      uint8_t mac[6];
      WiFi.macAddress(mac);
      return str_sprintf("%02X:%02X:%02X:%02X:%02X:%02X", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    }

    // NetworkReader
    //
    // Allows functions to be registered that are called at regular intervals and/or on request, in the
    // background. As the name of the class implies, this is intended to be used to execute network
    // requests, like for effects that require data from RESTful APIs.

    class NetworkReader
    {
      // We allow the main network task entry point function to access private members
      friend void IRAM_ATTR NetworkHandlingLoopEntry(void *);

    private:

      // Writer function and flag combo
      struct ReaderEntry
      {
          std::function<void()> reader;
          std::atomic_ulong readInterval;
          std::atomic_ulong lastReadMs;
          std::atomic_bool flag = false;
          std::atomic_bool canceled = false;

          ReaderEntry(std::function<void()> reader, unsigned long interval) :
              reader(reader),
              readInterval(interval)
          {}

          ReaderEntry(std::function<void()> reader, unsigned long interval, unsigned long lastReadMs, bool flag, bool canceled) :
              reader(reader),
              readInterval(interval),
              lastReadMs(lastReadMs),
              flag(flag),
              canceled(canceled)
          {}

          ReaderEntry(ReaderEntry&& entry) : ReaderEntry(entry.reader, entry.readInterval, entry.lastReadMs, entry.flag, entry.canceled)
          {}
      };

      std::vector<ReaderEntry, psram_allocator<ReaderEntry>> readers;

    public:

      // Add a reader to the collection. Returns the index of the added reader, for use with FlagReader().
      //   Note that if an interval (in ms) is specified, the reader will run for the first time after
      //   the interval has passed, unless "true" is passed to the last parameter.
      size_t RegisterReader(std::function<void()> reader, unsigned long interval = 0, bool flag = false);

      // Flag a reader for invocation and wake up the task that calls them
      void FlagReader(size_t index);

      // Cancel a reader. After this, it will no longer be invoked.
      void CancelReader(size_t index);
  };

#endif
