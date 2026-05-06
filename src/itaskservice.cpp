//+--------------------------------------------------------------------------
//
// File:        itaskservice.cpp
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
//    Implementation of ITaskService. Captures the launch/shutdown discipline
//    that used to be duplicated across AudioService, JSONWriter,
//    SocketServer, and RemoteControl into one place.
//
//---------------------------------------------------------------------------

#include "globals.h"

#include <exception>

#include "itaskservice.h"

// ITaskService provides a Start() method that launches a FreeRTOS task running
// the ITaskService::Run() method, and a Stop() method that signals the task to
// exit and waits for it to do so, with a force-delete fallback if it doesn't
// within a reasonable timeout. The task entry point is a static thunk that
// casts the void* back to ITaskService* and calls Run(), with try/catch to
// log any exceptions that escape from Run().

bool ITaskService::Start()
{
    if (_running.load())
    {
        debugI("%s: Start() ignored, already running", Name());
        return true;
    }

    if (!OnBeforeStart())
    {
        debugI("%s: Start() declined by OnBeforeStart hook", Name());
        return false;
    }

    _shutdownRequested.store(false);
    _taskExited.store(false);

    const TaskConfig cfg = GetTaskConfig();

    Serial.print( str_sprintf(">> Launching %s.  Mem: %zu, LargestBlk: %zu, PSRAM Free: %zu/%zu, ",
        cfg.taskName,
        (size_t)ESP.getFreeHeap(), (size_t)ESP.getMaxAllocHeap(),
        (size_t)ESP.getFreePsram(), (size_t)ESP.getPsramSize()) );

    BaseType_t rc = xTaskCreatePinnedToCore(
        TaskEntryThunk,
        cfg.taskName,
        cfg.stackSize,
        this,                       // ITaskService*; thunk casts back
        cfg.priority,
        &_taskHandle,
        cfg.core);

    if (rc != pdPASS)
    {
        debugE("%s: xTaskCreatePinnedToCore failed (rc=%d)", Name(), (int)rc);
        _taskHandle = nullptr;
        return false;
    }

    _running.store(true);
    return true;
}

void ITaskService::Stop()
{
    if (!_running.load() && _taskHandle == nullptr)
        return;

    debugI("%s: stop requested", Name());

    if (_taskHandle != nullptr)
    {
        _shutdownRequested.store(true);

        // Give the implementer a chance to break the task out of any
        // blocking call (e.g. close a listening socket, abort an I2S read).

        OnBeforeWaitForStop();

        const uint32_t timeoutMs = GetTaskConfig().shutdownTimeoutMs;
        const uint32_t deadline = millis() + timeoutMs;
        while (!_taskExited.load() && (int32_t)(deadline - millis()) > 0)
            delay(5);

        if (!_taskExited.load())
            debugW("%s: task did not exit gracefully within %lums; deleting forcibly",
                   Name(), (unsigned long)timeoutMs);

        // The task either acknowledged exit (and is now parked in
        // TaskEntryThunk's vTaskDelay) or it never acknowledged. Either
        // way, we delete it from this thread so the FreeRTOS task control
        // block is reclaimed before this object is allowed to be destroyed.
        vTaskDelete(_taskHandle);
        _taskHandle = nullptr;
    }

    // Service-specific resource cleanup. Runs whether the task exited
    // gracefully or was force-deleted, so implementers must keep it safe
    // against partial task progress.
    OnAfterStop();

    _running.store(false);
    _shutdownRequested.store(false);
    _taskExited.store(false);
    debugI("%s: stop completed", Name());
}

// TaskEntryThunk - static task entry point that casts the void* back to
// ITaskService* and calls Run(), with try/catch to log any exceptions that
// escape from Run(). Run() is expected to return when ShouldShutdown() is
// true; Stop() waits for the exit signal and force-deletes the task if it
// doesn't see one within shutdownTimeoutMs.
//
// The task does NOT self-delete: it signals exit, then sleeps forever.
// Stop() always performs the vTaskDelete from the caller's thread. This
// removes the classic FreeRTOS race where a self-deleting task races a
// destructor that has already returned from Stop() — by the time we
// vTaskDelete here, every member access on `self` has completed.

void ITaskService::TaskEntryThunk(void* p)
{
    auto* self = static_cast<ITaskService*>(p);
    if (self != nullptr)
    {
        try
        {
            self->Run();
        }
        catch (const std::exception& ex)
        {
            debugE("%s: Run() threw std::exception: %s", self->Name(), ex.what());
        }
        catch (...)
        {
            debugE("%s: Run() threw unknown exception", self->Name());
        }
        self->_taskExited.store(true);
    }

    // Park until Stop() reaps us. The block delay is long but bounded so
    // the task wakes if FreeRTOS tick wraparound math ever surprises us;
    // either way, control returns to the scheduler immediately.
    
    while (true)
        vTaskDelay(pdMS_TO_TICKS(60 * 1000));
}
