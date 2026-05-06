#pragma once

//+--------------------------------------------------------------------------
//
// File:        audioserialbridge.h
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
//    AudioSerialBridge sends compact spectrum/VU packets at 2400 baud out
//    Serial2 (and optionally a TCP socket for the VICE C64 emulator) so
//    the PETROCK project on a connected Commodore 64/PET can render the
//    visualization. Gated by ENABLE_AUDIOSERIAL.
//
//    Inherits ITaskService for the standard launch/shutdown discipline.
//    The task body lives in audio.cpp alongside the VICE socket helper.
//
// History:     May-04-2026         Davepl      Created
//
//---------------------------------------------------------------------------

#include "globals.h"

#if ENABLE_AUDIOSERIAL

#include "itaskservice.h"

class AudioSerialBridge : public ITaskService
{
  public:
    AudioSerialBridge() = default;
    ~AudioSerialBridge() override { Stop(); }

    const char* Name() const override { return "AudioSerialBridge"; }

  protected:
    TaskConfig GetTaskConfig() const override;
    void Run() override;
};

#endif // ENABLE_AUDIOSERIAL
