//+--------------------------------------------------------------------------
//
// File:        taskmgr.cpp
//
// NightDriverStrip - (c) 2018 Plummer's Software LLC.  All Rights Reserved.
//
// Description:
//
//    Implementations for NightDriverTaskManager
//
// History:     Feb-25-2026         Created to thin taskmgr.h
//
//---------------------------------------------------------------------------

#include "globals.h"
#include "ledstripeffect.h"
#include "taskmgr.h"

#include <esp_task_wdt.h>

void IdleTask::ProcessIdleTime()
{
    _lastMeasurement = millis();
    counter = 0;

    // We need to whack the watchdog so we delay in smaller bites until we've used up all the time

    while (true)
    {
        int delta = millis() - _lastMeasurement;
        if (delta >= kMillisPerCalc)
        {
            // We've used up all the time, so calculate the idle ratio and reset the counter
            _idleRatio = ((float) counter  / delta);
            _lastMeasurement = millis();
            counter = 0;
        }
        else
        {
            // Burn a little time and update the counter
            esp_task_wdt_reset();
            delayMicroseconds(kMillisPerLoop*1000);
            counter += kMillisPerLoop;
        }
    }
}

float IdleTask::GetCPUUsage() const
{
    // If the measurement failed to even get a chance to run, this core is maxed and there was no idle time

    if (millis() - _lastMeasurement > kMillisPerCalc)
        return 100.0f;

    // Otherwise, whatever cycles we were able to burn in the idle loop counts as "would have been idle" time
    return 100.0f-100*_idleRatio;
}

void TaskManager::begin()
{
    Serial.printf("Replacing Idle Tasks with TaskManager...\n");
    // The idle tasks get created with a priority just ABOVE idle so that they steal idle time but nothing else.  They then
    // measure how much time is "wasted" at that lower priority and deem it to have been free CPU

    xTaskCreatePinnedToCore(IdleTask::IdleTaskEntry, "Idle0", IDLE_STACK_SIZE, &_taskIdle0, tskIDLE_PRIORITY + 1, &_hIdle0, 0);
    xTaskCreatePinnedToCore(IdleTask::IdleTaskEntry, "Idle1", IDLE_STACK_SIZE, &_taskIdle1, tskIDLE_PRIORITY + 1, &_hIdle1, 1);

    // We need to turn off the watchdogs because our idle measurement tasks burn all of the idle time just
    // to see how much there is (it's how they measure free CPU).  Thus, we starve the system's normal idle tasks
    // and have to feed the watchdog on our own.

#if ESP_ARDUINO_VERSION >= ESP_ARDUINO_VERSION_VAL(3, 0, 0)
    esp_task_wdt_delete(xTaskGetIdleTaskHandleForCore(0));
    esp_task_wdt_delete(xTaskGetIdleTaskHandleForCore(1));
#else
    esp_task_wdt_delete(xTaskGetIdleTaskHandleForCPU(0));
    esp_task_wdt_delete(xTaskGetIdleTaskHandleForCPU(1));
#endif
    esp_task_wdt_add(_hIdle0);
    esp_task_wdt_add(_hIdle1);
}

void TaskManager::CheckHeap()
{
    if (false == heap_caps_check_integrity_all(true))
    {
        throw std::runtime_error("Heap FAILED checks!");
    }
}

// NightDriverTaskManager
//
// A superclass of the base TaskManager that knows how to start and track the tasks specific to this project

NightDriverTaskManager::EffectTaskParams::EffectTaskParams(EffectTaskFunction function, LEDStripEffect* pEffect)
  : function(std::move(function)),
    pEffect(pEffect)
{}

NightDriverTaskManager::~NightDriverTaskManager()
{
    for (auto& task : _vEffectTasks)
        vTaskDelete(task);

    DELETE_TASK(_taskDraw);
    DELETE_TASK(_taskScreen);
    DELETE_TASK(_taskRemote);
    DELETE_TASK(_taskSerial);
    DELETE_TASK(_taskColorData);
    DELETE_TASK(_taskAudio);
    DELETE_TASK(_taskSocket);
    DELETE_TASK(_taskNetwork);
    DELETE_TASK(_taskJSONWriter);
    DELETE_TASK(_taskDebug);
}

void NightDriverTaskManager::EffectTaskEntry(void *pVoid)
{
    auto *pTaskParams = static_cast<EffectTaskParams *>(pVoid);

    EffectTaskFunction function = pTaskParams->function;
    LEDStripEffect* pEffect = pTaskParams->pEffect;

    // Delete the params object before we invoke the actual function; they tend to run indefinitely
    delete pTaskParams;

    function(*pEffect);
}

