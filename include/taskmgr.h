//+--------------------------------------------------------------------------
//
// File:        taskmgr.h
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
//    Keeps track of CPU idle time and other stats.  Basic premise here
//    is that it creates an idle task on each core that burn any cycles they
//    can get.  They run at 1 above the system's own idle task priority so as
//    to not timeslice with them.
//
//    Since this totally starves those system idle tasks, the watchdog must
//    be turned off for them, which we do in begin().  We then turn the
//    watchdog on for our own idle tasks, and feed the watchdog in
//    ProcessIdleTime as we consume all available idle time.
//
//    BUGBUG(davepl): I think this means that vTaskDelete is never called
//                    since it was handled by the idle tasks.
//
//
// History:     Jul-12-2018         Davepl      Created
//              Apr-29-2019         Davepl      Adapted from BigBlueLCD project
//
//---------------------------------------------------------------------------

#pragma once

#include <Arduino.h>
#include <esp_task_wdt.h>
#include "ledstripeffect.h"

// Stack size for the taskmgr's idle threads
#define IDLE_STACK_SIZE 2048
#define DEFAULT_STACK_SIZE 2048+512
#define DRAWING_STACK_SIZE 4096
#define AUDIO_STACK_SIZE 4096


class IdleTask
{
  private:

    float _idleRatio = 0;
    unsigned long _lastMeasurement;

    const int kMillisPerLoop = 1;
    const int kMillisPerCalc = 1000;

    unsigned long counter = 0;

  public:

    void ProcessIdleTime()
    {
        _lastMeasurement = millis();
        counter = 0;

        // We need to whack the watchdog so we delay in smalle bites until we've used up all the time

        while (true)
        {
            int delta = millis() - _lastMeasurement;
            if (delta >= kMillisPerCalc)
            {
                //Serial.printf("Core %u Spent %lu in delay during a window of %d for a ratio of %f\n",
                //  xPortGetCoreID(), counter, delta, (float)counter/delta);
                _idleRatio = ((float) counter  / delta);
                _lastMeasurement = millis();
                counter = 0;
            }
            else
            {
                esp_task_wdt_reset();
                delayMicroseconds(kMillisPerLoop*1000);
                counter += kMillisPerLoop;
            }
        }
    }

    // If idle time is spent elsewhere, it can be credited to this task.  Shouldn't add up to more time than actual though!

    void CountBonusIdleMillis(uint millis)
    {
        counter += millis;
    }

    IdleTask() : _lastMeasurement(millis())
    {
    }

    // GetCPUUsage
    //
    // Returns 100 less the amount of idle time that we were able to squander.

    float GetCPUUsage() const
    {
        // If the measurement failed to even get a chance to run, this core is maxed and there was no idle time

        if (millis() - _lastMeasurement > kMillisPerCalc)
            return 100.0f;

        // Otherwise, whatever cycles we were able to burn in the idle loop counts as "would have been idle" time
        return 100.0f-100*_idleRatio;
    }

    // Stub entry point for calling into it without a THIS pointer

    static void IdleTaskEntry(void * that)
    {
        IdleTask * pTask = (IdleTask *)that;
        pTask->ProcessIdleTime();
    }
};

// TaskManager
//
// TaskManager runs two tasks at just over idle priority that do nothing but try to burn CPU, and they
// keep track of how much they can burn.   It's assumed that everything else runs at a higher priority
// and thus they "starve" the idle tasks when doing work.

class TaskManager
{
    TaskHandle_t _hIdle0 = nullptr;
    TaskHandle_t _hIdle1 = nullptr;

    IdleTask _taskIdle0;
    IdleTask _taskIdle1;

public:

    float GetCPUUsagePercent(int iCore = -1) const
    {
        if (iCore < 0)
            return (_taskIdle0.GetCPUUsage() + _taskIdle1.GetCPUUsage()) / 2;
        else if (iCore == 0)
            return _taskIdle0.GetCPUUsage();
        else if (iCore == 1)
            return _taskIdle1.GetCPUUsage();
        else
            throw new std::runtime_error("Invalid core passed to GetCPUUsagePercentCPU");
    }

