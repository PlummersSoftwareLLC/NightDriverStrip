//+--------------------------------------------------------------------------
//
// File:        console.h
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
//    Console session and manager for multiplexing Serial and Telnet I/O.
//
//---------------------------------------------------------------------------

#pragma once
#include "globals.h"
#include <Arduino.h>
#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <string_view>
#include <vector>
#include "logger.h"

enum class LineEndingPolicy
{
    None,   // No translation
    CRLF,   // '\n' -> "\r\n"
    LF      // Normalize to '\n'
};

class IConsoleSink
{
public:
    virtual ~IConsoleSink() = default;
    virtual void Write(const char* data, size_t len) = 0;
    virtual LineEndingPolicy LinePolicy() const { return LineEndingPolicy::None; }
    virtual void Flush() {}
};

class ConsoleSession : public LogSink
{
public:
    ConsoleSession(IConsoleSink* sink);

    // IConsoleSink methods
    void WriteRaw(std::string_view data);
    void WriteText(std::string_view text);   // no newline appended
    void WriteLine(std::string_view text);   // appends '\n'
    void Flush() override;

    // LogSink methods (from Logger.h)
    void Write(LogLevel level, const char* tag, const char* message) override;
    bool IsInteractive() const override { return true; }

    void SetEcho(bool enable) { _echo = enable; }
    bool EchoEnabled() const { return _echo; }

    void SetShowColors(bool enable) { _showColors = enable; }
    bool ShowColors() const { return _showColors; }

    std::string& StringBuffer() { return _buffer; }

private:
    IConsoleSink* _sink;
    bool _echo = true;
    bool _showColors = true;
    std::string _buffer;
};

using ConsoleByteHandler = void (*)(uint8_t byte, ConsoleSession& session);

class ConsoleManager
{
public:
    static ConsoleManager& Instance() {
        static ConsoleManager instance;
        return instance;
    }

    ConsoleSession& GetSerialSession() { return *_serialSession; }
    ConsoleSession* GetTelnetSession() { return _telnetSession.get(); }

    void SetByteHandler(ConsoleByteHandler handler) { _byteHandler = handler; }

    void FeedSerialByte(uint8_t byte);
    void FeedTelnetByte(uint8_t byte);

    void Broadcast(std::string_view data);

    // Internal use by TelnetServer
    void SetTelnetSink(IConsoleSink* sink);
    void ClearTelnetSink();

private:
    ConsoleManager();

    std::unique_ptr<ConsoleSession> _serialSession;
    std::unique_ptr<ConsoleSession> _telnetSession;
    ConsoleByteHandler _byteHandler = nullptr;
    std::recursive_mutex _mutex;
};

//
// SerialSink - adapts HardwareSerial to IConsoleSink
//
class SerialConsoleSink : public IConsoleSink
{
public:
    void Write(const char* data, size_t len) override
    {
        Serial.write(data, len);
    }

    void Flush() override
    {
        Serial.flush();
    }

    LineEndingPolicy LinePolicy() const override { return LineEndingPolicy::CRLF; }
};
