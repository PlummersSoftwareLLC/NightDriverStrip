//+--------------------------------------------------------------------------
//
// File:        taskmgr.cpp
//
// NightDriverStrip - (c) 2018 Plummer's Software LLC.  All Rights Reserved.
//
// Description:
//
//    Implementations for TaskManager. Owns the two pinned idle-priority
//    CPU-measurement tasks and the per-effect ad-hoc task launcher. All
//    project-level service tasks live as Run() methods on their respective
//    IService classes via ITaskService and are launched there.
//
// History:     Feb-25-2026         Created to thin taskmgr.h
//
//---------------------------------------------------------------------------

#include "globals.h"
#include "ledstripeffect.h"
#include "nd_network.h"
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

TaskManager::EffectTaskParams::EffectTaskParams(EffectTaskFunction function, LEDStripEffect* pEffect)
  : function(std::move(function)),
    pEffect(pEffect)
{}

TaskManager::~TaskManager()
{
    for (auto& task : _vEffectTasks)
        vTaskDelete(task);

    // The two idle tasks are intentionally not deleted; they're the entire
    // reason this object exists and they run for the lifetime of the chip.
}

void TaskManager::EffectTaskEntry(void *pVoid)
{
    auto *pTaskParams = static_cast<EffectTaskParams *>(pVoid);

    EffectTaskFunction function = pTaskParams->function;
    LEDStripEffect* pEffect = pTaskParams->pEffect;

    // Delete the params object before we invoke the actual function; they tend to run indefinitely
    delete pTaskParams;

    function(*pEffect);
}

// Effect threads run with NET priority and on the NET core by default. It seems a sensible choice
//   because effect threads tend to pull things from the Internet that they want to show

TaskHandle_t TaskManager::StartEffectThread(EffectTaskFunction function, LEDStripEffect* pEffect, const char* name, UBaseType_t priority, BaseType_t core)
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
