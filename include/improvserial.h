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
#include "hexdump.h"

#define IMPROV_LOG_FILE             "/improv.log"

// Define as 1 to enable Improv logging to SPIFFS, and add a URI to the on-board
// webserver to be able to retrieve it. The URL to retrieve the log will be
// http://<device_IP><IMPROV_LOG_FILE>, the latter being as defined just above.
// Note that any log file that has been written to SPIFFS will be deleted as soon
// as the board is booted with ENABLE_IMPROV_LOGGING set to 0!
#ifndef ENABLE_IMPROV_LOGGING
    #define ENABLE_IMPROV_LOGGING   0
#endif

enum ImprovSerialType : uint8_t
{
    TYPE_CURRENT_STATE = 0x01,
    TYPE_ERROR_STATE = 0x02,
    TYPE_RPC = 0x03,
    TYPE_RPC_RESPONSE = 0x04
};

static const uint8_t IMPROV_SERIAL_VERSION = 1;

extern DRAM_ATTR String WiFi_ssid;
extern DRAM_ATTR String WiFi_password;
bool ReadWiFiConfig();
bool WriteWiFiConfig();

template <typename SERIALTYPE>
class ImprovSerial
{
    #if !(ENABLE_IMPROV_LOGGING)
        #define log_write(...) do {} while(0)
    #endif

public:

    void setup(const String &firmware,
                             const String &version,
                             const String &variant,
                             const String &name,
                             SERIALTYPE *serial)
    {
        this->hw_serial_ = serial;
        this->firmware_name_ = firmware;
        this->firmware_version_ = version;
        this->hardware_variant_ = variant;
        this->device_name_ = name;

        #if !(ENABLE_IMPROV_LOGGING)
            SPIFFS.remove(IMPROV_LOG_FILE);
        #endif

        if (WiFi.getMode() == WIFI_STA && WiFi.isConnected())
            this->state_ = improv::STATE_PROVISIONED;
        else
            this->state_ = improv::STATE_AUTHORIZED;

        log_write("Settings ssid=\"%s\", password=******", WiFi_ssid.c_str());
    }

    // Main ImprovSerial loop.  Pulls available characters from the serial port, and tries to have them parsed
    // into IMPROV commands.  Periodically checks the provisioning state, and if in process, checks to see if
    // the WiFi is now connected.  If so, it updates the state to PROVISIONED and returns a WIFI_SETTINGS response.

    void loop()
    {
        const uint32_t now = millis();
        if (now - this->last_read_byte_ > 50)
        {
            this->rx_buffer_.clear();
            this->last_read_byte_ = now;
        }

        while (this->available_())
        {
            uint8_t byte = this->read_byte_();
            if (this->parse_improv_serial_byte_(byte))
                this->last_read_byte_ = now;
            else
            {
                log_write("Abandoning Improv buffer after %d received bytes:", this->rx_buffer_.size());
                log_write(this->rx_buffer_);
                this->rx_buffer_.clear();
            }
        }

        if (this->state_ == improv::STATE_PROVISIONING)
        {
            if (WiFi.getMode() == WIFI_AP || (WiFi.getMode() == WIFI_STA && WiFi.isConnected()))
            {
                this->set_state_(improv::STATE_PROVISIONED);
                std::vector<uint8_t> url = this->build_rpc_settings_response_(improv::WIFI_SETTINGS);
                this->send_response_(url);
            }
        }
    }

    improv::State get_state()
    {
        return this->state_;
    }

    String get_ssid()
    {
        return String(this->command_.ssid.c_str());
    }

    String get_password()
    {
        return String(this->command_.password.c_str());
    }

protected:

    #if ENABLE_IMPROV_LOGGING

        // Tell the compiler the arguments to this overload should be checked like printf's
        __attribute__((format(printf, 2, 3)))
        void log_write(const char* format, ...)
        {
            auto file = SPIFFS.open(IMPROV_LOG_FILE, FILE_APPEND);
            va_list args;

            va_start(args, format);

            // We shifted the printf parameter check to ourselves, so we can suppress the warning here
            #pragma GCC diagnostic ignored "-Wformat-nonliteral"
            file.printf(format, args);
            #pragma GCC diagnostic warning "-Wformat-nonliteral"

            va_end(args);

            file.println();

            file.close();
        }

