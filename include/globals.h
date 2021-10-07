#pragma once
//+--------------------------------------------------------------------------
//
// File:        Globals.h
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
// Description:
//
//    Adapted from old ESP32LEDStick box and BigBlueLCD code by Davepl
//
// History:     Apr-13-2019  Version    Davepl      Created for NightDriverStrip
//              Dec-09-2019  v001       Davepl      Unified from multiple projects
//
//               First version with color to indicate status, including 
//               purple during flashing, red for no wifi, blue for no clock.
//               And first version with flash version display on TFT.
//
//              Dec-13-2019  v002       Davepl      Task priorities and RAM
//
//               Freed up more ram so we get 40 frames of 8x144 and tuned the
//               task priorities so they're constantly processed and rev'd
//               the UI to include a time range of buffer frames.  Also added
//               partial draw support and variable framerate.  
//
//              Dec-16-2019  v003       Davepl      1st client using clockstream
//              Dec-19-2019  v004       Davepl      1st client using 128-bit clock
//              Dec-25-2019  v005       Davepl      Added PixelData64 
//
//                Made the clock timestamps full 64 bit and transmit same in new
//                PIXELDATA64 command.  Idles in white now even without clock.
//
//              Jun-24-2020  v006       Davepl      Reverted to NTP
//              Sep-14-2020  v008       Davepl      Adding support for M5StickC
//              Nov-28-2020  v009       Davepl      Added FANSET scenario
//              Apr-10-2021  v010       Davepl      BUG in length, bad version
//              Apr-13-2021  v011       Davepl      Fixed length issue
//              Apr-18-2021  v012       Davepl      Multiple Fixes
//              Apr-21-2021  v013       Davepl      SPIFFs reference in SetupOTA
//              Apr-22-2021  v014       Davepl      Moved OTA pump to net thread
//              Apr-23-2021  v015       Davepl      Fixed fan effects
//              Apr-23-2021  v016       Davepl      Fix max power limit!
//              Apr-24-2021  v017       Davepl      Fix compressed frames - stable!
//              May-01-2021  v018       Davepl      Put recive timeout back in, cRec'd to 0
//              Jun-17-2022  v019       Davepl      Atomlight2 + variable FPS
//              Jul-08-2022  v020       Davepl      Particle System, Insulators, lib deps
//              Sep-18-2022  v021       Davepl      Github Release
//---------------------------------------------------------------------------

// The goal here is to get two variables, one numeric and one string, from the *same* version
// value.  So if version = 020, 
//
// FLASH_VERSION==20
// FLASH_VERSION_NAME=="v020"
//
// If you know a cleaner way, please improve this!

#define FLASH_VERSION          0211  // Upate ONLY this to increment the version number

#define XSTR(x) STR(x)              // The defs will generate the stringized version of it
#define STR(x) "v"#x
#define FLASH_VERSION_NAME_X(x) "v"#x 
#define FLASH_VERSION_NAME XSTR(FLASH_VERSION)

#define FASTLED_INTERNAL        1   // Silence FastLED build banners
#define NTP_DELAY_COUNT         20  // delay count for ntp update
#define NTP_PACKET_LENGTH       48  // ntp packet length
#define TIME_ZONE             (-8)  // My offset from London (UTC-8)

// C Helpers and Macros

#define ARRAYSIZE(a)		(sizeof(a)/sizeof(a[0]))		// Returns the number of elements in an array
#define PERIOD_FROM_FREQ(f) (round(1000000 * (1.0 / f)))	// Calculate period in microseconds (us) from frequency in Hz
#define FREQ_FROM_PERIOD(p) (1.0 / p * 1000000)				// Calculate frequency in Hz given the priod in microseconds (us)

// I've built and run this on the Heltec Wifi 32 module and the M5StickC.  The
// main difference is pinout and the LCF/TFT screen.  The presense of absense
// of the TFT/OLED is now controlled separately, but M5 is always equipped
// with one (but it doesn't have to be used!).

#if M5STICKC
#define LED_BUILTIN 10                          // Not defined by the M5 headers, but it seems to be PIN 10
#include "M5StickC.h"
#undef min                                      // They define a min() on us
#endif

#if M5STICKCPLUS
#include "M5StickCPlus.h"
#undef min                                      // They define a min() on us
#endif

#define EFFECT_CROSS_FADE_TIME 600.0    // How long for an effect to ramp brightness fader down and back during effect change

// Thread priorities
//
// We have a half-dozen workers and these are their relative priorities.  It might survive if all were set equal,
// but I think drawing should be lower than audio so that a bad or greedy effect doesn't starve the audio system.

