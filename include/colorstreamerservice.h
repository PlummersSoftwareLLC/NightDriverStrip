#pragma once

//+--------------------------------------------------------------------------
//
// File:        colorstreamerservice.h
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
//    ColorStreamerService pushes per-frame LED color data out over a
//    TCP socket (LEDViewer protocol) and/or the WebSocket frame channel
//    so external tools and the local web UI can preview the strip.
//    Gated by COLORDATA_SERVER_ENABLED. Inherits ITaskService for
//    lifecycle. Implementation lives in network.cpp alongside the
//    LEDViewer + frame-event-listener machinery.
//
// History:     May-04-2026         Davepl      Created
//
//---------------------------------------------------------------------------

#include "globals.h"

#if COLORDATA_SERVER_ENABLED

#include "itaskservice.h"

class ColorStreamerService : public ITaskService
{
  public:
    ColorStreamerService() = default;
    ~ColorStreamerService() override { Stop(); }

    const char* Name() const override { return "ColorStreamerService"; }

  protected:
    TaskConfig GetTaskConfig() const override;
    void Run() override;
};

#endif // COLORDATA_SERVER_ENABLED
