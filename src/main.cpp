//+--------------------------------------------------------------------------
//
// File:        main.cpp  
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
//    Main setup and loop file for the LED Wifi Project, and main.cpp is
//    the ugliest file because of the conditional compilation, and the
//    external dependencies.
//
//    NightDriver is an LED display project composed of a client app
//    that runs on the ESP32 and an optional server that can run on 
//    a variety of platforms (anywhere .Net CORE works, like the Pi).
//    The app controls WS2812B style LEDs.  The number of LEDs in a
//    row is unbounded but realistically limited to about 1000 which
//    still allows for about 24 frames per second of updates.  There
//    can be 8 such channels connected to 8 different pins.
//    By default NightDriver draws client effects, and there many
//    built in, from marquees to fire.  But it can also receive color
//    data from a server.  So it firsts checks to see if there is 
//    data coming in, and if so, draws that.  If not it falls back
//    to internal drawing.  The server sends a simple packet with
//    an LED count, timestamp, and then the color data for the LEDs.
//    The ESP32 app has buffer of about 30 frames when 1000 LEDs are
//    in use.  The server generates frames 1/2 second in the future and
//    sets the timestamps accordinging.  The client app waits until
//    the next packet in its buffer is due for drawing and then draws
//    it and discards it.  A circular queue is used.
//
//    Both client and server require reliable access to an SNTP server
//    to keep their clocks in sync.  The client sets its time every
//    few hours to combat clock drifts on the ESP32.  Since all the 
//    clients (and the server) have the same clock, they can sync
//    shows across multiple clients.  Imagine a setup where a dozen
//    LED matrixes are arranged to form a small "jumbotron".  This
//    works because the screens would all be in time sync.
//
//    The app listens to the microphone and does an FFT and spectral
//    analysis on a separate thread about 25 times per second.  This
//    gives effects access to the audio data, and there are a number
//    of sound-reactive and beat-driven effects built in.
//
//    In addition to simple trips, the app handles matrixes as well.
//    It also handles groups of rings.  In one incarnation, 10 RGB 
//    LED PC fans are connected in a LianLi case plus the 32 or so
//    on the front of the case.  The fans are grouped into NUM_FANS
//    fans.  It also suports concentrically nested rings of varying
//    size, which I use for a Christmas-tree project where each tree
//    is made up of a "stack" of rings - 32 leds, 18, 10, 4, 1.  
//    It's up to individual effects to take advantage of them but
//    the drawing code provides APIs for "draw to LED x of RING q on
//    FAZN number z" and so on for convenience.
//
//    Support for features such as sound, web, wifi, etc can be
//    selectively disabled via #include settings.  There's an optional
//    web server built in that serves a sample jqueryUI based UI
//    directly from the chip.  You could add a settings page for
//    example.  It has the basics of a REST API we well.
//
//    A telnet server is also built in for easy debugging access.
//
//    An Over-the-air WiFi flash update function is built in, and the
//    lights will turn purple during a flash update.
//
//    MAIN.CPP --------------------------------------------------------
//
//    When the ESP32 chip boots it starts here, in the setup() function.
//    Setup sets pin directions, starts wifi, initalized the LEDs, etc.
//    Then loop() is called repeatedly until the end of time.  The loop
//    code (optionally) receives color data over Wifi.  If it hasn't had
//    any for a bit of time, it falls back to rotating through a table
//    of internal effects.
//    
//    A number of worker threads are created which:  
//
//      1) Draw the TFT and display stats like framerate and IP addr
//      2) Sync the clock periodically
//      3) Run a full web server
//      4) Run a debug monitor accessible via serial and over telnet
//      5) Listen to room audio, create a spectrum, look for beat
//      6) Re-connect to WiFi as needed and other networking tasks
//      7) Run the debug monitor over telnet
//      8) Receive color data on a socket
//
// Most of these features can be individually enabled and disabled via
// conditional includes.
//
// License:
//
// NightDriver is an open source embedded application governed by the terms 
// of the General Public License (GPL). This requires that anyone modifying 
// the NightDriver code (for anything other than personal use) or building 
// applications based on NightDriver code must also make their derived 
// product available under the same open source GPL terms. By purchasing 
// a license for NightDriver, you would not then be bound by the GPL and 
// you would gain an extended feature set and various levels of support.  
// Think commas, not zeros, when discussing product volumes and license 
// pricing.  
//
// Without restricting the author protections found in the GPL, Plummer's 
// Software LLC, its programmers, representatives and agents, etc.,
// specifically disclaim any liability related to safety or suitability 
// for any purpose whatsoever.  Hypothetical example so we all know what
// I mean:  If the code that limits LED power consumption has a horribly
// negligent bug that burns down your village, you have my sympathy but
// not my liability. It's not that I don't care, there's just no world
// where I'd casually release code that I was responsible for in that 
// manner without suitable engineering and testing, none of which this
// code has had.  Not sure?  Turn and flee.  By proceeding, you agree.
//
// License Purchases
//
// NightDriver is an open source embedded application governed by the 
// terms of the General Public License (GPL).  If you follow that license 
// it's yours at the amazing price of 'completely free'.
//
// If, on the other hand, you are building a commercial application for 
// internal use or resale:
//
// Anyone building applications based on NightDriver code, or modifying 
// the NightDriver code, must also make their derived product available 
// under the same open source GPL terms. Commercial licenses may be 
// purchased from Plummer's Software LLC if you do not wish to be bound 
// by the GPL terms. These licenses are valid for a specified term.
// 
// Contact Plummer's Software LLC for volume pricing and support questions.
//
// History:     Jul-12-2018         Davepl      Created
//
//              [See globals.h for project history]
//
//---------------------------------------------------------------------------

