#pragma once

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

#include "globals.h"

#include <Arduino.h>
#include <esp_arduino_version.h>
#include <esp_task_wdt.h>
#include <utility>

class LEDStripEffect;

// Stack size for the taskmgr's idle threads
#define DEFAULT_STACK_SIZE (2048 + 512)

#define IDLE_STACK_SIZE    2048
#define DRAWING_STACK_SIZE 4096
#define AUDIO_STACK_SIZE   4096
#define JSON_STACK_SIZE    4096
#define SOCKET_STACK_SIZE  4096
#define NET_STACK_SIZE     8192
#define DEBUG_STACK_SIZE   8192                 // Needs a lot of stack for output if UpdateClockFromWeb is called from debugger
#define REMOTE_STACK_SIZE  4096
#define SCREEN_STACK_SIZE  8192

class IdleTask
{
  private:

    float _idleRatio = 0;
    unsigned long _lastMeasurement;

    const int kMillisPerLoop = 1;
    const int kMillisPerCalc = 1000;

    unsigned long counter = 0;

  public:

    void ProcessIdleTime();

    IdleTask() : _lastMeasurement(millis())
    {
    }

    // GetCPUUsage
    //
    // Returns 100 less the amount of idle time that we were able to squander.

    float GetCPUUsage() const;

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
protected:
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
            throw std::runtime_error("Invalid core passed to GetCPUUsagePercentCPU");
    }

    TaskManager()
    = default;

    // CheckHeap
    //
    // Quick and dirty debug test to make sure the heap has not been corrupted

    static void CheckHeap();

    void begin();

};

// NightDriverTaskManager
//
// A superclass of the base TaskManager that knows how to start and track the tasks specific to this project

void ScreenUpdateLoopEntry(void *);
void AudioSerialTaskEntry(void *);
void DrawLoopTaskEntry(void *);
void AudioSamplerTaskEntry(void *);
void NetworkHandlingLoopEntry(void *);
void DebugLoopTaskEntry(void *);
void SocketServerTaskEntry(void *);
void RemoteLoopEntry(void *);
void JSONWriterTaskEntry(void *);
void ColorDataTaskEntry(void *);

#define DELETE_TASK(handle) if (handle != nullptr) vTaskDelete(handle)

class NightDriverTaskManager : public TaskManager
{
public:

    using EffectTaskFunction = std::function<void(LEDStripEffect&)>;

    struct EffectTaskParams
    {
        EffectTaskFunction function;
        LEDStripEffect* pEffect;

        EffectTaskParams(EffectTaskFunction function, LEDStripEffect* pEffect);
    };

private:

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

    static void EffectTaskEntry(void *pVoid);

public:

    ~NightDriverTaskManager();

    void StartScreenThread();
    void StartSerialThread();
    void StartColorDataThread();
    void StartDrawThread();
    void StartAudioThread();
    void StartNetworkThread();
    void StartDebugThread();
    void StartSocketThread();
    void StartRemoteThread();
    void StartJSONWriterThread();

    void NotifyJSONWriterThread();
    void NotifyNetworkThread();

    // Effect threads run with NET priority and on the NET core by default. It seems a sensible choice
    //   because effect threads tend to pull things from the Internet that they want to show

    TaskHandle_t StartEffectThread(EffectTaskFunction function, LEDStripEffect* pEffect, const char* name, UBaseType_t priority = NET_PRIORITY, BaseType_t core = NET_CORE);
};
