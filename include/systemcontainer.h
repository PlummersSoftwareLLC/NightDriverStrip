#pragma once

#include "effectmanager.h"
#include "taskmgr.h"
#include "jsonserializer.h"
#include "network.h"
#include "deviceconfig.h"
#include "screen.h"

// SystemContainer
//
// Holds a number of system-wide objects that take care of a number of core/supportive functions on the chip.

class SystemContainer
{
  private:
    std::unique_ptr<::NightDriverTaskManager> pTaskManager = nullptr;
    std::unique_ptr<::EffectManager<GFXBase>> pEffectManager = nullptr;
    std::unique_ptr<::DeviceConfig> pDeviceConfig = nullptr;
    std::unique_ptr<::JSONWriter> pJSONWriter = nullptr;
    #if ENABLE_WIFI
        std::unique_ptr<::NetworkReader> pNetworkReader = nullptr;
    #endif
    #if USE_SCREEN
        std::unique_ptr<::Screen> pDisplay = nullptr;
    #endif

    template<typename Tp>
    inline void CheckPointer(const std::unique_ptr<Tp>& pointer, const String& name)
    {
        if (!pointer)
        {
            debugE("Calling getter for %s with pointer uninitialized!", name.c_str());
            delay(1000);
            throw std::runtime_error("Calling SystemContainer getter with uninitialized pointer!");
        }
    }

  public:
    // Creates and returns the EffectManager, passing any arguments on to that class' constructor
    template<typename... Args>
    void SetupEffectManager(Args&&... args)
    {
        if (!pEffectManager)
            pEffectManager = std::make_unique<::EffectManager<GFXBase>>(std::forward<Args>(args)...);
    }

    // Returns true if an EffectManager has been created using SetupEffectManager()
    bool HasEffectManager() const
    {
        return !!pEffectManager;
    }

    // Returns the EffectManager held by this container. Throws an exception if no EffectManager exists.
    ::EffectManager<GFXBase>& EffectManager()
    {
        CheckPointer(pEffectManager, ACTUAL_NAME_OF(pEffectManager));

        return *pEffectManager;
    }

    // Creates and returns the TaskManager
    ::NightDriverTaskManager& SetupTaskManager()
    {
        pTaskManager = std::make_unique<::NightDriverTaskManager>();
        pTaskManager->begin();

        return *pTaskManager;
    }

    // Returns true if a TaskManager has been created using SetupTaskManager()
    bool HasTaskManager() const
    {
        return !!pTaskManager;
    }

    // Returns the TaskManager held by this container. Throws an exception if no TaskManager exists.
    ::NightDriverTaskManager& TaskManager()
    {
        CheckPointer(pTaskManager, ACTUAL_NAME_OF(pTaskManager));

        return *pTaskManager;
    }

    // Creates and returns the config objects
    void SetupConfig()
    {
        // Create the JSON writer and start its background thread
        pJSONWriter = std::make_unique<::JSONWriter>();
        pTaskManager->StartJSONWriterThread();

        // Create and load device config from SPIFFS if possible
        pDeviceConfig = std::make_unique<::DeviceConfig>();
    }

    // Returns true if a JSONWriter has been created using SetupConfig()
    bool HasJSONWriter() const
    {
        return !!pJSONWriter;
    }

    // Returns the JSONWriter held by this container. Throws an exception if no JSONWriter exists.
    ::JSONWriter& JSONWriter()
    {
        return *pJSONWriter;
    }

    // Returns true if a DeviceConfig has been created using SetupConfig()
    bool HasDeviceConfig() const
    {
        return !!pDeviceConfig;
    }

    // Returns the DeviceConfig held by this container. Throws an exception if no DeviceConfig exists.
    ::DeviceConfig& DeviceConfig()
    {
        CheckPointer(pDeviceConfig, ACTUAL_NAME_OF(pDeviceConfig));

        return *pDeviceConfig;
    }

    #if ENABLE_WIFI
        // Creates and returns the NetworkReader
        ::NetworkReader& SetupNetworkReader()
        {
            pNetworkReader = std::make_unique<::NetworkReader>();

            return *pNetworkReader;
        }

        // Returns true if a NetworkReader has been created using SetupNetworkReader()
        bool HasNetworkReader() const
        {
            return !!pNetworkReader;
        }

        // Returns the NetworkReader held by this container. Throws an exception if no NetworkReader exists.
        ::NetworkReader& NetworkReader()
        {
            CheckPointer(pNetworkReader, ACTUAL_NAME_OF(pNetworkReader));

            return *pNetworkReader;
        }
    #endif

    #if USE_SCREEN
        // Creates and returns the display
        template<typename Ts, typename... Args>
        ::Screen& SetupDisplay(Args&&... args)
        {
            pDisplay = std::make_unique<Ts>(std::forward<Args>(args)...);

            return *pDisplay;
        }

        // Returns true if a display has been created using SetupDisplay()
        bool HasDisplay() const
        {
            return !!pDisplay;
        }

        // Returns the NetworkReader held by this container. Throws an exception if no NetworkReader exists.
        ::Screen& Display()
        {
            CheckPointer(pDisplay, ACTUAL_NAME_OF(pDisplay));

            return *pDisplay;
        }
    #endif
};

extern DRAM_ATTR std::unique_ptr<SystemContainer> g_ptrSystem;