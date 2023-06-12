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
//              May-01-2021  v018       Davepl      Put receive timeout back in, cRec'd to 0
//              Jun-17-2021  v019       Davepl      Atomlight2 + variable FPS
//              Jul-08-2021  v020       Davepl      Particle System, Insulators, lib deps
//              Sep-18-2021  v021       Davepl      Github Release
//              Nov-07-2021  v022       Davepl      Rev'd with new Github PRs to date
//              Mar-16-2022  v023       Davepl      Response packet on socket with stats
//              Mar-17-2022  v024       Davepl      Catchup clock to server when in future
//              May-17-2022  v025       Davepl      After merge of RepsonsePacket into main
//              May-24-2022  v026       Davepl      Adding BaseGFX/LEDMatrixGFX/LEDStripGFX
//              Oct-01-2022  v027       Davepl      Mesmerizer integration and screen fixes
//              Oct-01-2022  v028       Davepl      Adjust buffer sizes due to lower mem free
//              Oct-02-2022  v029       Davepl      Change WiFiUDP to use free/malloc
//              Oct-03-2022  v030       Davepl      Smoother Draw and support for TFT S3 Feather
//              Oct-30-2022  v031       Davepl      Screen cleanup, core assignments moved around
//              Oct-30-2022  v032       Davepl      Better wait code, core assignments
//              Oct-30-2022  v033       Davepl      Fixed mistiming bug when no draw was ready
//              Nov-15-2022  v034       Davepl      Fixed buffer full condition
//              Jan-19-2023  v035       Davepl      After LaserLine episode merge
//              Jan-29-2023  v036       Davepl      After Char *, string, includes, soundanalyzer
//              Jun-10-2023  v037       Davepl      New Screen classes 
//
//---------------------------------------------------------------------------

#pragma once

#include <inttypes.h>
#include <iostream>
#include <memory>
#include <string>
#include <sstream>
#include <sys/time.h>
#include <exception>
#include <mutex>
#include <vector>
#include <errno.h>
#include <math.h>
#include <deque>
#include <algorithm>
#include <numeric>

#include <Arduino.h>
#include <ArduinoOTA.h>                         // For updating the flash over WiFi
#include <ESPmDNS.h>
#include <SPI.h>

#include <nvs_flash.h>                   // Non-volatile storage access
#include <nvs.h>

#define FASTLED_INTERNAL 1               // Suppresses build banners
#include <FastLED.h>

#include <WiFi.h>

#include "RemoteDebug.h"

// The goal here is to get two variables, one numeric and one string, from the *same* version
// value.  So if version = 020,
//
// FLASH_VERSION==20
// FLASH_VERSION_NAME=="v020"
//
// BUGBUG (davepl): If you know a cleaner way, please improve this!

#define FLASH_VERSION          37    // Update ONLY this to increment the version number

#ifndef USE_MATRIX                   // We support strips by default unless specifically defined out
    #ifndef USESTRIP
        #define USESTRIP 1
    #endif
#endif

#define XSTR(x) STR(x)              // The defs will generate the stringized version of it
#if FLASH_VERSION > 99
    #define STR(x) "v"#x
#else
    #define STR(x) "v0"#x
#endif
#define FLASH_VERSION_NAME_X(x) "v"#x
#define FLASH_VERSION_NAME XSTR(FLASH_VERSION)

#define FASTLED_INTERNAL        1   // Silence FastLED build banners
#define NTP_DELAY_COUNT         20  // delay count for ntp update
#define NTP_PACKET_LENGTH       48  // ntp packet length
#define TIME_ZONE             (-8)  // My offset from London (UTC-8)

// C Helpers and Macros

#define NAME_OF(x)          #x

// NAME_OF with first character cut off - addresses underscore-prefixed (member) variables
#define ACTUAL_NAME_OF(x)   ((#x) + 1)

#define ARRAYSIZE(a)        (sizeof(a)/sizeof(a[0]))        // Returns the number of elements in an array
#define PERIOD_FROM_FREQ(f) (round(1000000 * (1.0 / f)))    // Calculate period in microseconds (us) from frequency in Hz
#define FREQ_FROM_PERIOD(p) (1.0 / p * 1000000)             // Calculate frequency in Hz given the period in microseconds (us)

// I've built and run this on the Heltec Wifi 32 module and the M5StickC.  The
// main difference is pinout and the OLED/LCD screen.  The presense of absence
// of the OLED/LCD is now controlled separately, but M5 is always equipped
// with one (but it doesn't have to be used!).

#if M5STICKC
#include "M5StickC.h"
#undef min                                      // They define a min() on us
#endif

#if M5STICKCPLUS
#include "M5StickCPlus.h"
#undef min                                      // They define a min() on us
#endif

#if M5STACKCORE2
#include "M5Core2.h"
#undef min                                      // They define a min() on us
#endif

#define EFFECT_CROSS_FADE_TIME 1200.0    // How long for an effect to ramp brightness fader down and back during effect change

// Thread priorities
//
// We have a half-dozen workers and these are their relative priorities.  It might survive if all were set equal,
// but I think drawing should be lower than audio so that a bad or greedy effect doesn't starve the audio system.
//
// Idle tasks in taskmgr run at IDLE_PRIORITY+1 so you want to be at least +2

#define DRAWING_PRIORITY        tskIDLE_PRIORITY+8
#define SOCKET_PRIORITY         tskIDLE_PRIORITY+7
#define AUDIOSERIAL_PRIORITY    tskIDLE_PRIORITY+6      // If equal or lower than audio, will produce garbage on serial
#define NET_PRIORITY            tskIDLE_PRIORITY+5
#define AUDIO_PRIORITY          tskIDLE_PRIORITY+4
#define SCREEN_PRIORITY         tskIDLE_PRIORITY+3

#define REMOTE_PRIORITY         tskIDLE_PRIORITY+3
#define DEBUG_PRIORITY          tskIDLE_PRIORITY+2
#define JSONWRITER_PRIORITY     tskIDLE_PRIORITY+2
#define COLORDATA_PRIORITY      tskIDLE_PRIORITY+2

// If you experiment and mess these up, my go-to solution is to put Drawing on Core 0, and everything else on Core 1.
// My current core layout is as follows, and as of today it's solid as of (7/16/21).
//
// #define DRAWING_CORE            0
// #define NET_CORE                1
// #define AUDIO_CORE              0
// #define SCREEN_CORE             1
// #define DEBUG_CORE              1
// #define SOCKET_CORE             1
// #define REMOTE_CORE             1

// Some "Reliability Rules"
// Drawing must be on Core 1 if using SmartMatrix unless you specify SMARTMATRIX_OPTIONS_ESP32_CALC_TASK_CORE_1

#define DRAWING_CORE            1
#define NET_CORE                0
#define AUDIO_CORE              0
#define AUDIOSERIAL_CORE        1
#define SCREEN_CORE             0
#define DEBUG_CORE              1
#define SOCKET_CORE             1
#define REMOTE_CORE             1
#define JSONWRITER_CORE         0
#define COLORDATA_CORE          1

#define FASTLED_INTERNAL            1   // Suppresses the compilation banner from FastLED
#define __STDC_FORMAT_MACROS

extern RemoteDebug Debug;           // Let everyone in the project know about it.  If you don't have it, delete this

// Project Configuration
//
// One and only one of DEMO, SPECTRUM, ATOMLIGHT, etc should be set to true by the build config for your project
//
// I've used this code to build a dozen different projects, most of which can be created by defining
// the right built environment (like INSULATORS=1).  The config here defines everything about the
// LEDs, how many, on how many channels, laid out into how many fang/rings, and so on.  You can also
// specify the audio system config like how many band channels.

