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
#include <IRremoteESP8266.h>
#include <IRrecv.h>
#include <IRutils.h>
#include "FastLED.h"
#include "effects/effectmanager.h"

#if ENABLE_REMOTE

//extern RemoteDebug Debug;
extern unique_ptr<EffectManager> g_pEffectManager;

#define	key24  true
#define	key44  false

void IRAM_ATTR RemoteLoopEntry(void *);

#if key24
#define	IR_BPLUS  0xF700FF	// 
#define	IR_BMINUS 0xF7807F	// 
#define	IR_OFF 	  0xF740BF	// 
#define	IR_ON 	  0xF7C03F	// 
#define	IR_R 	  0xF720DF	// 
#define	IR_G 	  0xF7A05F	// 
#define	IR_B 	  0xF7609F	// 
#define	IR_W 	  0xF7E01F	// 
#define	IR_B1	  0xF710EF	// 
#define	IR_B2	  0xF7906F	// 
#define	IR_B3	  0xF750AF	// 
#define	IR_FLASH  0xF7D02F	// 
#define	IR_B4	  0xF730CF	// 
#define	IR_B5	  0xF7B04F	// 
#define	IR_B6	  0xF7708F	// 
#define	IR_STROBE 0xF7F00F	// 
#define	IR_B7	  0xF708F7	// 
#define	IR_B8	  0xF78877	// 
#define	IR_B9	  0xF748B7	// 
#define	IR_FADE   0xF7C837	// 
#define	IR_B10	  0xF728D7	// 
#define	IR_B11	  0xF7A857	// 
#define	IR_B12	  0xF76897	// 
#define	IR_SMOOTH 0xF7E817	// 
#endif

#if key44
#define	IR_BPlus  0xFF3AC5	// 
#define	IR_BMinus 0xFFBA45	// 
#define	IR_ON 	  0xFF827D	// 
#define	IR_OFF 	  0xFF02FD	// 
#define	IR_R 	  0xFF1AE5	// 
#define	IR_G 	  0xFF9A65	// 
#define	IR_B  	  0xFFA25D	// 
#define	IR_W 	  0xFF22DD	// 
#define	IR_B1	  0xFF2AD5	// 
#define	IR_B2	  0xFFAA55	// 
#define	IR_B3	  0xFF926D	// 
#define	IR_B4	  0xFF12ED	// 
#define	IR_B5	  0xFF0AF5	// 
#define	IR_B6	  0xFF8A75	// 
#define	IR_B7	  0xFFB24D	// 
#define	IR_B8	  0xFF32CD	// 
#define	IR_B9	  0xFF38C7	// 
#define	IR_B10	  0xFFB847	// 
#define	IR_B11	  0xFF7887	// 
#define	IR_B12	  0xFFF807	// 
#define	IR_B13	  0xFF18E7	// 
#define	IR_B14	  0xFF9867	// 
#define	IR_B15	  0xFF58A7	// 
#define	IR_B16	  0xFFD827	// 
#define	IR_UPR 	  0xFF28D7	// 
#define	IR_UPG 	  0xFFA857	// 
#define	IR_UPB 	  0xFF6897	// 
#define	IR_QUICK  0xFFE817	// 
#define	IR_DOWNR  0xFF08F7	// 
#define	IR_DOWNG  0xFF8877	// 
#define	IR_DOWNB  0xFF48B7	// 
#define	IR_SLOW   0xFFC837	// 
#define	IR_DIY1   0xFF30CF	// 
#define	IR_DIY2   0xFFB04F	// 
#define	IR_DIY3   0xFF708F	// 
#define	IR_AUTO   0xFFF00F	// 
#define	IR_DIY4   0xFF10EF	// 
#define	IR_DIY5   0xFF906F	// 
#define	IR_DIY6   0xFF50AF	// 
#define	IR_FLASH  0xFFD02F	// 
#define	IR_JUMP3  0xFF20DF	// 
#define	IR_JUMP7  0xFFA05F	// 
#define	IR_FADE3  0xFF609F	// 
#define	IR_FADE7  0xFFE01F	// 
#endif

