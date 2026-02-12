//+--------------------------------------------------------------------------
//
// File:        RemoteControl.h
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
//
// Description:
//
//    Handles a simple IR remote for changing effects, brightness, etc.
//
// History:     Jul-17-2021     Davepl      Documented
//---------------------------------------------------------------------------

#pragma once

#include "globals.h"

#if ENABLE_REMOTE

void RemoteLoopEntry(void *);

struct RemoteColorCode
{
    uint32_t code;
    CRGB     color;
    uint8_t  hue;
};

// Pimpl to hide RMT driver details from the 115 files including this header
class RemoteControlImpl;

class RemoteControl
{
  private:
    std::unique_ptr<RemoteControlImpl> _pImpl;

  public:
    RemoteControl();
    ~RemoteControl();

    bool begin();
    void end();
    void handle();
};

#endif