#if DEMO

    // This is a simple demo configuration.  To build, simply connect the data lead from a WS2812B
    // strip to pin 5.  This does not use the OLED, LCD, or anything fancy, it simply drives the
    // LEDs with a simple rainbow effect as specified in effects.cpp for DEMO.
    //
    // Please ensure you supply sufficent power to your strip, as even the DEMO of 144 LEDs, if set
    // to white, would overload a USB port.
    #ifndef PROJECT_NAME
    #define PROJECT_NAME            "Demo"
    #endif

    #define MATRIX_WIDTH            144
    #define MATRIX_HEIGHT           8
    #define NUM_LEDS                (MATRIX_WIDTH*MATRIX_HEIGHT)
    #define NUM_CHANNELS            1
    #define NUM_RINGS               5
    #define RING_SIZE_0             24
    #define ENABLE_AUDIO            0

    #define POWER_LIMIT_MW       12 * 10 * 1000   // 10 amp supply at 5 volts assumed

    // Once you have a working project, selectively enable various additional features by setting
    // them to 1 in the list below.  This DEMO config assumes no audio (mic), or screen, etc.

    #ifndef ENABLE_WIFI
        #define ENABLE_WIFI             0   // Connect to WiFi
    #endif

    #define INCOMING_WIFI_ENABLED   0   // Accepting incoming color data and commands
    #define TIME_BEFORE_LOCAL       0   // How many seconds before the lamp times out and shows local content
    #define ENABLE_NTP              0   // Set the clock from the web
    #define ENABLE_OTA              0   // Accept over the air flash updates

    #if M5STICKC || M5STICKCPLUS || M5STACKCORE2
        #define LED_PIN0 32
    #elif LILYGOTDISPLAYS3
        #define LED_PIN0 21
    #else
        #define LED_PIN0 5
    #endif

    // The webserver serves files from its SPIFFS filesystem, such as index.html, and those files must be
    // uploaded to SPIFFS with the "Upload Filesystem Image" command before it can work.  When running
    // you should be able to see/select the list of effects by visiting the chip's IP in a browser.  You can
    // get the chip's IP by watching the serial output or checking your router for the DHCP given to 'LEDWifi'

    #ifndef ENABLE_WEBSERVER
        #define ENABLE_WEBSERVER        0   // Turn on the internal webserver
    #endif

#elif LANTERN

    // A railway-style lantern with concentric rings of light (16+12+8+1)

    #ifndef PROJECT_NAME
    #define PROJECT_NAME            "Lantern"
    #endif

    #define NUM_FANS                1
    #define NUM_RINGS               4
    #define FAN_SIZE                (RING_SIZE_0 + RING_SIZE_1 + RING_SIZE_2 + RING_SIZE_3)
    #define RING_SIZE_0             16
    #define RING_SIZE_1             12
    #define RING_SIZE_2             8
    #define RING_SIZE_3             1
    #define MATRIX_WIDTH            FAN_SIZE
    #define MATRIX_HEIGHT           1
    #define NUM_LEDS                (MATRIX_WIDTH*MATRIX_HEIGHT)
    #define NUM_CHANNELS            1
    #define ENABLE_AUDIO            1

    #define POWER_LIMIT_MW       5000   // 1 amp supply at 5 volts assumed

    // Once you have a working project, selectively enable various additional features by setting
    // them to 1 in the list below.  This DEMO config assumes no audio (mic), or screen, etc.

    #define ENABLE_WIFI             0   // Connect to WiFi
    #define INCOMING_WIFI_ENABLED   0   // Accepting incoming color data and commands
    #define TIME_BEFORE_LOCAL       0   // How many seconds before the lamp times out and shows local contexnt
    #define ENABLE_NTP              0   // Set the clock from the web
    #define ENABLE_OTA              0   // Accept over the air flash updates

    #if M5STICKC
        #define LED_PIN0 26
    #elif M5STICKCPLUS || M5STACKCORE2
        #define LED_PIN0 32
    #else
        #define LED_PIN0 5
    #endif

    // The webserver serves files from its SPIFFS filesystem, such as index.html, and those files must be
    // uploaded to SPIFFS with the "Upload Filesystem Image" command before it can work.  When running
    // you should be able to see/select the list of effects by visiting the chip's IP in a browser.  You can
    // get the chip's IP by watching the serial output or checking your router for the DHCP given to 'LEDWifi'

    #define ENABLE_WEBSERVER        0                                       // Turn on the internal webserver
    #define DEFAULT_EFFECT_INTERVAL 1000 * 60 * 60 * 24                     // One a day!

    #define TOGGLE_BUTTON_1 37
    #define TOGGLE_BUTTON_2 39

    #define NUM_INFO_PAGES          2
    #define ONSCREEN_SPECTRUM_PAGE  1   // Show a little spctrum analyzer on one of the info pages (slower)

#elif TREESET

    #ifndef PROJECT_NAME
    #define PROJECT_NAME            "Treeset"
    #endif

    #define ENABLE_WIFI             1  // Connect to WiFi
    #define INCOMING_WIFI_ENABLED   0   // Accepting incoming color data and commands
    #define WAIT_FOR_WIFI           0   // Hold in setup until we have WiFi - for strips without effects
    #define TIME_BEFORE_LOCAL       0   // How many seconds before the lamp times out and shows local content
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
    #define ENABLE_REMOTE     1                     // IR Remote Control
    #define ENABLE_AUDIO      1                     // Listen for audio from the microphone and process it
    #define IR_REMOTE_PIN     25
    #define LED_FAN_OFFSET_BU 12
    #define POWER_LIMIT_MW    20000

    #define TOGGLE_BUTTON  37
    #define NUM_INFO_PAGES 2

#elif WROVERKIT

    #define MATRIX_WIDTH            144
    #define MATRIX_HEIGHT           1
    #define NUM_LEDS                (MATRIX_WIDTH*MATRIX_HEIGHT)
    #define NUM_CHANNELS            1
    #define NUM_RINGS               5
    #define RING_SIZE_0             24

    #define POWER_LIMIT_MW       5000   // 1 amp supply at 5 volts assumed
    #define USE_LCD                 1

    // Once you have a working project, selectively enable various additional features by setting
    // them to 1 in the list below.  This DEMO config assumes no audio (mic), or screen, etc.

    #define ENABLE_WIFI             1   // Connect to WiFi
    #define INCOMING_WIFI_ENABLED   1   // Doesn't work smoothly with the screen on for some reason!
    #define TIME_BEFORE_LOCAL       2   // How many seconds before the lamp times out and shows local content
    #define ENABLE_NTP              1   // Set the clock from the web
    #define ENABLE_OTA              1   // Accept over the air flash updates
    #define WAIT_FOR_WIFI           0   // Don't *need* it so don't wait for it

    #define LED_PIN0 5

    // The webserver serves files from its SPIFFS filesystem, such as index.html, and those files must be
    // uploaded to SPIFFS with the "Upload Filesystem Image" command before it can work.  When running
    // you should be able to see/select the list of effects by visiting the chip's IP in a browser.  You can
    // get the chip's IP by watching the serial output or checking your router for the DHCP given to 'LEDWifi'

    #define ENABLE_WEBSERVER        1   // Turn on the internal webserver


