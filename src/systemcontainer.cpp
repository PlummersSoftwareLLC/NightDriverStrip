//+--------------------------------------------------------------------------
//
// File:        systemcontainer.cpp
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
//
//---------------------------------------------------------------------------

#include "globals.h"
#include "audioservice.h"
#include "audioserialbridge.h"
#include "colorstreamerservice.h"
#include "debugconsole.h"
#include "deviceconfig.h"
#include "renderservice.h"
#include "effectmanager.h"
#include "gfxbase.h"
#include "jsonserializer.h"
#include "ledbuffer.h"
#include "nd_network.h"
#include "remotecontrol.h"
#include "screen.h"
#include "socketserver.h"
#include "systemcontainer.h"
#include "taskmgr.h"
#include "webserver.h"
#include "websocketserver.h"
#if USE_WS281X
#include "ws281xgfx.h"
#include "ws281xoutputmanager.h"
#endif

// SystemContainer
//
// Holds a number of system-wide objects that take care of core/supportive functions on the chip.
// The objects are added to this class in an "enriched property" style. This means that for each object, the class
// contains:
// - A declaration of the member variable (using SC_DECLARE)
// - A Setup method that creates and returns the object in question (mostly through SC_SIMPLE_SETUP_FOR or
//   SC_FORWARDING_SETUP_FOR)
// - A Has method that returns true if the object has been Setup, and a property getter that returns a reference to
//   the actual object (both using SC_GETTERS_FOR)
//
// The difference between SC_SIMPLE_SETUP_FOR and SC_FORWARDING_SETUP_FOR is that the former invokes a parameterless
// constructor when creating the object, and the latter forwards any arguments passed to it on to the constructor.
//
// SC_SIMPLE_PROPERTY and SC_FORWARDING_PROPERTY are provided for convenience; they combine a declaration, simple or
// forwarding Setup method, and the Has and getter methods.
//
// Most macros accept two parameters:
// - The name of the property, as used in the Setup, Has and getter methods
// - The type of the property, as held/returned by this class
//
// The actual composition of this class is largely driven by the macros mentioned, only irregular Setup methods are
// coded manually.

SystemContainer::SystemContainer()
{
}

SystemContainer::~SystemContainer()
{
}

// Helper method that checks if a pointer is initialized. Throws a runtime error if not.
void SystemContainer::CheckPointer(bool initialized, const char* name) const
{
    if (!initialized)
    {
        debugE("Calling getter for %s with pointer uninitialized!", name);
        delay(1000);
        throw std::runtime_error("Calling SystemContainer getter with uninitialized pointer!");
    }
}

SystemContainer::DeviceContainer& SystemContainer::GetDevices() const
{
    CheckPointer(!!_ptrDevices, "Devices");
    return *_ptrDevices;
}

SystemContainer::BufferManagerContainer& SystemContainer::GetBufferManagers() const
{
    CheckPointer(!!_ptrBufferManagers, "BufferManagers");
    return *_ptrBufferManagers;
}

EffectManager& SystemContainer::GetEffectManager() const
{
    CheckPointer(!!_ptrEffectManager, "EffectManager");
    return *_ptrEffectManager;
}

TaskManager& SystemContainer::GetTaskManager() const
{
    CheckPointer(!!_ptrTaskManager, "TaskManager");
    return *_ptrTaskManager;
}

JSONWriter& SystemContainer::GetJSONWriter() const
{
    CheckPointer(!!_ptrJSONWriter, "JSONWriter");
    return *_ptrJSONWriter;
}

DeviceConfig& SystemContainer::GetDeviceConfig() const
{
    CheckPointer(!!_ptrDeviceConfig, "DeviceConfig");
    return *_ptrDeviceConfig;
}

#if ENABLE_WIFI
NetworkReader& SystemContainer::GetNetworkReader() const
{
    CheckPointer(!!_ptrNetworkReader, "NetworkReader");
    return *_ptrNetworkReader;
}
#endif

