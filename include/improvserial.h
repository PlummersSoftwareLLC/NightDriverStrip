//+--------------------------------------------------------------------------
//
// File:        improvserial.h
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
//
// Description:
//
//   Wraps ImprovSerial to manage its state and provide an interface to it
//
// History:     Oct-9-2018         Davepl      File based on ESPHome GPLv3
//                     https://github.com/esphome/esphome/blob/dev/LICENSE
//
//---------------------------------------------------------------------------

#pragma once

#include <improv.h>

enum ImprovSerialType : uint8_t 
{
  TYPE_CURRENT_STATE = 0x01,
  TYPE_ERROR_STATE   = 0x02,
  TYPE_RPC           = 0x03,
  TYPE_RPC_RESPONSE  = 0x04
};

static const uint8_t IMPROV_SERIAL_VERSION = 1;

class ImprovSerial 
{
 public:

  void setup(const String &firmware, const String &version, const String &variant, const String &name,HardwareSerial *serial = &Serial);
  bool loop(bool timeout = false);
  improv::State get_state();
  String get_ssid();
  String get_password();

 protected:
 
  bool parse_improv_serial_byte_(uint8_t byte);
  bool parse_improv_payload_(improv::ImprovCommand &command);

  void set_state_(improv::State state);
  void set_error_(improv::Error error);
  void send_response_(std::vector<uint8_t> &response);
  void on_wifi_connect_timeout_();

  std::vector<uint8_t> build_rpc_settings_response_(improv::Command command);
  std::vector<uint8_t> build_version_info_();

  int available_();
  uint8_t read_byte_();
  void write_data_(std::vector<uint8_t> &data);

  HardwareSerial *hw_serial_{nullptr};

  std::vector<uint8_t> rx_buffer_;
  uint32_t last_read_byte_{0};
  improv::State state_{improv::STATE_AUTHORIZED};
  improv::ImprovCommand command_{improv::Command::UNKNOWN, "", ""};

  String firmware_name_;
  String firmware_version_;
  String hardware_variant_;
  String device_name_;
};

extern ImprovSerial g_ImprovSerial;  
extern String       WiFi_ssid;
extern String       WiFi_password;
bool ReadWiFiConfig();
bool WriteWiFiConfig();
bool ConnectToWiFi(uint cRetries);
