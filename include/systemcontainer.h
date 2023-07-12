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
// - The type of the property, as held/returned by this class
// - The name of the property, as used in the Setup, Has and getter methods
//
// The actual composition of this class is largely driven by the macros mentioned, only irregular Setup methods are
// coded manually.

// Name of the unique_ptr member for a specific property name
#define SC_MEMBER(name) _ptr ## name

// Declares the member variable for a property with indicated type and name
#define SC_DECLARE(typeName, name) \
  private: \
    std::unique_ptr<::typeName> SC_MEMBER(name) = nullptr;

// Creates a Setup method for a property (with indicated type and name) that invokes a parameterless constructor
#define SC_SIMPLE_SETUP_FOR(typeName, name) \
  public: \
    ::typeName& Setup ## name() \
    { \
        if (!SC_MEMBER(name)) \
            SC_MEMBER(name).reset(new ::typeName()); \
        return *SC_MEMBER(name); \
    }

// Creates a Setup method for a property (with indicated type and name) that forwards any arguments to the constructor
#define SC_FORWARDING_SETUP_FOR(typeName, name) \
  public: \
    template<typename... Args> \
    ::typeName& Setup ## name(Args&&... args) \
    { \
        if (!SC_MEMBER(name)) \
            SC_MEMBER(name).reset(new ::typeName(std::forward<Args>(args)...)); \
        return *SC_MEMBER(name); \
    }

// Creates the Has and getter methods for a property with indicated type and name
#define SC_GETTERS_FOR(typeName, name) \
  public: \
    bool Has ## name() const \
    { \
        return !!SC_MEMBER(name); \
    } \
    \
    ::typeName& name() const \
    { \
        CheckPointer(SC_MEMBER(name), #name); \
        return *SC_MEMBER(name); \
    }

// Creates a full property with the type and name indicated, having a simple Setup method
#define SC_SIMPLE_PROPERTY(typeName, name) \
    SC_DECLARE(typeName, name) \
    SC_SIMPLE_SETUP_FOR(typeName, name) \
    SC_GETTERS_FOR(typeName, name)

// Creates a full property with the type and name indicated, having a forwarding Setup method
#define SC_FORWARDING_PROPERTY(typeName, name) \
    SC_DECLARE(typeName, name) \
    SC_FORWARDING_SETUP_FOR(typeName, name) \
    SC_GETTERS_FOR(typeName, name)

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
    // EffectManager

    SC_FORWARDING_PROPERTY(EffectManager<GFXBase>, EffectManager)

    // -------------------------------------------------------------
    // TaskManager

    SC_DECLARE(NightDriverTaskManager, TaskManager)

    // Creates, begins and returns the TaskManager
    public: ::NightDriverTaskManager& SetupTaskManager()
    {
        if (!SC_MEMBER(TaskManager))
        {
            SC_MEMBER(TaskManager) = std::make_unique<::NightDriverTaskManager>();
            SC_MEMBER(TaskManager)->begin();
        }

        return *SC_MEMBER(TaskManager);
    }

    SC_GETTERS_FOR(NightDriverTaskManager, TaskManager)

    // -------------------------------------------------------------
    // Config objects: JSONWriter, DeviceConfig

    SC_DECLARE(DeviceConfig, DeviceConfig);
    SC_DECLARE(JSONWriter, JSONWriter);

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
            SC_MEMBER(JSONWriter) = std::make_unique<::JSONWriter>();
            SC_MEMBER(TaskManager)->StartJSONWriterThread();
        }

        // Create and load device config from SPIFFS if possible
        if (!SC_MEMBER(DeviceConfig))
            SC_MEMBER(DeviceConfig) = std::make_unique<::DeviceConfig>();
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
        SC_SIMPLE_PROPERTY(CWebServer, WebServer)
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
        SC_DECLARE(Screen, Display)

        // Creates and returns the display. The exact screen type is a template argument.
        public: template<typename Ts, typename... Args>
        ::Screen& SetupDisplay(Args&&... args)
        {
            SC_MEMBER(Display) = std::make_unique<Ts>(std::forward<Args>(args)...);

            return *SC_MEMBER(Display);
        }

        SC_GETTERS_FOR(Screen, Display)
    #endif
};

extern DRAM_ATTR std::unique_ptr<SystemContainer> g_ptrSystem;