    TaskManager()
    {
    }

    void begin()
    {
        Serial.printf("Replacing Idle Tasks with TaskManager...\n");
        // The idle tasks get created with a priority just ABOVE idle so that they steal idle time but nothing else.  They then
        // measure how much time is "wasted" at that lower priority and deem it to have been free CPU

        xTaskCreatePinnedToCore(_taskIdle0.IdleTaskEntry, "Idle0", IDLE_STACK_SIZE, &_taskIdle0, tskIDLE_PRIORITY + 1, &_hIdle0, 0);
        xTaskCreatePinnedToCore(_taskIdle1.IdleTaskEntry, "Idle1", IDLE_STACK_SIZE, &_taskIdle1, tskIDLE_PRIORITY + 1, &_hIdle1, 1);

        // We need to turn off the watchdogs because our idle measurement tasks burn all of the idle time just
        // to see how much there is (it's how they measure free CPU).  Thus, we starve the system's normal idle tasks
        // and have to feed the watchdog on our own.

        esp_task_wdt_delete(xTaskGetIdleTaskHandleForCPU(0));
        esp_task_wdt_delete(xTaskGetIdleTaskHandleForCPU(1));
        esp_task_wdt_add(_hIdle0);
        esp_task_wdt_add(_hIdle1);
    }

};

// NightDriverTaskManager
//
// A superclass of the base TaskManager that knows how to start and track the tasks specific to this project

void IRAM_ATTR ScreenUpdateLoopEntry(void *);
void IRAM_ATTR AudioSerialTaskEntry(void *);
void IRAM_ATTR DrawLoopTaskEntry(void *);
void IRAM_ATTR AudioSamplerTaskEntry(void *);
void IRAM_ATTR NetworkHandlingLoopEntry(void *);
void IRAM_ATTR DebugLoopTaskEntry(void *);
void IRAM_ATTR SocketServerTaskEntry(void *);
void IRAM_ATTR RemoteLoopEntry(void *);
void IRAM_ATTR JSONWriterTaskEntry(void *);
void IRAM_ATTR ColorDataTaskEntry(void *);

#define DELETE_TASK(handle) if (handle != nullptr) vTaskDelete(handle)

class NightDriverTaskManager : public TaskManager
{
public:

    using EffectTaskFunction = std::function<void(LEDStripEffect&)>;

private:

    struct EffectTaskParams
    {
        EffectTaskFunction function;
        LEDStripEffect* pEffect;

        EffectTaskParams(EffectTaskFunction function, LEDStripEffect* pEffect)
          : function(function),
            pEffect(pEffect)
        {}
    };

    TaskHandle_t _taskScreen        = nullptr;
    TaskHandle_t _taskNetwork       = nullptr;
    TaskHandle_t _taskDraw          = nullptr;
    TaskHandle_t _taskDebug         = nullptr;
    TaskHandle_t _taskAudio         = nullptr;
    TaskHandle_t _taskRemote        = nullptr;
    TaskHandle_t _taskSocket        = nullptr;
    TaskHandle_t _taskSerial        = nullptr;
    TaskHandle_t _taskColorData     = nullptr;
    TaskHandle_t _taskJSONWriter    = nullptr;

    std::vector<TaskHandle_t> _vEffectTasks;

    static void EffectTaskEntry(void *pVoid)
    {
        EffectTaskParams *pTaskParams = (EffectTaskParams *)pVoid;

        EffectTaskFunction function = pTaskParams->function;
        LEDStripEffect* pEffect = pTaskParams->pEffect;

        // Delete the params object before we invoke the actual function; they tend to run indefinitely
        delete pTaskParams;

        function(*pEffect);
    }

public:

    ~NightDriverTaskManager()
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

