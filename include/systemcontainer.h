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

#pragma once

#include "effectmanager.h"
#include "taskmgr.h"
#include "jsonserializer.h"
#include "network.h"
#include "deviceconfig.h"
#include "screen.h"
#include "socketserver.h"
#include "remotecontrol.h"
#include "webserver.h"
#include "types.h"

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

// Name of the unique_ptr member for a specific property name
#define SC_MEMBER(name) _ptr ## name

// Declares the member variable for a property with indicated type and name
#define SC_DECLARE(name, ...) \
  private: \
    std::unique_ptr<::__VA_ARGS__> SC_MEMBER(name) = nullptr;

// Creates a Setup method for a property (with indicated type and name) that invokes a parameterless constructor
#define SC_SIMPLE_SETUP_FOR(name, ...) \
  public: \
    ::__VA_ARGS__& Setup ## name() \
    { \
        if (!SC_MEMBER(name)) \
            SC_MEMBER(name).reset(new ::__VA_ARGS__()); \
        return *SC_MEMBER(name); \
    }

// Creates a Setup method for a property (with indicated type and name) that forwards any arguments to the constructor
#define SC_FORWARDING_SETUP_FOR(name, ...) \
  public: \
    template<typename... Args> \
    ::__VA_ARGS__& Setup ## name(Args&&... args) \
    { \
        if (!SC_MEMBER(name)) \
            SC_MEMBER(name).reset(new ::__VA_ARGS__(std::forward<Args>(args)...)); \
        return *SC_MEMBER(name); \
    }

