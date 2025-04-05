//+--------------------------------------------------------------------------
//
// File:        websocketserver.h
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
// Description:
//
//   Server class that optionally registers one or more web sockets with the
//   on-board webserver to allow interactions with web clients that include
//   messages being pushed from server to client.
//
// History:     Dec-21-2023         Rbergen     Created
//---------------------------------------------------------------------------

#pragma once

#include "globals.h"

#define WEB_SOCKETS_ANY_ENABLED (COLORDATA_WEB_SOCKET_ENABLED || EFFECTS_WEB_SOCKET_ENABLED)

#if WEB_SOCKETS_ANY_ENABLED

#include "effectmanager.h"
#include "webserver.h"

class WebSocketServer : public IEffectEventListener
{
    AsyncWebSocket _colorDataSocket;
    AsyncWebSocket _effectChangeSocket;
    static constexpr size_t _maxColorNumberLen = sizeof(NAME_OF(16777215)) - 1; // (uint32_t)CRGB(255,255,255) == 16777215
    static constexpr size_t _maxCountLen = sizeof(NAME_OF(4294967295)) - 1;     // SIZE_MAX == 4294967295

public:

    WebSocketServer(CWebServer& webServer) :
        _colorDataSocket("/ws/frames"),
        _effectChangeSocket("/ws/effects")
    {
        #if COLORDATA_WEB_SOCKET_ENABLED
        webServer.AddWebSocket(_colorDataSocket);
        #endif

        #if EFFECTS_WEB_SOCKET_ENABLED
        webServer.AddWebSocket(_effectChangeSocket);
        #endif
    }

    void CleanupClients()
    {
        _colorDataSocket.cleanupClients();
        _effectChangeSocket.cleanupClients();
    }

    bool HaveColorDataClients()
    {
        return _colorDataSocket.count() > 0;
    }

    // Send the color data for an array of leds of indicated length. We do this "semi-manually" (i.e. using an std::string) to
    // reduce memory allocations and avoid the overhead of using ArduinoJson.
    void SendColorData(CRGB* leds, size_t count)
    {
        if (!HaveColorDataClients() || leds == nullptr || count == 0 || !_colorDataSocket.availableForWriteAll())
            return;

        constexpr char messageHead[] = "{\"colorData\":[";
        constexpr char messageTail[] = "]}";

        char numberBuffer[_maxCountLen + _maxColorNumberLen + 2];  // Small buffer we ask snprintf to write the pixel counts and color numbers to
        std::basic_string<char, std::char_traits<char>, psram_allocator<char>> colorDataMessage; // (std::string) buffer for the message

        // Reserve space for message head, "count" sequential pixel counts and LED color numbers, and message tail.
        // We're almost certainly over-allocating because we reserve space for "count" times the maximum color number
        // length plus extras (pixel count and commas). We will only use that space if the hex RGB color code of each
        // LED is 0x989680 or higher, and no two sequential pixels have the same color.
        colorDataMessage.reserve(sizeof(messageHead) + (count * (_maxColorNumberLen + 3)) + sizeof(messageTail) - 3); // -3 for head and message terminating \0 and absence of the last comma
        colorDataMessage = messageHead;
        CRGB activeColor = leds[0];
        size_t activeCount = 1;

        for (size_t i = 1; i < count; i++)
        {
            if (leds[i] != activeColor)
            {
                // New active color, add the data to the message and reset active and count
                snprintf(numberBuffer, sizeof(numberBuffer), "%zu,%" PRIu32 ",", activeCount, toUint32(activeColor));
                colorDataMessage += numberBuffer;
                activeColor = leds[i];
                activeCount = 1;
            }
            else
                activeCount++;
        }

        // Add the last color block to the array
        snprintf(numberBuffer, sizeof(numberBuffer), "%zu,%" PRIu32, activeCount, toUint32(activeColor));
        colorDataMessage += numberBuffer;
        colorDataMessage += messageTail;

        _colorDataSocket.textAll(colorDataMessage.c_str());
    }

    void OnCurrentEffectChanged(size_t currentEffectIndex) override
    {
        _effectChangeSocket.textAll(str_sprintf("{\"currentEffectIndex\":%zu}", currentEffectIndex));
    }

    void OnEffectListDirty() override
    {
        _effectChangeSocket.textAll("{\"effectListDirty\":true}");
    }

    void OnEffectEnabledStateChanged(size_t effectIndex, bool newState) override
    {
        _effectChangeSocket.textAll(str_sprintf("{\"effectsEnabledState\":[{\"index\":%zu,\"enabled\":%s}]}", effectIndex, newState ? "true" : "false"));
    }

    void OnIntervalChanged(uint interval) override
    {
        _effectChangeSocket.textAll(str_sprintf("{\"interval\":%u}", interval));
    }
};
#endif // WEB_SOCKETS_ANY_ENABLED