//+--------------------------------------------------------------------------
//
// File:        debug_cli.cpp
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
//    Implementation of debugging command line interface to NightDriver.
//

#include <algorithm>
#include <charconv>
#include <cctype>
#include <cstring>
#include <esp_heap_caps.h>
#include <esp_log.h>
#include <esp_system.h> // esp_restart
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <string>
#include <string_view>
#include <vector>
#include <optional>

#include "globals.h"
#include "debug_cli.h"
#include "effectmanager.h"
#include "systemcontainer.h"
#include <FS.h>
#include <SPIFFS.h>

namespace DebugCLI
{

//
// Private Globals
//
static std::vector<const command *> g_CommandTable;

//
// Tokenizer (Internal)
// Uses NVRO to allocate in caller's frame to make array of pointer
// pairs of starts/ends. No copies. No allocs.
// Simple quoting like 'setname "My Cool Blinkies"' is supported.
//
static std::vector<std::string_view> Tokenize(std::string_view input)
{
    std::vector<std::string_view> output;
    size_t start = 0;
    bool in_quotes = false;

    const size_t input_len = input.length();
    for (size_t i = 0; i < input_len; ++i)
    {
        char c = input[i];

        if (in_quotes)
        {
            if (c == '"')
            {
                size_t len = i - start;
                output.push_back(input.substr(start, len));
                in_quotes = false;
                start = i + 1;
            }
            continue;
        }

        if (std::isspace(c))
        {
            if (i > start)
            {
                output.push_back(input.substr(start, i - start));
            }
            start = i + 1;
        }
        else if (c == '"')
        {
            if (i > start)
            {
                output.push_back(input.substr(start, i - start));
            }
            start = i + 1;
            in_quotes = true;
        }
    }

    if (start < input.length())
    {
        output.push_back(input.substr(start));
    }

    return output;
}

//
// String Comparison
//
bool StringCompareInsensitive(std::string_view sv, const char *in_s)
{
    if (!in_s)
        return false;
    const unsigned char *s = (const unsigned char *)in_s;
    for (unsigned char c : sv)
    {
        if (!*s || tolower(c) != tolower(*s))
            return false;
        s++;
    }
    return *s == '\0';
}

bool StringStartsWithInsensitive(std::string_view haystack, std::string_view needle)
{
    if (needle.length() > haystack.length())
        return false;
    return std::equal(needle.begin(), needle.end(), haystack.begin(),
                      [](char ch1, char ch2) { return std::tolower(ch1) == std::tolower(ch2); });
}

bool ContainsInsensitive(std::string_view haystack, std::string_view needle)
{
    if (needle.empty())
        return true;
    auto it = std::search(haystack.begin(), haystack.end(), needle.begin(), needle.end(),
                          [](char ch1, char ch2) { return std::tolower(ch1) == std::tolower(ch2); });
    return it != haystack.end();
}


//
// Registration
//
void RegisterCommand(const command *cmd)
{
    if (cmd)
        g_CommandTable.push_back(cmd);
}

void RegisterCommands(const command *cmds, size_t count)
{
    for (size_t i = 0; i < count; i++)
    {
        g_CommandTable.push_back(&cmds[i]);
    }
}

//
// Core Command Helpers
//
static void PrintHelp(const cli_argv &argv)
{
    if (argv.size() > 1)
    {
        std::string_view target = argv[1];
        const command *match = nullptr;
        int matches = 0;

        for (const auto *cmd : g_CommandTable)
        {
            if (StringCompareInsensitive(target, cmd->command))
            {
                match = cmd;
                matches = 1;
                break;
            }
            if (StringStartsWithInsensitive(cmd->command, target))
            {
                match = cmd;
                matches++;
            }
        }

        if (matches == 1 && match)
        {
            cli_printf("%-20s %s\n", match->command, match->help);
        }
        else
        {
            cli_printf("Command '%.*s' not found or ambiguous.\n", (int)target.length(), target.data());
        }
    }
    else
    {
        for (const auto *cmd : g_CommandTable)
        {
            cli_printf("%-20s %s\n", cmd->command, cmd->help);
        }
    }
}

//
// Parse and run cmd_line. Zero copies or allocations.
//
void RunCommand(const char *cmd_line)
{
    const auto argv = Tokenize(cmd_line);

    // If command is valid
    if (!argv.empty())
    {
        auto it = std::find_if(g_CommandTable.begin(), g_CommandTable.end(),
                               [&](const command *c) { return StringCompareInsensitive(argv[0], c->command); });

        if (it != g_CommandTable.end())
        {
            if ((*it)->announcement)
                cli_printf("%s\n", (*it)->announcement);
            (*it)->helper(argv);
        }
        else
        {
            cli_printf("Unknown command: %.*s\n", (int)argv[0].length(), argv[0].data());
        }
    }

    // Always emit prompt after command execution (or empty command)
    // Serial: Inline prompt (cleaner)
    Serial.print("> ");

// RemoteDebug: Direct write to TelnetClient to bypass cleaner buffering and avoid extra newline.
// We check if the debugger is enabled (which exposes getTelnetClient) and if we have an active client.
#ifdef DEBUGGER_ENABLED
    if (Debug.isActive(Debug.ANY))
    {
        WiFiClient *client = Debug.getTelnetClient();
        if (client && client->connected())
        {
            client->print("> ");
            client->flush();
        }
    }
#endif
}

// If you have command 'reboot', reb<TAB> will complete "reboot". Caller will
// print a space that's not in the command until newline is found. It will
// not complete arguments, e.g. not 'reboot n<tab>' would not complete the
// 'now' if that happened to be an option.
std::string_view TabComplete(std::string_view partial, std::string_view full_line)
{
    if (partial.empty())
        return "";

    // If we're completing the first token (the command)
    if (partial.data() == full_line.data())
    {
        std::string_view match = "";
        int matches = 0;

        for (const auto *cmd : g_CommandTable)
        {
            if (StringStartsWithInsensitive(cmd->command, partial))
            {
                match = cmd->command;
                matches++;
            }
        }

        if (matches == 1)
            return match.substr(partial.length());

        if (matches > 1)
        {
            // Calculate longest common prefix among all matching commands
            size_t common_len = match.length();
            for (const auto *cmd : g_CommandTable)
            {
                if (StringStartsWithInsensitive(cmd->command, partial))
                {
                    size_t j = partial.length();
                    while (j < common_len && j < strlen(cmd->command) &&
                           tolower(match[j]) == tolower(cmd->command[j]))
                        j++;
                    common_len = j;
                }
            }
            if (common_len > partial.length())
                return match.substr(partial.length(), common_len - partial.length());
        }
    }
    // If we're completing an argument for 'effect'
    else if (full_line.substr(0, 6) == "effect")
    {
        auto& effectManager = g_ptrSystem->EffectManager();
        std::string_view firstMatch = "";
        int matches = 0;
        size_t common_len = 0;

        for (size_t i = 0; i < effectManager.EffectCount(); ++i)
        {
            const String& name = effectManager.EffectsList()[i]->FriendlyName();
            if (StringStartsWithInsensitive(name.c_str(), partial))
            {
                if (matches == 0)
                {
                    firstMatch = name.c_str();
                    common_len = firstMatch.length();
                }
                else
                {
                    size_t j = partial.length();
                    while (j < common_len && j < (size_t)name.length() &&
                           tolower(firstMatch[j]) == tolower(name[j]))
                        j++;
                    common_len = j;
                }
                matches++;
            }
        }

        if (matches > 0 && common_len > partial.length())
            return firstMatch.substr(partial.length(), common_len - partial.length());
    }

    return "";
}


//
// Helper: Resolve Effect by index or name
//
static std::optional<size_t> ResolveEffect(std::string_view arg)
{
    auto& effectManager = g_ptrSystem->EffectManager();

    // Try as index first
    // Try as index first
    size_t val = 0;
    auto [ptr, ec] = std::from_chars(arg.begin(), arg.end(), val);
    if (ec == std::errc() && ptr == arg.end())
    {
        if (val < effectManager.EffectCount())
        {
            return val;
        }
        else
        {
            cli_printf("Error: Effect index %zu out of range (0-%u)\n", val, effectManager.EffectCount() - 1);
            return std::nullopt;
        }
    }

    // Try fuzzy name match
    int match_index = -1;
    int matches = 0;
    std::vector<std::string> candidates;

    for (size_t i = 0; i < effectManager.EffectCount(); ++i)
    {
        const String& name = effectManager.EffectsList()[i]->FriendlyName();
        if (ContainsInsensitive(name.c_str(), arg))
        {
            match_index = i;
            matches++;
            candidates.push_back(name.c_str());
        }
    }

    if (matches == 1)
    {
        return (size_t)match_index;
    }
    else if (matches > 1)
    {
        cli_printf("Error: Ambiguous match for '%.*s'. Candidates:\n", (int)arg.length(), arg.data());
        for (const auto& c : candidates)
            cli_printf("  %s\n", c.c_str());
        return std::nullopt;
    }
    else
    {
        cli_printf("Error: No effect matching '%.*s' found.\n", (int)arg.length(), arg.data());
        return std::nullopt;
    }
}

//
// Core Commands Table
//
static void DoEffectCommand(const cli_argv &argv)
{
    auto& effectManager = g_ptrSystem->EffectManager();
    cli_printf("Current Effect: %s\n", effectManager.GetCurrentEffectName().c_str());

    if (argv.size() > 1)
    {
        std::string_view arg = argv[1];

        if (arg == "next")
        {
            effectManager.NextEffect();
        }
        else if (arg == "prev")
        {
            effectManager.PreviousEffect();
        }
        else
        {
            auto idx = ResolveEffect(arg);
            if (idx)
            {
                effectManager.SetCurrentEffectIndex(*idx);
            }
            else
            {
                return;
            }
        }
        cli_printf("New Effect: %s\n", effectManager.GetCurrentEffectName().c_str());
    }
}

static void DoDirectoryListing(const cli_argv &argv)
{
    fs::FS *fileSystem = &SPIFFS;

    fs::File root = fileSystem->open("/");

    fs::File file = root.openNextFile();
    while (file)
    {
        time_t t = file.getLastWrite();
        struct tm tm = {0};
        localtime_r(&t, &tm);

        if (file.isDirectory())
        {
            cli_printf("%-32s DIR      %d-%02d-%02d %02d:%02d:%02d\n", file.name(), (int)(tm.tm_year + 1900), (int)(tm.tm_mon + 1),
                       (int)tm.tm_mday, (int)tm.tm_hour, (int)tm.tm_min, (int)tm.tm_sec);
        }
        else
        {
            cli_printf("%-32s %8zu %d-%02d-%02d %02d:%02d:%02d\n", file.name(), (size_t)file.size(), (int)(tm.tm_year + 1900),
                       (int)(tm.tm_mon + 1), (int)tm.tm_mday, (int)tm.tm_hour, (int)tm.tm_min, (int)tm.tm_sec);
        }
        file = root.openNextFile();
    }
}

// Display a file. Hope it's text!
static void DoCat(const cli_argv &argv)
{
    if (argv.size() < 2)
    {
        cli_printf("Usage: cat <filename>\n");
        return;
    }

    std::string fname(argv[1]);
    if (fname[0] != '/')
        fname = "/" + fname;

    fs::FS *fileSystem = &SPIFFS;
    if (!fileSystem->exists(fname.c_str()))
    {
        cli_printf("File not found: %s\n", fname.c_str());
        return;
    }

    fs::File file = fileSystem->open(fname.c_str(), "r");
    if (!file || file.isDirectory())
    {
        cli_printf("Error opening file: %s\n", fname.c_str());
        if (file)
            file.close();
        return;
    }

    char buf[65]; // +1 for null terminator, 64 chars per chunk
    while (file.available())
    {
        size_t len = file.readBytes(buf, sizeof(buf) - 1);
        buf[len] = 0;
        cli_printf("%s", buf);
    }
    // cli_printf("\n"); // Optional: Ensure newline? Left raw for now.

    file.close();
}

static const command core_commands[] = {
    {"cat", "Display file content", "Printing file...", DoCat},
    {"reboot", "Reboot system", "Rebooting. Please stand by...", [](const cli_argv &) { esp_restart(); }},
    {"clearsettings", "Reset persisted user settings", "Removing persisted settings",
     [](const cli_argv &) {
         g_ptrSystem->DeviceConfig().RemovePersisted();
         RemoveEffectManagerConfig(); // Helper from effectmanager.h
     }},
    {"tasks", "Display FreeRTOS task list", "Task List:",
     [](const cli_argv &) {
#if configUSE_TRACE_FACILITY && configUSE_STATS_FORMATTING_FUNCTIONS
         char buf[1024];
         vTaskList(buf);
         cli_printf("\nName          State      Prio  Stack  Num Core\n%s", buf);
#else
         cli_printf("Task list not enabled in FreeRTOS config\n");
#endif
     }},
    {"heap", "Display heap memory info",
     "Heap usage:", [](const cli_argv &) { heap_caps_print_heap_info(MALLOC_CAP_DEFAULT); }},
    {"log", "[tag] <level> Set log level", "Setting log level...",
     [](const cli_argv &argv) {
         static const struct
         {
             const char *name;
             esp_log_level_t level;
         } levels[] = {{"none", ESP_LOG_NONE}, {"error", ESP_LOG_ERROR}, {"warn", ESP_LOG_WARN},
                       {"info", ESP_LOG_INFO}, {"debug", ESP_LOG_DEBUG}, {"verbose", ESP_LOG_VERBOSE}};

         if (argv.size() < 2)
         {
             cli_printf("Usage: log [tag] <none|error|warn|info|debug|verbose>\n");
             return;
         }
         std::string_view levelStr = argv.back();
         std::string_view tag = "*";
         if (argv.size() > 2)
             tag = argv[1];

         for (const auto &l : levels)
         {
             if (StringCompareInsensitive(levelStr, l.name))
             {
                 esp_log_level_set(std::string(tag).c_str(), l.level);
                 cli_printf("Set level for tag '%s' to %d (%s)\n", std::string(tag).c_str(), l.level, l.name);
                 return;
             }
         }
         cli_printf("Invalid level '%s'.\n", std::string(levelStr).c_str());
     }},
    {"bright", "[level] Display/Set brightness 0-255", "Brightness:",
     [](const cli_argv &argv) {
         if (argv.size() > 1)
         {
             int val = atoi(std::string(argv[1]).c_str());
             g_ptrSystem->DeviceConfig().SetBrightness(val);
         }
         cli_printf("Brightness: %lu\n", (unsigned long)g_ptrSystem->DeviceConfig().GetBrightness());
     }},
    {"ls", "Show filesytem directory", "NAME", DoDirectoryListing},                    // Function pointer
    {"effect", "[next|prev|name|index] Show/change current effect", "Effects.", DoEffectCommand}, // Function pointer
    {"+", "Nudge brightness up", "Brightness +10", [](const cli_argv &) {
        int val = std::min(255, g_ptrSystem->DeviceConfig().GetBrightness() + 10);
        g_ptrSystem->DeviceConfig().SetBrightness(val);
        cli_printf("Brightness: %d\n", val);
    }},
    {"-", "Nudge brightness down", "Brightness -10", [](const cli_argv &) {
        int val = std::max(0, g_ptrSystem->DeviceConfig().GetBrightness() - 10);
        g_ptrSystem->DeviceConfig().SetBrightness(val);
        cli_printf("Brightness: %d\n", val);
    }},
    {"help", "Display command line options", "Displaying system help",
     PrintHelp} // Function pointer logic requires PrintHelp signature match. It does.
};

//
// Helper: cli_printf is needed to bypass the stupid (I) tags
// forced by Debug.printf(). This outputs to both serial and telnet.
//
void cli_printf(const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    char buf[256];
    vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);