#define DRAWING_PRIORITY        tskIDLE_PRIORITY+1
#define SOCKET_PRIORITY         tskIDLE_PRIORITY+2
#define NET_PRIORITY            tskIDLE_PRIORITY+2
#define AUDIO_PRIORITY          tskIDLE_PRIORITY+2
#define TFT_PRIORITY            tskIDLE_PRIORITY+2
#define DEBUG_PRIORITY          tskIDLE_PRIORITY+1
#define REMOTE_PRIORITY         tskIDLE_PRIORITY+1

// If you experiment and mess these up, my go-to solution is to put Drawing on Core 0, and everything else on Core 1. 
// My current core layout is as follows, and as of today it's solid as of (7/16/21).
//
// #define DRAWING_CORE            0
// #define INCOMING_CORE           1
// #define NET_CORE                1
// #define AUDIO_CORE              0
// #define TFT_CORE                1
// #define DEBUG_CORE              1
// #define SOCKET_CORE             1
// #define REMOTE_CORE             1

#define DRAWING_CORE            1
#define INCOMING_CORE           0
#define NET_CORE                1
#define AUDIO_CORE              0
#define TFT_CORE                1
#define DEBUG_CORE              1
#define SOCKET_CORE             1
#define REMOTE_CORE             1

#define FASTLED_INTERNAL        1   // Suppresses the compilation banner from FastLED

#define __STDC_FORMAT_MACROS
#include <inttypes.h>
#include <Arduino.h>

#include <iostream>
#include <memory>
#include <string>

#include <FastLED.h>                // FastLED for the LED panels
#include <pixeltypes.h>             // Handy color and hue stuff
#include <WiFi.h>

#include <sys/time.h>
#include <exception>
#include "RemoteDebug.h"

using namespace std;

#include<sstream>

// I don't know why to_string is missing, but it seems to be a compiler/cygwin
// issue. If this turns into a redefinition at some point because the real one
// comes online in the future, this to_string can be removed.

template <typename T>
std::string to_string(T value)
{
    //create an output string stream
    std::ostringstream os ;

    //throw the value into the string stream
    os << value ;

      //convert the string stream into a string and return
    return os.str();
}

#define STRING(num) STR(num)

extern RemoteDebug Debug;           // Let everyone in the project know about it.  If you don't have it, delete this

// Project Configuration
//
// One and only one of DEMO, SPECTRUM, ATOMLIGHT, etc should be set to true by the build config for your project
// 
// I've used this code to build a dozen different projects, most of which can be created by defining
// the right built environment (like INSULATORS=1).  The config here defines everythig about the
// LEDs, how many, on how many channels, laid out into how many fang/rings, and so on.  You can also
// specify the audio system config like how many band channels.

#if DEMO

    // This is a simple demo configuration.  To build, simply connect the data lead from a WS2812B
    // strip to pin 5.  This does not use the TFT, OLED, or anything fancy, it simply drives the
    // LEDs with a simple rainbow effect as specified in effects.cpp for DEMO.
    //
    // Please ensure you supply sufficent power to your strip, as even the DEMO of 144 LEDs, if set
    // to white, would overload a USB port.

    #define MATRIX_WIDTH            144
    #define MATRIX_HEIGHT           1
    #define NUM_LEDS                (MATRIX_WIDTH*MATRIX_HEIGHT)
    #define NUM_CHANNELS            1
    #define LED_PIN0                5
    #define NUM_RINGS               5
    #define RING_SIZE_0             24

    #define POWER_LIMIT_MW       5000   // 1 amp supply at 5 volts assumed

    // Once you have a working project, selectively enable various additional features by setting
    // them to 1 in the list below.  This DEMO config assumes no audio (mic), or screen, etc.

    #define ENABLE_WIFI             1   // Connect to WiFi
    #define INCOMING_WIFI_ENABLED   1   // Accepting incoming color data and commands
    #define TIME_BEFORE_LOCAL       2   // How many seconds before the lamp times out and shows local contexnt
    #define ENABLE_NTP              1   // Set the clock from the web
    #define ENABLE_OTA              1   // Accept over the air flash updates

    #define USE_TFT                 0   // Set to 1 if you have the Heltec module w/TFT 

    // The webserver serves files from its SPIFFS filesystem, such as index.html, and those files must be
    // uploaded to SPIFFS with the "Upload Filesystem Image" command before it can work.  When running
    // you should be able to see/select the list of effects by visiting the chip's IP in a browser.  You can
    // get the chip's IP by watching the serial output or checking your router for the DHCP given to 'LEDWifi'

    #define ENABLE_WEBSERVER        1   // Turn on the internal webserver

