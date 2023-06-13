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


#define HSPI_MISO   27
#define HSPI_MOSI   26    // This is the only IO pin used in this code (master out, slave in)
#define HSPI_SCLK   25
#define HSPI_SS     32
#define FASTLED_ALL_PINS_HARDWARE_SPI
#define FASTLED_ESP32_SPI_BUS HSPI

#include "globals.h"
#include "deviceconfig.h"

void IRAM_ATTR ScreenUpdateLoopEntry(void *);
extern volatile double g_FreeDrawTime;
extern uint32_t g_Watts;
extern float g_Brite;

//
// Task Handles to our running threads
//
NightDriverTaskManager g_TaskManager;

// The one and only instance of ImprovSerial.  We instantiate it as the type needed
// for the serial port on this module.  That's usually HardwareSerial but can be
// other types on the S2, etc... which is why it's a template class.
ImprovSerial<typeof(Serial)> g_ImprovSerial;

//
// Global Variables
//

#if NUM_INFO_PAGES > 0
DRAM_ATTR uint8_t giInfoPage = NUM_INFO_PAGES - 1;                                  // Default to last page
#else
DRAM_ATTR uint8_t giInfoPage = 0;                                                   // Default to first page
#endif

DRAM_ATTR WiFiUDP g_Udp;                                                            // UDP object used for NNTP, etc
DRAM_ATTR uint32_t g_FPS = 0;                                                       // Our global framerate
DRAM_ATTR bool g_bUpdateStarted = false;                                            // Has an OTA update started?
DRAM_ATTR AppTime g_AppTime;                                                        // Keeps track of frame times
DRAM_ATTR bool NTPTimeClient::_bClockSet = false;                                   // Has our clock been set by SNTP?

extern DRAM_ATTR std::unique_ptr<EffectManager<GFXBase>> g_ptrEffectManager;       // The one and only global effect manager

DRAM_ATTR std::shared_ptr<GFXBase> g_aptrDevices[NUM_CHANNELS];                     // The array of GFXBase devices (each strip channel, for example)
DRAM_ATTR std::mutex NTPTimeClient::_clockMutex;                                    // Clock guard mutex for SNTP client
DRAM_ATTR RemoteDebug Debug;                                                        // Instance of our telnet debug server

extern bool bitmap_output(int16_t x, int16_t y, uint16_t w, uint16_t h, uint16_t *bitmap);  // Global function for drawing a bitmap to channel 0

// If an insulator or tree or fan has multiple rings, this table defines how those rings are laid out such
// that they add up to FAN_SIZE pixels total per ring.
//
// Imagine a setup of 5 christmas trees, where each tree was made up of 4 concentric rings of descreasing
// size, like 16, 12, 8, 4.  You would have NUM_FANS of 5 and MAX_RINGS of 4 and your ring table would be 16, 12, 8 4.

DRAM_ATTR const int g_aRingSizeTable[MAX_RINGS] =
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

extern DRAM_ATTR std::unique_ptr<LEDBufferManager> g_aptrBufferManager[NUM_CHANNELS];

//
// Optional Components
//

#if ENABLE_WIFI && ENABLE_WEBSERVER
    DRAM_ATTR CWebServer g_WebServer;
#endif

#if INCOMING_WIFI_ENABLED
    DRAM_ATTR SocketServer g_SocketServer(49152, NUM_LEDS);  // $C000 is free RAM on the C64, fwiw!
#endif

#if ENABLE_REMOTE
    DRAM_ATTR RemoteControl g_RemoteControl;
#endif

#if ENABLE_WIFI && ENABLE_NTP
void UpdateNTPTime();
#endif

// CheckHeap
//
// Quick and dirty debug test to make sure the heap has not been corrupted

inline void CheckHeap()
{
    #if CHECK_HEAP
        if (false == heap_caps_check_integrity_all(true))
        {
            throw std::runtime_error("Heap FAILED checks!");
        }
    #endif
}

// PrintOutputHeader
//
// Displays an info header with info about the system

void PrintOutputHeader()
{
    debugI("NightDriverStrip\n");
    debugI("------------------------------------------------------------------------------------------------------------");
    debugI("M5STICKC: %d, USE_M5DISPLAY: %d, USE_OLED: %d, USE_TFTSPI: %d, USE_LCD: %d, USE_AUDIO: %d, ENABLE_REMOTE: %d", M5STICKC, USE_M5DISPLAY, USE_OLED, USE_TFTSPI, USE_LCD, ENABLE_AUDIO, ENABLE_REMOTE);

    #if USE_PSRAM
        debugI("ESP32 PSRAM Init: %s", psramInit() ? "OK" : "FAIL");
    #endif

    debugI("Version %u: Wifi SSID: \"%s\" - ESP32 Free Memory: %u, PSRAM:%u, PSRAM Free: %u",
            FLASH_VERSION, cszSSID, ESP.getFreeHeap(), ESP.getPsramSize(), ESP.getFreePsram());
    debugI("ESP32 Clock Freq : %d MHz", ESP.getCpuFreqMHz());
}

