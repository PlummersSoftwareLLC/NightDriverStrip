//+--------------------------------------------------------------------------
//
// File:        logger.cpp
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

#include "logger.h"

#include "console.h"
#include <esp_log.h>
#include <string_view>

LogLevel Logger::_level = LogLevel::Verbose;

// Mapping LogLevel to esp_log_level_t
static esp_log_level_t ToEspLevel(LogLevel level)
{
    switch (level)
    {
        case LogLevel::Fatal:
        case LogLevel::Error:   return ESP_LOG_ERROR;
        case LogLevel::Warn:    return ESP_LOG_WARN;
        case LogLevel::Info:    return ESP_LOG_INFO;
        case LogLevel::Debug:   return ESP_LOG_DEBUG;
        case LogLevel::Verbose:
        case LogLevel::Trace:   return ESP_LOG_VERBOSE;
        default:                return ESP_LOG_INFO;
    }
}

void Logger::SetLevel(LogLevel level)
{
    _level = level;
    esp_log_level_set("*", ToEspLevel(level));
}

LogLevel Logger::GetLevel()
{
    return _level;
}

bool Logger::IsEnabled(LogLevel level)
{
    return level <= _level;
}

void Logger::Logf(LogLevel level, const char* tag, const char* fmt, ...)
{
    if (!IsEnabled(level))
        return;

    va_list args;
    va_start(args, fmt);
    Logv(level, tag, fmt, args);
    va_end(args);
}

void Logger::Logv(LogLevel level, const char* tag, const char* fmt, va_list args)
{
    if (!IsEnabled(level))
        return;

    // Route through esp_log_writev. The installed vprintf hook (LogHookVprintf)
    // intercepts the fully-formatted output and broadcasts it through
    // ConsoleManager::Broadcast -> WriteText, which applies each sink's
    // LineEndingPolicy (CRLF for serial and Telnet).
    esp_log_writev(ToEspLevel(level), tag, fmt, args);
}

// LogHookVprintf
//
// Installed via esp_log_set_vprintf. Called by the ESP-IDF log system with
// the fully-formatted log line (including [L][TAG] prefix and trailing \n).
// Routes output through ConsoleManager::Broadcast -> WriteText so that each
// sink's LineEndingPolicy (CRLF for serial and Telnet) is applied.

static int LogHookVprintf(const char* fmt, va_list args)
{
    // Guard: vsnprintf and ConsoleManager are not ISR-safe.
    if (xPortInIsrContext())
        return vprintf(fmt, args);  // Fall back to default UART output

    // Two-pass heap allocation: no fixed-size stack buffer of any size.
    // A stack buffer of any fixed size either burns the caller's stack on
    // every task that logs (the "War and Peace" problem), or silently truncates
    // long IDF messages (WiFi scan results, cert dumps, etc.).
    // First pass: measure the formatted length without filling any buffer.
    // Second pass: allocate exactly that much from the heap and fill it.
    // Falls back to direct UART output on heap exhaustion.
    va_list args_copy;
    va_copy(args_copy, args);
    int len = vsnprintf(nullptr, 0, fmt, args_copy);
    va_end(args_copy);

    if (len <= 0)
        return len;

    char* buf = static_cast<char*>(malloc(len + 1));
    if (!buf)
        return vprintf(fmt, args);  // OOM: fall back to UART

    vsnprintf(buf, len + 1, fmt, args);
    ConsoleManager::Instance().Broadcast(std::string_view(buf, static_cast<size_t>(len)));
    free(buf);
    return len;
}

void Logger::InstallLogHook()
{
    esp_log_set_vprintf(LogHookVprintf);
}