// Creates the Has and getter methods for a property with indicated type and name
#define SC_GETTERS_FOR(name, ...) \
  public: \
    bool Has ## name() const \
    { \
        return !!SC_MEMBER(name); \
    } \
    \
    ::__VA_ARGS__& name() const \
    { \
        CheckPointer(SC_MEMBER(name), #name); \
        return *SC_MEMBER(name); \
    }

// Creates a full property with the type and name indicated, having a simple Setup method
#define SC_SIMPLE_PROPERTY(name, ...) \
    SC_DECLARE(name, __VA_ARGS__) \
    SC_SIMPLE_SETUP_FOR(name, __VA_ARGS__) \
    SC_GETTERS_FOR(name, __VA_ARGS__)

// Creates a full property with the type and name indicated, having a forwarding Setup method
#define SC_FORWARDING_PROPERTY(name, ...) \
    SC_DECLARE(name, __VA_ARGS__) \
    SC_FORWARDING_SETUP_FOR(name, __VA_ARGS__) \
    SC_GETTERS_FOR(name, __VA_ARGS__)

class SystemContainer
{
  private:
    // Helper method that checks if a pointer is initialized. Throws a runtime error if not.
    template<typename Tp>
    inline void CheckPointer(const std::unique_ptr<Tp>& pointer, const String& name) const
    {
        if (!pointer)
        {
            debugE("Calling getter for %s with pointer uninitialized!", name.c_str());
            delay(1000);
            throw std::runtime_error("Calling SystemContainer getter with uninitialized pointer!");
        }
    }

    // -------------------------------------------------------------
    // Devices

    SC_SIMPLE_PROPERTY(Devices, std::vector<std::shared_ptr<GFXBase>>)

    // -------------------------------------------------------------
    // BufferManagers

    SC_DECLARE(BufferManagers, std::vector<LEDBufferManager, psram_allocator<LEDBufferManager>>)

    public: std::vector<LEDBufferManager, psram_allocator<LEDBufferManager>>& SetupBufferManagers()
    {
        if (!!SC_MEMBER(BufferManagers))
            return *SC_MEMBER(BufferManagers);

        if (!SC_MEMBER(Devices) || SC_MEMBER(Devices)->size() == 0)
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

        uint32_t memtoalloc = (SC_MEMBER(Devices)->size() * (sizeof(LEDBuffer) + NUM_LEDS * sizeof(CRGB)));
        uint32_t cBuffers = memtouse / memtoalloc;

        if (cBuffers < MIN_BUFFERS)
        {
            debugI("Not enough memory, could only allocate %d buffers and need %d\n", cBuffers, MIN_BUFFERS);
            throw std::runtime_error("Could not allocate all buffers");
        }
        if (cBuffers > MAX_BUFFERS)
        {
            debugI("Could allocate %d buffers but limiting it to %d\n", cBuffers, MAX_BUFFERS);
            cBuffers = MAX_BUFFERS;
        }

        debugW("Reserving %d LED buffers for a total of %d bytes...", cBuffers, memtoalloc * cBuffers);

        SC_MEMBER(BufferManagers) = make_unique_psram<std::vector<LEDBufferManager, psram_allocator<LEDBufferManager>>>();

        for (auto& device : *SC_MEMBER(Devices))
            SC_MEMBER(BufferManagers)->emplace_back(cBuffers, device);

        return *SC_MEMBER(BufferManagers);
    }

    SC_GETTERS_FOR(BufferManagers, std::vector<LEDBufferManager, psram_allocator<LEDBufferManager>>)

    // -------------------------------------------------------------
    // EffectManager

    SC_FORWARDING_PROPERTY(EffectManager, EffectManager)

    // -------------------------------------------------------------
    // TaskManager

    SC_DECLARE(TaskManager, NightDriverTaskManager)

    // Creates, begins and returns the TaskManager
    public: ::NightDriverTaskManager& SetupTaskManager()
    {
        if (!SC_MEMBER(TaskManager))
        {
            SC_MEMBER(TaskManager) = make_unique_psram<::NightDriverTaskManager>();
            SC_MEMBER(TaskManager)->begin();
        }

        return *SC_MEMBER(TaskManager);
    }

    SC_GETTERS_FOR(TaskManager, NightDriverTaskManager)

    // -------------------------------------------------------------
    // Config objects: JSONWriter, DeviceConfig

    SC_DECLARE(DeviceConfig, DeviceConfig)
    SC_DECLARE(JSONWriter, JSONWriter)

    // Creates and returns the config objects. Requires TaskManager to have already been setup.
    public: void SetupConfig()
    {
        if (!SC_MEMBER(TaskManager))
        {
            debugE("Can't setup config objects without TaskManager!");
            delay(1000);
            throw std::runtime_error("Attempt to setup config objects without TaskManager");
        }

        // Create the JSON writer and start its background thread
        if (!SC_MEMBER(JSONWriter))
        {
            SC_MEMBER(JSONWriter) = make_unique_psram<::JSONWriter>();
            SC_MEMBER(TaskManager)->StartJSONWriterThread();
        }

        // Create and load device config from SPIFFS if possible
        if (!SC_MEMBER(DeviceConfig))
            SC_MEMBER(DeviceConfig) = make_unique_psram<::DeviceConfig>();
    }

    SC_GETTERS_FOR(JSONWriter, JSONWriter)
    SC_GETTERS_FOR(DeviceConfig, DeviceConfig)

    // -------------------------------------------------------------
    // NetworkReader

    #if ENABLE_WIFI
        SC_SIMPLE_PROPERTY(NetworkReader, NetworkReader)
    #endif

    // -------------------------------------------------------------
    // WebServer

    #if ENABLE_WIFI && ENABLE_WEBSERVER
        SC_SIMPLE_PROPERTY(WebServer, CWebServer)
    #endif

    // -------------------------------------------------------------
    // SocketServer

    #if INCOMING_WIFI_ENABLED
        SC_FORWARDING_PROPERTY(SocketServer, SocketServer)
    #endif

    // -------------------------------------------------------------
    // RemoteControl

    #if ENABLE_REMOTE
        SC_SIMPLE_PROPERTY(RemoteControl, RemoteControl)
    #endif

    // -------------------------------------------------------------
    // Display

    #if USE_SCREEN
        SC_DECLARE(Display, Screen)

        // Creates and returns the display. The exact screen type is a template argument.
        public: template<typename Ts, typename... Args>
        ::Screen& SetupDisplay(Args&&... args)
        {
            SC_MEMBER(Display) = make_unique_psram<Ts>(std::forward<Args>(args)...);

            return *SC_MEMBER(Display);
        }

        SC_GETTERS_FOR(Display, Screen)
    #endif
};

extern DRAM_ATTR std::unique_ptr<SystemContainer> g_ptrSystem;
