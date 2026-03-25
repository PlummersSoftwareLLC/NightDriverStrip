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

#include <esp_log.h>
#include <string_view>
#include "console.h"
#include "logger.h"

LogLevel Logger::_level = LogLevel::Info;

// Mapping LogLevel to esp_log_level_t
static esp_log_level_t ToEspLevel(LogLevel level)
{
    switch (level)
    {
        case LogLevel::Fatal:   return ESP_LOG_ERROR;
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
    // Fatal (0), Error (1), ..., Trace (6)
    // If _level is Info (3), we enable Error (1) and Info (3), but not Debug (4)
    return static_cast<int>(level) <= static_cast<int>(_level);
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

    // Use a stack buffer for typical log lines to avoid heap fragmentation.
    // This structured broadcast preserves ANSI colors and consistent prefixing.
    char stack_buf[256];
    va_list args_copy;
    va_copy(args_copy, args);
    int len = vsnprintf(stack_buf, sizeof(stack_buf), fmt, args_copy);
    va_end(args_copy);

    if (len < 0) return;

    char* buf = stack_buf;
    bool must_free = false;

    if (static_cast<size_t>(len) >= sizeof(stack_buf))
    {
        buf = static_cast<char*>(malloc(len + 1));
        if (buf)
        {
            must_free = true;
            vsnprintf(buf, len + 1, fmt, args);
        }
        else
        {
            buf = stack_buf; // fallback to truncated view if OOM
        }
    }

    ConsoleManager::Instance().Broadcast(level, tag, buf);

    if (must_free)
        free(buf);
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

    // Optimized buffer handling: use a stack buffer for typical log lines (~128 chars)
    // to avoid heap fragmentation. Use heap only for "War and Peace" payloads.
    char stack_buf[256];
    va_list args_copy;
    va_copy(args_copy, args);
    int len = vsnprintf(stack_buf, sizeof(stack_buf), fmt, args_copy);
    va_end(args_copy);

    if (len < 0)
        return len;

    char* buf = stack_buf;
    bool must_free = false;

    if (static_cast<size_t>(len) >= sizeof(stack_buf))
    {
        buf = static_cast<char*>(malloc(len + 1));
        if (!buf)
            return vprintf(fmt, args);  // OOM: fall back to UART
        must_free = true;
        vsnprintf(buf, len + 1, fmt, args);
    }

    // Newline Coalescing and Progress Bar protection
    //
    // 1. If we have \n\n at the end, drop one. This happens when a debugX macro
    //    appends \n to a format string that already had one.
    // 2. If the message ends in \r (progress bar), don't let it be mangled
    //    if possible, though the logger usually appends \n after the TAG/LEVEL.
    if (len >= 2 && buf[len-1] == '\n' && buf[len-2] == '\n')
        len--;

    ConsoleManager::Instance().Broadcast(std::string_view(buf, static_cast<size_t>(len)));

    if (must_free)
        free(buf);

    return len;
}

void Logger::InstallLogHook()
{
    esp_log_set_vprintf(LogHookVprintf);
}

