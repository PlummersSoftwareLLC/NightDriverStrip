//+--------------------------------------------------------------------------
//
// File:        remotecontrol.cpp
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
// History:     Jun-14-2023     Rbergen        Extracted handle() from header
//---------------------------------------------------------------------------

#include "globals.h"

#if ENABLE_REMOTE

#include "systemcontainer.h"

void RemoteControl::handle()
{
    decode_results results;
    static uint lastResult = 0;

    if (!_IR_Receive.decode(&results))
        return;

    uint result = results.value;    
    _IR_Receive.resume();

    debugW("Received IR Remote Code: 0x%08X, Decode: %08X\n", result, results.decode_type);

    if (0xFFFFFFFF == result)
    {
        debugV("Remote Repeat; lastResult == %08x\n", lastResult);
        result = lastResult;
    }
    lastResult = result;

    auto &effectManager = g_ptrSystem->EffectManager();

    if (IR_ON == result)
    {
        debugV("Turning ON via remote");
        effectManager.ClearRemoteColor();
        effectManager.SetInterval(0);
        effectManager.StartEffect();
        g_Values.Brightness = 255;
        return;
    }
    else if (IR_OFF == result)
    {
        g_Values.Brightness = std::max(MIN_BRIGHTNESS, (int) g_Values.Brightness - BRIGHTNESS_STEP);
        return;
    }
    else if (IR_BPLUS == result)
    {
        effectManager.ClearRemoteColor();
        effectManager.NextEffect();
        return;
    }
    else if (IR_BMINUS == result)
    {
        effectManager.ClearRemoteColor();
        effectManager.PreviousEffect();
        return;
    }
    else if (IR_SMOOTH == result)
    {
        effectManager.ClearRemoteColor();
        effectManager.SetInterval(EffectManager<GFXBase>::csSmoothButtonSpeed);
    }
    else if (IR_STROBE == result)
    {
        effectManager.NextPalette();
    }
    else if (IR_FLASH == result)
    {
        effectManager.PreviousPalette();
    }
    else if (IR_FADE == result)
    {
        effectManager.ShowVU( !effectManager.IsVUVisible() );
    }

    for (int i = 0; i < ARRAYSIZE(RemoteColorCodes); i++)
    {
        if (RemoteColorCodes[i].code == result)
        {
            debugV("Changing Color via remote: %08X\n", (uint) RemoteColorCodes[i].color);
            effectManager.SetGlobalColor(RemoteColorCodes[i].color);
            return;
        }
    }
}

#endif