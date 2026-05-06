#pragma once

//+--------------------------------------------------------------------------
//
// File:        debugconsole.h
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
//    DebugConsole runs the BSD-socket telnet server that gives remote
//    debug access to the chip. Inherits ITaskService for lifecycle.
//    Implementation lives in telnetserver.cpp alongside the protocol
//    handling helpers.
//
// History:     May-04-2026         Davepl      Created
//
//---------------------------------------------------------------------------

#include "globals.h"

#if ENABLE_WIFI

#include <atomic>

#include "itaskservice.h"

class DebugConsole : public ITaskService
{
  public:
    DebugConsole() = default;
    ~DebugConsole() override { Stop(); }

    const char* Name() const override { return "DebugConsole"; }

  protected:
    TaskConfig GetTaskConfig() const override;
    void Run() override;

    // Close the listening (and any active client) socket from the Stop() thread
    // so accept()/recv() return promptly and Run() can observe ShouldShutdown().

    void OnBeforeWaitForStop() override;

  private:
    
  // Both fds are set by Run() and may be cleared by OnBeforeWaitForStop()
    // running on a different task. atomic<int> gives us a lock-free
    // exchange so we never call close() twice on the same fd.  I can't 
    // believe POSIX doesn't define an invalid handle constant, but here we are.

    std::atomic<int> _listenFd{-1};
    std::atomic<int> _clientFd{-1};
};

#endif // ENABLE_WIFI
