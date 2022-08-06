//+--------------------------------------------------------------------------
//
// File:        audio.cpp
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
//    Source files for NightDriverStrip's audio processing
//
// History:     Apr-13-2019         Davepl      Created for NightDriverStrip
//              
//---------------------------------------------------------------------------

#include "globals.h"                      // CONFIG and global headers

#if ENABLE_AUDIO

#include "soundanalyzer.h"
#include <esp_task_wdt.h>

extern DRAM_ATTR uint32_t g_FPS;          // Our global framerate
extern uint32_t g_Watts; 
extern double g_Brite;
extern int g_serialFPS;                       // Frames per sec reported on serial

float SampleBuffer::_oldVU;
float SampleBuffer::_oldPeakVU;
float SampleBuffer::_oldMinVU;

float PeakData::_Min[NUM_BANDS] = { 0.0 };
float PeakData::_Max[NUM_BANDS] = { 0.0 }; 
float PeakData::_Last[NUM_BANDS] = { 0.0 }; 
float PeakData::_allBandsMax = 1.0;

// BUGBUG (Davepl) - Time to collect all of these into an Audio class, I'd say!

float gScaler = 0.0f;                       // Instantaneous read of LED display vertical scaling
float gLogScale = 1.0f;                     // How exponential the peaks are made to be
volatile float gVURatio = 1.0;              // Current VU as a ratio to its recent min and max
volatile float gVURatioFade = 1.0;          // Same as gVURatio but with a slow decay
volatile float gVU = 0;                     // Instantaneous read of VU value
volatile float gPeakVU = MAX_VU;            // How high our peak VU scale is in live mode
volatile float gMinVU = 0;                  // How low our peak VU scale is in live mode
volatile unsigned long g_cSamples = 0;      // Total number of samples successfully collected
int g_AudioFPS = 0;                         // Framerate of the audio sampler
int g_serialFPS = 0;                        // How many serial packets are processed per second

unsigned long g_lastPeak1Time[NUM_BANDS] = { 0 } ;
float g_peak1Decay[NUM_BANDS] = { 0 };
float g_peak2Decay[NUM_BANDS] = { 0 };
float g_peak1DecayRate = 1.0f;
float g_peak2DecayRate = 1.0f;

// Depending on how many bands have been defined, one of these tables will contain the frequency
// cutoffs for that "size" of a spectrum display.  

#if NUM_BANDS == 8
    static int cutOffsBand [8] =
    {
        100, 250, 450, 565, 715, 900, 1125, 1500
    };
    static float scalarsBand[8] = 
    {
        0.1f, 0.2f, 0.3f, 0.3f, 0.4f, 0.5f, 0.6f, 0.75f
    };
#else
    static_assert(NUM_BANDS == 16);

    const int cutOffsBand[16] =
    {
        25, 40, 63, 100, 160, 250, 400, 630, 1000, 1600, 2500, 4000, 6300, 10000, 16000, 20000
    };
    #if TTGO    
        const float scalarsBand[16] = 
        {
            0.05f, 0.15f, 0.2f, 0.225f, 0.25f, 0.3f, 0.35f, 0.4f, 0.425f, 0.6f, 0.7f, 0.8f, 0.8f, 0.9f, 1.0f, 1.0f
        };
    #else
        const float scalarsBand[16] = 
        {
            #if M5STICKC || M5STICKCPLUS
                0.15f, 0.25f, 0.35f, 0.45f, 0.6f, 0.6f, 0.6f, 0.6f, 0.6f, 0.6f, 0.7f, 0.8f, 0.8f, 0.9f, 1.0f, 0.75f
            #else
                0.18f, 0.25f, 0.25f, 0.30f, 0.35f, 0.35f, 0.45f, 0.55f, 0.5f, 0.5f, 0.5f, 0.6f, .6f, 1.0f, 2.0f, 0.5f
            #endif
        };
    #endif
#endif


// BandCutoffTable
//
// Depending on how many bands we have, returns the cutoffs of where those bands are in the spectrum

const int * SampleBuffer::BandCutoffTable(int bandCount)                
{
    return cutOffsBand;
}

const float * SampleBuffer::GetBandScalars(int bandCount)
{
    return scalarsBand;     
}

PeakData g_Peaks;


// AudioSamplerTaskEntry
// A background task that samples audio, computes the VU, stores it for effect use, etc.