#include "globals.h"                            // CONFIG and global headers

// If the Atomi TM1814 lights are being used, we include the NeoPixelBus code
// to drive them, but otherwise we do not use or include NeoPixelBus

#include <ArduinoOTA.h>                         // For updating the flash over WiFi
#include <ESPmDNS.h>

#include "ntptimeclient.h"                      // setting the system clock from ntp
#include "socketserver.h"                       // our socket server for incoming
#if ENABLE_AUDIO
    #include "soundanalyzer.h"                  // for audio sound processing
#endif
#include "network.h"                            // For WiFi credentials
#include "ledbuffer.h"
#include "Bounce2.h"                            // For Bounce button class
#if ENABLE_WEBSERVER
    #include "spiffswebserver.h"                // handle web server requests
#endif

#include "colordata.h"                          // color palettes
#include "drawing.h"                            // drawing code
#include "taskmgr.h"                            // for cpu usage, etc

#if ENABLE_REMOTE
    #include "remotecontrol.h" // Allows us to use a IR remote with it
#endif

void IRAM_ATTR ScreenUpdateLoopEntry(void *);
extern volatile double g_FreeDrawTime;
extern volatile float gPeakVU; // How high our peak VU scale is in live mode
extern volatile float gMinVU;  // How low our peak VU scale is in live mode
extern uint32_t g_Watts;
extern double g_Brite;

//
// Task Handles to our running threads
//

TaskHandle_t g_taskScreen = nullptr;
TaskHandle_t g_taskSync   = nullptr;
TaskHandle_t g_taskWeb    = nullptr;
TaskHandle_t g_taskDraw   = nullptr;
TaskHandle_t g_taskDebug  = nullptr;
TaskHandle_t g_taskAudio  = nullptr;
TaskHandle_t g_taskNet    = nullptr;
TaskHandle_t g_taskRemote = nullptr;
TaskHandle_t g_taskSocket = nullptr;

TaskManager g_TaskManager;

//
// Global Variables
//

#ifdef SPECTRUM
DRAM_ATTR uint8_t giInfoPage = NUM_INFO_PAGES - 1;  // Default to last page
#else
DRAM_ATTR uint8_t giInfoPage = 0;                   // Default to first page
#endif

DRAM_ATTR WiFiUDP g_Udp;                            // UDP object used for NNTP, etc
DRAM_ATTR uint32_t g_FPS = 0;                       // Our global framerate
DRAM_ATTR bool g_bUpdateStarted = false;            // Has an OTA update started?
DRAM_ATTR AppTime g_AppTime;                        // Keeps track of frame times
DRAM_ATTR bool NTPTimeClient::_bClockSet = false;   // Has our clock been set by SNTP?

#ifdef USEMATRIX
    #include "ledmatrixgfx.h"
    #include <YouTubeSight.h>                       // For fetching YouTube sub count
    #include "effects/matrix/PatternSubscribers.h"  // For subscriber count effect
#endif

#ifdef USESTRIP
    #include "ledstripgfx.h"
#endif

extern DRAM_ATTR std::unique_ptr<EffectManager<GFXBase>> g_pEffectManager;       // The one and only global effect manager

DRAM_ATTR std::shared_ptr<GFXBase> g_pDevices[NUM_CHANNELS];
DRAM_ATTR mutex NTPTimeClient::_clockMutex;                                      // Clock guard mutex for SNTP client
DRAM_ATTR RemoteDebug Debug;                                                     // Instance of our telnet debug server