#elif TREESET

    #define ENABLE_WIFI             1  // Connect to WiFi
    #define INCOMING_WIFI_ENABLED   0   // Accepting incoming color data and commands
    #define WAIT_FOR_WIFI           0   // Hold in setup until we have WiFi - for strips without effects
    #define TIME_BEFORE_LOCAL       0   // How many seconds before the lamp times out and shows local contexnt
    #define ENABLE_WEBSERVER        1   // Turn on the internal webserver
    #define ENABLE_NTP              0   // Set the clock from the web
    #define ENABLE_OTA              1   // Accept over the air flash updates
    #define ENABLE_REMOTE           1   // IR Remote Control
    #define ENABLE_AUDIO            1   // Listen for audio from the microphone and process it
    
    #define LED_PIN0          26
    #define NUM_CHANNELS      1
    #define RING_SIZE_0       24
    #define BONUS_PIXELS      0                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                     
    #define MATRIX_WIDTH      5
    #define MATRIX_HEIGHT     RING_SIZE_0
    #define NUM_FANS          MATRIX_WIDTH
    #define FAN_SIZE          MATRIX_HEIGHT
    #define NUM_BANDS         16
    #define NUM_LEDS          (MATRIX_WIDTH*MATRIX_HEIGHT)
    #define RESERVE_MEMORY    150000
    #define ENABLE_REMOTE     1                     // IR Remote Control
    #define ENABLE_AUDIO      1                     // Listen for audio from the microphone and process it
    #define IR_REMOTE_PIN     25                   
    #define LED_FAN_OFFSET_BU 12
    #define POWER_LIMIT_MW    20000

    #define NOISE_CUTOFF   75
    #define NOISE_FLOOR    200.0f

    #define TOGGLE_BUTTON  37
    #define NUM_INFO_PAGES 2

#elif SPECTRUM

    // This project is set up as a 48x16 matrix of 16x16 WS2812B panels such as: https://amzn.to/3ABs5DK
    // It uses an M5StickCPlus which has a microphone and OLED built in:  https://amzn.to/3CrvCFh
    // It displays a spectrum analyzer and music visualizer
    
    #define ENABLE_WIFI             1  // Connect to WiFi
    #define INCOMING_WIFI_ENABLED   1   // Accepting incoming color data and commands
    #define WAIT_FOR_WIFI           1   // Hold in setup until we have WiFi - for strips without effects
    #define TIME_BEFORE_LOCAL       2   // How many seconds before the lamp times out and shows local content
    #define ENABLE_WEBSERVER        1   // Turn on the internal webserver
    #define ENABLE_NTP              1   // Set the clock from the web
    #define ENABLE_OTA              0  // Accept over the air flash updates
    #define ENABLE_REMOTE           1   // IR Remote Control
    #define ENABLE_AUDIO            1   // Listen for audio from the microphone and process it
    
    #define DEFAULT_EFFECT_INTERVAL     (60*60*24)

    #define MAX_BUFFERS     20

    #define LED_PIN0        26
    #define NUM_CHANNELS    1
    #define BONUS_PIXELS    0
    #define MATRIX_WIDTH    48
    #define MATRIX_HEIGHT   16
    #define NUM_FANS        MATRIX_WIDTH
    #define FAN_SIZE        MATRIX_HEIGHT
    #define NUM_BANDS       16
    #define NUM_LEDS        (MATRIX_WIDTH*MATRIX_HEIGHT)
    #define RESERVE_MEMORY  150000
    #define IR_REMOTE_PIN   25                    
    #define LED_FAN_OFFSET_BU 6
    #define POWER_LIMIT_MW  (5 * 5 * 1000)         // Expects at least a 5V, 5A supply

    #define NOISE_CUTOFF   75
    #define NOISE_FLOOR    200.0f

    #define TOGGLE_BUTTON  37
    #define NUM_INFO_PAGES 2

#elif ATOMLIGHT 

    // This is the "Tiki Atomic Fire Lamp" project, which is an LED lamp with 4 arms of 53 LEDs each.
    // Each arm is wired as a separate channel.

    #define ENABLE_WIFI             0               `// Connect to WiFi
    #define INCOMING_WIFI_ENABLED   0               // Accepting incoming color data and commands
    #define WAIT_FOR_WIFI           0               // Hold in setup until we have WiFi - for strips without effects
    #define TIME_BEFORE_LOCAL       0               // How many seconds before the lamp times out and shows local contexnt

    #define NUM_LEDS               (MATRIX_WIDTH * MATRIX_HEIGHT)

    #define NUM_CHANNELS    4                       // One per spoke
    #define MATRIX_WIDTH    53                      // Number of pixels wide (how many LEDs per channel)
    #define MATRIX_HEIGHT   1                       // Number of pixels tall
    #define RESERVE_MEMORY  120000                  // How much to leave free for system operation (it's not stable in low mem)
    #define ENABLE_REMOTE   0                       // IR Remote Control
    #define ENABLE_AUDIO    1                       // Listen for audio from the microphone and process it
    #define USE_TFT         0                       // Normally we use a tiny board inside the lamp with no screen
    #define FAN_SIZE        NUM_LEDS                // Allows us to use fan effects on the spokes
    #define NUM_FANS        1                       // Our fans are on channels, not in sequential order, so only one "fan"
    #define NUM_RINGS       1   
    #define LED_FAN_OFFSET_BU 0
    #define BONUS_PIXELS      0

    #define IR_REMOTE_PIN 15                        // Eric's is PIN 35

    #define POWER_LIMIT_MW (1000 * 8 * 5)           // 8 amps, 5 volts

    // Original Wiring:
    //   Fine red     = 3.3v
    //        brown   = gnd
    //        orange  = IO15
    //        yellow  = IO14
    //        green   = IO13
    //        blue    = IO12
    //        purple  = IO4

    // Eric's Version Wiring is the same.  Which is a complete coincidence but handy!

    #define LED_PIN0         5
    #define LED_PIN1        16
    #define LED_PIN2        17
    #define LED_PIN3        18

    #define DEFAULT_EFFECT_INTERVAL     (1000*60*5)