void NightDriverTaskManager::StartScreenThread()
{
    #if USE_SCREEN
        Serial.print( str_sprintf(">> Launching Screen Thread.  Mem: %zu, LargestBlk: %zu, PSRAM Free: %zu/%zu, ", (size_t)ESP.getFreeHeap(), (size_t)ESP.getMaxAllocHeap(), (size_t)ESP.getFreePsram(), (size_t)ESP.getPsramSize()) );
        xTaskCreatePinnedToCore(ScreenUpdateLoopEntry, "Screen Loop", SCREEN_STACK_SIZE, nullptr, SCREEN_PRIORITY, &_taskScreen, SCREEN_CORE);
        CheckHeap();
    #endif
}

void NightDriverTaskManager::StartSerialThread()
{
    #if ENABLE_AUDIOSERIAL
        Serial.print( str_sprintf(">> Launching Serial Thread.  Mem: %zu, LargestBlk: %zu, PSRAM Free: %zu/%zu, ", (size_t)ESP.getFreeHeap(), (size_t)ESP.getMaxAllocHeap(), (size_t)ESP.getFreePsram(), (size_t)ESP.getPsramSize()) );
        xTaskCreatePinnedToCore(AudioSerialTaskEntry, "Audio Serial Loop", DEFAULT_STACK_SIZE, nullptr, AUDIOSERIAL_PRIORITY, &_taskSerial, AUDIOSERIAL_CORE);
        CheckHeap();
    #endif
}

void NightDriverTaskManager::StartColorDataThread()
{
    #if COLORDATA_SERVER_ENABLED
        Serial.print( str_sprintf(">> Launching ColorData Thread.  Mem: %zu, LargestBlk: %zu, PSRAM Free: %zu/%zu, ", (size_t)ESP.getFreeHeap(), (size_t)ESP.getMaxAllocHeap(), (size_t)ESP.getFreePsram(), (size_t)ESP.getPsramSize()) );
        xTaskCreatePinnedToCore(ColorDataTaskEntry, "ColorData Loop", DEFAULT_STACK_SIZE, nullptr, COLORDATA_PRIORITY, &_taskColorData, COLORDATA_CORE);
        CheckHeap();
    #endif
}

void NightDriverTaskManager::StartDrawThread()
{
    Serial.print( str_sprintf(">> Launching Drawing Thread.  Mem: %zu, LargestBlk: %zu, PSRAM Free: %zu/%zu, ", (size_t)ESP.getFreeHeap(), (size_t)ESP.getMaxAllocHeap(), (size_t)ESP.getFreePsram(), (size_t)ESP.getPsramSize()) );
    xTaskCreatePinnedToCore(DrawLoopTaskEntry, "Draw Loop", DRAWING_STACK_SIZE, nullptr, DRAWING_PRIORITY, &_taskDraw, DRAWING_CORE);
    CheckHeap();
}

void NightDriverTaskManager::StartAudioThread()
{
    #if ENABLE_AUDIO
        Serial.print( str_sprintf(">> Launching Audio Thread.  Mem: %zu, LargestBlk: %zu, PSRAM Free: %zu/%zu, ", (size_t)ESP.getFreeHeap(), (size_t)ESP.getMaxAllocHeap(), (size_t)ESP.getFreePsram(), (size_t)ESP.getPsramSize()) );
        xTaskCreatePinnedToCore(AudioSamplerTaskEntry, "Audio Sampler Loop", AUDIO_STACK_SIZE, nullptr, AUDIO_PRIORITY, &_taskAudio, AUDIO_CORE);
        CheckHeap();
    #endif
}

void NightDriverTaskManager::StartNetworkThread()
{
    #if ENABLE_WIFI
        Serial.print( str_sprintf(">> Launching Network Thread.  Mem: %zu, LargestBlk: %zu, PSRAM Free: %zu/%zu, ", (size_t)ESP.getFreeHeap(), (size_t)ESP.getMaxAllocHeap(), (size_t)ESP.getFreePsram(), (size_t)ESP.getPsramSize()) );
        xTaskCreatePinnedToCore(NetworkHandlingLoopEntry, "NetworkHandlingLoop", NET_STACK_SIZE, nullptr, NET_PRIORITY, &_taskNetwork, NET_CORE);
        CheckHeap();
    #endif
}

