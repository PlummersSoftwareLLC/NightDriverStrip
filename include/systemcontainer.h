#pragma once

//+--------------------------------------------------------------------------
//
// File:        systemcontainer.h
//
// NightDriverStrip - (c) 2023 Plummer's Software LLC.  All Rights Reserved.
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
//    Container class for core objects used throughout NightDriver
//
// History:     May-23-2023         Rbergen      Created
//
//---------------------------------------------------------------------------

#include "globals.h"
#include "interfaces.h"

#include <ArduinoJson.h>
#include <memory>
#include <vector>

template<typename T>
class psram_allocator;

class GFXBase;
class EffectManager;
class LEDStripEffect;
class JSONWriter;
class DeviceConfig;
class LEDBuffer;
class LEDBufferManager;
class TaskManager;
class RemoteControl;
class Screen;
class SocketServer;
class WebSocketServer;
class CWebServer;
class WS281xOutputManager;
class AudioService;
class AudioSerialBridge;
class DebugConsole;
class ColorStreamerService;
class RenderService;
namespace nd_network { class NetworkReader; }
using nd_network::NetworkReader;

enum NetworkPort : int;

// SystemContainer
//
// Holds a number of system-wide objects that take care of core/supportive functions on the chip.
class SystemContainer
{
  public:
    using DeviceContainer = std::vector<std::shared_ptr<GFXBase>>;
    using BufferManagerContainer = std::vector<LEDBufferManager, psram_allocator<LEDBufferManager>>;

  private:
    std::unique_ptr<DeviceContainer> _ptrDevices;
    std::unique_ptr<BufferManagerContainer> _ptrBufferManagers;
    std::unique_ptr<EffectManager> _ptrEffectManager;
    std::unique_ptr<TaskManager> _ptrTaskManager;
    std::unique_ptr<JSONWriter> _ptrJSONWriter;
    std::unique_ptr<DeviceConfig> _ptrDeviceConfig;

    #if ENABLE_REMOTE
        std::unique_ptr<RemoteControl> _ptrRemoteControl;
    #endif

    #if ENABLE_WIFI
        std::unique_ptr<NetworkReader> _ptrNetworkReader;
    #endif

    #if ENABLE_WIFI && ENABLE_WEBSERVER
        std::unique_ptr<CWebServer> _ptrWebServer;
    #endif

    #if INCOMING_WIFI_ENABLED
        std::unique_ptr<SocketServer> _ptrSocketServer;
    #endif

    #if USE_WS281X
        std::unique_ptr<WS281xOutputManager> _ptrWS281xOutputManager;
    #endif

    #if WEB_SOCKETS_ANY_ENABLED
        std::unique_ptr<WebSocketServer> _ptrWebSocketServer;
    #endif

    #if USE_SCREEN
        std::unique_ptr<Screen> _ptrDisplay;
    #endif

    // AudioService is constructed in audio-enabled and non-audio builds alike.
    // The service exposes a stable interface and degrades to a silent stub
    // when ENABLE_AUDIO is 0, which keeps consumers (e.g. webserver, drawing
    // VU meter) free of #if guards.

    std::unique_ptr<AudioService> _ptrAudioService;

    #if ENABLE_AUDIOSERIAL
        std::unique_ptr<AudioSerialBridge> _ptrAudioSerialBridge;
    #endif

    #if ENABLE_WIFI
        std::unique_ptr<DebugConsole> _ptrDebugConsole;
    #endif

    #if COLORDATA_SERVER_ENABLED
        std::unique_ptr<ColorStreamerService> _ptrColorStreamerService;
    #endif

    std::unique_ptr<RenderService> _ptrRenderService;

    // Helper method that checks if a pointer is initialized.
    void CheckPointer(bool initialized, const char* name) const;

  public:
    SystemContainer();
    ~SystemContainer();

    // Devices
    DeviceContainer& SetupDevices();
    bool HasDevices() const { return !!_ptrDevices; }
    DeviceContainer& GetDevices() const;

    // BufferManagers
    BufferManagerContainer& SetupBufferManagers();
    bool HasBufferManagers() const { return !!_ptrBufferManagers; }
    BufferManagerContainer& GetBufferManagers() const;