// If an insulator or tree or fan has multiple rings, this table defines how those rings are laid out such
// that they add up to FAN_SIZE pixels total per ring.
// 
// Imagine a setup of 5 christmas trees, where each tree was made up of 4 concentric rings of descreasing
// size, like 16, 12, 8, 4.  You would have NUM_FANS of 5 and MAX_RINGS of 4 and your ring table would be 16, 12, 8 4.

DRAM_ATTR const int gRingSizeTable[MAX_RINGS] = 
{ 
    RING_SIZE_0, 
    RING_SIZE_1, 
    RING_SIZE_2, 
    RING_SIZE_3, 
    RING_SIZE_4 
};

//
// External Variables
//

extern DRAM_ATTR LEDStripEffect * AllEffects[];      // Main table of internal events in effects.cpp
extern DRAM_ATTR std::unique_ptr<LEDBufferManager> g_apBufferManager[NUM_CHANNELS];

//
// Optional Components
//

#if ENABLE_WIFI && ENABLE_WEBSERVER
    DRAM_ATTR CSPIFFSWebServer g_WebServer;
#endif

#if ENABLE_WIFI && INCOMING_WIFI_ENABLED
    DRAM_ATTR SocketServer g_SocketServer(49152, NUM_LEDS);  // $C000 is free RAM on the C64, fwiw!
#endif

#if ENABLE_REMOTE
    DRAM_ATTR RemoteControl g_RemoteControl;
#endif

// DebugLoopTaskEntry
//
// Entry point for the Debug task, pumps the Debug handler

#if ENABLE_WIFI
void IRAM_ATTR DebugLoopTaskEntry(void *)
{    
    debugI(">> DebugLoopTaskEntry\n");

   // Initialize RemoteDebug

    debugV("Starting RemoteDebug server...\n");

    Debug.setResetCmdEnabled(true);                         // Enable the reset command
    Debug.showProfiler(false);                              // Profiler (Good to measure times, to optimize codes)
    Debug.showColors(false);                                // Colors
    Debug.setCallBackProjectCmds(&processRemoteDebugCmd);   // Func called to handle any debug externsions we add

    while (!WiFi.isConnected())                             // Wait for wifi, no point otherwise
        delay(100);

    Debug.begin(cszHostname, RemoteDebug::INFO);            // Initialize the WiFi debug server

    for (;;)                                                // Call Debug.handle() 20 times a second
    {
        EVERY_N_MILLIS(50)
        {
            Debug.handle();
        }
        
        delay(10);        
    }    
}
#endif

// NetworkHandlingLoopEntry
//
// Pumps the various network loops and sets the time periodically, as well as reconnecting
// to WiFi if the connection drops.  Also pumps the OTA (Over the air updates) loop.

// Data for Dave's Garage as an example,

#if USEMATRIX
    const char PatternSubscribers::szChannelID[] = "UCNzszbnvQeFzObW0ghk0Ckw";
    const char PatternSubscribers::szChannelName1[] = "Daves Garage";

    #define SUB_CHECK_INTERVAL 60000
    #define SUB_CHECK_ERROR_INTERVAL 10000
    #define CHANNEL_GUID "9558daa1-eae8-482f-8066-17fa787bc0e4" 

    WiFiClient http;
    YouTubeSight sight(CHANNEL_GUID, http);
#endif

void IRAM_ATTR NetworkHandlingLoopEntry(void *)
{    
    debugI(">> NetworkHandlingLoopEntry\n");

    if(!MDNS.begin("esp32")) {
        Serial.println("Error starting mDNS");
    }
    
    for (;;)
    {
        /* Every few seconds we check WiFi, and reconnect if we've lost the connection.  If we are unable to restart
           it for any reason, we reboot the chip in cases where its required, which we assume from WAIT_FOR_WIFI */

        #if ENABLE_WIFI
            EVERY_N_SECONDS(2)
            {
                if (WiFi.isConnected() == false && ConnectToWiFi(10) == false)
                {
                    debugE("Cannot Connect to Wifi!");
                    #if WAIT_FOR_WIFI
                        debugE("Rebooting in 5 seconds due to no Wifi available.");
                        delay(5000);
                        throw new std::runtime_error("Rebooting due to no Wifi available.");
                    #endif
                }
            }
            EVERY_N_SECONDS(60)
            {
                // Get Subscriber Count

                if (WiFi.isConnected())
                {
                    #if USEMATRIX
                    static uint64_t     _NextRunTime = millis();
                    if (millis() > _NextRunTime)
                    {
                        debugV("Fetching YouTube Data...");

                        sight._debug = false;
                        if (sight.getData())
                        {
                            debugV("Got YouTube Data...");
                            long result = atol(sight.channelStats.subscribers_count.c_str());
                            PatternSubscribers::cSubscribers = result;
                            _NextRunTime = millis() + SUB_CHECK_INTERVAL;
                            PatternSubscribers::cViews = atol(sight.channelStats.views.c_str());
                        }
                        else
                        {
                            debugW("YouTubeSight Subscriber API failed\n");
                            _NextRunTime = millis() + SUB_CHECK_ERROR_INTERVAL;
                        }
                    }
                    #endif
                }                
            }
        #endif


        #if ENABLE_WIFI && ENABLE_NTP
            EVERY_N_MILLIS(TIME_CHECK_INTERVAL_MS)
            {
                if (WiFi.isConnected())
                {
                    debugV("Refreshing Time from Server...");
                    digitalWrite(BUILTIN_LED_PIN, 1);
                    NTPTimeClient::UpdateClockFromWeb(&g_Udp);
                    digitalWrite(BUILTIN_LED_PIN, 0);
                }
            }
        #endif            
        delay(10);
    }
}