#elif LASERLINE

    // This project is set up as a 48x16 matrix of 16x16 WS2812B panels such as: https://amzn.to/3ABs5DK
    // It uses an M5StickCPlus which has a microphone and LCD built in:  https://amzn.to/3CrvCFh
    // It displays a spectrum analyzer and music visualizer

    #ifndef PROJECT_NAME
    #define PROJECT_NAME            "Laser Line"
    #endif


    #define ENABLE_AUDIOSERIAL      0   // Report peaks at 2400baud on serial port for PETRock consumption
    #define ENABLE_WIFI             0   // Connect to WiFi
    #define INCOMING_WIFI_ENABLED   0   // Accepting incoming color data and commands
    #define WAIT_FOR_WIFI           0   // Hold in setup until we have WiFi - for strips without effects
    #define TIME_BEFORE_LOCAL       0   // How many seconds before the lamp times out and shows local content
    #define ENABLE_WEBSERVER        0   // Turn on the internal webserver
    #define ENABLE_NTP              0   // Set the clock from the web
    #define ENABLE_OTA              0   // Accept over the air flash updates
    #define ENABLE_REMOTE           0   // IR Remote Control
    #define ENABLE_AUDIO            1   // Listen for audio from the microphone and process it

    #define DEFAULT_EFFECT_INTERVAL     (60*60*24*5)

    #define NUM_CHANNELS    1
    #define RING_SIZE_0     24
    #define BONUS_PIXELS    0
    #define MATRIX_WIDTH    700
    #define MATRIX_HEIGHT   1
    #define NUM_FANS        MATRIX_WIDTH
    #define FAN_SIZE        MATRIX_HEIGHT
    #define NUM_BANDS       16
    #define NUM_LEDS        (MATRIX_WIDTH*MATRIX_HEIGHT)
    #define LED_FAN_OFFSET_BU 6
    #define POWER_LIMIT_MW  (8 * 5 * 1000)         // Expects at least a 5V, 20A supply (100W)

    #define TOGGLE_BUTTON_1 37
    #define TOGGLE_BUTTON_2 39

    #define LED_PIN0 32

    #define NUM_INFO_PAGES          2
    #define ONSCREEN_SPECTRUM_PAGE  1   // Show a little spectrum analyzer on one of the info pages (slower)

#elif MESMERIZER

    // This project is set up as a 48x16 matrix of 16x16 WS2812B panels such as: https://amzn.to/3ABs5DK
    // It uses an M5StickCPlus which has a microphone and LCD built in:  https://amzn.to/3CrvCFh
    // It displays a spectrum analyzer and music visualizer

    #ifndef PROJECT_NAME
    #define PROJECT_NAME            "Mesmerizer"
    #endif

    #define SHOW_FPS_ON_MATRIX      0
    #define ENABLE_AUDIOSERIAL      0   // Report peaks at 2400baud on serial port for PETRock consumption
    #define ENABLE_WIFI             1   // Connect to WiFi
    #define INCOMING_WIFI_ENABLED   1   // Accepting incoming color data and commands
    #define WAIT_FOR_WIFI           0   // Hold in setup until we have WiFi - for strips without effects
    #define TIME_BEFORE_LOCAL       2   // How many seconds before the lamp times out and shows local content
    #define ENABLE_WEBSERVER        1  // Turn on the internal webserver
    #define ENABLE_NTP              1   // Set the clock from the web
    #define ENABLE_OTA              0   // Accept over the air flash updates
    #define ENABLE_REMOTE           1   // IR Remote Control
    #define ENABLE_AUDIO            1   // Listen for audio from the microphone and process it
    #define SCALE_AUDIO_EXPONENTIAL 0
    #define ENABLE_AUDIO_SMOOTHING  1

    #define DEFAULT_EFFECT_INTERVAL     (MILLIS_PER_SECOND * 60 * 2)
    #define MILLIS_PER_FRAME        0

    #define NUM_CHANNELS    1
    #define RING_SIZE_0     24
    #define BONUS_PIXELS    0
    #define MATRIX_WIDTH    64
    #define MATRIX_HEIGHT   32
    #define NUM_FANS        128
    #define FAN_SIZE        16
    #define NUM_BANDS       16
    #define NUM_LEDS        (MATRIX_WIDTH*MATRIX_HEIGHT)
    #define IR_REMOTE_PIN   39
    #define INPUT_PIN       36
    #define LED_FAN_OFFSET_BU 6
    #define POWER_LIMIT_MW  (5 * 8 * 1000)         // Expects at least a 5V, 8A supply

    #define TOGGLE_BUTTON_1 0

#elif TTGO

    // Variant of Spectrum set up for a TTGO using a MAX4466 microphone on pin27

    // This project is set up as a 48x16 matrix of 16x16 WS2812B panels such as: https://amzn.to/3ABs5DK
    // It uses an M5StickCPlus which has a microphone and LCD built in:  https://amzn.to/3CrvCFh
    // It displays a spectrum analyzer and music visualizer

    #ifndef PROJECT_NAME
    #define PROJECT_NAME            "TTGO"
    #endif

    #define ENABLE_WIFI             1  // Connect to WiFi
    #define INCOMING_WIFI_ENABLED   1   // Accepting incoming color data and commands
    #define WAIT_FOR_WIFI           0   // Hold in setup until we have WiFi - for strips without effects
    #define TIME_BEFORE_LOCAL       2   // How many seconds before the lamp times out and shows local content
    #define ENABLE_WEBSERVER        1   // Turn on the internal webserver
    #define ENABLE_NTP              1   // Set the clock from the web
    #define ENABLE_OTA              0  // Accept over the air flash updates
    #define ENABLE_REMOTE           1   // IR Remote Control
    #define ENABLE_AUDIO            1   // Listen for audio from the microphone and process it

    #define DEFAULT_EFFECT_INTERVAL     (60*60*24)

    #define MAX_BUFFERS     20

    #define LED_PIN0        21          // Note that TFT board on TFTGO uses ping 19, 18, 5, 16, 23, and 4
    #define NUM_CHANNELS    1
    #define RING_SIZE_0     24
    #define BONUS_PIXELS    0
    #define MATRIX_WIDTH    48
    #define MATRIX_HEIGHT   16
    #define NUM_FANS        MATRIX_WIDTH
    #define FAN_SIZE        MATRIX_HEIGHT
    #define NUM_BANDS       16
    #define NUM_LEDS        (MATRIX_WIDTH*MATRIX_HEIGHT)
    #define IR_REMOTE_PIN   22
    #define LED_FAN_OFFSET_BU 6
    #define POWER_LIMIT_MW  (1 * 5 * 1000)         // Expects at least a 5V, 1A supply

    #define TOGGLE_BUTTON_1         35
    #define NUM_INFO_PAGES          4
    #define ONSCREEN_SPECTRUM_PAGE  2   // Show a little spectrum analyzer on one of the info pages (slower)

#elif XMASTREES

    // This project is set up as a 48x16 matrix of 16x16 WS2812B panels such as: https://amzn.to/3ABs5DK
    // It uses an M5StickCPlus which has a microphone and LCD built in:  https://amzn.to/3CrvCFh
    // It displays a spectrum analyzer and music visualizer

    #ifndef PROJECT_NAME
    #define PROJECT_NAME            "X-mas Trees"
    #endif

    #define ENABLE_WIFI             1  // Connect to WiFi
    #define INCOMING_WIFI_ENABLED   1   // Accepting incoming color data and commands
    #define WAIT_FOR_WIFI           0   // Hold in setup until we have WiFi - for strips without effects
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
    #define RING_SIZE_0     24
    #define BONUS_PIXELS    0
    #define MATRIX_WIDTH    24
    #define MATRIX_HEIGHT   5
    #define FAN_SIZE        MATRIX_WIDTH
    #define NUM_FANS        MATRIX_HEIGHT
    #define NUM_BANDS       16
    #define NUM_LEDS        (MATRIX_WIDTH*MATRIX_HEIGHT)
    #define IR_REMOTE_PIN   25
    #define LED_FAN_OFFSET_BU 6
    #define POWER_LIMIT_MW  (5 * 5 * 1000)         // Expects at least a 5V, 5A supply

    #define TOGGLE_BUTTON_1         37
    #define TOGGLE_BUTTON_2         39
    #define NUM_INFO_PAGES 2