    void StartScreenThread()
    {
        #if USE_SCREEN
            Serial.print( str_sprintf(">> Launching Screen Thread.  Mem: %u, LargestBlk: %u, PSRAM Free: %u/%u, ", ESP.getFreeHeap(),ESP.getMaxAllocHeap(), ESP.getFreePsram(), ESP.getPsramSize()) );
            xTaskCreatePinnedToCore(ScreenUpdateLoopEntry, "Screen Loop", DEFAULT_STACK_SIZE, nullptr, SCREEN_PRIORITY, &_taskScreen, SCREEN_CORE);
        #endif
    }

    void StartSerialThread()
    {
        #if ENABLE_SERIAL
            Serial.print( str_sprintf(">> Launching Serial Thread.  Mem: %u, LargestBlk: %u, PSRAM Free: %u/%u, ", ESP.getFreeHeap(),ESP.getMaxAllocHeap(), ESP.getFreePsram(), ESP.getPsramSize()) );
            xTaskCreatePinnedToCore(AudioSerialTaskEntry, "Audio Serial Loop", DEFAULT_STACK_SIZE, nullptr, AUDIOSERIAL_PRIORITY, &_taskSerial, AUDIOSERIAL_CORE);
        #endif
    }

    void StartColorDataThread()
    {
        #if COLORDATA_SERVER_ENABLED
            Serial.print( str_sprintf(">> Launching ColorData Thread.  Mem: %u, LargestBlk: %u, PSRAM Free: %u/%u, ", ESP.getFreeHeap(),ESP.getMaxAllocHeap(), ESP.getFreePsram(), ESP.getPsramSize()) );
            xTaskCreatePinnedToCore(ColorDataTaskEntry, "ColorData Loop", DEFAULT_STACK_SIZE, nullptr, COLORDATA_PRIORITY, &_taskColorData, COLORDATA_CORE);
        #endif
    }

    void StartDrawThread()
    {
        Serial.print( str_sprintf(">> Launching Drawing Thread.  Mem: %u, LargestBlk: %u, PSRAM Free: %u/%u, ", ESP.getFreeHeap(),ESP.getMaxAllocHeap(), ESP.getFreePsram(), ESP.getPsramSize()) );
        xTaskCreatePinnedToCore(DrawLoopTaskEntry, "Draw Loop", DRAWING_STACK_SIZE, nullptr, DRAWING_PRIORITY, &_taskDraw, DRAWING_CORE);
    }

    void StartAudioThread()
    {
        #if ENABLE_AUDIO
            Serial.print( str_sprintf(">> Launching Audio Thread.  Mem: %u, LargestBlk: %u, PSRAM Free: %u/%u, ", ESP.getFreeHeap(),ESP.getMaxAllocHeap(), ESP.getFreePsram(), ESP.getPsramSize()) );
            xTaskCreatePinnedToCore(AudioSamplerTaskEntry, "Audio Sampler Loop", AUDIO_STACK_SIZE, nullptr, AUDIO_PRIORITY, &_taskAudio, AUDIO_CORE);
        #endif
    }

    void StartNetworkThread()
    {
        #if ENABLE_WIFI
            Serial.print( str_sprintf(">> Launching Network Thread.  Mem: %u, LargestBlk: %u, PSRAM Free: %u/%u, ", ESP.getFreeHeap(),ESP.getMaxAllocHeap(), ESP.getFreePsram(), ESP.getPsramSize()) );
            xTaskCreatePinnedToCore(NetworkHandlingLoopEntry, "NetworkHandlingLoop", DEFAULT_STACK_SIZE, nullptr, NET_PRIORITY, &_taskNetwork, NET_CORE);
        #endif
    }

    void StartDebugThread()
    {
        #if ENABLE_WIFI
            Serial.print( str_sprintf(">> Launching Debug Thread.  Mem: %u, LargestBlk: %u, PSRAM Free: %u/%u, ", ESP.getFreeHeap(),ESP.getMaxAllocHeap(), ESP.getFreePsram(), ESP.getPsramSize()) );
            xTaskCreatePinnedToCore(DebugLoopTaskEntry, "Debug Loop", DEFAULT_STACK_SIZE, nullptr, DEBUG_PRIORITY, &_taskDebug, DEBUG_CORE);
        #endif
    }