static struct 
{
    uint code;
    CRGB color;
} 
RemoteColorCodes[] =
{
    { IR_OFF, CRGB(000, 000, 000) },

    { IR_R,   CRGB(255, 000, 000) },
    { IR_G,   CRGB(000, 255, 000) },
    { IR_B,   CRGB(000, 000, 255) },
    { IR_W,   CRGB(255, 255, 255) },

    { IR_B1,  CRGB(255,  64, 000) },
    { IR_B2,  CRGB(000, 255,  64) },
    { IR_B3,  CRGB( 64, 000, 255) },
    
    { IR_B4,  CRGB(255, 128, 000) },
    { IR_B5,  CRGB(000, 255, 128) },
    { IR_B6,  CRGB(128, 000, 255) },
    
    { IR_B7,  CRGB(255, 192, 000) },
    { IR_B8,  CRGB(000, 255, 192) },
    { IR_B9,  CRGB(192, 000, 255) },

    { IR_B10,  CRGB(255, 255, 000) },
    { IR_B11,  CRGB(000, 255, 255) },
    { IR_B12,  CRGB(255, 000, 255) }
};

class RemoteControl
{
  private:
    IRrecv _IR_Receive;

  public:

    RemoteControl() : _IR_Receive(IR_REMOTE_PIN)
    {
    }

    bool begin()
    {
        _IR_Receive.enableIRIn();
        return true;
    }

    void end()
    {
		_IR_Receive.disableIRIn(); 
    }

    void handle()
    {
        decode_results results;
        static uint lastResult = 0;

        if (!_IR_Receive.decode(&results))
        {
            //Serial.printf("IR: Nothing received, nothing decoded\n");
            return;
        }

        uint result = results.value;
        _IR_Receive.resume();

        debugI("Received IR Remote Code: 0x%08X, Decode: %08X\n", result, results.decode_type);

        if ((uint)-1 == result)
        {
            debugI("Remote Repeat; lastResult == %08x\n", lastResult);
            if (lastResult == IR_BMINUS)
            {
                g_Brightness = std::max(MIN_BRIGHTNESS, g_Brightness - BRIGHTNESS_STEP);
                debugI("Dimming to %d\n", g_Brightness);
                return;
            }
            else if (lastResult == IR_BPLUS)
            {
                g_Brightness = std::min(MAX_BRIGHTNESS, g_Brightness + BRIGHTNESS_STEP);
                debugI("Brightening to %d\n", g_Brightness);
                return;
            }
        }
        else
        {
            lastResult = result;
        }

        if (IR_ON == result)
        {
            debugI("Turning ON via remote");
            g_pEffectManager->ClearRemoteColor();
            g_pEffectManager->SetInterval(0x7FFFFFF);
            g_Brightness = 255;
            return;
        }
        else if (IR_BPLUS == result)
        {
            g_pEffectManager->ClearRemoteColor();
            g_pEffectManager->NextEffect();
            
            return;
        }
        else if (IR_BMINUS == result)
        {
            g_pEffectManager->ClearRemoteColor();
            g_pEffectManager->PreviousEffect();
            return;
        }
        else if (IR_SMOOTH == result)
        {
            g_pEffectManager->ClearRemoteColor();
            g_pEffectManager->SetInterval(EffectManager::csSmoothButtonSpeed);
        }
        else if (IR_FADE == result)
        {
            g_pEffectManager->ClearRemoteColor();
            g_pEffectManager->SetInterval(EffectManager::csSmoothButtonSpeed);
        }
        else if (IR_FLASH == result)
        {
            g_pEffectManager->ClearRemoteColor();
            g_pEffectManager->SetCurrentEffectIndex(EffectManager::FireEffectIndex);
        }
        else if (IR_STROBE == result)
        {
#if ATOMLIGHT            
            g_pEffectManager->ClearRemoteColor();
            g_pEffectManager->SetCurrentEffectIndex(EffectManager::VUEffectIndex);
#endif            
        }
        

        for (int i = 0; i < ARRAYSIZE(RemoteColorCodes); i++)
        {
            if (RemoteColorCodes[i].code == result)
            {
                debugI("Changing Color via remote: %08X\n", (uint) RemoteColorCodes[i].color);
                g_pEffectManager->SetGlobalColor(RemoteColorCodes[i].color);
                return;
            }
        }

        if (result == (uint)-1)
        {
            debugI("Remote REPEAT\n");
            return;
        }
    }
};

#endif