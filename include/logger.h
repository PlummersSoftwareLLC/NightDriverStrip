//+--------------------------------------------------------------------------
//
// File:        logger.h
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
//    Core logging infrastructure: LogSink chain, Logger, and debugX macros.
//
//---------------------------------------------------------------------------

#pragma once

#include <Arduino.h>
#include <string>
#include <vector>
#include <memory>
#include <cstdarg>

//
// LogLevel enum class as noodled by the user
//
enum class LogLevel
{
    Error,
    Warn,
    Info,
    Debug,
    Verbose,
    Trace,
    Fatal
};

//
// Abstract LogSink for different destinations (Serial, Telnet, etc.)
//
class LogSink
{
public:
    virtual ~LogSink() = default;
    virtual void Write(LogLevel level, const char* tag, const char* message) = 0;
    virtual void Flush() {}
    virtual bool IsInteractive() const { return false; }
};

//
// Logger core for managing sinks and dispatching messages
//
class Logger
{
public:
    static void SetLevel(LogLevel level);
    static LogLevel GetLevel();
    static bool IsEnabled(LogLevel level);

    static void Logf(LogLevel level, const char* tag, const char* fmt, ...);
    static void Logv(LogLevel level, const char* tag, const char* fmt, va_list args);

    // Install the esp_log vprintf hook that routes all log output through
    // ConsoleManager::Broadcast -> WriteText for CRLF translation.
    // Call once from setup() after Serial is initialized.
    static void InstallLogHook();

private:
    static LogLevel _level;
};

//
// The "Jackpot" - Variadic templates to automatically unwrap String/std::string to c_str()
//
namespace LoggerInternal
{
    // Default: pass through
    template <typename T>
    inline decltype(auto) Unwrap(T&& val) {
        return std::forward<T>(val);
    }

    // Specialization for Arduino String
    inline const char* Unwrap(const String& s) {
        return s.c_str();
    }

    // Specialization for std::string
    inline const char* Unwrap(const std::string& s) {
        return s.c_str();
    }

    template <typename... Args>
    void DispatchLog(LogLevel level, const char* tag, const char* fmt, Args&&... args) {
        Logger::Logf(level, tag, fmt, Unwrap(std::forward<Args>(args))...);
    }
}

//
// debugX macros mapped to the Logger
// Use TAG if defined in the local TU, otherwise default to "PROJECT"
//
#ifndef TAG
#define TAG "ND"
#endif

#define debugE(fmt, ...) LoggerInternal::DispatchLog(LogLevel::Error,   TAG, fmt "\n", ##__VA_ARGS__)
#define debugW(fmt, ...) LoggerInternal::DispatchLog(LogLevel::Warn,    TAG, fmt "\n", ##__VA_ARGS__)
#define debugI(fmt, ...) LoggerInternal::DispatchLog(LogLevel::Info,    TAG, fmt "\n", ##__VA_ARGS__)
#define debugD(fmt, ...) LoggerInternal::DispatchLog(LogLevel::Debug,   TAG, fmt "\n", ##__VA_ARGS__)
#define debugV(fmt, ...) LoggerInternal::DispatchLog(LogLevel::Verbose, TAG, fmt "\n", ##__VA_ARGS__)
#define debugF(fmt, ...) LoggerInternal::DispatchLog(LogLevel::Fatal,   TAG, fmt "\n", ##__VA_ARGS__)
#define debugT(fmt, ...) LoggerInternal::DispatchLog(LogLevel::Trace,   TAG, fmt "\n", ##__VA_ARGS__)
#define debugA(fmt, ...) LoggerInternal::DispatchLog(LogLevel::Trace,   TAG, fmt "\n", ##__VA_ARGS__) // For "ANY" or legacy