void IRAM_ATTR AudioSamplerTaskEntry(void *)
{
     SoundAnalyzer Analyzer(INPUT_PIN);

    debugI(">>> Sampler Task Started");

    for (;;)
    {
        static uint64_t lastFrame = millis();
        g_AudioFPS = FPS(lastFrame, millis());
        static double lastVU = 0.0;

        // VURatio with a fadeout

        constexpr auto VU_DECAY_PER_SECOND = 3.0;
        if (gVURatio > lastVU)
            lastVU = gVURatio;
        else
            lastVU -= (millis() - lastFrame) / 1000.0 * VU_DECAY_PER_SECOND;
        lastVU = std::max(lastVU, 0.0);
        lastVU = std::min(lastVU, 2.0);
        gVURatioFade = lastVU;

        lastFrame = millis();

        EVERY_N_SECONDS(10)
        {
            debugI("Mem: %u LED FPS: %d, LED Watt: %d, LED Brite: %0.0lf%%, Audio FPS: %d, Serial FPS: %d, PeakVU: %0.2lf, MinVU: %0.2lf, VURatio: %0.2lf",
                        ESP.getFreeHeap(), g_FPS, g_Watts, g_Brite, g_AudioFPS, g_serialFPS, gPeakVU, gMinVU, gVURatio);
        }
        g_Peaks = Analyzer.RunSamplerPass();
        UpdatePeakData();        
        DecayPeaks();

        // Instantaneous VURatio

        gVURatio = (gPeakVU == gMinVU) ? 0.0 : (gVU-gMinVU) / std::max(gPeakVU - gMinVU, (float) MIN_VU) * 2.0f;


        // Delay enough time to yield 25ms total used this frame, which will net 40FPS exactly (as long as the CPU keeps up)

        unsigned long elapsed = millis() - lastFrame;

        const auto targetDelay = PERIOD_FROM_FREQ(40) * MILLIS_PER_SECOND / MICROS_PER_SECOND;
        delay(elapsed >= targetDelay ? 1 : targetDelay - elapsed);

        if (g_bUpdateStarted)
            delay(1000);

        delay(1);
    }
}

#if ENABLE_AUDIOSERIAL

// AudioSerial
//
// There is a project at https://github.com/PlummersSoftwareLLC/PETRock which allows you to connect the ESP to the USERPORT
// of a Commodore 64 or PET (40 or 80 cols) and display the spectrum visualization on the computer's screen in assembly 
// language.  
//
// To support this, when enabled, this task repeatedly sends out copies of the latest data peaks, scaled to 20, which is
// the max height of the PET/C64 spectrum bar.  This should manage around 24 fps at 2400baud.
//
// The VICESocketServer acts as a server that sends serial data to the socket on the emulator machine to emulate serial data.

#include <fcntl.h>

/** Returns true on success, or false if there was an error */
bool SetSocketBlockingEnabled(int fd, bool blocking)
{
   if (fd < 0) return false;

   int flags = fcntl(fd, F_GETFL, 0);
   if (flags == -1) return false;
   flags = blocking ? (flags & ~O_NONBLOCK) : (flags | O_NONBLOCK);
   return (fcntl(fd, F_SETFL, flags) == 0) ? true : false;
}

class VICESocketServer
{
private:

    int                    _port;
    int                    _server_fd;
    struct sockaddr_in     _address; 
    unique_ptr<uint8_t []> _pBuffer;
    unique_ptr<uint8_t []> _abOutputBuffer;

    const int BUFFER_SIZE = 255;

public:

    size_t              _cbReceived;

    VICESocketServer(int port) :
        _port(port),
        _server_fd(0),
        _cbReceived(0)
    {
        _abOutputBuffer = make_unique<uint8_t []>(BUFFER_SIZE);
        memset(&_address, 0, sizeof(_address));
    }

    void release()
    {
        _pBuffer.release();
        _pBuffer = nullptr;

        if (_server_fd)
        {
            close(_server_fd);
            _server_fd = 0;
        }
    }