    void StartSocketThread()
    {
        #if INCOMING_WIFI_ENABLED
            Serial.print( str_sprintf(">> Launching Socket Thread.  Mem: %u, LargestBlk: %u, PSRAM Free: %u/%u, ", ESP.getFreeHeap(),ESP.getMaxAllocHeap(), ESP.getFreePsram(), ESP.getPsramSize()) );
            xTaskCreatePinnedToCore(SocketServerTaskEntry, "Socket Server Loop", DEFAULT_STACK_SIZE, nullptr, SOCKET_PRIORITY, &_taskSocket, SOCKET_CORE);
        #endif
    }

    void StartRemoteThread()
    {
        #if ENABLE_REMOTE
            Serial.print( str_sprintf(">> Launching Remote Thread.  Mem: %u, LargestBlk: %u, PSRAM Free: %u/%u, ", ESP.getFreeHeap(),ESP.getMaxAllocHeap(), ESP.getFreePsram(), ESP.getPsramSize()) );
            xTaskCreatePinnedToCore(RemoteLoopEntry, "IR Remote Loop", DEFAULT_STACK_SIZE, nullptr, REMOTE_PRIORITY, &_taskRemote, REMOTE_CORE);
        #endif
    }

    void StartJSONWriterThread()
    {
        Serial.print( str_sprintf(">> Launching JSON Writer Thread.  Mem: %u, LargestBlk: %u, PSRAM Free: %u/%u, ", ESP.getFreeHeap(),ESP.getMaxAllocHeap(), ESP.getFreePsram(), ESP.getPsramSize()) );
        xTaskCreatePinnedToCore(JSONWriterTaskEntry, "JSON Writer Loop", DEFAULT_STACK_SIZE, nullptr, JSONWRITER_PRIORITY, &_taskJSONWriter, JSONWRITER_CORE);
    }

    void NotifyJSONWriterThread()
    {
        if (_taskJSONWriter == nullptr)
            return;

        debugW(">> Notifying JSON Writer Thread");
        // Wake up the writer invoker task if it's sleeping, or request another write cycle if it isn't
        xTaskNotifyGive(_taskJSONWriter);
    }

    void NotifyNetworkThread()
    {
        if (_taskNetwork == nullptr)
            return;

        debugW(">> Notifying Network Thread");
        // Wake up the network task if it's sleeping, or request another read cycle if it isn't
        xTaskNotifyGive(_taskNetwork);
    }

    // Effect threads run with NET priority and on the NET core by default. It seems a sensible choice
    //   because effect threads tend to pull things from the Internet that they want to show
    TaskHandle_t StartEffectThread(EffectTaskFunction function, LEDStripEffect* pEffect, const char* name, UBaseType_t priority = NET_PRIORITY, BaseType_t core = NET_CORE)
    {
        // We use a raw pointer here just to cross the thread/task boundary. The EffectTaskEntry method
        //   deletes the object as soon as it can.
        EffectTaskParams* pTaskParams = new EffectTaskParams(function, pEffect);
        TaskHandle_t effectTask = nullptr;

        Serial.print( str_sprintf(">> Launching %s Effect Thread.  Mem: %u, LargestBlk: %u, PSRAM Free: %u/%u, ", name, ESP.getFreeHeap(),ESP.getMaxAllocHeap(), ESP.getFreePsram(), ESP.getPsramSize()) );

        if (xTaskCreatePinnedToCore(EffectTaskEntry, name, DEFAULT_STACK_SIZE, pTaskParams, priority, &effectTask, core) == pdPASS)
            _vEffectTasks.push_back(effectTask);
        else
            // Clean up the task params object if the thread was not actually created
            delete pTaskParams;

        return effectTask;
    }
};

extern NightDriverTaskManager g_TaskManager;

