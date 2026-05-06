#pragma once

//+--------------------------------------------------------------------------
//
// File:        renderservice.h
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
//    RenderService is the heart of the system: the per-frame rendering
//    loop that pulls from either incoming WiFi color data or the local
//    EffectManager and pushes pixels to the GFXBase devices. Pinned to
//    DRAWING_CORE at DRAWING_PRIORITY. Inherits ITaskService for the
//    standard lifecycle. Implementation lives in drawing.cpp alongside
//    WiFiDraw / LocalDraw / CalcDelayUntilNextFrame.
//
// History:     May-04-2026         Davepl      Created
//
//---------------------------------------------------------------------------

#include "globals.h"

#include "itaskservice.h"

class RenderService : public ITaskService
{
  public:
    RenderService() = default;
    ~RenderService() override { Stop(); }

    const char* Name() const override { return "RenderService"; }

  protected:
    TaskConfig GetTaskConfig() const override;
    void Run() override;
};
