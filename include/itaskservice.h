#pragma once

//+--------------------------------------------------------------------------
//
// File:        itaskservice.h
//
// NightDriverStrip - (c) 2026 Plummer's Software LLC.  All Rights Reserved.
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
//    ITaskService is the intermediate base class for IService implementers
//    that own a FreeRTOS task. It captures the standard launch / shutdown
//    discipline (atomics for run state, signal-and-wait shutdown, force-
//    delete fallback) in exactly one place so individual services like
//    AudioService, JSONWriter, SocketServer, and RemoteControl don't each
//    reimplement it.
//
//    A service that wants its own task should:
//
//      1. Inherit from ITaskService instead of directly from IService.
//      2. Implement GetTaskConfig() to specify the task's name, stack size,
//         priority, and core. The shutdown timeout has a sensible default
//         and only needs to be overridden when the task can block longer
//         (e.g. SocketServer with its accept timeout).
//      3. Implement Run() — the task body. Run() should poll
//         ShouldShutdown() between iterations and return when it sees true,
//         which triggers the rest of the cleanup automatically.
//      4. Optionally override OnBeforeStart() to gate the start (return
//         false to abort, e.g. when AudioConfig::enabled is false), or
//         OnBeforeWaitForStop() to break the task out of a blocking call
//         (e.g. close the listening socket so accept() returns), or
//         OnAfterStop() to release service-specific resources after the
//         task has confirmed exit (e.g. uninstall the I2S driver).
//
//    Start() and Stop() are marked final so a derived class can't acci-
//    dentally override the lifecycle and break the contract.
//
//    IMPORTANT - destruction discipline:
//
//      Every concrete derived class MUST call Stop() from its own destructor
//      *before* any derived state is torn down. Without that, the base
//      destructor would be the one calling Stop(), but by the time the base
//      runs the derived vtable has already been swapped out -- so virtual
//      hooks like OnBeforeWaitForStop() and OnAfterStop() would no longer
//      dispatch to the derived overrides, leaving the task blocked on a
//      socket/recv/etc. with no way to be unblocked. The boilerplate is just:
//
//          ~MyService() override { Stop(); }
//
//      Stop() is idempotent and a no-op when the task isn't running, so it's
//      always safe to call.
//
// History:     May-04-2026         Davepl      Created
//
//---------------------------------------------------------------------------

#include "globals.h"

#include <atomic>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#include "iservice.h"

class ITaskService : public IService
{
  public:

    // IService lifecycle. Final so derived classes use the hooks below
    // rather than reimplement the discipline.

    bool Start() override final;
    void Stop()  override final;
    bool IsRunning() const override final { return _running.load(); }

    // Restart() retains the IService default (Stop then Start).

  protected:

    struct TaskConfig
    {
        const char*  taskName             = "ITaskService";
        size_t       stackSize            = 4096;
        UBaseType_t  priority             = tskIDLE_PRIORITY + 1;
        BaseType_t   core                 = tskNO_AFFINITY;

        // How long Stop() waits for Run() to return after the shutdown
        // signal is raised. The default fits services whose loops yield
        // every few milliseconds; raise it for tasks that may block
        // longer if needed.

        uint32_t     shutdownTimeoutMs    = 1000;
    };

    // Derived class must implement these.

    virtual TaskConfig GetTaskConfig() const = 0;
    virtual void Run() = 0;

    // Optional hooks. Defaults are no-ops / accept-start.

    // Called from Start() before xTaskCreatePinnedToCore. Return false to
    // decline the start (e.g. when configuration is missing or audio is
    // disabled in this run). Logging is the implementer's responsibility.

    virtual bool OnBeforeStart()       { return true; }

    // Called from Stop() after the shutdown flag is raised but before
    // waiting for the task to acknowledge. Use to break the task out of
    // a blocking call: close a listening socket, abort an I2S read, etc.

    virtual void OnBeforeWaitForStop() {}

    // Called from Stop() once the task has confirmed exit (or been force-
    // deleted on timeout). Use for service-specific resource teardown:
    // uninstalling I2S/ADC drivers, releasing buffers, etc. Always runs,
    // even on the force-delete path, so it must be safe against partial
    // task progress.

    virtual void OnAfterStop()         {}

    // The task body polls this between iterations to detect a graceful
    // shutdown. Once it returns true, Run() should clean up any local
    // state and return; the framework handles vTaskDelete from there.

    bool ShouldShutdown() const { return _shutdownRequested.load(); }

    // Wake the owned task via xTaskNotifyGive. Useful for services whose
    // Run() loop sleeps in ulTaskNotifyTake and needs an external nudge
    // (e.g. JSONWriter, NetworkReader). Safe to call before Start() or
    // after Stop() — both are no-ops.

    void WakeTask() const
    {
        if (_taskHandle != nullptr)
            xTaskNotifyGive(_taskHandle);
    }

  private:

    // FreeRTOS task entry point. Casts back to ITaskService* and dispatches
    // to Run(). Records exit, then parks until Stop() deletes the task from
    // the caller's thread.

    static void TaskEntryThunk(void* p);

    TaskHandle_t       _taskHandle = nullptr;
    std::atomic<bool>  _running{false};
    std::atomic<bool>  _shutdownRequested{false};
    std::atomic<bool>  _taskExited{false};
};