#if ENABLE_WIFI && ENABLE_WEBSERVER
CWebServer& SystemContainer::GetWebServer() const
{
    CheckPointer(!!_ptrWebServer, "WebServer");
    return *_ptrWebServer;
}
#endif

#if INCOMING_WIFI_ENABLED
SocketServer& SystemContainer::GetSocketServer() const
{
    CheckPointer(!!_ptrSocketServer, "SocketServer");
    return *_ptrSocketServer;
}
#endif

#if USE_WS281X
WS281xOutputManager& SystemContainer::GetWS281xOutputManager() const
{
    CheckPointer(!!_ptrWS281xOutputManager, "WS281xOutputManager");
    return *_ptrWS281xOutputManager;
}
#endif

#if WEB_SOCKETS_ANY_ENABLED
WebSocketServer& SystemContainer::GetWebSocketServer() const
{
    CheckPointer(!!_ptrWebSocketServer, "WebSocketServer");
    return *_ptrWebSocketServer;
}
#endif

#if ENABLE_REMOTE
RemoteControl& SystemContainer::GetRemoteControl() const
{
    CheckPointer(!!_ptrRemoteControl, "RemoteControl");
    return *_ptrRemoteControl;
}
#endif

#if USE_SCREEN
Screen& SystemContainer::GetDisplay() const
{
    CheckPointer(!!_ptrDisplay, "Display");
    return *_ptrDisplay;
}
#endif

SystemContainer::DeviceContainer& SystemContainer::SetupDevices()
{
    if (!_ptrDevices)
        _ptrDevices = std::make_unique<DeviceContainer>();
    return *_ptrDevices;
}

SystemContainer::BufferManagerContainer& SystemContainer::SetupBufferManagers()
{
    if (_ptrBufferManagers)
        return *_ptrBufferManagers;

    if (!_ptrDevices || _ptrDevices->empty())
    {
        debugE("Can't setup BufferManagers without Devices!");
        delay(1000);
        throw std::runtime_error("Attempt to setup BufferManagers without Devices");
    }

    #if USE_PSRAM
        uint32_t memtouse = ESP.getFreePsram() - RESERVE_MEMORY;
    #else
        uint32_t memtouse = ESP.getFreeHeap() - RESERVE_MEMORY;
    #endif

    uint32_t memtoalloc = 0;
    for (const auto& device : *_ptrDevices)
        memtoalloc += sizeof(LEDBuffer) + (device->GetLEDCount() * sizeof(CRGB));
    uint32_t cBuffers = memtouse / memtoalloc;

    if (cBuffers < MIN_BUFFERS)
    {
        debugI("Not enough memory, could only allocate %lu buffers and need %lu", (unsigned long)cBuffers, (unsigned long)MIN_BUFFERS);
        throw std::runtime_error("Could not allocate all buffers");
    }
    if (cBuffers > MAX_BUFFERS)
    {
        debugI("Could allocate %lu buffers but limiting it to %lu", (unsigned long)cBuffers, (unsigned long)MAX_BUFFERS);
        cBuffers = MAX_BUFFERS;
    }

    debugW("Reserving %lu LED buffers for a total of %lu bytes...", (unsigned long)cBuffers, (unsigned long)(memtoalloc * cBuffers));

    _ptrBufferManagers = make_unique_psram<BufferManagerContainer>();

    for (auto& device : *_ptrDevices)
        _ptrBufferManagers->emplace_back(cBuffers, device);

    return *_ptrBufferManagers;
}

EffectManager& SystemContainer::SetupEffectManager(const std::shared_ptr<LEDStripEffect>& effect, DeviceContainer& devices)
{
    if (!_ptrEffectManager)
        _ptrEffectManager = make_unique_psram<EffectManager>(effect, devices);
    return *_ptrEffectManager;
}

EffectManager& SystemContainer::SetupEffectManager(DeviceContainer& devices)
{
    if (!_ptrEffectManager)
        _ptrEffectManager = make_unique_psram<EffectManager>(devices);
    return *_ptrEffectManager;
}

