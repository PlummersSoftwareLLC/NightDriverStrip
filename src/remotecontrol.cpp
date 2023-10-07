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
#define BRIGHTNESS_STEP     20
/*
We will need to define the user remote control
*/

UserRemoteControl myRemoteController (44);

void RemoteControl::handle()
{
    decode_results results;
    static uint lastResult = 0;
    static CRGB lastManualColor = CRGB(0,0,0);

    auto& deviceConfig = g_ptrSystem->DeviceConfig();

    if (!_IR_Receive.decode(&results))
        return;

    uint result = results.value;    
    _IR_Receive.resume();

    debugV("Received IR Remote Code: 0x%08X, Decode: %08X\n", result, results.decode_type);
    
    
    /*
    To see what your remote button code is, enable the line immediately below this comment block.
    Open up the terminal monitor (e.g. in Visual Studio).
    Press the remote buttonm and watch the output.
    Copy the code from the console before it scrolls out of view.
    */
    
    //debugI("Received IR Remote Code: 0x%08X, Decode: %08X\n", result, results.decode_type); // Uncomment to see the debug code in your terminal. 
 
    if (0xFFFFFFFF == result || result == lastResult)
    {
        static uint lastRepeatTime = millis();

        // Only the OFF key runs at the full unbounded speed, so you can rapidly dim.  But everything
        // else has its repeat rate clamped here.

        const auto kMinRepeatms = (lastResult == IR_OFF) ? 0 : 200;

        if (millis() - lastRepeatTime > kMinRepeatms)
        {
            debugV("Remote Repeat; lastResult == %08x, elapsed = %lu\n", lastResult, millis()-lastRepeatTime);
            result = lastResult;
            lastRepeatTime = millis();
        }
        else
        {
            return;
        }
    }
    lastResult = result;

    auto &effectManager = g_ptrSystem->EffectManager();
    
    
    
    //We are going to search the remote control buttons vector for a matching button
    auto keyCodeMatch = std::find_if(
        myRemoteController.buttons.begin(), 
        myRemoteController.buttons.end(),
        [&result](const RemoteButton& potentialButton) { 
            return potentialButton.keyCode == result;
            }
        );


    if (keyCodeMatch != myRemoteController.buttons.end())
        {
        //debugV("We have a matching keycode 0x%08X \n", result);
        auto index = std::distance(myRemoteController.buttons.begin(), keyCodeMatch);
        RemoteButton thisButton = myRemoteController.buttons[index];
        auto &myEffect = effectManager.GetCurrentEffect();

        switch (thisButton.buttonAction){
            case ButtonActions::BRIGHTNESS_UP:
               deviceConfig.SetBrightness((int)deviceConfig.GetBrightness() + BRIGHTNESS_STEP);

            break;
            case ButtonActions::BRIGHTNESS_DOWN:
                deviceConfig.SetBrightness((int)deviceConfig.GetBrightness() - BRIGHTNESS_STEP);
                debugI("After brightness down global brightness is  %i\n",g_Values.Brightness);

            break;
            case ButtonActions::POWER_ON:
                effectManager.ClearRemoteColor();
                effectManager.SetInterval(0);
                effectManager.SetGlobalColor(lastManualColor);
                effectManager.StartEffect();
            break;
            case ButtonActions::POWER_OFF:
                //effectManager.SetGlobalColor(CRGB(0,0,0));
                //g_Values.Brightness = std::max(0, (int) g_Values.Brightness - BRIGHTNESS_STEP);
                effectManager.ClearRemoteColor();
            break;
            case ButtonActions::FILL_COLOR:
                lastManualColor = hexToCrgb(thisButton.actionArgs);
                lastManualColor.maximizeBrightness(myRemoteController.currentBrightness);
                effectManager.SetGlobalColor(lastManualColor); 
                debugI("Current FastLED brightness %i\n",FastLED.getBrightness());
                effectManager.SetInterval(0);
            break;
            
            case ButtonActions::TRIGGER_EFFECT:
            
            break;
            case ButtonActions::CHANGER:
                if (lastManualColor.red + thisButton.actionArgs.toInt() > 255 ) {
                    lastManualColor.red = 255;
                } else if (lastManualColor.red + thisButton.actionArgs.toInt() < 0) {
                    lastManualColor.red = 0;
                } else {
                    lastManualColor.red += thisButton.actionArgs.toInt();
                }
                effectManager.SetGlobalColor(lastManualColor); 
                effectManager.SetInterval(0);

            break;
            case ButtonActions::CHANGEG:
                if (lastManualColor.green + thisButton.actionArgs.toInt() > 255 ) {
                    lastManualColor.green = 255;
                } else if (lastManualColor.green + thisButton.actionArgs.toInt() < 0) {
                    lastManualColor.green = 0;
                } else {
                    lastManualColor.green += thisButton.actionArgs.toInt();
                }
                effectManager.SetGlobalColor(lastManualColor);
                effectManager.SetInterval(0);
            break;
            case ButtonActions::CHANGEB:
                if (lastManualColor.blue + thisButton.actionArgs.toInt() > 255 ) {
                    lastManualColor.blue = 255;
                } else if (lastManualColor.blue + thisButton.actionArgs.toInt() < 0) {
                    lastManualColor.blue = 0;
                } else {
                    lastManualColor.blue += thisButton.actionArgs.toInt();
                }
                effectManager.SetGlobalColor(lastManualColor); 
                effectManager.SetInterval(0);
            break;
            case DIY1:
               
            break;
            case DIY2: 
                
                
            break;
            case DIY3: 
                
            break;
            case DIY4: 
                
            break;

            //case ButtonActions::JUMP3:
            //break;
            //case ButtonActions::JUMP7:
            //break;
            //case ButtonActions::FADE3:
            //break;
            //case ButtonActions::FADE7:
            //break;
            //case ButtonActions::STROBE:
            //break;
            //case ButtonActions::AUTO:
            //break;
            //case ButtonActions::FLASH:
            //break;
            //case ButtonActions::QUICK:
            //break;
            //case ButtonActions::SLOW:
            //break;
            
        }
        
    }
    
    //debugI("KeyCode: %08X for first button\n", myRemoteController.Buttons[0].KeyCode);
    
   /*
   std::vector<RemoteButton>::iterator itr = myRemoteController.Buttons.begin();
   while (itr != myRemoteController.Buttons.end()){
    if (myRemoteController.Buttons[*itr].KeyCode == result){

        debugI("We have a matching keycode.");
    } else {
        debugI("We do not have a matching keycode");
    }
   }
   */
  /*
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
        effectManager.SetInterval(EffectManager::csSmoothButtonSpeed);
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
    } else if (IR_BPLUS == result) {
        

    }

    
    for (int i = 0; i < ARRAYSIZE(RemoteColorCodes); i++) 
    {
        if (RemoteColorCodes[i].code == result)
        {
            //debugV("Changing Color via remote: %08X\n", (uint) RemoteColorCodes[i].color);
            debugI("Changing Color via remote: %08X\n", (uint) RemoteColorCodes[i].color);
            effectManager.SetGlobalColor(RemoteColorCodes[i].color);
            return;
        }
    }
    */
}
#endif