// SocketServerTaskEntry
//
// Repeatedly calls the code to open up a socket and receive new connections

#if ENABLE_WIFI && INCOMING_WIFI_ENABLED
    void IRAM_ATTR SocketServerTaskEntry(void *)
    {
        for (;;)
        {
            if (WiFi.isConnected())
            {
                g_SocketServer.ProcessIncomingConnectionsLoop();
                debugW("Socket connection closed.  Retrying...\n");
            }
            delay(500);
        }
    }
#endif


// CheckHeap
//
// Quick and dirty debug test to make sure the heap has not been corrupted

inline void CheckHeap()
{
    if (false == heap_caps_check_integrity_all(true))
    {
        throw runtime_error("Heap FAILED checks!");
    }
}

// PrintOutputHeader
//
// Displays an info header with info about the system

void PrintOutputHeader()
{
    debugI("NightDriverStrip\n");
    debugI("-------------------------------------------------------------------------------------");
    debugI("M5STICKC: %d, USE_M5DISPLAY: %d, USE_OLED: %d, USE_TFTSPI: %d, USE_LCD: %d", M5STICKC, USE_M5DISPLAY, USE_OLED, USE_TFTSPI, USE_LCD);

    #if USE_PSRAM
        debugI("ESP32 PSRAM Init: %s", psramInit() ? "OK" : "FAIL");
    #endif

    debugI("Version %u: Wifi SSID: %s - ESP32 Free Memory: %u, PSRAM:%u, PSRAM Free: %u", 
            FLASH_VERSION, cszSSID, ESP.getFreeHeap(), ESP.getPsramSize(), ESP.getFreePsram());
    debugI("ESP32 Clock Freq : %d MHz", ESP.getCpuFreqMHz());
}

// TerminateHandler
//
// Set as the handler for unhandled exceptions, prints some debug info and reboots the chip!

void TerminateHandler()
{
    debugI("-------------------------------------------------------------------------------------");
    debugI("- NightDriverStrip Guru Meditation                              Unhandled Exception -");
    debugI("-------------------------------------------------------------------------------------");
    PrintOutputHeader();

    std::exception_ptr exptr = std::current_exception();
    try {
        std::rethrow_exception(exptr);
    }
    catch (std::exception &ex) {
        debugI("Terminated due to exception: %s", ex.what());
    }

    abort();
}

#ifdef TOGGLE_BUTTON_1
Bounce2::Button Button1;
#endif

#ifdef TOGGLE_BUTTON_2
Bounce2::Button Button2;
#endif

// setup
//
// Invoked once at boot, does initial chip setup and application initial init, then spins off worker tasks and returns
// control to the system so it can invoke the main loop() function.
// 
// Threads (tasks) created here can include:
//
// DebugLoopTaskEntry           - Run a little debug console accessible via telnet and serial
// ScreenUpdateLoopEntry        - Displays stats on the attached OLED/TFT screen about drawing, network, etc
// DrawLoopTaskEntry            - Handles drawing from local or wifi data
// RemoteLoop                   - Handles the remote control loop
// NetworkHandlingLoopEntry     - Connects to WiFi, handles reconnects, OTA updates, web server
// SocketServerTaskEntry        - Creates the socket and listens for incoming wifi color data
// AudioSamplerTaskEntry        - Listens to room audio, creates spectrum analysis, beat detection, etc.

