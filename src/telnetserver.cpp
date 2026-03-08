//+--------------------------------------------------------------------------
//
// File:        telnetserver.cpp
//
// NightDriverStrip - (c) 2026 Plummer's Software LLC.  All Rights Reserved.
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
//    BSD-socket based Telnet server for remote console access.
//
//    Telnet protocol handling (RFC 854):
//      - Negotiates WILL ECHO so the client suppresses its local echo
//        (we echo characters back ourselves as they are typed).
//      - Negotiates WILL SGA + DO SGA to enable character-at-a-time mode
//        (suppress go-ahead, disabling Telnet's line-buffered default).
//      - Filters IAC command sequences out of the data stream.
//      - Translates RFC 854 CR-NUL (bare CR) and CR-LF to a single '\r'
//        for ProcessCLIByte, which then handles command dispatch.
//        The state machine survives across recv() boundaries so a split
//        \r\0 pair arriving in two separate packets is handled correctly.
//
//---------------------------------------------------------------------------

#include <Arduino.h>
#include <cerrno>
#include <cstring>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstddef>
#include <cassert>

#include "globals.h"
#include "console.h"
#include "logger.h"
#include "network.h"

#if ENABLE_WIFI

// Telnet protocol constants (RFC 854)
static constexpr uint8_t TELNET_IAC  = 255;  // Interpret As Command
static constexpr uint8_t TELNET_WILL = 251;  // I will use option
static constexpr uint8_t TELNET_WONT = 252;  // I won't use option
static constexpr uint8_t TELNET_DO   = 253;  // Please use option
static constexpr uint8_t TELNET_DONT = 254;  // Please don't use option
static constexpr uint8_t TELNET_SB   = 250;  // Subnegotiation begin
static constexpr uint8_t TELNET_SE   = 240;  // Subnegotiation end
static constexpr uint8_t OPT_ECHO    =   1;  // Echo option
static constexpr uint8_t OPT_SGA     =   3;  // Suppress Go Ahead

class TelnetSink : public IConsoleSink
{
public:
    TelnetSink(int fd) : _fd(fd) {}
    void Write(const char* data, size_t len) override {
        size_t total_sent = 0;
        while (total_sent < len) {
            int sent = send(_fd, data + total_sent, len - total_sent, 0);
            if (sent <= 0) {
                // Benign disconnect or error. We stop writing here and let the
                // main loop's recv() handle the cleanup and socket closure.
                return;
            }
            total_sent += sent;
        }

        // Tautological, but confirms the loop logic to any vampire programmers
        // that literally lack abilities beyond self-respect to look themsleves
        // in the mirror.
        assert(total_sent == len);
    }
    LineEndingPolicy LinePolicy() const override { return LineEndingPolicy::CRLF; }
private:
    int _fd;
};

// SendTelnetOption
//
// Sends a single 3-byte Telnet option negotiation (IAC <verb> <option>).

static void SendTelnetOption(int fd, uint8_t verb, uint8_t option)
{
    uint8_t buf[3] = { TELNET_IAC, verb, option };
    send(fd, buf, sizeof(buf), 0);
}

// NegotiateTelnetOptions
//
// On connect, tell the client:
//   WILL ECHO  - we will echo characters (client should suppress local echo)
//   WILL SGA   - we suppress go-ahead (character-at-a-time mode, our side)
//   DO SGA     - client should also suppress go-ahead

static void NegotiateTelnetOptions(int fd)
{
    SendTelnetOption(fd, TELNET_WILL, OPT_ECHO);
    SendTelnetOption(fd, TELNET_WILL, OPT_SGA);
    SendTelnetOption(fd, TELNET_DO,   OPT_SGA);
}