#elif UMBRELLA

    // The "Tiki Fire Umbrella" project, with 8 channels

    #define ENABLE_WIFI             1   // Connect to WiFi
    #define INCOMING_WIFI_ENABLED   1   // Accepting incoming color data and commands
    #define WAIT_FOR_WIFI           1   // Hold in setup until we have WiFi - for strips without effects
    #define TIME_BEFORE_LOCAL       5   // How many seconds before the lamp times out and shows local contexnt

    #define NUM_CHANNELS    8
    #define MATRIX_WIDTH    228                   // Number of pixels wide (how many LEDs per channel)
    #define MATRIX_HEIGHT   1                     // Number of pixels tall
    #define RESERVE_MEMORY  200000                // How much to leave free for system operation (it's not stable in low mem)
    #define ENABLE_AUDIO    1                     // Listen for audio from the microphone and process it

#elif MAGICMIRROR

    // A magic infinity mirror such as: https://amzn.to/3lEZo2D
    // I then replaced the white LEDs with a WS2812B strip and a heltec32 module:

    #define ENABLE_WIFI             1   // Connect to WiFi
    #define INCOMING_WIFI_ENABLED   1   // Accepting incoming color data and commands
    #define WAIT_FOR_WIFI           0   // Hold in setup until we have WiFi - for strips without effects
    #define TIME_BEFORE_LOCAL       1   // How many seconds before the lamp times out and shows local contexnt

    #define DEFAULT_EFFECT_INTERVAL     (10*60*24)

    #define LED_PIN0        26
    #define NUM_CHANNELS    1
    #define BONUS_PIXELS    0
    #define NUM_FANS        1
    #define FAN_SIZE        100 
    #define MATRIX_WIDTH    (NUM_FANS * FAN_SIZE + BONUS_PIXELS)    
    #define NUM_LEDS        (MATRIX_WIDTH*MATRIX_HEIGHT)
    #define MATRIX_HEIGHT   NUM_FANS
    #define RESERVE_MEMORY  150000
    #define ENABLE_REMOTE   1                     // IR Remote Control
    #define ENABLE_AUDIO    1                     // Listen for audio from the microphone and process it
    #define IR_REMOTE_PIN   15                    

    #define LED_FAN_OFFSET_BU 6

    #define POWER_LIMIT_MW 10000

#elif LEDSTRIP

    // The LED strips I use for Christmas lights under my eaves

    #define ENABLE_WIFI             1   // Connect to WiFi
    #define INCOMING_WIFI_ENABLED   1   // Accepting incoming color data and commands
    #define WAIT_FOR_WIFI           1   // Hold in setup until we have WiFi - for strips without effects
    #define TIME_BEFORE_LOCAL       5   // How many seconds before the lamp times out and shows local contexnt

    #define NUM_CHANNELS    1
    #define MATRIX_WIDTH    (8*144)   
    #define MATRIX_HEIGHT   1
    #define NUM_LEDS        (MATRIX_WIDTH * MATRIX_HEIGHT)
    #define RESERVE_MEMORY  200000                // WiFi needs about 100K free to be able to (re)connect!
    #define ENABLE_REMOTE   0                     // IR Remote Control
    #define ENABLE_AUDIO    0                     // Listen for audio from the microphone and process it

    #if M5STICKC || M5STICKCPLUS
        #define LED_PIN0 26
    #else
        #define LED_PIN0 5
    #endif

    #define POWER_LIMIT_MW (100000)                 // 100W transformer for an 8M strip max

    #define DEFAULT_EFFECT_INTERVAL     (1000*60*60*24)

