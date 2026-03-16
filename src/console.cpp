//+--------------------------------------------------------------------------
//
// File:        console.cpp
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
//    Implementation of ConsoleSession and ConsoleManager.
//
//---------------------------------------------------------------------------

#include "globals.h"

#include "console.h"
#include "debug_cli.h"
#include "logger.h"

//
// ConsoleSession
//

ConsoleSession::ConsoleSession(IConsoleSink* sink) : _sink(sink) {}

void ConsoleSession::WriteRaw(std::string_view data)
{
    if (_sink)
        _sink->Write(data.data(), data.size());
}

void ConsoleSession::WriteText(std::string_view text)
{
    if (!_sink) return;

    if (_sink->LinePolicy() == LineEndingPolicy::CRLF)
    {
        size_t start = 0;
        for (size_t i = 0; i < text.size(); ++i)
        {
            if (text[i] == '\n')
            {
                if (i > start)
                    _sink->Write(text.data() + start, i - start);
                _sink->Write("\r\n", 2);
                start = i + 1;
            }
        }
        if (start < text.size())
            _sink->Write(text.data() + start, text.size() - start);
    }
    else
    {
        _sink->Write(text.data(), text.size());
    }
}

void ConsoleSession::WriteLine(std::string_view text)
{
    WriteText(text);
    WriteText("\n");
    Flush();
}

void ConsoleSession::Flush()
{
    if (_sink)
        _sink->Flush();
}

void ConsoleSession::Write(LogLevel level, const char* tag, const char* message)
{
    if (!_sink) return;

    // Assemble the entire log line into one buffer before writing to avoid
    // interleaving from concurrent tasks calling _sink->Write() in separate calls.
    char lv;
    const char* color;
    const char* reset = "\x1B[0m";

    switch (level)
    {
        case LogLevel::Fatal:   lv = 'F'; color = "\x1B[1;31m"; break; // Bold Red
        case LogLevel::Error:   lv = 'E'; color = "\x1B[31m";   break; // Red
        case LogLevel::Warn:    lv = 'W'; color = "\x1B[33m";   break; // Yellow
        case LogLevel::Info:    lv = 'I'; color = "\x1B[32m";   break; // Green
        case LogLevel::Debug:   lv = 'D'; color = "\x1B[36m";   break; // Cyan
        case LogLevel::Verbose: lv = 'V'; color = "\x1B[39m";   break; // Default
        case LogLevel::Trace:   lv = 'T'; color = "\x1B[90m";   break; // Gray
        default:                lv = '?'; color = "";            break;
    }

    // Build the entire log line into one buffer before writing to avoid
    // interleaving from concurrent tasks. lv is a char so operator+=(char)
    // is a direct store — no strlen scan like operator+=(const char*) would do.
    std::string line;
    if (_showColors) line += color;
    line += '[';
    line += lv;
    line += "][";
    line += tag;
    line += "] ";
    line += message;
    line += '\n';
    if (_showColors) line += reset;

    WriteText(line);
    Flush();
}

//
// ConsoleManager
//

static SerialConsoleSink g_SerialSink;

ConsoleManager::ConsoleManager()
{
    _serialSession = std::make_unique<ConsoleSession>(&g_SerialSink);
}

void ConsoleManager::FeedSerialByte(uint8_t byte)
{
    std::lock_guard<std::recursive_mutex> lock(_mutex);
    if (_byteHandler && _serialSession)
        _byteHandler(byte, *_serialSession);
}

void ConsoleManager::FeedTelnetByte(uint8_t byte)
{
    std::lock_guard<std::recursive_mutex> lock(_mutex);
    if (_byteHandler && _telnetSession)
        _byteHandler(byte, *_telnetSession);
}

void ConsoleManager::Broadcast(std::string_view data)
{
    std::lock_guard<std::recursive_mutex> lock(_mutex);
    if (_serialSession) {
        _serialSession->WriteText(data);
        _serialSession->Flush();
    }
    if (_telnetSession) {
        _telnetSession->WriteText(data);
        _telnetSession->Flush();
    }
}

void ConsoleManager::SetTelnetSink(IConsoleSink* sink)
{
    std::lock_guard<std::recursive_mutex> lock(_mutex);
    _telnetSession = std::make_unique<ConsoleSession>(sink);
    if (_telnetSession) {
        _telnetSession->SetEcho(true);
        DebugCLI::RunCommand("", *_telnetSession); // Force an initial prompt
    }
}

void ConsoleManager::ClearTelnetSink()
{
    std::lock_guard<std::recursive_mutex> lock(_mutex);
    if (_telnetSession)
    {
        _telnetSession.reset();
    }
}
