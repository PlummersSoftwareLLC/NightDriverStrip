//+--------------------------------------------------------------------------
//
// File:        improv.h
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
//    Uses the improv namespace to call functions from that file. It
//    then sets up a callback function to be called when data is received
//    from the serial port. When data is received, the callback function parses
//    the data using parse_improv_serial_byte from the improv namespace and sends
//    a response using build_rpc_response from the improv namespace. The response
//    consists of the command type and a vector of strings.
//
// Description:
//
//   Basic IMPROV states and rpc functions
//
// History:     Oct-9-2018         Davepl      File based on (Apache 2.0 License)
//                                         https://github.com/improv-wifi/sdk-cpp
//
//---------------------------------------------------------------------------

#pragma once

#ifdef ARDUINO
#include <Arduino.h>
#endif // ARDUINO

#include <cstdint>
#include <functional>
#include <string>
#include <vector>

namespace improv
{

    static const char *const SERVICE_UUID      = "00467768-6228-2272-4663-277478268000";
    static const char *const STATUS_UUID       = "00467768-6228-2272-4663-277478268001";
    static const char *const ERROR_UUID        = "00467768-6228-2272-4663-277478268002";
    static const char *const RPC_COMMAND_UUID  = "00467768-6228-2272-4663-277478268003";
    static const char *const RPC_RESULT_UUID   = "00467768-6228-2272-4663-277478268004";
    static const char *const CAPABILITIES_UUID = "00467768-6228-2272-4663-277478268005";

    enum Error : uint8_t
    {
        ERROR_NONE                   = 0x00,
        ERROR_INVALID_RPC            = 0x01,
        ERROR_UNKNOWN_RPC            = 0x02,
        ERROR_UNABLE_TO_CONNECT      = 0x03,
        ERROR_NOT_AUTHORIZED         = 0x04,
        ERROR_UNKNOWN                = 0xFF,
    };

    enum State : uint8_t
    {
        STATE_STOPPED                = 0x00,
        STATE_AWAITING_AUTHORIZATION = 0x01,
        STATE_AUTHORIZED             = 0x02,
        STATE_PROVISIONING           = 0x03,
        STATE_PROVISIONED            = 0x04,
    };

    enum Command : uint8_t
    {
        UNKNOWN                     = 0x00,
        WIFI_SETTINGS               = 0x01,
        IDENTIFY                    = 0x02,
        GET_CURRENT_STATE           = 0x02,
        GET_DEVICE_INFO             = 0x03,
        GET_WIFI_NETWORKS           = 0x04,
        BAD_CHECKSUM                = 0xFF,
    };

    static const uint8_t CAPABILITY_IDENTIFY = 0x01;
    static const uint8_t IMPROV_SERIAL_VERSION = 1;

    enum ImprovSerialType : uint8_t
    {
        TYPE_CURRENT_STATE          = 0x01,
        TYPE_ERROR_STATE            = 0x02,
        TYPE_RPC                    = 0x03,
        TYPE_RPC_RESPONSE           = 0x04
    };

    struct ImprovCommand
    {
        Command command;
        std::string ssid;
        std::string password;
    };

    ImprovCommand parse_improv_data(const std::vector<uint8_t> &data, bool check_checksum = true);
    ImprovCommand parse_improv_data(const uint8_t *data, size_t length, bool check_checksum = true);

    std::vector<uint8_t> build_rpc_response(Command command, const std::vector<String> &datum, bool add_checksum = true);

} // namespace improv