#elif BELT

    // I was asked to wear something sparkly once, so I made an LED belt...

    #define ENABLE_WIFI             0   // Connect to WiFi
    #define INCOMING_WIFI_ENABLED   0   // Accepting incoming color data and commands
    #define WAIT_FOR_WIFI           0   // Hold in setup until we have WiFi - for strips without effects
    #define TIME_BEFORE_LOCAL       1   // How many seconds before the lamp times out and shows local contexnt

    #define NUM_CHANNELS    1
    #define MATRIX_WIDTH    (1*144)   
    #define MATRIX_HEIGHT   1
    #define NUM_LEDS        (MATRIX_WIDTH * MATRIX_HEIGHT)
    #define RESERVE_MEMORY  170000                // WiFi needs about 100K free to be able to (re)connect!
    #define ENABLE_REMOTE   0                     // IR Remote Control
    #define ENABLE_AUDIO    0                     // Listen for audio from the microphone and process it
    #define LED_PIN0        17
    #define POWER_LIMIT_MW (3000)                 // 100W transformer for an 8M strip max
    #define DEFAULT_EFFECT_INTERVAL     (1000*60*60*24)

#elif BROOKLYNROOM

    // A decorative strip for a rec room or similar

    #define ENABLE_WIFI             1   // Connect to WiFi
    #define INCOMING_WIFI_ENABLED   1   // Accepting incoming color data and commands
    #define WAIT_FOR_WIFI           1   // Hold in setup until we have WiFi - for strips without effects
    #define TIME_BEFORE_LOCAL       5   // How many seconds before the lamp times out and shows local contexnt

    #define NUM_CHANNELS    1
    #define NUM_LEDS        600
    #define MATRIX_WIDTH    (NUM_LEDS)    
    #define MATRIX_HEIGHT   1
    #define RESERVE_MEMORY  150000
    #define ENABLE_REMOTE   1                     // IR Remote Control
    #define FAN_SIZE        NUM_LEDS
    #define NUM_FANS        1
    #define ENABLE_AUDIO    1
    #define LED_FAN_OFFSET_BU  0                    
    #define LED_FAN_OFFSET_TD  0                   
    #define LED_FAN_OFFSET_LR  0                     
    #define LED_FAN_OFFSET_RL  0   
    #define IR_REMOTE_PIN 36
    #define POWER_LIMIT_MW (1000 * 12 * 8)
    #define ENABLE_AUDIO    1                     // Listen for audio from the microphone and process it

    #if M5STICKC || M5STICKCPLUS
        #define LED_PIN0 26
    #else
        #define LED_PIN0 5
    #endif

    #define DEFAULT_EFFECT_INTERVAL     (5*60*24)

#elif FANSET

    // An M5 stick that controls the 10 RGB fans in my PC

    #define ENABLE_WIFI             0   // Connect to WiFi
    #define INCOMING_WIFI_ENABLED   0   // Accepting incoming color data and commands
    #define WAIT_FOR_WIFI           0   // Hold in setup until we have WiFi - for strips without effects

    #define NUM_CHANNELS    1           // Everythig wired sequentially on a single channel
    #define NUM_FANS        10          // My system has 10 fans.  Because RGB.
    #define NUM_BANDS      8
    #define NUM_RINGS       1           // Fans have a single outer ring of pixels
    #define FAN_SIZE        16          // Each fan's pixel ring has 16 LEDs
    #define FAN_LEN         (NUM_FANS * FAN_SIZE)
    #define MATRIX_WIDTH    (NUM_FANS * FAN_SIZE + 32)    
    #define BONUS_PIXELS    32          // Extra pixels - in this case, my case strip
    #define NUM_LEDS        (MATRIX_WIDTH)
    #define LED_FAN_OFFSET_BU  3                         
    #define ENABLE_REMOTE   1           // IR Remote Control
    #define ENABLE_AUDIO    1           // Listen for audio from the microphone and process it
    #define TIME_BEFORE_LOCAL       2   // How many seconds before the lamp times out and shows local contexnt
    #define POWER_LIMIT_MW  4000
    #define MATRIX_HEIGHT   1
    #define RESERVE_MEMORY  150000
    
    #if M5STICKC || M5STICKCPLUS
        #define LED_PIN0 26
    #else
        #define LED_PIN0 5
    #endif

    #define DEFAULT_EFFECT_INTERVAL     (5*60*60*24)

    #define IR_REMOTE_PIN 36
    

#elif ATOMISTRING

    // Some experimentation with TM1814 and RGBW lamps. I haven't tested this config in a while but
    // am preserving it in case it's useful in the future

    #define ENABLE_WIFI             1   // Connect to WiFi
    #define INCOMING_WIFI_ENABLED   1   // Accepting incoming color data and commands
    #define WAIT_FOR_WIFI           0   // Hold in setup until we have WiFi - for strips without effects
    #define TIME_BEFORE_LOCAL       2   // How many seconds before the lamp times out and shows local contexnt

    #define DEFAULT_EFFECT_INTERVAL     (60*60*24)

    #define NUM_CHANNELS    1
    #define MATRIX_WIDTH    (144)    
    #define NUM_LEDS        MATRIX_WIDTH
    #define MATRIX_HEIGHT   1
    #define RESERVE_MEMORY  150000
    #define ENABLE_REMOTE   0                     // IR Remote Control
    #define ENABLE_AUDIO    0                     // Listen for audio from the microphone and process it

    #define FAN_SIZE        MATRIX_WIDTH
    #define NUM_FANS        1
    #define LED_FAN_OFFSET_BU 0

    #if M5STICKC || M5STICKCPLUS
        #define LED_PIN0 26
    #else
        #define LED_PIN0 5
    #endif