    bool begin()
    {
        _pBuffer = make_unique<uint8_t []>(BUFFER_SIZE);
        _cbReceived = 0;
        
        // Creating socket file descriptor 

        if ((_server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) 
        { 
            debugW("socket error\n");
            release();
            return false;
        } 
        SetSocketBlockingEnabled(_server_fd, false);

        memset(&_address, 0, sizeof(_address));
        _address.sin_family = AF_INET; 
        _address.sin_addr.s_addr = INADDR_ANY; 
        _address.sin_port = htons( _port ); 
       
        if (bind(_server_fd, (struct sockaddr *)&_address, sizeof(_address)) < 0)       // Bind socket to port 
        { 
            debugW("bind failed\n"); 
            release();
            return false;
        } 
        if (listen(_server_fd, 6) < 0)                                                  // Start listening for connections
        { 
            debugW("listen failed\n"); 
            release();
            return false;
        } 
        return true;
    }

    int CheckForConnection()
    {
        int new_socket = -1;
        // Accept a new incoming connnection
        int addrlen = sizeof(_address); 
        struct timeval to;
        to.tv_sec = 1;
        to.tv_usec = 0;     
        if ((new_socket = accept(_server_fd, (struct sockaddr *)&_address, (socklen_t*)&addrlen))<0) 
        { 
            return -1;
        } 
        if (setsockopt(new_socket,SOL_SOCKET,SO_SNDTIMEO,&to,sizeof(to)) < 0)
        {
            debugW("Unable to set send timeout on socket!");
            close(new_socket);
            return false;
        }
        if (setsockopt(new_socket,SOL_SOCKET,SO_RCVTIMEO,&to,sizeof(to)) < 0)
        {
            debugW("Unable to set receive timeout on socket!");
            close(new_socket);
            return false;
        }
        /*
        int flag = 1;
        if (setsockopt(new_socket,IPPROTO_TCP,TCP_NODELAY,&flag,sizeof(flag)) < 0)
        {
            debugW("Unable to set TCP_NODELAY on socket!");
            close(new_socket);
            return false;
        }
        */
        Serial.println("Accepted new VICE Client!");
        return new_socket;
    }

    bool SendPacketToVICE(int socket, void * pData, size_t cbSize)
    {
        // Send data to the emulator's virtual serial port

        if (cbSize != write(socket, pData, cbSize))
        {
            debugW("Could not write to socket\n");
            return false;
        }
        return true;
    }
};

struct SerialData
{
    uint8_t header[1];          // 'DP' in the high and low nibbles
    uint8_t vu;                 // VU meter, 0-31
    uint8_t peaks[8];           // 16 4-bit values representing the band peaks, 0-31 (constrained to 0-19)
    uint8_t tail;               // (0x0D)
};

void IRAM_ATTR AudioSerialTaskEntry(void *)
{
//  SoftwareSerial Serial64(SERIAL_PINRX, SERIAL_PINTX);
    debugI(">>> Sampler Task Started");

    SoundAnalyzer Analyzer(INPUT_PIN);

    #if ENABLE_VICE_SERVER
        VICESocketServer socketServer(25232);
        if (!socketServer.begin()) {
            debugE("Unable to start socket server for VICE!");
        } else {
            debugW("Started socket server for VICE!");
        }
    #endif

    Serial2.begin(2400, SERIAL_8N1, SERIAL_PINRX, SERIAL_PINTX);
    debugI("    Opened Serial2 on pins %d,%d\n", SERIAL_PINRX, SERIAL_PINTX);

    int socket = -1;

    for (;;)
    {
        unsigned long startTime = millis();

        SerialData data;
            
        const int MAXPET = 16;                                      // Highest value that the PET can display in a bar
        data.header[0] = ((3 << 4) + 15);
        data.vu = mapDouble(gVURatioFade, 0, 2, 1, 16);           // Convert VU to a 1-16 value

        // We treat 0 as a NUL terminator and so we don't want to send it in-band.  Since a band has to be 2 before
        // it is displayed, this has no effect on the display

        for (int i = 0; i < 8; i++)
        {
            uint8_t low   = g_peak2Decay[i*2] * MAXPET;
            uint8_t high  = g_peak2Decay[i*2+1] * MAXPET;
            data.peaks[i] = (high << 4) + low;
        }

        data.tail = 00;
        if (Serial2.availableForWrite())
        {
            Serial2.write((uint8_t *)&data, sizeof(data));
            //Serial2.flush(true);
            static int lastFrame = millis();
            g_serialFPS = FPS(lastFrame, millis());
            lastFrame = millis();
        }

        // When the CBM processes a packet, it sends us a * to ACK.  We count those to determine the number
        // of packets per second being processed

        while (Serial2.available() > 0)
        {
            char read = Serial2.read();
            Serial.print(read);
            if (read == '*')
            {
            }
        }

        #if ENABLE_VICE_SERVER
            // If we have a socket open, send our packet to its virtual serial port now as well.
        
            if (socket < 0)
                socket = socketServer.CheckForConnection();

            if (socket >= 0)
            {
                if (!socketServer.SendPacketToVICE(socket, (uint8_t *)&data, sizeof(data)))
                {
                    // If anything goes wrong, we close the socket so it can accept new incoming attempts
                    debugI("Error on socket, so closing");
                    close(socket);
                    socket = -1;
                }
            }
        #endif

        auto targetFPS = 480.0 / sizeof(data);
        auto targetElapsed = 1.0 / targetFPS * 1000;
        auto elapsed = millis() - startTime;
        if (targetElapsed > elapsed)
            delay(targetElapsed - elapsed);
        else    
            delay(1);
    }
}

#endif // EMABLE_AUDIOSERIAL

#endif