void setup()
{
    g_TaskManager.begin();
    
    // Initialize Serial output
    Serial.begin(115200);      

    esp_log_level_set("*", ESP_LOG_WARN);        // set all components to ERROR level  

    // Set the unhandled exception handler to be our own special exit function                 
    std::set_terminate(TerminateHandler);

    // Re-route debug output to the serial port
    Debug.setSerialEnabled(true);

    // Display a simple statup header on the serial port
    PrintOutputHeader();
    debugI("Startup!");

    // Start Debug

    #if ENABLE_WIFI
        debugI("Starting DebugLoopTaskEntry");
        xTaskCreatePinnedToCore(DebugLoopTaskEntry, "Debug Loop", STACK_SIZE, nullptr, DEBUG_PRIORITY, &g_taskDebug, DEBUG_CORE);
        CheckHeap();
    #endif

    delay(100);

    // If we have a remote control enabled, set the direction on its input pin accordingly

    #if ENABLE_REMOTE
        pinMode(IR_REMOTE_PIN, INPUT);             
    #endif

    #if ENABLE_AUDIO
        pinMode(INPUT_PIN, INPUT);
        #if TTGO                                    
            pinMode(37, OUTPUT);            // This pin layout allows for mounting a MAX4466 to the backside
            digitalWrite(37, LOW);          //   of a TTGO with the OUT pin on 36, GND on 37, and Vcc on 38
            pinMode(38, OUTPUT);
            digitalWrite(38, HIGH);
        #endif
    #endif

    #ifdef TOGGLE_BUTTON_1
        Button1.attach(TOGGLE_BUTTON_1, INPUT_PULLUP);
        Button1.interval(1);
        Button1.setPressedState(LOW);
    #endif

    #ifdef TOGGLE_BUTTON_2
        Button2.attach(TOGGLE_BUTTON_2, INPUT_PULLUP);
        Button2.interval(1);
        Button2.setPressedState(LOW);
    #endif

    // Init the U8G2 compatible SSD1306, 128X64 OLED display on the Heltec board

#if USE_OLED
    debugI("Intializizing OLED display");
    g_pDisplay->begin();
    debugI("Initializing OLED display");
#endif

#if USE_TFTSPI
    debugI("Initializing TFTSPI");
    extern TFT_eSPI * g_pDisplay;

    pinMode(TFT_BL, OUTPUT);
    digitalWrite(TFT_BL, 128);

    g_pDisplay->init();
    g_pDisplay->setRotation(3);
    g_pDisplay->fillScreen(TFT_GREEN);
#endif

#if M5STICKC || M5STICKCPLUS
    #if USE_M5DISPLAY
        debugI("Intializizing TFT display\n");
        extern M5Display * g_pDisplay;
        M5.begin();
        g_pDisplay = &M5.Lcd;
        g_pDisplay->setRotation(1);
        g_pDisplay->setTextDatum(C_BASELINE);
        g_pDisplay->printf("NightDriver: " FLASH_VERSION_NAME);
    #else
        debugI("Initializing M5 withOUT display");
        M5.begin(false);
    #endif
#endif

#if USE_LCD
    extern Adafruit_ILI9341 * g_pDisplay;
    debugI("Initializing LCD display\n");

    // Without these two magic lines, you get no picture, which is pretty annoying...
    
    #ifndef TFT_BL
      #define TFT_BL 5 // LED back-light
    #endif
    pinMode(TFT_BL, OUTPUT); //initialize BL

    // We need-want hardware SPI, but the default constructor that lets us specify the pins we need
    // forces software SPI, so we need to use the constructor that explicitly lets us use hardware SPI.

    SPIClass * hspi = new SPIClass(HSPI);
    hspi->begin(TFT_SCK, TFT_MISO, TFT_MOSI, -1);
    g_pDisplay = new Adafruit_ILI9341(hspi, TFT_DC, TFT_CS, TFT_RST);
    g_pDisplay->begin();
    g_pDisplay->fillScreen(BLUE16);
    g_pDisplay->setRotation(1);

    uint8_t x = g_pDisplay->readcommand8(ILI9341_RDMODE);
    debugI("Display Power Mode: %x", x); 
    x = g_pDisplay->readcommand8(ILI9341_RDMADCTL);
    debugI("MADCTL Mode: %x", x); 
    x = g_pDisplay->readcommand8(ILI9341_RDPIXFMT);
    debugI("Pixel Format: %x", x); 
    x = g_pDisplay->readcommand8(ILI9341_RDIMGFMT);
    debugI("Image Format: %x", x); 
    x = g_pDisplay->readcommand8(ILI9341_RDSELFDIAG);
    debugI("Self Diagnostic: %x", x); 

#endif

#if ENABLE_WEBSERVER                                                    
    debugI("Starting SPIFFS...");
    if (!SPIFFS.begin(true))
    {
        debugI("ERROR: SPIFFS Mount Failed");
    }
    else
    {
        debugI("SPIFFS OK!");
    }
#endif

    // Initialize the strand controllers depending on how many channels we have

    #ifdef USESTRIP
        for (int i = 0; i < NUM_CHANNELS; i++)
            g_pDevices[i] = make_unique<LEDStripGFX>(MATRIX_WIDTH, MATRIX_HEIGHT);
    #endif

    #if USEMATRIX
        for (int i = 0; i < NUM_CHANNELS; i++)
        {
            g_pDevices[i] = make_unique<LEDMatrixGFX>(MATRIX_WIDTH, MATRIX_HEIGHT);
            g_pDevices[i]->loadPalette(0);
        }
    #endif

    #if USE_PSRAM
        uint32_t memtouse = ESP.getFreePsram() - RESERVE_MEMORY;
    #else
        uint32_t memtouse = ESP.getFreeHeap() - RESERVE_MEMORY;
    #endif

    uint32_t memtoalloc = (NUM_CHANNELS * ((sizeof(LEDBuffer) + NUM_LEDS * sizeof(CRGB))));
    uint32_t cBuffers = memtouse / memtoalloc;

    if (cBuffers < MIN_BUFFERS)
    {
        debugI("Not enough memory, could only allocate %d buffers and need %d\n", cBuffers, MIN_BUFFERS);
        throw std::runtime_error("Could not allocate all buffers");
    }
    if (cBuffers > MAX_BUFFERS)
    {
        debugI("Could allocate %d buffers but limiting it to %d\n", cBuffers, MAX_BUFFERS);
        cBuffers = MAX_BUFFERS;
    }

    debugW("Reserving %d LED buffers for a total of %d bytes...", cBuffers, memtoalloc * cBuffers);

    for (int iChannel = 0; iChannel < NUM_CHANNELS; iChannel++)
        g_apBufferManager[iChannel] = make_unique<LEDBufferManager>(cBuffers, g_pDevices[iChannel]);

    // Initialize all of the built in effects

    debugI("Adding LEDs to FastLED...");
    CheckHeap();

    // Due to the nature of how FastLED compiles, the LED_PINx must be passed as a literal, not a variable (template stuff)

    #if USEMATRIX
        LEDMatrixGFX::StartMatrix();
    #endif

        // Onboard PWM LED 

    #ifdef ONBOARD_LED_R
        ledcAttachPin(ONBOARD_LED_R,  1);   // assign RGB led pins to PWM channels
        ledcAttachPin(ONBOARD_LED_G,  2);
        ledcAttachPin(ONBOARD_LED_B,  3);
        ledcSetup(1, 12000, 8);             // 12 kHz PWM, 8-bit resolution
        ledcSetup(2, 12000, 8);
        ledcSetup(3, 12000, 8);
    #endif

    #if USESTRIP

        #if NUM_CHANNELS == 1
            debugI("Adding %d LEDs to FastLED.", g_pDevices[0]->GetLEDCount());
            
            FastLED.addLeds<WS2812B, LED_PIN0, COLOR_ORDER>(((LEDStripGFX *)g_pDevices[0].get())->leds, 3*144);
            FastLED[0].setLeds(((LEDStripGFX *)g_pDevices[0].get())->leds, g_pDevices[0]->GetLEDCount());   
            FastLED.setMaxRefreshRate(0, false);  // turn OFF the refresh rate constraint
            pinMode(LED_PIN0, OUTPUT);
        #endif

        #if NUM_CHANNELS >= 2
            FastLED.addLeds<WS2812B, LED_PIN0, COLOR_ORDER>(((LEDStripGFX *)g_pDevices[0].get())->leds, ((LEDStripGFX *)g_pDevices[0].get())->GetLEDCount());
            pinMode(LED_PIN0, OUTPUT);

            FastLED.addLeds<WS2812B, LED_PIN1, COLOR_ORDER>(g_pDevices[1]->leds, g_pDevices[1]->GetLEDCount());
            pinMode(LED_PIN1, OUTPUT);
        #endif

        #if NUM_CHANNELS >= 3
            FastLED.addLeds<WS2812B, LED_PIN2, COLOR_ORDER>(g_pDevices[2]->leds, g_pDevices[2]->GetLEDCount());
            pinMode(LED_PIN2, OUTPUT);
        #endif

        #if NUM_CHANNELS >= 4
            FastLED.addLeds<WS2812B, LED_PIN3, COLOR_ORDER>(g_pDevices[3]->leds, g_pDevices[3]->GetLEDCount());
            pinMode(LED_PIN3, OUTPUT);
        #endif

        #if NUM_CHANNELS >= 5
            FastLED.addLeds<WS2812B, LED_PIN4, COLOR_ORDER>(g_pDevices[4]->leds, g_pDevices[4]->GetLEDCount());
            pinMode(LED_PIN4, OUTPUT);
        #endif

        #if NUM_CHANNELS >= 6
            FastLED.addLeds<WS2812B, LED_PIN5, COLOR_ORDER>(g_pDevices[5]->leds, g_pDevices[5]->GetLEDCount());
            pinMode(LED_PIN5, OUTPUT);
        #endif

        #if NUM_CHANNELS >= 7
            FastLED.addLeds<WS2812B, LED_PIN6, COLOR_ORDER>(g_pDevices[6]->leds, g_pDevices[6]->GetLEDCount());
            pinMode(LED_PIN6, OUTPUT);
        #endif

        #if NUM_CHANNELS >= 8
            FastLED.addLeds<WS2812B, LED_PIN7, COLOR_ORDER>(g_pDevices[7]->leds, g_pDevices[7]->GetLEDCount());
            pinMode(LED_PIN7, OUTPUT);
        #endif
           
        #ifdef POWER_LIMIT_MW
            set_max_power_in_milliwatts(POWER_LIMIT_MW);                // Set brightness limit
            #ifdef LED_BUILTIN
                set_max_power_indicator_LED(LED_BUILTIN);
            #endif
        #endif

            g_Brightness = 255;
            
        #if ATOMLIGHT
            pinMode(4, INPUT);
            pinMode(12, INPUT);
            pinMode(13, INPUT);
            pinMode(14, INPUT);
            pinMode(15, INPUT);
        #endif
    #endif


    pinMode(BUILTIN_LED_PIN, OUTPUT);

    // Microphone stuff
    #if ENABLE_AUDIO    
        pinMode(INPUT_PIN, INPUT);
    #endif

    debugI("Initializing effects manager...");
    InitEffectsManager();

#if USE_SCREEN
    xTaskCreatePinnedToCore(ScreenUpdateLoopEntry, "Screen Loop", STACK_SIZE, nullptr, SCREEN_PRIORITY, &g_taskScreen, SCREEN_CORE);
#endif

    debugI("Launching Drawing:");
    debugE("Heap before launch: %s", heap_caps_check_integrity_all(true) ? "PASS" : "FAIL");
    xTaskCreatePinnedToCore(DrawLoopTaskEntry, "Draw Loop", STACK_SIZE, nullptr, DRAWING_PRIORITY, &g_taskDraw, DRAWING_CORE);
    CheckHeap();

#if ENABLE_WIFI && WAIT_FOR_WIFI
    debugI("Calling ConnectToWifi()\n");
    if (false == ConnectToWiFi(99))
    {
        debugI("Unable to connect to WiFi, but must have it, so rebooting...\n");
        throw runtime_error("Unable to connect to WiFi, but must have it, so rebooting");
    }
    Debug.setSerialEnabled(true);
#endif

    // Init the zlib compression

    debugV("Initializing compression...");
    uzlib_init();
    CheckHeap();

#if ENABLE_REMOTE
    // Start Remote Control
    xTaskCreatePinnedToCore(RemoteLoopEntry, "IR Remote Loop", STACK_SIZE, nullptr, REMOTE_PRIORITY, &g_taskRemote, REMOTE_CORE);
    CheckHeap();
#endif

    xTaskCreatePinnedToCore(NetworkHandlingLoopEntry, "NetworkHandlingLoop", STACK_SIZE, nullptr, NET_PRIORITY, &g_taskSync, NET_CORE);
    CheckHeap();

#if ENABLE_WIFI && INCOMING_WIFI_ENABLED
    xTaskCreatePinnedToCore(SocketServerTaskEntry, "Socket Server Loop", STACK_SIZE, nullptr, SOCKET_PRIORITY, &g_taskSocket, SOCKET_CORE);
#endif

#if ENABLE_AUDIO
    // The audio sampler task might as well be on a different core from the LED stuff
    xTaskCreatePinnedToCore(AudioSamplerTaskEntry, "Audio Sampler Loop", STACK_SIZE, nullptr, AUDIO_PRIORITY, &g_taskAudio, AUDIO_CORE);
    CheckHeap();
#endif

#if ENABLE_AUDIOSERIAL
    // The audio sampler task might as well be on a different core from the LED stuff
    xTaskCreatePinnedToCore(AudioSerialTaskEntry, "Audio Serial Loop", STACK_SIZE, nullptr, AUDIOSERIAL_PRIORITY, &g_taskAudio, AUDIOSERIAL_CORE);
    CheckHeap();
#endif

    debugI("Setup complete - ESP32 Free Memory: %d\n", ESP.getFreeHeap());
    CheckHeap();
}

