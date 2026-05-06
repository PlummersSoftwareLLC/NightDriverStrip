#pragma once

//+--------------------------------------------------------------------------
//
// File:        iservice.h
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
//    IService is a tiny lifecycle interface shared by classes in NightDriver
//    that own a FreeRTOS task, a hardware driver, or a network endpoint.
//    Adopting it gives those classes a uniform Start/Stop discipline and
//    lets orchestration code (boot sequencing, shutdown for OTA, a "services"
//    debug command, etc.) treat them generically.
//
//    Deliberately small. The interface does NOT include:
//
//      - Reconfigure(...) or any config accessor — config types differ per
//        service, so they stay on the concrete class.
//      - Resource handles (sockets, task handles, web routes) — also concrete.
//      - State machines beyond running / not-running — anything richer is
//        the implementer's business.
//
//    Lifecycle contract for implementers:
//
//      - The constructor must NOT install hardware, allocate large buffers,
//        launch tasks, or open sockets. Construction is metadata-only.
//      - Start() may install/allocate/launch/open. Returns true if running
//        after the call. Calling Start() while already running is a no-op
//        that returns true.
//      - Stop() must be idempotent and safe against partial state. It must
//        wait for any owned task to exit cleanly (with a timeout fallback)
//        before releasing shared hardware resources.
//      - IsRunning() reflects the live state visible to other threads.
//        Implementers typically back this with std::atomic<bool>.
//      - Name() returns a short, stable identifier suitable for log lines
//        and CLI output (e.g. "AudioService", "JSONWriter").
//
//    Restart() has a default implementation of Stop() then Start(); override
//    only if a service can do something cheaper than a full cycle.
//
// History:     May-04-2026         Davepl      Created
//
//---------------------------------------------------------------------------

class IService
{
  public:
    virtual ~IService() = default;

    // Idempotent. Returns true if the service is running after the call.
    
    virtual bool Start() = 0;

    // Idempotent. Releases owned resources and waits for owned tasks to exit.
    
    virtual void Stop() = 0;

    // Reflects the live running state. Cheap to call from any thread.
    
    virtual bool IsRunning() const = 0;

    // Short stable identifier for logging and diagnostics. Should be a
    // string literal so the returned pointer outlives the service.

    virtual const char* Name() const = 0;

    // Default Stop()+Start() cycle. Override for cheaper service-specific paths.
    
    virtual bool Restart()
    {
        Stop();
        return Start();
    }
};