    // EffectManager
    EffectManager& SetupEffectManager(const std::shared_ptr<LEDStripEffect>& effect, DeviceContainer& devices);
    EffectManager& SetupEffectManager(DeviceContainer& devices);
    EffectManager& SetupEffectManager(const JsonObjectConst& jsonObject, DeviceContainer& devices);
    bool HasEffectManager() const { return !!_ptrEffectManager; }
    EffectManager& GetEffectManager() const;

    // TaskManager
    TaskManager& SetupTaskManager();
    bool HasTaskManager() const { return !!_ptrTaskManager; }
    TaskManager& GetTaskManager() const;

    // Config objects
    void SetupConfig();
    bool ApplyRuntimeConfiguration(String* errorMessage = nullptr);
    int GetConfiguredAudioInputPin() const;
    bool HasJSONWriter() const { return !!_ptrJSONWriter; }
    JSONWriter& GetJSONWriter() const;
    bool HasDeviceConfig() const { return !!_ptrDeviceConfig; }
    DeviceConfig& GetDeviceConfig() const;

    #if ENABLE_WIFI
        NetworkReader& SetupNetworkReader();
        bool HasNetworkReader() const { return !!_ptrNetworkReader; }
        NetworkReader& GetNetworkReader() const;
    #endif

    #if ENABLE_WIFI && ENABLE_WEBSERVER
        CWebServer& SetupWebServer();
        bool HasWebServer() const { return !!_ptrWebServer; }
        CWebServer& GetWebServer() const;
    #endif

    #if INCOMING_WIFI_ENABLED
        SocketServer& SetupSocketServer(NetworkPort port, int ledCount);
        bool HasSocketServer() const { return !!_ptrSocketServer; }
        SocketServer& GetSocketServer() const;
    #endif

    #if USE_WS281X
        WS281xOutputManager& SetupWS281xOutputManager();
        bool HasWS281xOutputManager() const { return !!_ptrWS281xOutputManager; }
        WS281xOutputManager& GetWS281xOutputManager() const;
    #endif

    #if WEB_SOCKETS_ANY_ENABLED
        WebSocketServer& SetupWebSocketServer(CWebServer& webServer);
        bool HasWebSocketServer() const { return !!_ptrWebSocketServer; }
        WebSocketServer& GetWebSocketServer() const;
    #endif

    #if ENABLE_REMOTE
        RemoteControl& SetupRemoteControl();
        bool HasRemoteControl() const { return !!_ptrRemoteControl; }
        RemoteControl& GetRemoteControl() const;
    #endif

    #if USE_SCREEN
        Screen& SetupHardwareDisplay(int w, int h);
        bool HasDisplay() const { return !!_ptrDisplay; }
        Screen& GetDisplay() const;
    #endif

    // AudioService. SetupAudioService() creates the service in stopped state;
    // call Start() on the returned reference once DeviceConfig is available.

    AudioService& SetupAudioService();
    bool HasAudioService() const { return nullptr != _ptrAudioService; }
    AudioService& GetAudioService() const;

    #if ENABLE_AUDIOSERIAL
        AudioSerialBridge& SetupAudioSerialBridge();
        bool HasAudioSerialBridge() const { return nullptr != _ptrAudioSerialBridge; }
        AudioSerialBridge& GetAudioSerialBridge() const;
    #endif

    #if ENABLE_WIFI
        DebugConsole& SetupDebugConsole();
        bool HasDebugConsole() const { return nullptr != _ptrDebugConsole; }
        DebugConsole& GetDebugConsole() const;
    #endif

    #if COLORDATA_SERVER_ENABLED
        ColorStreamerService& SetupColorStreamerService();
        bool HasColorStreamerService() const { return nullptr != _ptrColorStreamerService; }
        ColorStreamerService& GetColorStreamerService() const;
    #endif

    RenderService& SetupRenderService();
    bool HasRenderService() const { return nullptr != _ptrRenderService; }
    RenderService& GetRenderService() const;
};

extern std::unique_ptr<SystemContainer> g_ptrSystem;