EffectManager& SystemContainer::SetupEffectManager(const ArduinoJson::JsonObjectConst& jsonObject, DeviceContainer& devices)
{
    if (!_ptrEffectManager)
        _ptrEffectManager = make_unique_psram<EffectManager>(jsonObject, devices);
    return *_ptrEffectManager;
}

// Creates, begins and returns the TaskManager
TaskManager& SystemContainer::SetupTaskManager()
{
    if (!_ptrTaskManager)
    {
        _ptrTaskManager = make_unique_psram<TaskManager>();
        _ptrTaskManager->begin();
    }

    return *_ptrTaskManager;
}

// Creates and returns the config objects. Requires TaskManager to have already been setup.
void SystemContainer::SetupConfig()
{
    if (!_ptrTaskManager)
    {
        debugE("Can't setup config objects without TaskManager!");
        delay(1000);
        throw std::runtime_error("Attempt to setup config objects without TaskManager");
    }

    // Create the JSON writer and start its background thread. JSONWriter is an
    // IService so it owns its own task; we just call Start() here.
    
    if (!_ptrJSONWriter)
    {
        _ptrJSONWriter = make_unique_psram<JSONWriter>();
        _ptrJSONWriter->Start();
    }

    // Create and load device config from SPIFFS if possible
    if (!_ptrDeviceConfig)
        _ptrDeviceConfig = make_unique_psram<DeviceConfig>();
}

int SystemContainer::GetConfiguredAudioInputPin() const
{
    if (_ptrDeviceConfig)
        return _ptrDeviceConfig->GetAudioInputPin();

    return DeviceConfig::GetCompiledAudioInputPin();
}

bool SystemContainer::ApplyRuntimeConfiguration(String* errorMessage)
{
    std::scoped_lock guard(g_render_mutex, g_effect_manager_mutex);

    auto& config = GetDeviceConfig();

    if (config.RequiresRecompileForCurrentRuntimeConfig())
    {
        if (errorMessage)
            *errorMessage = "recompile needed";
        return false;
    }

    #if USE_WS281X
    try
    {
        if (_ptrDevices)
        {
            // Reconfiguring the already-owned GFX objects keeps the rest of the renderer stable while
            // still letting the active WS281x layout, channel count, and pins move inside build limits.
            for (auto& device : *_ptrDevices)
                device->ConfigureTopology(config.GetMatrixWidth(), config.GetMatrixHeight(), config.IsMatrixSerpentine());
        }

        if (_ptrBufferManagers && _ptrDevices)
        {
            for (size_t i = 0; i < _ptrBufferManagers->size() && i < _ptrDevices->size(); ++i)
                (*_ptrBufferManagers)[i].Reconfigure((*_ptrDevices)[i]);
        }

        #if INCOMING_WIFI_ENABLED
        if (_ptrSocketServer)
            _ptrSocketServer->SetLEDCount(config.GetActiveLEDCount());
        #endif

        // The mutable WS281x output layer is now always owned by NightDriver so live pin/count/color-order
        // changes stay on one transport path instead of handing off between different ESP32 RMT backends.
        if (_ptrDevices && !SetupWS281xOutputManager().ApplyConfig(config, *_ptrDevices, errorMessage))
            return false;

        if (_ptrEffectManager && !_ptrEffectManager->ReinitializeEffects())
        {
            if (errorMessage)
                *errorMessage = "failed to reinitialize effects for new topology";
            return false;
        }
    }
    catch (const std::bad_alloc&)
    {
        debugE("Runtime configuration failed due to insufficient memory");
        if (errorMessage)
            *errorMessage = "insufficient memory for runtime reconfiguration";
        return false;
    }
    #endif

    if (errorMessage)
        *errorMessage = "";

    return true;
}

#if ENABLE_WIFI
NetworkReader& SystemContainer::SetupNetworkReader()
{
    if (!_ptrNetworkReader)
        _ptrNetworkReader = make_unique_psram<NetworkReader>();
    return *_ptrNetworkReader;
}
#endif