// loop - main execution loop
//
// This is where an Arduino app spends most of its life but since we spin off tasks for much of our work, all that remains here is
// to maintain the WiFi connection and process any data that comes in over the WiFi

void loop()
{
    while(true)
    {
        #if ENABLE_OTA
            EVERY_N_MILLIS(10)
            {
                try
                {
                    if (WiFi.isConnected())
                        ArduinoOTA.handle();
                }
                catch(const std::exception& e)
                {
                    debugW("Exception in OTA code caught");
                }
                
            }
        #endif

        EVERY_N_SECONDS(5)
        {
            #if ENABLE_AUDIO && ENABLE_WIFI
                debugI("Wifi: %s, IP: %s, Mem: %u, LargestBlk: %u, PSRAM Free: %u/%u, Buffer: %d/%d, LED FPS: %d, LED Watt: %d, LED Brite: %0.0lf%%, Audio FPS: %d, Serial FPS: %d, PeakVU: %0.2lf, MinVU: %0.2lf, VURatio: %0.2lf, CPU: %02.0f%%, %02.0f%%, FreeDraw: %lf",
                        WLtoString(WiFi.status()), WiFi.localIP().toString().c_str(), // Wifi
                        ESP.getFreeHeap(),ESP.getMaxAllocHeap(), ESP.getFreePsram(), ESP.getPsramSize(), // Mem
                        g_FPS, g_Watts, g_Brite, // LED
                        g_AudioFPS, g_serialFPS, gPeakVU, gMinVU, gVURatio, // Audio
                        g_TaskManager.GetCPUUsagePercent(0), g_TaskManager.GetCPUUsagePercent(1), 
                        g_FreeDrawTime);
            #elif ENABLE_AUDIO // Implied !ENABLE_WIFI from 1st condition
                debugI("Mem: %u, LargestBlk: %u, PSRAM Free: %u/%u, Buffer: %d/%d, LED FPS: %d, LED Watt: %d, LED Brite: %0.0lf%%, Audio FPS: %d, Serial FPS: %d, PeakVU: %0.2lf, MinVU: %0.2lf, VURatio: %0.2lf, CPU: %02.0f%%, %02.0f%%, FreeDraw: %lf",
                    ESP.getFreeHeap(), ESP.getMaxAllocHeap(), ESP.getFreePsram(), ESP.getPsramSize(), // Mem
                    g_apBufferManager[0]->Depth(), g_apBufferManager[0]->BufferCount(), // Buffer
                    g_FPS, g_Watts, g_Brite, // LED
                    g_AudioFPS, g_serialFPS, gPeakVU, gMinVU, gVURatio, // Audio
                    g_TaskManager.GetCPUUsagePercent(0), g_TaskManager.GetCPUUsagePercent(1), 
                    g_FreeDrawTime);
            #elif ENABLE_WIFI // Implied !ENABLE_AUDIO from 1st condition
                debugI("Wifi: %s, IP: %s, Mem: %u, LargestBlk: %u, PSRAM Free: %u/%u, Buffer: %d/%d, LED FPS: %d, LED Watt: %d, LED Brite: %0.0lf%%, CPU: %02.0f%%, %02.0f%%, FreeDraw: %lf",
                   WLtoString(WiFi.status()), WiFi.localIP().toString().c_str(), // Wifi
                   ESP.getFreeHeap(), ESP.getMaxAllocHeap(), ESP.getFreePsram(), ESP.getPsramSize(), // Mem
                   g_apBufferManager[0]->Depth(), g_apBufferManager[0]->BufferCount(), // Buffer
                   g_FPS, g_Watts, g_Brite, // LED
                   g_TaskManager.GetCPUUsagePercent(0), g_TaskManager.GetCPUUsagePercent(1),  // CPU
                   g_FreeDrawTime); // FreeDraw
            #else
                debugI("Mem: %u, LargestBlk: %u, PSRAM Free: %u/%u, Buffer: %d/%d, LED FPS: %d, LED Watt: %d, LED Brite: %0.0lf%%, CPU: %02.0f%%, %02.0f%%, FreeDraw: %lf",
                   ESP.getFreeHeap(), // Mem
                   ESP.getMaxAllocHeap(), // LargestBlk
                   ESP.getFreePsram(), ESP.getPsramSize(), // PSRAM
                   g_apBufferManager[0]->Depth(), g_apBufferManager[0]->BufferCount(), // Buffer
                   g_FPS, g_Watts, g_Brite, // LED
                   g_TaskManager.GetCPUUsagePercent(0), g_TaskManager.GetCPUUsagePercent(1),  // CPU
                   g_FreeDrawTime); // FreeDraw
            #endif
        }

        delay(10);        
    }
}