        void log_write(std::vector<uint8_t>& data)
        {
            auto file = SPIFFS.open(IMPROV_LOG_FILE, FILE_APPEND);

            HexDump(file, data.data(), data.size());

            file.close();
        }

    #endif // ENABLE_IMPROV_LOGGING

    bool parse_improv_serial_byte_(uint8_t byte)
    {
        size_t at = this->rx_buffer_.size();
        this->rx_buffer_.push_back(byte);

        // Checks the bytestream to see if we're still seeing what looks like the IMPROV header
        // There are many more elegant and less readable ways to do this, but... let's keep it simple.

        const uint8_t *raw = &this->rx_buffer_[0];
        if (at == 0)
            return byte == 'I';
        if (at == 1)
            return byte == 'M';
        if (at == 2)
            return byte == 'P';
        if (at == 3)
            return byte == 'R';
        if (at == 4)
            return byte == 'O';
        if (at == 5)
            return byte == 'V';

        if (at == 6)
            return byte == IMPROV_SERIAL_VERSION;

        if (at == 7)
            return true;

        uint8_t type = raw[7];

        if (at == 8)
            return true;

        uint8_t data_len = raw[8];

        if (at < 8 + data_len)
            return true;

        if (at == 8 + data_len)
            return true;

        // THe last byte of the packet needs to be a checksum, so compute it and stuff it in

        if (at == 8 + data_len + 1)
        {
            uint8_t checksum = 0x00;
            for (uint8_t i = 0; i < at; i++)
                checksum += raw[i];

            if (checksum != byte)
            {
                log_write("Checksum mismatch in Improv payload. Expected 0x%x. Got 0x%x", checksum, byte);
                this->set_error_(improv::ERROR_INVALID_RPC);
                return false;
            }

            if (type == TYPE_RPC)
            {
                this->set_error_(improv::ERROR_NONE);
                auto command = improv::parse_improv_data(&raw[9], data_len, false);
                return this->parse_improv_payload_(command);
            }
        }

        // If we got here then the command coming is improv, but not an RPC command
        log_write("Improv command not RPC, so not handled.");

        return false;
    }

    bool parse_improv_payload_(improv::ImprovCommand &command)
    {
        switch (command.command)
        {
            // When a "Set the WiFi" RPC call comes in we save the credentials to
            //   NVS, then disconnect and reconnect the WiFi using whatever creds
            //   were supplied.  Returns before the connection is complete, as it
            //   sets the state to PROVISIONING so the remote caller can check
            //   back on the status to see progress

            case improv::WIFI_SETTINGS:
            {
                WiFi_ssid = command.ssid.c_str();
                WiFi_password = command.password.c_str();

                if (!WriteWiFiConfig())
                    debugI("Failed writing WiFi config to NVS");

                log_write("Received Improv wifi settings ssid=\"%s\", password=******", command.ssid.c_str());

                WiFi.disconnect();
                WiFi.mode(WIFI_STA);
                WiFi.begin(WiFi_ssid.c_str(), WiFi_password.c_str());
                this->set_state_(improv::STATE_PROVISIONING);

                this->command_.command  = command.command;
                this->command_.ssid     = command.ssid;
                this->command_.password = command.password;

                return true;
            }

                // Return the current state of the WiFi setup:  authorized, provisioning, or provisioned

            case improv::GET_CURRENT_STATE:
            {
                this->set_state_(this->state_);
                if (this->state_ == improv::STATE_PROVISIONED)
                {
                    std::vector<uint8_t> url = this->build_rpc_settings_response_(improv::GET_CURRENT_STATE);
                    this->send_response_(url);
                }
                return true;
            }

                // Return info about the ESP32 itself

            case improv::GET_DEVICE_INFO:
            {
                std::vector<uint8_t> info = this->build_version_info_();
                this->send_response_(info);
                return true;
            }

            case improv::GET_WIFI_NETWORKS:
            {
                auto numSsid = WiFi.scanNetworks();
                if (numSsid > 0)
                {
                    for (int i = 0; i < numSsid; i++)
                    {
                        // Send each ssid separately to avoid overflowing the buffer
                        std::vector<uint8_t> data = improv::build_rpc_response(
                            improv::GET_WIFI_NETWORKS, {WiFi.SSID(i), str_sprintf("%d", WiFi.RSSI(i)), WiFi.encryptionType(i) != WIFI_AUTH_OPEN ? "YES" : "NO"}, false);
                        this->send_response_(data);
                    }
                }

                // Send empty response to signify the end of the list.

                std::vector<uint8_t> data = improv::build_rpc_response(improv::GET_WIFI_NETWORKS, std::vector<String>{}, false);
                this->send_response_(data);
                return true;
            }
            default:
            {
                log_write("Unknown Improv payload. 0x%x", command.command);
                this->set_error_(improv::ERROR_UNKNOWN_RPC);
                return false;
            }
        }
    }
    // Allows the remote caller to set the provisioning state