#elif ATOMLIGHT

    // This is the "Tiki Atomic Fire Lamp" project, which is an LED lamp with 4 arms of 53 LEDs each.
    // Each arm is wired as a separate channel.

    #ifndef PROJECT_NAME
    #define PROJECT_NAME            "Atom Light"
    #endif

    #define ENABLE_WIFI             0               // Connect to WiFi
    #define INCOMING_WIFI_ENABLED   0               // Accepting incoming color data and commands
    #define WAIT_FOR_WIFI           0               // Hold in setup until we have WiFi - for strips without effects
    #define TIME_BEFORE_LOCAL       0               // How many seconds before the lamp times out and shows local content

    #define NUM_LEDS               (MATRIX_WIDTH * MATRIX_HEIGHT)

    #define NUM_CHANNELS    4                       // One per spoke
    #define MATRIX_WIDTH    53                      // Number of pixels wide (how many LEDs per channel)
    #define MATRIX_HEIGHT   1                       // Number of pixels tall
    #define ENABLE_REMOTE   0                       // IR Remote Control
    #define ENABLE_AUDIO    1                       // Listen for audio from the microphone and process it
    #define USE_SCREEN      0                       // Normally we use a tiny board inside the lamp with no screen
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

    #ifndef PROJECT_NAME
    #define PROJECT_NAME            "Umbrella"
    #endif

    #define COLOR_ORDER     EOrder::RGB
    #define ENABLE_WIFI             1   // Connect to WiFi
    #define INCOMING_WIFI_ENABLED   1   // Accepting incoming color data and commands
    #define WAIT_FOR_WIFI           0   // Hold in setup until we have WiFi - for strips without effects
    #define TIME_BEFORE_LOCAL       2   // How many seconds before the lamp times out and shows local content
    #define ENABLE_OTA              1   // Enable over the air updates to the flash
    #define ENABLE_REMOTE   1                     // IR Remote Control
    #define IR_REMOTE_PIN   39
    #define ENABLE_AUDIO    1                     // Listen for audio from the microphone and process it
    #define MAX_BUFFERS     40

    #define POWER_LIMIT_MW (1600 * 1000)                 // 100W transformer for an 8M strip max
    #define DEFAULT_EFFECT_INTERVAL     (1000*30 * 60)


    // The "Tiki Fire Umbrella" project, with 8 channels

    #define LED_PIN0                5   // Only one pin, it's routed to all 8 spokes.  Independent turned out not to be that useful.
    #define NUM_CHANNELS    1
    #define MATRIX_WIDTH    228                   // Number of pixels wide (how many LEDs per channel)
    #define MATRIX_HEIGHT   1                     // Number of pixels tall

    #define ONBOARD_LED_R    16
    #define ONBOARD_LED_G    17
    #define ONBOARD_LED_B    18

    #define TOGGLE_BUTTON_2  0

#elif MAGICMIRROR

    // A magic infinity mirror such as: https://amzn.to/3lEZo2D
    // I then replaced the white LEDs with a WS2812B strip and a heltec32 module:

    #ifndef PROJECT_NAME
    #define PROJECT_NAME            "Magic Mirror"
    #endif

    #define ENABLE_WIFI             1   // Connect to WiFi
    #define INCOMING_WIFI_ENABLED   1   // Accepting incoming color data and commands
    #define WAIT_FOR_WIFI           0   // Hold in setup until we have WiFi - for strips without effects
    #define TIME_BEFORE_LOCAL       1   // How many seconds before the lamp times out and shows local content

    #define DEFAULT_EFFECT_INTERVAL     (10*60*24)

    #define LED_PIN0        26
    #define NUM_CHANNELS    1
    #define BONUS_PIXELS    0
    #define NUM_FANS        1
    #define FAN_SIZE        100
    #define MATRIX_WIDTH    (NUM_FANS * FAN_SIZE + BONUS_PIXELS)
    #define NUM_LEDS        (MATRIX_WIDTH*MATRIX_HEIGHT)
    #define MATRIX_HEIGHT   NUM_FANS
    #define ENABLE_REMOTE   1                     // IR Remote Control
    #define ENABLE_AUDIO    1                     // Listen for audio from the microphone and process it
    #define IR_REMOTE_PIN   15

    #define LED_FAN_OFFSET_BU 6

    #define POWER_LIMIT_MW 10000

#elif LEDSTRIP

    // The LED strips I use for Christmas lights under my eaves

    #ifndef PROJECT_NAME
    #define PROJECT_NAME            "Ledstrip"
    #endif

    #define ENABLE_WEBSERVER        1   // Turn on the internal webserver
    #define ENABLE_WIFI             1   // Connect to WiFi
    #define INCOMING_WIFI_ENABLED   1   // Accepting incoming color data and commands
    #define WAIT_FOR_WIFI           1   // Hold in setup until we have WiFi - for strips without effects
    #define TIME_BEFORE_LOCAL       5   // How many seconds before the lamp times out and shows local content

    #define NUM_CHANNELS    1
    #define MATRIX_WIDTH    (8*144)     // My maximum run, and about all you can do at 30fps
    #define MATRIX_HEIGHT   1
    #define NUM_LEDS        (MATRIX_WIDTH * MATRIX_HEIGHT)
    #define ENABLE_REMOTE   0                     // IR Remote Control
    #define ENABLE_AUDIO    0                     // Listen for audio from the microphone and process it
    #define LED_PIN0        5

    #define POWER_LIMIT_MW (INT_MAX)              // Unlimited power for long strips, up to you to limit here or supply enough!

    #define DEFAULT_EFFECT_INTERVAL     (1000*20)

    #define RING_SIZE_0 1
    #define RING_SIZE_1 2
    #define RING_SIZE_2 4
    #define RING_SIZE_3 8
    #define RING_SIZE_4 16

#elif CHIEFTAIN

    // The LED strips I use for Christmas lights under my eaves

    #ifndef PROJECT_NAME
    #define PROJECT_NAME            "Chieftain"
    #endif

    #define ENABLE_WIFI             1   // Connect to WiFi
    #define INCOMING_WIFI_ENABLED   1   // Accepting incoming color data and commands
    #define WAIT_FOR_WIFI           0   // Hold in setup until we have WiFi - for strips without effects
    #define TIME_BEFORE_LOCAL       5   // How many seconds before the lamp times out and shows local content

    #define NUM_CHANNELS    1
    #define MATRIX_WIDTH    (12)
    #define MATRIX_HEIGHT   1
    #define NUM_LEDS        (MATRIX_WIDTH * MATRIX_HEIGHT)
    #define ENABLE_REMOTE   0                     // IR Remote Control
    #define ENABLE_AUDIO    1                     // Listen for audio from the microphone and process it
    #define LED_PIN0        5

    #define POWER_LIMIT_MW (4500)                 // Assumes modern USB3 powered port or supply

    #define DEFAULT_EFFECT_INTERVAL     (1000*20)

    #define RING_SIZE_0 1
    #define RING_SIZE_1 2
    #define RING_SIZE_2 4
    #define RING_SIZE_3 8
    #define RING_SIZE_4 16