#if ENABLE_WIFI && ENABLE_WEBSERVER
CWebServer& SystemContainer::SetupWebServer()
{
    if (!_ptrWebServer)
        _ptrWebServer = make_unique_psram<CWebServer>();
    return *_ptrWebServer;
}
#endif

#if ENABLE_REMOTE
RemoteControl& SystemContainer::SetupRemoteControl()
{
    if (!_ptrRemoteControl)
    {
        debugI("Remote configured: enabled=1 pin=%d", IR_REMOTE_PIN);
        _ptrRemoteControl = make_unique_psram<RemoteControl>();
    }
    return *_ptrRemoteControl;
}
#endif

#if INCOMING_WIFI_ENABLED
SocketServer& SystemContainer::SetupSocketServer(NetworkPort port, int ledCount)
{
    if (!_ptrSocketServer)
        _ptrSocketServer = make_unique_psram<SocketServer>(port, ledCount);
    else
        _ptrSocketServer->SetLEDCount(ledCount);
    return *_ptrSocketServer;
}
#endif

#if USE_WS281X
WS281xOutputManager& SystemContainer::SetupWS281xOutputManager()
{
    if (!_ptrWS281xOutputManager)
        _ptrWS281xOutputManager = make_unique_psram<WS281xOutputManager>();
    return *_ptrWS281xOutputManager;
}
#endif

#if WEB_SOCKETS_ANY_ENABLED
WebSocketServer& SystemContainer::SetupWebSocketServer(CWebServer& webServer)
{
    if (!_ptrWebSocketServer)
        _ptrWebSocketServer = make_unique_psram<WebSocketServer>(webServer);
    return *_ptrWebSocketServer;
}
#endif

#if USE_SCREEN
Screen& SystemContainer::SetupHardwareDisplay(int w, int h)
{
    _ptrDisplay = CreateHardwareScreen(w, h);
    return *_ptrDisplay;
}
#endif

AudioService& SystemContainer::SetupAudioService()
{
    if (!_ptrAudioService)
        _ptrAudioService = make_unique_psram<AudioService>();
    return *_ptrAudioService;
}

AudioService& SystemContainer::GetAudioService() const
{
    CheckPointer(!!_ptrAudioService, "AudioService");
    return *_ptrAudioService;
}

#if ENABLE_AUDIOSERIAL
AudioSerialBridge& SystemContainer::SetupAudioSerialBridge()
{
    if (!_ptrAudioSerialBridge)
        _ptrAudioSerialBridge = make_unique_psram<AudioSerialBridge>();
    return *_ptrAudioSerialBridge;
}

AudioSerialBridge& SystemContainer::GetAudioSerialBridge() const
{
    CheckPointer(!!_ptrAudioSerialBridge, "AudioSerialBridge");
    return *_ptrAudioSerialBridge;
}
#endif

#if ENABLE_WIFI
DebugConsole& SystemContainer::SetupDebugConsole()
{
    if (!_ptrDebugConsole)
        _ptrDebugConsole = make_unique_psram<DebugConsole>();
    return *_ptrDebugConsole;
}

DebugConsole& SystemContainer::GetDebugConsole() const
{
    CheckPointer(!!_ptrDebugConsole, "DebugConsole");
    return *_ptrDebugConsole;
}
#endif

#if COLORDATA_SERVER_ENABLED
ColorStreamerService& SystemContainer::SetupColorStreamerService()
{
    if (!_ptrColorStreamerService)
        _ptrColorStreamerService = make_unique_psram<ColorStreamerService>();
    return *_ptrColorStreamerService;
}

ColorStreamerService& SystemContainer::GetColorStreamerService() const
{
    CheckPointer(!!_ptrColorStreamerService, "ColorStreamerService");
    return *_ptrColorStreamerService;
}
#endif

RenderService& SystemContainer::SetupRenderService()
{
    if (!_ptrRenderService)
        _ptrRenderService = make_unique_psram<RenderService>();
    return *_ptrRenderService;
}

RenderService& SystemContainer::GetRenderService() const
{
    CheckPointer(!!_ptrRenderService, "RenderService");
    return *_ptrRenderService;
}