    void set_state_(improv::State state)
    {
        this->state_ = state;

        std::vector<uint8_t> data = {'I', 'M', 'P', 'R', 'O', 'V'};
        data.resize(11);
        data[6] = IMPROV_SERIAL_VERSION;
        data[7] = TYPE_CURRENT_STATE;
        data[8] = 1;
        data[9] = state;

        uint8_t checksum = 0x00;
        for (uint8_t d : data)
            checksum += d;
        data[10] = checksum;

        this->write_data_(data);
    }

    // Allows the caller to inform us that an error has occured

    void set_error_(improv::Error error)
    {
        std::vector<uint8_t> data = {'I', 'M', 'P', 'R', 'O', 'V'};
        data.resize(11);
        data[6] = IMPROV_SERIAL_VERSION;
        data[7] = TYPE_ERROR_STATE;
        data[8] = 1;
        data[9] = error;

        log_write("Improv Serial received an error condition from the caller: %d", error);

        uint8_t checksum = 0x00;
        for (uint8_t d : data)
            checksum += d;
        data[10] = checksum;
        this->write_data_(data);
    }

    void send_response_(std::vector<uint8_t> &response)
    {
        std::vector<uint8_t> data = {'I', 'M', 'P', 'R', 'O', 'V'};
        data.resize(9);
        data[6] = IMPROV_SERIAL_VERSION;
        data[7] = TYPE_RPC_RESPONSE;
        data[8] = response.size();
        data.insert(data.end(), response.begin(), response.end());

        uint8_t checksum = 0x00;
        for (uint8_t d : data)
            checksum += d;
        data.push_back(checksum);

        this->write_data_(data);
    }

    void on_wifi_connect_timeout_()
    {
        this->set_error_(improv::ERROR_UNABLE_TO_CONNECT);
        this->set_state_(improv::STATE_AUTHORIZED);
        log_write("Timed out trying to connect to given WiFi network.");
        WiFi.disconnect();
    }

    std::vector<uint8_t> build_rpc_settings_response_(improv::Command command)
    {
        std::vector<String> urls;

        String webserver_url = String("http://") + WiFi.localIP().toString();
        urls.push_back(webserver_url);
        std::vector<uint8_t> data = improv::build_rpc_response(command, urls, false);

        return data;
    }

    std::vector<uint8_t> build_version_info_()
    {
        std::vector<String> infos = {this->firmware_name_, this->firmware_version_, this->hardware_variant_, this->device_name_};
        std::vector<uint8_t> data = improv::build_rpc_response(improv::GET_DEVICE_INFO, infos, false);
        return data;
    };

    int available_()
    {
        return this->hw_serial_->available();
    }

    uint8_t read_byte_()
    {
        uint8_t data;
        this->hw_serial_->readBytes(&data, 1);
        return data;
    }

    void write_data_(std::vector<uint8_t> &data)
    {
        data.push_back('\n');

        log_write("Sending Improv response:");
        log_write(data);

        this->hw_serial_->write(data.data(), data.size());
    }

    SERIALTYPE *hw_serial_ = nullptr;

    std::vector<uint8_t> rx_buffer_;
    uint32_t last_read_byte_{0};
    improv::State state_{improv::STATE_AUTHORIZED};
    improv::ImprovCommand command_{improv::Command::UNKNOWN, "", ""};

    String firmware_name_;
    String firmware_version_;
    String hardware_variant_;
    String device_name_;

    #if !(ENABLE_IMPROV_LOGGING)
        #undef log_write
    #endif
};

extern ImprovSerial<typeof(Serial)> g_ImprovSerial;