#elif BELT

    // I was asked to wear something sparkly once, so I made an LED belt...

    #ifndef PROJECT_NAME
    #define PROJECT_NAME            "Belt"
    #endif

    #define ENABLE_WIFI             0   // Connect to WiFi
    #define INCOMING_WIFI_ENABLED   0   // Accepting incoming color data and commands
    #define WAIT_FOR_WIFI           0   // Hold in setup until we have WiFi - for strips without effects
    #define TIME_BEFORE_LOCAL       1   // How many seconds before the lamp times out and shows local content

    #define NUM_CHANNELS    1
    #define MATRIX_WIDTH    (1*144)
    #define MATRIX_HEIGHT   1
    #define NUM_LEDS        (MATRIX_WIDTH * MATRIX_HEIGHT)
    #define ENABLE_REMOTE   0                     // IR Remote Control
    #define ENABLE_AUDIO    0                     // Listen for audio from the microphone and process it
    #define LED_PIN0        17
    #define POWER_LIMIT_MW (3000)                 // 100W transformer for an 8M strip max
    #define DEFAULT_EFFECT_INTERVAL     (1000*60*60*24)

#elif SPECTRUM

    // This project is set up as a 48x16 matrix of 16x16 WS2812B panels such as: https://amzn.to/3ABs5DK
    // It uses an M5StickCPlus which has a microphone and LCD built in:  https://amzn.to/3CrvCFh
    // It displays a spectrum analyzer and music visualizer

    #ifndef PROJECT_NAME
    #define PROJECT_NAME            "Spectrum"
    #endif

    #define ENABLE_AUDIOSERIAL      0   // Report peaks at 2400baud on serial port for PETRock consumption
    #define ENABLE_WIFI             1   // Connect to WiFi
    #define INCOMING_WIFI_ENABLED   1   // Accepting incoming color data and commands
    #define WAIT_FOR_WIFI           0   // Hold in setup until we have WiFi - for strips without effects
    #define TIME_BEFORE_LOCAL       2   // How many seconds before the lamp times out and shows local content
    #define ENABLE_WEBSERVER        1   // Turn on the internal webserver
    #define ENABLE_NTP              1   // Set the clock from the web
    #define ENABLE_OTA              0   // Accept over the air flash updates
    #define ENABLE_REMOTE           1   // IR Remote Control
    #define ENABLE_AUDIO            1   // Listen for audio from the microphone and process it

    #if USE_PSRAM
        #define MAX_BUFFERS         500
    #else
        #define MAX_BUFFERS         10
    #endif

    #define DEFAULT_EFFECT_INTERVAL     (60*60*24*5)

    #if SPECTRUM_WROVER_KIT
        #define LED_PIN0        5
    #else
        #define LED_PIN0        26
    #endif

    #define NUM_CHANNELS    1
    #define RING_SIZE_0     24
    #define BONUS_PIXELS    0
    #define MATRIX_WIDTH    48
    #define MATRIX_HEIGHT   16
    #define NUM_FANS        MATRIX_WIDTH
    #define FAN_SIZE        MATRIX_HEIGHT
    #define NUM_BANDS       16
    #define NUM_LEDS        (MATRIX_WIDTH*MATRIX_HEIGHT)
    #define LED_FAN_OFFSET_BU 6
    #define POWER_LIMIT_MW  (10 * 5 * 1000)         // Expects at least a 5V, 20A supply (100W)

    #define TOGGLE_BUTTON_1 37
    #define TOGGLE_BUTTON_2 39

    #ifdef SPECTRUM_WROVER_KIT
    #else
        #define NUM_INFO_PAGES          2
        #define ONSCREEN_SPECTRUM_PAGE  1   // Show a little spectrum analyzer on one of the info pages (slower)
    #endif

#elif FANSET

    // An M5 stick that controls the 10 RGB fans in my PC

    #ifndef PROJECT_NAME
    #define PROJECT_NAME            "Fan set"
    #endif

    #define ENABLE_AUDIOSERIAL      1   // Report peaks at 2400baud on serial port for PETRock consumption
    #define ENABLE_WIFI             1           // Connect to WiFi
    #define INCOMING_WIFI_ENABLED   1           // Accepting incoming color data and commands
    #define WAIT_FOR_WIFI           0           // Hold in setup until we have WiFi - for strips without effects
    #define TIME_BEFORE_LOCAL       2           // How many seconds before the lamp times out and shows local content

    #define ENABLE_WEBSERVER        1   // Turn on the internal webserver
    #define ENABLE_NTP              1   // Set the clock from the web
    #define ENABLE_OTA              0   // Accept over the air flash updates
    #define ENABLE_REMOTE           1   // IR Remote Control
    #define ENABLE_AUDIO            1   // Listen for audio from the microphone and process it

    #define DEFAULT_EFFECT_INTERVAL     (60*60*24*5)

    #define LED_PIN0        26

    #define BONUS_PIXELS            32          // Extra pixels - in this case, my case strip
    #define NUM_CHANNELS            1           // Everything wired sequentially on a single channel
    #define NUM_FANS                10          // My system has 10 fans.  Because RGB.
    #define NUM_BANDS               8
    #define NUM_RINGS               1           // Fans have a single outer ring of pixels
    #define FAN_SIZE                16          // Each fan's pixel ring has 16 LEDs
    #define FAN_LEN                 (NUM_FANS * FAN_SIZE)
    #define MATRIX_WIDTH            (NUM_FANS * FAN_SIZE + BONUS_PIXELS)
    #define NUM_LEDS                (MATRIX_WIDTH)
    #define LED_FAN_OFFSET_BU       3
    #define ENABLE_REMOTE           1           // IR Remote Control
    #define ENABLE_AUDIO            1           // Listen for audio from the microphone and process it
    #define POWER_LIMIT_MW          8000
    #define MATRIX_HEIGHT           1

    // Being case-mounted normally, the FANSET needs a more sensitive mic so the NOISE_CUTOFF value is are lower than spectrum

    #define NOISE_CUTOFF            0
    #define NOISE_FLOOR             0.0f

    #define TOGGLE_BUTTON_1         37
    #define TOGGLE_BUTTON_2         39

    #ifdef SPECTRUM_WROVER_KIT
    #else
        #define NUM_INFO_PAGES          2
        #define ONSCREEN_SPECTRUM_PAGE  1   // Show a little spectrum analyzer on one of the info pages (slower)
    #endif


#elif SINGLE_INSULATOR

    #ifndef PROJECT_NAME
    #define PROJECT_NAME            "Single Insulator"
    #endif

    // A single glass insulator with a 12-pixel ring and then a 7=pixel "bonus" ring in the middle
    #define ENABLE_WIFI             0   // Connect to WiFi
    #define INCOMING_WIFI_ENABLED   0   // Accepting incoming color data and commands
    #define WAIT_FOR_WIFI           0   // Hold in setup until we have WiFi - for strips without effects
    #define TIME_BEFORE_LOCAL       1   // How many seconds before the lamp times out and shows local content

    #define DEFAULT_EFFECT_INTERVAL     (10*60*24)

    #define NUM_CHANNELS    1
    #define BONUS_PIXELS    7
    #define NUM_FANS        1
    #define FAN_SIZE        12
    #define MATRIX_WIDTH    (NUM_FANS * FAN_SIZE + BONUS_PIXELS)
    #define NUM_LEDS        (MATRIX_WIDTH*MATRIX_HEIGHT)
    #define MATRIX_HEIGHT   NUM_FANS

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

    #ifndef PROJECT_NAME
    #define PROJECT_NAME            "Insulators"
    #endif

    #define ENABLE_WIFI             0   // Connect to WiFi
    #define INCOMING_WIFI_ENABLED   0   // Accepting incoming color data and commands
    #define WAIT_FOR_WIFI           0   // Hold in setup until we have WiFi - for strips without effects
    #define TIME_BEFORE_LOCAL       0   // How many seconds before the lamp times out and shows local content

    #define DEFAULT_EFFECT_INTERVAL     (0)

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
    #define ENABLE_REMOTE     0                     // IR Remote Control
    #define ENABLE_AUDIO      1                     // Listen for audio from the microphone and process it
    #define IR_REMOTE_PIN     26
    #define LED_FAN_OFFSET_BU 6
    #define POWER_LIMIT_MW    50000

    #define TOGGLE_BUTTON_1 37
    #define TOGGLE_BUTTON_2 39

    #define NUM_INFO_PAGES          2
    #define ONSCREEN_SPECTRUM_PAGE  1   // Show a little spectrum analyzer on one of the info pages (slower)



