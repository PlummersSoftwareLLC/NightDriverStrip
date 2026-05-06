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
#include <freertos/task.h>
#include <functional>
#include <utility>
#include <vector>

class LEDStripEffect;

// Stack size for the taskmgr's idle threads
#define DEFAULT_STACK_SIZE (2048 + 512)

#define IDLE_STACK_SIZE    2048
#define DRAWING_STACK_SIZE 4096
#define AUDIO_STACK_SIZE   4096
#define JSON_STACK_SIZE    4096
#define SOCKET_STACK_SIZE  4096
#define NET_STACK_SIZE     8192
#define COLORDATA_STACK_SIZE 4096
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
// Owns the two pinned idle-priority "CPU monitor" tasks that measure how much
// CPU the rest of the system *isn't* using, and the per-effect task launcher
// (StartEffectThread) for effects that want to spin off their own background
// thread. All project-level service threads (AudioService, AudioSerialBridge,
// ColorStreamerService, DebugConsole, JSONWriter, NetworkReader,
// RemoteControl, RenderService, Screen, SocketServer) live as Run() methods
// on their respective IService classes via ITaskService and are launched
// through g_ptrSystem->Get*().Start() — TaskManager doesn't track them.

class TaskManager
{
public:
    using EffectTaskFunction = std::function<void(LEDStripEffect&)>;

    struct EffectTaskParams
    {
        EffectTaskFunction function;
        LEDStripEffect* pEffect;

        EffectTaskParams(EffectTaskFunction function, LEDStripEffect* pEffect);
    };

protected:
    TaskHandle_t _hIdle0 = nullptr;
    TaskHandle_t _hIdle1 = nullptr;

    IdleTask _taskIdle0;
    IdleTask _taskIdle1;

private:
    std::vector<TaskHandle_t> _vEffectTasks;

    static void EffectTaskEntry(void *pVoid);

public:
    TaskManager() = default;
    ~TaskManager();

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

    // CheckHeap
    //
    // Quick and dirty debug test to make sure the heap has not been corrupted

    static void CheckHeap();

    void begin();

    // Effect threads run with NET priority and on the NET core by default. It seems a sensible choice
    //   because effect threads tend to pull things from the Internet that they want to show

    TaskHandle_t StartEffectThread(EffectTaskFunction function, LEDStripEffect* pEffect, const char* name, UBaseType_t priority = NET_PRIORITY, BaseType_t core = NET_CORE);
};