// TerminateHandler
//
// Set as the handler for unhandled exceptions, prints some debug info and reboots the chip!

void TerminateHandler()
{
    Serial.println("-------------------------------------------------------------------------------------");
    Serial.println("- NightDriverStrip Guru Meditation                              Unhandled Exception -");
    Serial.println("-------------------------------------------------------------------------------------");

    PrintOutputHeader();

/*
    try {
        std::rethrow_exception(std::current_exception());
    }
    catch (std::exception &ex) {
        debugE("Terminated due to exception: %s", ex.what());
    }
*/
    Serial.flush();

    while(true)
    {
        sleep(1);
    }
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
    // Set the unhandled exception handler to be our own special exit function
    std::set_terminate(TerminateHandler);

    // Initialize Serial output
    Serial.begin(115200);
    // Re-route debug output to the serial port
    Debug.setSerialEnabled(true);

    if (!SPIFFS.begin(true))
        Serial.println("WARNING: SPIFFs could not be intialized!");

    // Enabling PSRAM allows us to use the extra 4MB of RAM on the ESP32-WROVER chip, but it caused
    // problems with the S3 rebooting when WiFi connected, so for now, I've limited the default
    // allocator to be PSRAM only on the MESMERIZER project where it's well tested.

    #if MESMERIZER
        heap_caps_malloc_extmem_enable(128);
    #endif

    uzlib_init();

    // Start the Task Manager which takes over the watchdog role and measures CPU usage
    g_TaskManager.begin();

    esp_log_level_set("*", ESP_LOG_WARN);        // set all components to an appropriate logging level

    // Display a simple statup header on the serial port
    PrintOutputHeader();
    debugI("Startup!");

    // Start Debug
    debugI("Starting DebugLoopTaskEntry");
    g_TaskManager.StartDebugThread();
    CheckHeap();

    // Initialize Non-Volatile Storage
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        // Looks like NVS is trash, or a future version we can't read.  Erase it.
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    ESP_ERROR_CHECK(err);

    // Create the JSON writer and start its background thread
    g_ptrJSONWriter = std::make_unique<JSONWriter>();
    g_TaskManager.StartJSONWriterThread();

    // Create and load device config from SPIFFS if possible
    g_ptrDeviceConfig = std::make_unique<DeviceConfig>();

#if ENABLE_WIFI

    debugW("Starting ImprovSerial");
    String name = "NDESP32" + get_mac_address().substring(6);
    g_ImprovSerial.setup(PROJECT_NAME, FLASH_VERSION_NAME, "ESP32", name.c_str(), &Serial);

    // Read the WiFi crendentials from NVS.  If it fails, writes the defaults based on secrets.h

    if (!ReadWiFiConfig())
    {
        debugW("Could not read WiFI Credentials");
        WiFi_password = cszPassword;
        WiFi_ssid     = cszSSID;
        if (!WriteWiFiConfig())
            debugW("Could not even write defaults to WiFi Credentials");
    }
    else if (WiFi_ssid == "Unset" || WiFi_ssid.length() == 0)
    {
        WiFi_password = cszPassword;
        WiFi_ssid     = cszSSID;
    }

    // We create the network reader here, so classes can register their readers from this point onwards.
    //   Note that the thread that executes the readers is started further down, along with other networking
    //   threads.
    g_ptrNetworkReader = std::make_unique<NetworkReader>();
#endif

    // If we have a remote control enabled, set the direction on its input pin accordingly

    #if ENABLE_REMOTE
        pinMode(IR_REMOTE_PIN, INPUT);
    #endif

    #if ENABLE_AUDIO
        #if INPUT_PIN
            pinMode(INPUT_PIN, INPUT);
        #endif
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

    #if USE_TFTSPI
        // Height and width get reversed here because the display is actually portrait, not landscape.  Once
        // we set the rotation, it works as expected in landscape.
        Serial.println("Creating TFT Screen");
        g_pDisplay = std::make_unique<TFTScreen>(TFT_HEIGHT, TFT_WIDTH);

    #elif USE_LCD

        Serial.println("Creating LCD Screen");
        g_pDisplay = std::make_unique<LCDScreen>(TFT_HEIGHT, TFT_WIDTH);

    #elif M5STICKC || M5STICKCPLUS || M5STACKCORE2

        #if USE_M5DISPLAY
            M5.begin();
            Serial.println("Creating M5 Screen");
            g_pDisplay = std::make_unique<M5Screen>(TFT_HEIGHT, TFT_WIDTH);
        #else
            M5.begin(false);
        #endif

    #elif USE_OLED

        #if USE_SSD1306
            Serial.println("Creating SSD1306 Screen");
            g_pDisplay = std::make_unique<SSD1306Screen>(128, 64);
        #else
        Serial.println("Creating OLED Screen");
            g_pDisplay = std::make_unique<OLEDScreen>(128, 64);
        #endif

    #endif

    #if USE_MATRIX
        LEDMatrixGFX::StartMatrix();
    #endif

    // Initialize the strand controllers depending on how many channels we have

    #if USESTRIP
        for (int i = 0; i < NUM_CHANNELS; i++)
        {
            debugW("Allocating LEDStripGFX for channel %d", i);
            g_aptrDevices[i] = std::make_shared<LEDStripGFX>(MATRIX_WIDTH, MATRIX_HEIGHT);
        }
    #endif

    #if USE_MATRIX
        for (int i = 0; i < NUM_CHANNELS; i++)
        {
            g_aptrDevices[i] = std::make_shared<LEDMatrixGFX>(MATRIX_WIDTH, MATRIX_HEIGHT);
            g_aptrDevices[i]->loadPalette(0);
        }
    #endif

    TJpgDec.setJpgScale(1);
    TJpgDec.setCallback(bitmap_output);

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
        g_aptrBufferManager[iChannel] = std::make_unique<LEDBufferManager>(cBuffers, g_aptrDevices[iChannel]);

    // Initialize all of the built in effects

    debugI("Adding LEDs to FastLED...");

    // Due to the nature of how FastLED compiles, the LED_PINx must be passed as a literal, not a variable (template stuff)
    // Onboard PWM LED

    #if ONBOARD_LED_R
        ledcAttachPin(ONBOARD_LED_R,  1);   // assign RGB led pins to PWM channels
        ledcAttachPin(ONBOARD_LED_G,  2);
        ledcAttachPin(ONBOARD_LED_B,  3);
        ledcSetup(1, 12000, 8);             // 12 kHz PWM, 8-bit resolution
        ledcSetup(2, 12000, 8);
        ledcSetup(3, 12000, 8);
    #endif

    //g_pDisplay->ScreenStatus("Initializing LED strips");

    #if USESTRIP

        #if NUM_CHANNELS == 1
            debugI("Adding %d LEDs to FastLED.", g_aptrDevices[0]->GetLEDCount());

            FastLED.addLeds<WS2812B, LED_PIN0, COLOR_ORDER>(g_aptrDevices[0]->leds, g_aptrDevices[0]->GetLEDCount());
            //FastLED.setMaxRefreshRate(100, false);
            pinMode(LED_PIN0, OUTPUT);
        #endif

        #if NUM_CHANNELS >= 2
            pinMode(LED_PIN0, OUTPUT);
            FastLED.addLeds<WS2812B, LED_PIN0, COLOR_ORDER>(g_aptrDevices[1]->leds,g_aptrDevices[0]->GetLEDCount());

            pinMode(LED_PIN1, OUTPUT);
            FastLED.addLeds<WS2812B, LED_PIN0, COLOR_ORDER>(g_aptrDevices[1]->leds,g_aptrDevices[1]->GetLEDCount());
        #endif

        #if NUM_CHANNELS >= 3
            pinMode(LED_PIN2, OUTPUT);
            FastLED.addLeds<WS2812B, LED_PIN0, COLOR_ORDER>(g_aptrDevices[2]->leds,g_aptrDevices[2]->GetLEDCount());
        #endif

        #if NUM_CHANNELS >= 4
            pinMode(LED_PIN3, OUTPUT);
            FastLED.addLeds<WS2812B, LED_PIN0, COLOR_ORDER>(g_aptrDevices[3]->leds,g_aptrDevices[3]->GetLEDCount());
        #endif

        #if NUM_CHANNELS >= 5
            pinMode(LED_PIN4, OUTPUT);
            FastLED.addLeds<WS2812B, LED_PIN0, COLOR_ORDER>(g_aptrDevices[4]->leds,g_aptrDevices[4]->GetLEDCount());
        #endif

        #if NUM_CHANNELS >= 6
            pinMode(LED_PIN5, OUTPUT);
            FastLED.addLeds<WS2812B, LED_PIN0, COLOR_ORDER>(g_aptrDevices[5]->leds,g_aptrDevices[5]->GetLEDCount());
        #endif

        #if NUM_CHANNELS >= 7
            pinMode(LED_PIN6, OUTPUT);
            FastLED.addLeds<WS2812B, LED_PIN0, COLOR_ORDER>(g_aptrDevices[6]->leds,g_aptrDevices[6]->GetLEDCount());
        #endif

        #if NUM_CHANNELS >= 8
            pinMode(LED_PIN7, OUTPUT);
            FastLED.addLeds<WS2812B, LED_PIN0, COLOR_ORDER>(g_aptrDevices[7]->leds,g_aptrDevices[7]->GetLEDCount());
        #endif

        #ifdef POWER_LIMIT_MW
            set_max_power_in_milliwatts(POWER_LIMIT_MW);                // Set brightness limit
        #endif

        g_Brightness = 255;

        #if ATOMLIGHT                                                   // BUGBUG Why input?  Shouldn't they be output?
            pinMode(4, INPUT);
            pinMode(12, INPUT);
            pinMode(13, INPUT);
            pinMode(14, INPUT);
            pinMode(15, INPUT);
        #endif
    #endif

    //g_pDisplay->ScreenStatus("Initializing Effects Manager");

    // Show splash effect on matrix
    #if USE_MATRIX
        debugI("Initializing splash effect manager...");
        InitSplashEffectManager();
    #endif

    // Connect to Wifi and wait for it if so requested


    InitEffectsManager();

    g_TaskManager.StartDrawThread();
    g_TaskManager.StartScreenThread();
    g_TaskManager.StartAudioThread();
    g_TaskManager.StartRemoteThread();

    #if ENABLE_WIFI && WAIT_FOR_WIFI
        debugI("Calling ConnectToWifi()\n");
        if (false == ConnectToWiFi(99))
        {
            debugI("Unable to connect to WiFi, but must have it, so rebooting...\n");
            throw std::runtime_error("Unable to connect to WiFi, but must have it, so rebooting");
        }
        Debug.setSerialEnabled(true);
    #endif

    // Start the services

    g_TaskManager.StartSerialThread();
    g_TaskManager.StartNetworkThread();
    g_TaskManager.StartColorDataThread();
    g_TaskManager.StartSocketThread();

    SaveEffectManagerConfig();
}