#elif CUBE

    // A cube of 5 x 5 x 5 LEDs

    #ifndef PROJECT_NAME
    #define PROJECT_NAME            "Cube"
    #endif

    #define ENABLE_WIFI             1   // Connect to WiFi
    #define INCOMING_WIFI_ENABLED   1   // Accepting incoming color data and commands
    #define WAIT_FOR_WIFI           0   // Hold in setup until we have WiFi - for strips without effects
    #define TIME_BEFORE_LOCAL       5   // How many seconds before the cube times out and shows local content
    #define ENABLE_WEBSERVER        1   // Enable the webserver to control the effects

    #define DEFAULT_EFFECT_INTERVAL     (1000 * 60 * 10)    // 10 min

    #define LED_PIN0          26
    #define NUM_CHANNELS      1
    #define RING_SIZE_0       25                    // Treat each layer as one ring
    #define BONUS_PIXELS      0
    #define MATRIX_WIDTH      5                     // 5 layers
    #define MATRIX_HEIGHT     RING_SIZE_0
    #define NUM_FANS          MATRIX_WIDTH
    #define FAN_SIZE          MATRIX_HEIGHT
    #define NUM_BANDS         16
    #define NUM_LEDS          (MATRIX_WIDTH*MATRIX_HEIGHT)
    #define ENABLE_REMOTE     0                     // IR Remote Control
    #define ENABLE_AUDIO      1                     // Listen for audio from the microphone and process it
    #define IR_REMOTE_PIN     26
    #define LED_FAN_OFFSET_BU 6
    #define POWER_LIMIT_MW    5000
    #define ENABLE_OTA        0

    #define TOGGLE_BUTTON  37
    #define NUM_INFO_PAGES 1

    #define COLOR_ORDER EOrder::RGB

#else

    // This is a simple demo configuration used when no other project is defined; it's only purpose is
    // to serve as a build to be run for [all-deps]

    #define MATRIX_WIDTH            144
    #define MATRIX_HEIGHT           8
    #define NUM_LEDS                (MATRIX_WIDTH*MATRIX_HEIGHT)
    #define NUM_CHANNELS            8
    #define NUM_RINGS               5
    #define RING_SIZE_0             24
    #define POWER_LIMIT_MW          3 * 1000   // 3 watt power supply

    // Once you have a working project, selectively enable various additional features by setting
    // them to 1 in the list below.  This DEMO config assumes no audio (mic), or screen, etc.

    #define ENABLE_AUDIO            1
    #define ENABLE_WIFI             1   // Connect to WiFi
    #define INCOMING_WIFI_ENABLED   1   // Accepting incoming color data and commands
    #define TIME_BEFORE_LOCAL       1   // How many seconds before the lamp times out and shows local content
    #define ENABLE_NTP              1   // Set the clock from the web
    #define ENABLE_OTA              1
    #define ENABLE_WEBSERVER        1   // Turn on the internal webserver

    #define LED_PIN0         5
    #define LED_PIN1        16
    #define LED_PIN2        17
    #define LED_PIN3        18
    #define LED_PIN4        32
    #define LED_PIN5        33
    #define LED_PIN6        23
    #define LED_PIN7        22
#endif

#ifndef PROJECT_NAME
#define PROJECT_NAME        "NightDriver"
#endif

#if USE_MATRIX
#include "MatrixHardware_ESP32_Custom.h"
#define SM_INTERNAL     // Silence build messages from their header
#include <SmartMatrix.h>
#endif

#if ENABLE_AUDIOSERIAL
#ifndef SERIAL_PINRX
    #define SERIAL_PINRX    33
    #define SERIAL_PINTX    32
#endif
#endif

#define STACK_SIZE (ESP_TASK_MAIN_STACK) // Stack size for each new thread
#define TIME_CHECK_INTERVAL_MS (1000 * 60 * 5)   // How often in ms we resync the clock from NTP
#define MIN_BRIGHTNESS  10
#define MAX_BRIGHTNESS  255
#define BRIGHTNESS_STEP 20          // Amount to step brightness on each remote control repeat
#define MAX_RINGS       5


// Default Settings
//
// Set rest of things to reasonable defaults that were not specified by the project config above.

#ifndef MIN_BUFFERS
#define MIN_BUFFERS 3               // How many copies of LED buffers this board will keep at a minimum per strand
#endif

#ifndef MAX_BUFFERS
#define MAX_BUFFERS (500)           // Just some reasonable guess, limiting it to 24 frames per second for 20 seconds
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

#ifndef NUM_LEDS
#define NUM_LEDS (MATRIX_HEIGHT * MATRIX_WIDTH)
#endif

#ifndef FAN_SIZE                // How man LEDs around the circumference
#define FAN_SIZE 1
#define NUM_FANS NUM_LEDS
#endif

#if ENABLE_AUDIO
    #ifndef NUM_BANDS              // How many bands in the spectrum analyzer
        #define NUM_BANDS 16
    #endif
    #ifndef NOISE_FLOOR
        #define NOISE_FLOOR 10000.0
    #endif
    #ifndef NOISE_CUTOFF
        #define NOISE_CUTOFF   2000
    #endif
    #ifndef AUDIO_PEAK_REMOTE_TIMEOUT
        #define AUDIO_PEAK_REMOTE_TIMEOUT 1000.0f       // How long after remote PeakData before local microphone is used again
    #endif
    #ifndef ENABLE_AUDIO_SMOOTHING
        #define ENABLE_AUDIO_SMOOTHING 1
    #endif
#endif

#ifndef LED_PIN0                // Which pin the LEDs are connected to
#define LED_PIN0 5
#endif

#ifndef NUM_RINGS               // How many rings in each tree/insulator/etc
#define NUM_RINGS 1
#endif

#ifndef NUM_INFO_PAGES
#define NUM_INFO_PAGES 1
#endif

#ifndef COLOR_ORDER
#define COLOR_ORDER EOrder::GRB
#endif

// Define fan ordering for drawing into the fan directionally

#ifndef LED_FAN_OFFSET_LR
#define LED_FAN_OFFSET_LR  (LED_FAN_OFFSET_BU + (FAN_SIZE * 1 / 4))         // High level stuff right here!
#endif

#ifndef LED_FAN_OFFSET_TD
#define LED_FAN_OFFSET_TD  (LED_FAN_OFFSET_BU + (FAN_SIZE * 2 / 4))
#endif

#ifndef LED_FAN_OFFSET_RL
#define LED_FAN_OFFSET_RL  (LED_FAN_OFFSET_BU + (FAN_SIZE * 3 / 4))
#endif

#ifndef NUM_FANS
#define NUM_FANS 1
#endif

#ifndef RING_SIZE_0
#define RING_SIZE_0 FAN_SIZE
#endif

#ifndef RING_SIZE_1
#define RING_SIZE_1 FAN_SIZE
#endif

#ifndef RING_SIZE_2
#define RING_SIZE_2 FAN_SIZE
#endif

#ifndef RING_SIZE_3
#define RING_SIZE_3 FAN_SIZE
#endif

#ifndef RING_SIZE_4
#define RING_SIZE_4 FAN_SIZE
#endif

#ifndef DEFAULT_EFFECT_INTERVAL
#define DEFAULT_EFFECT_INTERVAL 1000*30
#endif

