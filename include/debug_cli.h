//+--------------------------------------------------------------------------
//
// File:        debug_cli.h
//
// NightDriverStrip - (c) 2025 Plummer's Software LLC.  All Rights Reserved.
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
//    Protos and data for debugging command line interface to NightDriver.

#pragma once

#include <vector>
#include <string_view>

namespace DebugCLI
{

    using cli_argv = std::vector<std::string_view>;

    // Helper function pointer type for static command initialization.
    using command_handler_t = void (*)(const cli_argv&);

    // Each command gets one of these to describe it.
    struct command
    {
        const char* const command;
        const char* const help;
        const char* const announcement;
        command_handler_t helper;
    };

    // Case-insensitive string comparison for commands.
    bool StringCompareInsensitive(std::string_view sv, const char* s);

    // Register a single command.
    void RegisterCommand(const command* cmd);

    // Register an array of commands.
    void RegisterCommands(const command* cmds, size_t count);

    // Template helper for array registration.
    template <size_t N>
    void RegisterCommands(const command (&cmds)[N])
    {
        RegisterCommands(cmds, N);
    }

    // printf-style output to Debug/Serial without log-level tags
    void cli_printf(const char *fmt, ...) __attribute__((format(printf, 1, 2)));

    // Process and execute command line: telnet, serial, or (someday) a script.
    void RunCommand(const char* cmd);

    // Tab completion.
    std::string_view TabComplete(std::string_view partial, std::string_view full_line);

    // Process a single byte of input from the CLI.
    void ProcessCLIByte(uint8_t byte);

    // Initialization (registers core commands).
    void InitDebugCLI();

} // namespace debug_cli