#elif SINGLE_INSULATOR

    // A single glass insulator with a 12-pixel ring and then a 7=pixel "bonus" ring in the middle
    #define ENABLE_WIFI             0   // Connect to WiFi
    #define INCOMING_WIFI_ENABLED   0   // Accepting incoming color data and commands
    #define WAIT_FOR_WIFI           0   // Hold in setup until we have WiFi - for strips without effects
    #define TIME_BEFORE_LOCAL       1   // How many seconds before the lamp times out and shows local contexnt

    #define DEFAULT_EFFECT_INTERVAL     (10*60*24)

    #define NUM_CHANNELS    1
    #define BONUS_PIXELS    7
    #define NUM_FANS        1
    #define FAN_SIZE        12 
    #define MATRIX_WIDTH    (NUM_FANS * FAN_SIZE + BONUS_PIXELS)    
    #define NUM_LEDS        (MATRIX_WIDTH*MATRIX_HEIGHT)
    #define MATRIX_HEIGHT   NUM_FANS
    #define RESERVE_MEMORY  150000
    #define ENABLE_REMOTE   0                     // IR Remote Control
    #define ENABLE_AUDIO    1                     // Listen for audio from the microphone and process it

    #define LED_FAN_OFFSET_BU 6

    #define POWER_LIMIT_MW 10000


    #if M5STICKC
        #define LED_PIN0 26
    #else
        #define LED_PIN0 5
    #endif
    
#elif INSULATORS

    // A set of 5 Hemmingray glass insulators that each have a ring of 12 LEDs.  Music reactive to the beat.

    #define ENABLE_WIFI             0   // Connect to WiFi
    #define INCOMING_WIFI_ENABLED   0   // Accepting incoming color data and commands
    #define WAIT_FOR_WIFI           0   // Hold in setup until we have WiFi - for strips without effects
    #define TIME_BEFORE_LOCAL       0   // How many seconds before the lamp times out and shows local contexnt

    #define DEFAULT_EFFECT_INTERVAL     (10*60*24)

    #define LED_PIN0          26
    #define NUM_CHANNELS      1
    #define RING_SIZE_0       12
    #define BONUS_PIXELS      0
    #define MATRIX_WIDTH      5
    #define MATRIX_HEIGHT     RING_SIZE_0
    #define NUM_FANS          MATRIX_WIDTH
    #define FAN_SIZE          MATRIX_HEIGHT
    #define NUM_BANDS         16
    #define NUM_LEDS          (MATRIX_WIDTH*MATRIX_HEIGHT)
    #define RESERVE_MEMORY    150000
    #define ENABLE_REMOTE     0                     // IR Remote Control
    #define ENABLE_AUDIO      1                     // Listen for audio from the microphone and process it
    #define IR_REMOTE_PIN     26                    
    #define LED_FAN_OFFSET_BU 6
    #define POWER_LIMIT_MW    10000

    #define TOGGLE_BUTTON  37
    #define NUM_INFO_PAGES 1
#endif


#ifndef BUILTIN_LED_PIN
#define BUILTIN_LED_PIN 25          // Pin for the built in LED on the Heltec board
#endif

#define STACK_SIZE (ESP_TASK_MAIN_STACK) // Stack size for each new thread
#define TIME_CHECK_INTERVAL_MS (1000 * 60 * 15)   // 15 min - How often in ms we resync the clock from NTP
#define MIN_BRIGHTNESS  4                   
#define MAX_BRIGHTNESS  255
#define BRIGHTNESS_STEP 10          // Amnount to step brightness on each remote control repeat 
#define MAX_RINGS       5


// Default Settings
//
// Set rest of things to reasonable defaults that were not specified by the project config above.

#ifndef MIN_BUFFERS
#define MIN_BUFFERS 3               // How many copies of LED buffers this board will keep at a minimum per strand
#endif

#ifndef MAX_BUFFERS
#define MAX_BUFFERS (99)            // SHould be enough for two seconds at 30fps
#endif

#ifndef ENABLE_WEBSERVER
#define ENABLE_WEBSERVER        0   // Chip provides a web server with controls to adjust effects
#endif

#ifndef ENABLE_OTA
#define ENABLE_OTA              1   // Listen for over the air update to the flash
#endif

#ifndef ENABLE_NTP
#define ENABLE_NTP              1   // Update the clock from NTP
#endif

