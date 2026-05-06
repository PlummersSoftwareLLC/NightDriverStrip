#pragma once

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

#include "globals.h"

#if WEB_SOCKETS_ANY_ENABLED

#include <atomic>

#include "effectmanager.h"
#include "iservice.h"
#include "webserver.h"

// WebSocketServer
//
// Pushes frame and effect-change events to connected web clients. The
// listening socket and per-handler routing live inside CWebServer's
// underlying AsyncWebServer; this service is responsible for binding
// its handlers when the bound CWebServer is running and unbinding them
// on Stop. Lifecycle:
//
//   - Construction is metadata-only: no handlers are registered yet.
//   - Start() requires the bound CWebServer to be running. It registers
//     the handlers and flips IsRunning() to true.
//   - Stop() closes any open client connections, removes the handlers,
//     and flips IsRunning() to false so the next Start() is a clean re-bind.
//
// Always Start CWebServer before WebSocketServer, and Stop in reverse
// order; otherwise Start() refuses (Stop is harmless).

class WebSocketServer : public IEffectEventListener, public IService
{
    AsyncWebSocket _colorDataSocket;
    AsyncWebSocket _effectChangeSocket;
    CWebServer&    _webServer;
    std::atomic<bool> _running{false};
    static constexpr size_t _maxColorNumberLen = sizeof(NAME_OF(16777215)) - 1; // (uint32_t)CRGB(255,255,255) == 16777215
    static constexpr size_t _maxCountLen = sizeof(NAME_OF(4294967295)) - 1;     // SIZE_MAX == 4294967295

public:

    WebSocketServer(CWebServer& webServer) :
        _colorDataSocket("/ws/frames"),
        _effectChangeSocket("/ws/effects"),
        _webServer(webServer)
    {
        // Handler registration deferred to Start() so construction is
        // metadata-only and IsRunning() reflects the actual bound state.
    }

    ~WebSocketServer() override { Stop(); }

    // IService lifecycle. Start binds our handlers into the bound
    // CWebServer and flips our running flag; Stop unbinds them. We track
    // an independent running flag rather than aliasing CWebServer's so
    // callers can distinguish "websockets configured" from "web server up".
    bool Start() override
    {
        if (_running.load())
            return true;
        if (!_webServer.IsRunning())
        {
            debugW("WebSocketServer::Start ignored, bound CWebServer is not running");
            return false;
        }

        #if COLORDATA_WEB_SOCKET_ENABLED
            _webServer.AddWebSocket(_colorDataSocket);
        #endif
        #if EFFECTS_WEB_SOCKET_ENABLED
            _webServer.AddWebSocket(_effectChangeSocket);
        #endif

        _running.store(true);
        return true;
    }

    void Stop() override
    {
        if (!_running.load())
            return;

        // Close any open client connections so they don't outlast the
        // handler-removal step.
        _colorDataSocket.closeAll();
        _effectChangeSocket.closeAll();

        #if COLORDATA_WEB_SOCKET_ENABLED
            _webServer.RemoveWebSocket(_colorDataSocket);
        #endif
        #if EFFECTS_WEB_SOCKET_ENABLED
            _webServer.RemoveWebSocket(_effectChangeSocket);
        #endif

        _running.store(false);
    }

    bool IsRunning() const override   { return _running.load(); }
    const char* Name() const override { return "WebSocketServer"; }

    void CleanupClients()
    {
        _colorDataSocket.cleanupClients();
        _effectChangeSocket.cleanupClients();
    }

    bool HaveColorDataClients()
    {
        return _colorDataSocket.count() > 0;
    }

    // Send the color data for an array of leds of indicated length.
    void SendColorData(CRGB* leds, size_t count)
    {
        if (!HaveColorDataClients() || leds == nullptr || count == 0 || !_colorDataSocket.availableForWriteAll())
            return;

        _colorDataSocket.binaryAll((uint8_t *)leds, count * sizeof(CRGB));
    }

    void OnCurrentEffectChanged(size_t currentEffectIndex) override
    {
        if (_effectChangeSocket.availableForWriteAll())
            _effectChangeSocket.textAll(str_sprintf("{\"currentEffectIndex\":%zu}", currentEffectIndex));
    }

    void OnEffectListDirty() override
    {
        if (_effectChangeSocket.availableForWriteAll())
            _effectChangeSocket.textAll("{\"effectListDirty\":true}");
    }

    void OnEffectEnabledStateChanged(size_t effectIndex, bool newState) override
    {
        if (_effectChangeSocket.availableForWriteAll())
            _effectChangeSocket.textAll(str_sprintf("{\"effectsEnabledState\":[{\"index\":%zu,\"enabled\":%s}]}", effectIndex, newState ? "true" : "false"));
    }

    void OnIntervalChanged(uint interval) override
    {
        if (_effectChangeSocket.availableForWriteAll())
            _effectChangeSocket.textAll(str_sprintf("{\"interval\":%u}", interval));
    }
};
#endif // WEB_SOCKETS_ANY_ENABLED
