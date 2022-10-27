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

#define IDLE_STACK_SIZE 2048        
// Stack size for the taskmgr's idle threads

class IdleTask
{
  private:

    double _idleRatio = 0;
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
                //  xPortGetCoreID(), counter, delta, (double)counter/delta);
                _idleRatio = ((double) counter  / delta);
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

    double GetCPUUsage() const
    {
        // If the measurement failed to even get a chance to run, this core is maxed and there was no idle time

        if (millis() - _lastMeasurement > kMillisPerCalc)
            return 100.0;

        // Otherwise, whatever cycles we were able to burn in the idle loop counts as "would have been idle" time
        return 100.0-100*_idleRatio;
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

    double GetCPUUsagePercent(int iCore = -1) const
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
        // The idle tasks get created with a priority just ABOVE idle so that they steal idle time but nothing else.  They then
        // measure how much time is "wasted" at that lower priority and deem it to have been free CPU

        xTaskCreatePinnedToCore(_taskIdle0.IdleTaskEntry, "Idle0", IDLE_STACK_SIZE, &_taskIdle0, tskIDLE_PRIORITY + 1, &_hIdle0, 0);
        xTaskCreatePinnedToCore(_taskIdle1.IdleTaskEntry, "Idle1", IDLE_STACK_SIZE, &_taskIdle1, tskIDLE_PRIORITY + 1, &_hIdle1, 1);

        // We need to turn off the watchdogs because our idle measurement tasks burn all of the idle time just
        // to see how much there is (it's how they measure free CPU).

        esp_task_wdt_delete(xTaskGetIdleTaskHandleForCPU(0));
        esp_task_wdt_delete(xTaskGetIdleTaskHandleForCPU(1));
        esp_task_wdt_add(_hIdle0);
        esp_task_wdt_add(_hIdle1);
    }

};