#ifndef FAN_SIZE                // How man LEDs around the circumference
#define FAN_SIZE 1
#define NUM_FANS NUM_LEDS
#endif

#ifdef ENABLE_AUDIO
#ifndef NUM_BANDS              // How many bands in the spectrum analyzer
#define NUM_BANDS NUM_FANS
#endif
#endif

#ifndef LED_PIN0                // Which pin the LEDs are connected to
#define LED_PIN0 5
#endif

#ifndef NUM_RINGS               // How many rings in each tree/inslator/etc
#define NUM_RINGS 1
#endif

#ifndef NUM_INFO_PAGES
#define NUM_INFO_PAGES 1
#endif

// Define fan ordering for drawing into the fan directionally

#define LED_FAN_OFFSET_LR  (LED_FAN_OFFSET_BU + (FAN_SIZE * 1 / 4))         // High level stuff right here!
#define LED_FAN_OFFSET_TD  (LED_FAN_OFFSET_BU + (FAN_SIZE * 2 / 4))
#define LED_FAN_OFFSET_RL  (LED_FAN_OFFSET_BU + (FAN_SIZE * 3 / 4))

#ifndef RING_SIZE_0
#define RING_SIZE_0 0
#endif

#ifndef RING_SIZE_1
#define RING_SIZE_1 0
#endif

#ifndef RING_SIZE_2
#define RING_SIZE_2 0
#endif

#ifndef RING_SIZE_3
#define RING_SIZE_3 0
#endif

#ifndef RING_SIZE_4
#define RING_SIZE_4 0
#endif

#ifndef DEFAULT_EFFECT_INTERVAL
#define DEFAULT_EFFECT_INTERVAL 1000*30
#endif

#ifndef LED_FAN_OFFSET_BU
#define LED_FAN_OFFSET_BU 0
#endif

#ifndef RESERVE_MEMORY
#define RESERVE_MEMORY 120000
#endif

#ifndef STRAND_LEDS
#define STRAND_LEDS NUM_LEDS
#endif

#ifndef TIME_BEFORE_LOCAL
#define TIME_BEFORE_LOCAL 5
#endif

// Power Limit
//
// The limit, in watts, that the power supply for your project can supply.  If your demands
// exceed this, the code wil try to scale back brightness to hit this.  Don't rely on this
// for safety, obviously, design your hardware to protect against it with a fuse, etc.

#ifndef POWER_LIMIT_MW
#define POWER_LIMIT_MW 500*5                // Define for your power supply, default is a low 2500mA for USB
#endif

#ifndef USE_TFT
#define USE_TFT 0
#endif

#ifndef USE_OLED
#define USE_OLED 0
#endif

// gRingSizeTable
//
// Items with rings must provide a table indicating how big each ring is.  If an insulator had 60 LEDs grouped
// into rings of 30, 20, and 10, you'd have (NUM_RINGS = 3) and this table would contain (30, 20, 10).

extern DRAM_ATTR const int gRingSizeTable[];

#define MICROS_PER_SECOND   1000000
#define MILLIS_PER_SECOND   1000 

#ifndef M5STICKC
#define M5STICKC 0
#endif

#ifndef M5STICKCPLUS
#define M5STICKCPLUS 0
#endif 

// Microphone
//
// The M5 mic is on Pin34, but when I wire up my own microphone module I usually put it on pin 36.

#if ENABLE_AUDIO
#if M5STICKC || M5STICKCPLUS
#define INPUT_PIN (34)	 
#define IO_PIN (0)
#else
#define INPUT_PIN (ADC1_CHANNEL_0_GPIO_NUM)	  // Audio line input, ADC #1, input line 0 (GPIO pin 36)
#endif
#endif

// Custom WiFi Commands
//
// A Wifi packet can contain color data or potentially other info, like a clock.  Or it coud be
// a stats request.  Beyond color data these are poorly tested and likely can be removed, though
// stats and clock are handy for debugging!

#define WIFI_COMMAND_PIXELDATA   0             // Wifi command contains color data for the strip
#define WIFI_COMMAND_VU          1             // Wifi command to set the current VU reading
#define WIFI_COMMAND_CLOCK       2             // Wifi command telling us current time at the server
#define WIFI_COMMAND_PIXELDATA64 3             // Wifi command with color data and 64-bit clock vals
#define WIFI_COMMAND_STATS       4             // Wifi command to request stats from chip back to server
#define WIFI_COMMAND_REBOOT      5             // Wifi command to reboot the client chip (that's us!)
#define WIFI_COMMAND_VU_SIZE     16
#define WIFI_COMMAND_CLOCK_SIZE  20

// Final headers
// 
// Headers that are only included when certain features are enabled