// loop - main execution loop
//
// This is where an Arduino app spends most of its life but since we spin off tasks for much of our work, all that remains here is
// to maintain the WiFi connection, watch for OTA updates, and output logging

void loop()
{
    while(true)
    {
        #if ENABLE_WIFI
            EVERY_N_MILLIS(20)
            {
                g_ImprovSerial.loop();
            }
        #endif

        #if ENABLE_OTA
            EVERY_N_MILLIS(20)
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
            String strOutput;

            #if ENABLE_WIFI
                strOutput += str_sprintf("WiFi: %s, IP: %s, ", WLtoString(WiFi.status()), WiFi.localIP().toString().c_str());
            #endif

            strOutput += str_sprintf("Mem: %u, LargestBlk: %u, PSRAM Free: %u/%u, ", ESP.getFreeHeap(),ESP.getMaxAllocHeap(), ESP.getFreePsram(), ESP.getPsramSize());
            strOutput += str_sprintf("LED FPS: %d ", g_FPS);

            #if USESTRIP
                strOutput += str_sprintf("LED Bright: %d, LED Watts: %d, ", g_Watts, g_Brite);
            #endif

            #if USE_MATRIX
                strOutput += str_sprintf("Refresh: %d Hz, ", LEDMatrixGFX::matrix.getRefreshRate());
            #endif

            #if ENABLE_AUDIO
                strOutput += str_sprintf("Audio FPS: %d, MinVU: %6.1f, PeakVU: %6.1f, VURatio: %3.1f ", g_Analyzer._AudioFPS, g_Analyzer._MinVU, g_Analyzer._PeakVU, g_Analyzer._VURatio);
            #endif

            #if ENABLE_SERIAL
                strOutput += str_sprintf("Serial FPS: %d, ", g_Analyzer._serialFPS);
            #endif

            #if INCOMING_WIFI_ENABLED
                strOutput += str_sprintf("Buffer: %d/%d, ", g_aptrBufferManager[0]->Depth(), g_aptrBufferManager[0]->BufferCount());
            #endif

            strOutput += str_sprintf("CPU: %03.0f%%, %03.0f%%, FreeDraw: %4.3lf", g_TaskManager.GetCPUUsagePercent(0), g_TaskManager.GetCPUUsagePercent(1), g_FreeDrawTime);

            Serial.println(strOutput);
        }

        delay(10);
    }
}