#ifndef MILLIS_PER_FRAME
#define MILLIS_PER_FRAME 0
#endif

#ifndef LED_FAN_OFFSET_BU
#define LED_FAN_OFFSET_BU 0
#endif

#ifndef RESERVE_MEMORY
  #ifdef USE_PSRAM
    #define RESERVE_MEMORY 1000000
  #else
    #define RESERVE_MEMORY 180000
  #endif
#endif

#ifndef TIME_BEFORE_LOCAL
#define TIME_BEFORE_LOCAL 5
#endif

#ifndef ENABLE_REMOTE
#define ENABLE_REMOTE 0
#endif

#ifndef MATRIX_REFRESH_RATE
#define MATRIX_REFRESH_RATE 120
#endif

#ifndef MATRIX_CALC_DIVIDER
#define MATRIX_CALC_DIVIDER 2
#endif


// Power Limit
//
// The limit, in watts, that the power supply for your project can supply.  If your demands
// exceed this, the code wil try to scale back brightness to hit this.  Don't rely on this
// for safety, obviously, design your hardware to protect against it with a fuse, etc.

#ifndef POWER_LIMIT_MW
#define POWER_LIMIT_MW 500*5                // Define for your power supply, default is a low 2500mA for USB
#endif

// Display
//
// Enable USE_OLED or USE_TFT based on selected board definition
// These board definations are added by platformio

#if USE_SCREEN

    #if ARDUINO_HELTEC_WIFI_KIT_32_V3

        #define USE_OLED 1
        #define USE_SSD1306 1

    #elif ARDUINO_HELTEC_WIFI_KIT_32      
                        // screen definations for heltec_wifi_kit_32 or heltec_wifi_kit_32_v2

        #define USE_OLED 1                                    // Enable the Heltec's monochrome OLED

    #elif M5STICKCPLUS                                        // screen definitions for m5stick-c-plus

        #define USE_M5DISPLAY 1                               // enable the M5's LCD screen

    #elif M5STICKC                                            // screen definitions for m5stick-c (or m5stick-c plus)

        #define USE_M5DISPLAY 1                               // enable the M5's LCD screen

    #elif M5STACKCORE2                                        // screen definitions for m5stick-c (or m5stick-c plus)

        #define USE_M5DISPLAY 1                               // enable the M5's LCD screen

    #elif ESP32FEATHERTFT || PANLEE || LILYGOTDISPLAYS3

        #define USE_TFTSPI 1                                  // Use TFT_eSPI

    #elif WROVERKIT || SPECTRUM_WROVER_KIT

        #define USE_LCD 1                                      // Use the ILI9341 onboard

    #elif TTGO

        #define USE_TFTSPI 1                                  // Use TFT_eSPI

    #else                                                     // unsupported board defined in platformio
        #error Unknown Display! Check platformio.ini board definition.
    #endif

#endif // end USE_SCREEN

#if USE_LCD
    // These pins are based on the Espressif WROVER-KIT, which uses an ILI9314 chipset for its display
    // connected as follows:
    #define TFT_CS      22
    #define TFT_DC      21
    #define TFT_MOSI    23
    #define TFT_SCK     19
    #define TFT_RST     18
    #define TFT_MISO    25
    #define TFT_WIDTH   240
    #define TFT_HEIGHT  320
#endif

#ifdef ESP32FEATHERTFT
    #define ONBOARD_PIXEL_ORDER     EOrder::GRB
    #define ONBOARD_PIXEL_POWER     34
    #define ONBOARD_PIXEL_DATA      33
#endif

#ifndef USE_OLED
#define USE_OLED 0
#endif

#ifndef USE_M5DISPLAY
#define USE_M5DISPLAY 0
#endif

#ifndef USE_LCD
#define USE_LCD 0
#endif

#ifndef USE_TFTSPI
#define USE_TFTSPI 0
#endif

#ifndef CAPTION_TIME
#define CAPTION_TIME 3000
#endif

#ifndef MATRIX_CENTER_X
#define MATRIX_CENTER_X ((MATRIX_WIDTH + 1) / 2)
#endif

#ifndef MATRIX_CENTER_Y
#define MATRIX_CENTER_Y ((MATRIX_HEIGHT + 1) / 2)
#endif

// Common globals

extern DRAM_ATTR uint32_t g_FPS;               // Our global framerate (BUGBUG: davepl - why are some DRAM?)

// g_aRingSizeTable
//
// Items with rings must provide a table indicating how big each ring is.  If an insulator had 60 LEDs grouped
// into rings of 30, 20, and 10, you'd have (NUM_RINGS = 3) and this table would contain (30, 20, 10).

extern DRAM_ATTR const int g_aRingSizeTable[];

#define MICROS_PER_SECOND   1000000
#define MILLIS_PER_SECOND   1000
#define MICROS_PER_MILLI    1000

#ifndef M5STICKC
#define M5STICKC 0
#endif

#ifndef M5STICKCPLUS
#define M5STICKCPLUS 0
#endif

#ifndef M5STACKCORE2
#define M5STACKCORE2 0
#endif

#ifndef COLORDATA_SERVER_ENABLED
  #if ENABLE_WIFI
    #define COLORDATA_SERVER_ENABLED 1
  #else
    #define COLORDATA_SERVER_ENABLED 0
  #endif
#endif

// Microphone
//
// The M5 mic is on Pin34, but when I wire up my own microphone module I usually put it on pin 36.

#if ENABLE_AUDIO
    #ifndef INPUT_PIN
        #if TTGO
            #define INPUT_PIN (36)
        #elif M5STACKCORE2
            #define INPUT_PIN (0)
            #define IO_PIN    (0)
        #elif M5STICKC || M5STICKCPLUS
            #define INPUT_PIN (34)
            #define IO_PIN (0)
        #else
            #define INPUT_PIN (36)    // Audio line input, ADC #1, input line 0 (GPIO pin 36)
        #endif
    #endif
#else
    #define INPUT_PIN 0
#endif

#ifndef IR_REMOTE_PIN
#define IR_REMOTE_PIN   25
#endif


// Custom WiFi Commands
//
// A Wifi packet can contain color data or potentially other info, like a clock.  Or it could be
// a stats request.  Beyond color data these are poorly tested and likely can be removed, though
// stats and clock are handy for debugging!

#define WIFI_COMMAND_PIXELDATA64 3             // Wifi command with color data and 64-bit clock vals
#define WIFI_COMMAND_PEAKDATA    4             // Wifi command that delivers audio peaks

// Final headers
//
// Headers that are only included when certain features are enabled


// FPS
//
// Given a time value for when the last frame took place and the current timestamp returns the number of
// frames per second, as low as 0.  Never exceeds 999 so you can make some width assumptions.

inline int FPS(uint32_t start, uint32_t end, uint32_t perSecond = MILLIS_PER_SECOND)
{
    uint32_t duration = end - start;
    if (duration == 0)
        return 999;

    float fpsf = 1.0f / (duration / (float) perSecond);
    int FPS = (int)fpsf;
    if (FPS > 999)
        FPS = 999;
    return FPS;
}

// Custom Allocator for allocate_unique, which is like make_unique but can use PSRAM

template<typename Alloc>
struct alloc_deleter
{
  alloc_deleter(const Alloc& a) : a(a) { }

  typedef typename std::allocator_traits<Alloc>::pointer pointer;

  void operator()(pointer p) const
  {
    Alloc aa(a);
    std::allocator_traits<Alloc>::destroy(aa, std::addressof(*p));
    std::allocator_traits<Alloc>::deallocate(aa, p, 1);
  }

private:
  Alloc a;
};