#if USE_TFT
#include <U8g2lib.h>                // So we can talk to the CUU text
#include <gfxfont.h>                // Adafruit GFX for the panels
#include <Fonts/FreeSans9pt7b.h>    // A nice font for the VFD
#include <Adafruit_GFX.h>           // GFX wrapper so we can draw on matrix
#endif

// FPS
// 
// Given a time value for when the last frame took place and the current timestamp returns the number of
// frames per second, as low as 0.  Never exceeds 999 so you can make some width assumptions.

inline int FPS(uint32_t start, uint32_t end, uint32_t perSecond = MILLIS_PER_SECOND)
{
	uint32_t duration = end - start;
    if (duration == 0)
        return 999;

	double fpsf = 1.0f / (duration / (double) perSecond);
	int FPS = (int)fpsf;
	if (FPS > 999)
		FPS = 999;
	return FPS;
}

// PreferPSRAMAlloc
//
// Will return PSRAM if it's available, regular ram otherwise

inline void * PreferPSRAMAlloc(size_t s)
{
    if (psramInit())
    {
        //Serial.printf("PSRAM Array Request for %u bytes\n", s);
        return ps_malloc(s);
    }
    else
    {
        return malloc(s);
    }
}

// DelayedReboot
//
// Used for fatal errors and asserts, displays a message on the serial terminal and then resets the chip

inline void DelayedReboot(const char * psz) 
{ 
    debugE("This is an ERROR message from RemoteDebug");
    Serial.printf("\n\nUNHANDLED FATAL ERROR: %s\n", psz ? psz : "Not Specified");
    Serial.printf("** REBOOT **\n"); 
    Serial.printf("-------------------------------------------------------------------------------------\n");
    Serial.flush(); 
    delay(2500); 
    ESP.restart();
    exit(0);
}  

// AppTime
//
// A class that keeps track of the clock, how long the last frame took, calculating FPS, etc.

class AppTime
{
  protected:

    double _lastFrame;
    double _deltaTime;
  
  public:

    // NewFrame
    //
    // Call this at the start of every frame or udpate, and it'll figure out and keep track of how
    // long between frames 

    void NewFrame()
    {
        timeval tv;
        gettimeofday(&tv, nullptr);
        double current = CurrentTime();
        _deltaTime = current - _lastFrame;

        // Cap the delta time at one full second

        if (_deltaTime > 1.0f)
            _deltaTime = 1.0f;

        _lastFrame = current;
    }

    AppTime() : _lastFrame(CurrentTime())
    {
        NewFrame();
    }

    double FrameStartTime() const
    {
        return _lastFrame;
    }

    static double CurrentTime()
    {
        timeval tv;
        gettimeofday(&tv, nullptr);
        return tv.tv_sec + (tv.tv_usec/(double)MICROS_PER_SECOND);
    }

    static double TimeFromTimeval(const timeval & tv)
    {
        return tv.tv_sec + (tv.tv_usec/(double)MICROS_PER_SECOND);
    }

    static timeval TimevalFromTime(double t)
    {
        timeval tv;
        tv.tv_sec = (long)t;
        tv.tv_usec = t - tv.tv_sec;
        return tv;
    }

    double DeltaTime() const
    {
        return _deltaTime;
    }
};

// C Helpers
//
// Simple inline utility functions like random numbers, mapping, conversion, etc

inline static double randomDouble(double lower, double upper)
{
    double result = (lower + ((upper - lower) * rand()) / RAND_MAX);
    return result;
}

inline double mapDouble(double x, double in_min, double in_max, double out_min, double out_max)
{
    return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

inline uint64_t ULONGFromMemory(byte * payloadData)
{
    return  (uint64_t)payloadData[7] << 56  | 
            (uint64_t)payloadData[6] << 48  | 
            (uint64_t)payloadData[5] << 40  | 
            (uint64_t)payloadData[4] << 32  | 
            (uint64_t)payloadData[3] << 24  | 
            (uint64_t)payloadData[2] << 16  | 
            (uint64_t)payloadData[1] << 8   | 
            (uint64_t)payloadData[0];
}

inline uint32_t DWORDFromMemory(byte * payloadData)
{
    return  (uint32_t)payloadData[3] << 24  | 
            (uint32_t)payloadData[2] << 16  | 
            (uint32_t)payloadData[1] << 8   | 
            (uint32_t)payloadData[0];
}

inline uint16_t WORDFromMemory(byte * payloadData)
{
    return  (uint16_t)payloadData[1] << 8   | 
            (uint16_t)payloadData[0];
}

inline bool CheckBlueBuffer(CRGB * prgb, size_t count)
{
    bool bOK = true;
    for (int i = 0; i < count; i++)
    {
        if (bOK)
        {
            if (prgb[i].r > 0 || prgb[i].g > 0)
            {
                Serial.printf("Other color detected at offset %d\n", i);
                bOK = false;
            }
        }
    }
    return bOK;
}