void NightDriverTaskManager::StartDebugThread()
{
    #if ENABLE_WIFI
        Serial.print( str_sprintf(">> Launching Debug Thread.  Mem: %zu, LargestBlk: %zu, PSRAM Free: %zu/%zu, ", (size_t)ESP.getFreeHeap(), (size_t)ESP.getMaxAllocHeap(), (size_t)ESP.getFreePsram(), (size_t)ESP.getPsramSize()) );
        xTaskCreatePinnedToCore(DebugLoopTaskEntry, "Debug Loop", DEBUG_STACK_SIZE, nullptr, DEBUG_PRIORITY, &_taskDebug, DEBUG_CORE);
        CheckHeap();
    #endif
}

void NightDriverTaskManager::StartSocketThread()
{
    #if INCOMING_WIFI_ENABLED
        Serial.print( str_sprintf(">> Launching Socket Thread.  Mem: %zu, LargestBlk: %zu, PSRAM Free: %zu/%zu, ", (size_t)ESP.getFreeHeap(), (size_t)ESP.getMaxAllocHeap(), (size_t)ESP.getFreePsram(), (size_t)ESP.getPsramSize()) );
        xTaskCreatePinnedToCore(SocketServerTaskEntry, "Socket Server Loop", SOCKET_STACK_SIZE, nullptr, SOCKET_PRIORITY, &_taskSocket, SOCKET_CORE);
        CheckHeap();
    #endif
}

void NightDriverTaskManager::StartRemoteThread()
{
    #if ENABLE_REMOTE
        Serial.print( str_sprintf(">> Launching Remote Thread.  Mem: %zu, LargestBlk: %zu, PSRAM Free: %zu/%zu, ", (size_t)ESP.getFreeHeap(), (size_t)ESP.getMaxAllocHeap(), (size_t)ESP.getFreePsram(), (size_t)ESP.getPsramSize()) );
        xTaskCreatePinnedToCore(RemoteLoopEntry, "IR Remote Loop", REMOTE_STACK_SIZE, nullptr, REMOTE_PRIORITY, &_taskRemote, REMOTE_CORE);
        CheckHeap();
    #endif
}

void NightDriverTaskManager::StartJSONWriterThread()
{
    Serial.print( str_sprintf(">> Launching JSON Writer Thread.  Mem: %zu, LargestBlk: %zu, PSRAM Free: %zu/%zu, ", (size_t)ESP.getFreeHeap(), (size_t)ESP.getMaxAllocHeap(), (size_t)ESP.getFreePsram(), (size_t)ESP.getPsramSize()) );
    xTaskCreatePinnedToCore(JSONWriterTaskEntry, "JSON Writer Loop", JSON_STACK_SIZE, nullptr, JSONWRITER_PRIORITY, &_taskJSONWriter, JSONWRITER_CORE);
    CheckHeap();
}

void NightDriverTaskManager::NotifyJSONWriterThread()
{
    if (_taskJSONWriter == nullptr)
        return;

    debugW(">> Notifying JSON Writer Thread");
    // Wake up the writer invoker task if it's sleeping, or request another write cycle if it isn't
    xTaskNotifyGive(_taskJSONWriter);
}

void NightDriverTaskManager::NotifyNetworkThread()
{
    if (_taskNetwork == nullptr)
        return;

    debugW(">> Notifying Network Thread");
    // Wake up the network task if it's sleeping, or request another read cycle if it isn't
    xTaskNotifyGive(_taskNetwork);
}

// Effect threads run with NET priority and on the NET core by default. It seems a sensible choice
//   because effect threads tend to pull things from the Internet that they want to show

TaskHandle_t NightDriverTaskManager::StartEffectThread(EffectTaskFunction function, LEDStripEffect* pEffect, const char* name, UBaseType_t priority, BaseType_t core)
{
    // We use a raw pointer here just to cross the thread/task boundary. The EffectTaskEntry method
    //   deletes the object as soon as it can.
    auto* pTaskParams = new EffectTaskParams(std::move(function), pEffect);
    TaskHandle_t effectTask = nullptr;

    Serial.print( str_sprintf(">> Launching %s Effect Thread.  Mem: %zu, LargestBlk: %zu, PSRAM Free: %zu/%zu, ", name, (size_t)ESP.getFreeHeap(), (size_t)ESP.getMaxAllocHeap(), (size_t)ESP.getFreePsram(), (size_t)ESP.getPsramSize()) );

    if (xTaskCreatePinnedToCore(EffectTaskEntry, name, DEFAULT_STACK_SIZE, pTaskParams, priority, &effectTask, core) == pdPASS)
        _vEffectTasks.push_back(effectTask);
    else
        // Clean up the task params object if the thread was not actually created
        delete pTaskParams;

    CheckHeap();

    return effectTask;
}