void IRAM_ATTR DebugLoopTaskEntry(void* pvParameters)
{
    int listen_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (listen_fd < 0) {
        debugE("Failed to create telnet socket");
        vTaskDelete(NULL);
        return;
    }

    struct sockaddr_in serv_addr;
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(NetworkPort::Telnet);

    if (bind(listen_fd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0) {
        debugE("Failed to bind telnet socket");
        close(listen_fd);
        vTaskDelete(NULL);
        return;
    }

    listen(listen_fd, 1);

    while (true)
    {
        struct sockaddr_in cli_addr;
        socklen_t cli_len = sizeof(cli_addr);
        int client_fd = accept(listen_fd, (struct sockaddr*)&cli_addr, &cli_len);

        if (client_fd < 0) {
            delay(100);
            continue;
        }

        debugI("Telnet client connected from %s", inet_ntoa(cli_addr.sin_addr));

        // Negotiate character-at-a-time mode and server-side echo before
        // registering the sink, so the client is in the right mode before
        // we start sending any output.
        NegotiateTelnetOptions(client_fd);

        TelnetSink sink(client_fd);
        ConsoleManager::Instance().SetTelnetSink(&sink);

        // State machine for Telnet protocol and CR handling.
        // Survives across recv() boundaries.
        enum class RecvState : uint8_t
        {
            Normal,     // Normal data byte
            IAC,        // Saw 0xFF, waiting for verb
            IACVerb,    // Saw IAC + verb, waiting for option byte
            IACsb,      // Inside subnegotiation, waiting for IAC SE
            IACInSB,    // Saw IAC inside subnegotiation
            AfterCR,    // Saw \r, waiting to see if next is \0 or \n (RFC 854)
        };
        RecvState state = RecvState::Normal;

        uint8_t buf[128];
        while (true)
        {
            int n = recv(client_fd, buf, sizeof(buf), 0);
            if (n == 0)
            {
                // Clean shutdown from client side
                break;
            }
            if (n < 0)
            {
                if (errno == EINTR)
                    continue;   // Signal interrupted syscall, retry
                // Use Serial directly to avoid any risk of recursion through
                // the Telnet sink that may itself be in a broken state.
                Serial.printf("[TelnetServer] recv error: %s\n", strerror(errno));
                break;
            }

            for (int i = 0; i < n; ++i)
            {
                const uint8_t byte = buf[i];

                switch (state)
                {
                case RecvState::Normal:
                    if (byte == TELNET_IAC)
                    {
                        state = RecvState::IAC;
                    }
                    else if (byte == '\r')
                    {
                        // RFC 854: bare CR must be followed by \0 or \n.
                        // Defer until we see the next byte.
                        state = RecvState::AfterCR;
                    }
                    else
                    {
                        ConsoleManager::Instance().FeedTelnetByte(byte);
                    }
                    break;

                case RecvState::AfterCR:
                    // RFC 854: CR-NUL means a bare CR; CR-LF means end-of-line.
                    // Either way we deliver a single '\r' to the CLI handler.
                    // Any other byte after CR is non-standard; deliver both.
                    state = RecvState::Normal;
                    if (byte == '\0' || byte == '\n')
                    {
                        // Canonical end-of-line: deliver the CR
                        ConsoleManager::Instance().FeedTelnetByte('\r');
                    }
                    else
                    {
                        // Non-standard: deliver CR then the unexpected byte
                        ConsoleManager::Instance().FeedTelnetByte('\r');
                        ConsoleManager::Instance().FeedTelnetByte(byte);
                    }
                    break;

                case RecvState::IAC:
                    if (byte == TELNET_IAC)
                    {
                        // Escaped 0xFF in data stream — deliver it literally
                        ConsoleManager::Instance().FeedTelnetByte(0xFF);
                        state = RecvState::Normal;
                    }
                    else if (byte == TELNET_SB)
                    {
                        state = RecvState::IACsb;
                    }
                    else if (byte == TELNET_WILL || byte == TELNET_WONT ||
                             byte == TELNET_DO   || byte == TELNET_DONT)
                    {
                        state = RecvState::IACVerb;
                    }
                    else
                    {
                        // Single-byte IAC command (NOP, DM, GA, etc.) — ignore
                        state = RecvState::Normal;
                    }
                    break;

                case RecvState::IACVerb:
                    // Option byte following the verb — silently absorb.
                    // We don't need to respond to client DO/DONT for options
                    // we haven't offered; our WILL ECHO / WILL SGA / DO SGA
                    // were already sent on connect.
                    state = RecvState::Normal;
                    break;

                case RecvState::IACsb:
                    // Inside subnegotiation — absorb until IAC SE
                    if (byte == TELNET_IAC)
                        state = RecvState::IACInSB;
                    break;

                case RecvState::IACInSB:
                    state = (byte == TELNET_SE) ? RecvState::Normal : RecvState::IACsb;
                    break;
                }
            }
        }

        ConsoleManager::Instance().ClearTelnetSink();
        close(client_fd);
        debugI("Telnet client disconnected");
    }
}

#endif