    // Write to Serial directly (ensure CR for every LF)
    for (const char *p = buf; *p; p++)
    {
        if (*p == '\n')
            Serial.write('\r');
        Serial.write(*p);
    }

    // Write to RemoteDebug (Telnet/WebSocket)
    // We suppress the Serial echo to avoid double-printing, and use showRaw to avoid tags.
    // Note: We assume SerialEnabled should be restored to true, which is the standard config.
    Debug.setSerialEnabled(false);
    Debug.showRaw(true);
    Debug.print(buf);
    Debug.showRaw(false);
    Debug.setSerialEnabled(true);
}

void ProcessCLIByte(uint8_t byte)
{
    // Essentially a global that never shrinks. We quickly reach
    // the size of the length that people type, but it's free if
    // never used.
    static std::string cmd;

    switch (byte)
    {
    case '\t':
    {
        size_t lastSpace = cmd.find_last_of(' ');
        std::string_view partial = (lastSpace == std::string::npos) ? std::string_view(cmd) : std::string_view(cmd).substr(lastSpace + 1);
        std::string_view suffix = TabComplete(partial, cmd);
        if (!suffix.empty())
        {
            cmd += suffix;
            cmd += " ";
            Serial.print(suffix.data());
            Serial.print(" ");
        }
        break;
    }
    case '\b':
    case 0x7f:
        if (!cmd.empty())
        {
            cmd.pop_back();
            cli_printf("\b \b");
        }
        else
        {
            // Optional: Ring bell or ignore if buffer empty
        }
        break;

    case '\r':
    case '\n':
        if (byte == '\r')
            Serial.println(); // Correctly handle CRLF for local echo
        if (cmd.empty())
        {
            // If buffer was empty (just Enter), RunCommand("") handles the prompt
            RunCommand("");
            cmd.clear();
        }
        else
        {
            // User entered a command
            cli_printf("\n");
            RunCommand(cmd.c_str());
            cmd.clear();
        }
        break;

    default:
        Serial.write(byte);
        cmd += (char)byte;
        break;
    }
}

void InitDebugCLI()
{
    RegisterCommands(core_commands);
}

} // namespace DebugCLI