template<typename T, typename Alloc, typename... Args>
auto
allocate_unique(const Alloc& alloc, Args&&... args)
{
  using AT = std::allocator_traits<Alloc>;
  static_assert(std::is_same<typename AT::value_type, std::remove_cv_t<T>>{}(),
                "Allocator has the wrong value_type");

  Alloc a(alloc);
  auto p = AT::allocate(a, 1);
  try {
    AT::construct(a, std::addressof(*p), std::forward<Args>(args)...);
    using D = alloc_deleter<Alloc>;
    return std::unique_ptr<T, D>(p, D(a));
  }
  catch (...)
  {
    AT::deallocate(a, p, 1);
    throw;
  }
}

// PreferPSRAMAlloc
//
// Will return PSRAM if it's available, regular ram otherwise

inline void * PreferPSRAMAlloc(size_t s)
{
    if (psramInit())
    {
        debugV("PSRAM Array Request for %u bytes\n", s);
        return ps_malloc(s);
    }
    else
    {
        return malloc(s);
    }
}

// psram_allocator
//
// A C++ allocator that allocates from PSRAM instead of the regular heap. Initially
// I had just overloaded new for the classes I wanted in PSRAM, but that doesn't work
// with make_shared<> so I had to provide this allocator instead.
//
// When enabled, this puts all of the LEDBuffers in PSRAM.  The table that keeps track
// of them is still in base ram.
//
// (Davepl - I opted to make this *prefer* psram but return regular ram otherwise. It
//           avoids a lot of ifdef USE_PSRAM in the code.  But I've only proved it
//           correct, not tried it on a chip without yet.

template <typename T>
class psram_allocator
{
public:
    typedef size_t size_type;
    typedef ptrdiff_t difference_type;
    typedef T* pointer;
    typedef const T* const_pointer;
    typedef T& reference;
    typedef const T& const_reference;
    typedef T value_type;

    psram_allocator(){}
    ~psram_allocator(){}

    template <class U> struct rebind { typedef psram_allocator<U> other; };
    template <class U> psram_allocator(const psram_allocator<U>&){}

    pointer address(reference x) const {return &x;}
    const_pointer address(const_reference x) const {return &x;}
    size_type max_size() const throw() {return size_t(-1) / sizeof(value_type);}

    pointer allocate(size_type n, const void * hint = 0)
    {
        void * pmem = PreferPSRAMAlloc(n*sizeof(T));
        return static_cast<pointer>(pmem) ;
    }

    void deallocate(pointer p, size_type n)
    {
        free(p);
    }

    template< class U, class... Args >
    void construct( U* p, Args&&... args )
    {
        ::new((void *) p ) U(std::forward<Args>(args)...);
    }

    void destroy(pointer p)
    {
        p->~T();
    }
};

// str_snprintf
//
// va-args style printf that returns the formatted string as a reuslt

inline String str_sprintf(const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);

    va_list args_copy;
    va_copy(args_copy, args);

    int requiredLen = vsnprintf(nullptr, 0, fmt, args) + 1;
    va_end(args);

    if (requiredLen <= 0) {
        va_end(args_copy);
        return {};
    }

    std::unique_ptr<char []> str = std::make_unique<char []>(requiredLen);
    vsnprintf(str.get(), requiredLen, fmt, args_copy);
    va_end(args_copy);

    return String(str.get());
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

        if (_deltaTime > 1.0)
            _deltaTime = 1.0;

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
        return (double)tv.tv_sec + (tv.tv_usec/(double)MICROS_PER_SECOND);
    }

    double FrameElapsedTime() const
    {
        return FrameStartTime() - CurrentTime();
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

    double LastFrameTime() const
    {
        return _deltaTime;
    }
};

// C Helpers
//
// Simple inline utility functions like random numbers, mapping, conversion, etc

inline static float randomfloat(float lower, float upper)
{
    float result = (lower + ((upper - lower) * rand()) / RAND_MAX);
    return result;
}

inline static double randomdouble(double lower, double upper)
{
    double result = (lower + ((upper - lower) * rand()) / (double)RAND_MAX);
    return result;
}

template<typename T> inline float map(T x, T in_min, T in_max, T out_min, T out_max)
{
    return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

inline uint64_t ULONGFromMemory(uint8_t * payloadData)
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

inline uint32_t DWORDFromMemory(uint8_t * payloadData)
{
    return  (uint32_t)payloadData[3] << 24  |
            (uint32_t)payloadData[2] << 16  |
            (uint32_t)payloadData[1] << 8   |
            (uint32_t)payloadData[0];
}

inline uint16_t WORDFromMemory(uint8_t * payloadData)
{
    return  (uint16_t)payloadData[1] << 8   |
            (uint16_t)payloadData[0];
}

// SetSocketBlockingEnabled
//
// In blocking mode, socket API calls wait until the operation is complete before returning control to the application.
// For example, a call to the send() function won't return until all data has been sent. This can lead to the application
// hanging if the operation takes a long time.

// In contrast, in non-blocking mode, socket API calls return immediately. If an operation cannot be completed immediately, the function returns an error (usually EWOULDBLOCK or EAGAIN). The application can then decide how to handle the situation, such as by retrying the operation later. This provides more control and can make the application more responsive. However, it also requires more sophisticated programming, as the application must be prepared to handle these error conditions.

inline bool SetSocketBlockingEnabled(int fd, bool blocking)
{
   if (fd < 0) return false;

   int flags = fcntl(fd, F_GETFL, 0);
   if (flags == -1) return false;
   flags = blocking ? (flags & ~O_NONBLOCK) : (flags | O_NONBLOCK);
   return (fcntl(fd, F_SETFL, flags) == 0) ? true : false;
}

// 16-bit (5:6:5) color definitions for common colors

#define BLACK16     0x0000
#define BLUE16      0x001F
#define RED16       0xF800
#define GREEN16     0x07E0
#define CYAN16      0x07FF
#define MAGENTA16   0xF81F
#define YELLOW16    0xFFE0
#define WHITE16     0xFFFF

// Main includes

#include <TJpg_Decoder.h>
#include "improvserial.h"                       // ImprovSerial impl for setting WiFi credentials over the serial port
#include "gfxbase.h"                            // GFXBase drawing interface
#include "screen.h"                             // LCD/TFT/OLED handling
#include "socketserver.h"                       // Incoming WiFi data connections
#include "soundanalyzer.h"                      // for audio sound processing
#include "ledstripgfx.h"                        // Essential drawing code for strips
#include "ledmatrixgfx.h"                       // For drawing to HUB75 matrices
#include "ledstripeffect.h"                     // Defines base led effect classes
#include "ntptimeclient.h"                      // setting the system clock from ntp
#include "effectmanager.h"                      // For g_EffectManager
#include "ledviewer.h"                          // For the LEDViewer task and object
#include "network.h"                            // Networking
#include "ledbuffer.h"                          // Buffer manager for strip
#include "Bounce2.h"                            // For Bounce button class
#include "colordata.h"                          // color palettes
#include "drawing.h"                            // drawing code
#include "taskmgr.h"                            // for cpu usage, etc

#if USE_TFTSPI
    #define DISABLE_ALL_LIBRARY_WARNINGS 1
    #include <TFT_eSPI.h>
    #include <SPI.h>

    extern std::unique_ptr<Screen> g_pDisplay;
#endif

// Conditional includes depending on which project is being build

#if USE_MATRIX
    #include <YouTubeSight.h>                       // For fetching YouTube sub count
    #include "effects/matrix/PatternSubscribers.h"  // For subscriber count effect
#endif

#if USE_SCREEN
    // #include "freefonts.h"
#endif

#if ENABLE_WIFI && ENABLE_WEBSERVER
    #include "webserver.h"
#endif

#if ENABLE_REMOTE
    #include "remotecontrol.h"
#endif

