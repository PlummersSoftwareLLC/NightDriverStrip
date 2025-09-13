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
    #include "effects/strip/misceffects.h"
    #include "systemcontainer.h"

    #define BRIGHTNESS_STEP 20

    void RemoteControl::handle()
    {
        decode_results results;
        static uint lastResult = 0;

        if(!_IR_Receive.decode(&results))
            return;

        uint result = results.value;
        _IR_Receive.resume();

        debugI("Received IR Remote Code: 0x%08X, Decode: %08X\n", result, results.decode_type);

        if(0xFFFFFFFF == result || result == lastResult)
        {
            static uint lastRepeatTime = millis();

            // Only the OFF key runs at the full unbounded speed, so you can rapidly dim.  But everything
            // else has its repeat rate clamped here.

            auto kMinRepeatms = 200;
            if(result == IR_OFF)
                kMinRepeatms = 0;           // Dim as fast as the remote sends it
            else if(result == 0xFFFFFFFF)
                kMinRepeatms = 500;         // Repeats happen at 500ms
            else if(result == lastResult)
                kMinRepeatms = 50;          // Manual presses get debounced to at least 50ms

            if(millis() - lastRepeatTime > kMinRepeatms)
            {
                debugV("Remote Repeat; lastResult == %08x, elapsed = %lu\n", lastResult, millis() - lastRepeatTime);
                result         = lastResult;
                lastRepeatTime = millis();
            }
            else
                return;
        }

        lastResult = result;

        auto&effectManager = g_ptrSystem->EffectManager();
        auto&deviceConfig  = g_ptrSystem->DeviceConfig();

        if(IR_ON == result)
        {
            debugV("Turning ON via remote");
            effectManager.ClearRemoteColor();
            effectManager.SetInterval(0);
            effectManager.StartEffect();
            deviceConfig.SetBrightness(BRIGHTNESS_MAX);

            return;
        }
        else if(IR_OFF == result)
        {
            #if USE_HUB75
                deviceConfig.SetBrightness((int) deviceConfig.GetBrightness() - BRIGHTNESS_STEP);
            #else
                effectManager.ClearRemoteColor();
                effectManager.SetInterval(0);
                effectManager.StartEffect();
                deviceConfig.SetBrightness(0);
            #endif

            return;
        }
        else if(IR_BPLUS == result)
        {
            effectManager.SetInterval(DEFAULT_EFFECT_INTERVAL, true);
            if(deviceConfig.ApplyGlobalColors())
                effectManager.ClearRemoteColor();
            else
                effectManager.NextEffect();

            return;
        }
        else if(IR_BMINUS == result)
        {
            effectManager.SetInterval(DEFAULT_EFFECT_INTERVAL, true);
            if(deviceConfig.ApplyGlobalColors())
                effectManager.ClearRemoteColor();
            else
                effectManager.PreviousEffect();

            return;
        }
        else if(IR_SMOOTH == result)
        {
            effectManager.ClearRemoteColor();
            effectManager.SetInterval(EffectManager::csSmoothButtonSpeed);
        }
        else if(IR_STROBE == result)
            effectManager.NextPalette();
        else if(IR_FLASH == result)
            effectManager.PreviousPalette();
        else if(IR_FADE == result)
            effectManager.ShowVU( !effectManager.IsVUVisible() );

        debugI("Looking for match for remote color code: %08X\n", result);

        for(const auto&RemoteColorCode : RemoteColorCodes)
        {
            if(RemoteColorCode.code == result)
            {
                // To set a solid color fill based on the remote buttons, we take the color from the table,
                // crate a ColorFillEffect, and apply it as a temporary effect.  This will override the current
                // effect until the next effect change or remote command.

                debugI("Changing Color via remote: %08X\n", (uint32_t) RemoteColorCode.color);
                effectManager.ApplyGlobalColor(RemoteColorCode.color);
                #if FULL_COLOR_REMOTE_FILL
                    auto effect = make_shared_psram<ColorFillEffect>("Remote Color", RemoteColorCode.color, 1, true);
                    if(effect->Init(g_ptrSystem->EffectManager().GetBaseGraphics()))
                        g_ptrSystem->EffectManager().SetTempEffect(effect);
                    else
                        debugE("Could not initialize new color fill effect");

                #endif

                return;
            }
        }
    }

#endif
