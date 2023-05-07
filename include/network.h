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

#include "secrets.h"                          // copy include/secrets.example.h to include/secrets.h

#if INCOMING_WIFI_ENABLED
    #include "socketserver.h"
    extern SocketServer g_SocketServer;
#endif

#if ENABLE_WIFI
    extern uint8_t g_Brightness;
    extern bool    g_bUpdateStarted;
    extern WiFiUDP g_Udp;
    void processRemoteDebugCmd();

    bool ConnectToWiFi(uint cRetries);
    void SetupOTA(const String & strHostname);
    bool ReadWiFiConfig();
    bool WriteWiFiConfig();
    extern String WiFi_password;
    extern String WiFi_ssid;

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

    inline static const char* WLtoString(wl_status_t status) {
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



#endif
