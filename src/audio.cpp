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

#include "globals.h"
// The SoundAnalyzer is present even when Audio is not defined, but it then a mere stub class
// with a few stats fields. In the Audio case, it's the full class

SoundAnalyzer g_Analyzer;                    // Dummy stub class in non-audio case, real in audio case

#if ENABLE_AUDIO

#include <esp_task_wdt.h>


extern NightDriverTaskManager g_TaskManager;
extern DRAM_ATTR uint32_t g_FPS;          // Our global framerate
extern uint32_t g_Watts;
extern float g_Brite;
extern DRAM_ATTR bool g_bUpdateStarted;                     // Has an OTA update started?

// AudioSamplerTaskEntry
// A background task that samples audio, computes the VU, stores it for effect use, etc.

void IRAM_ATTR AudioSamplerTaskEntry(void *)
{
    debugI(">>> Sampler Task Started");

    // Enable microphone input 
    pinMode(INPUT_PIN, INPUT);

    g_Analyzer.SampleBufferInitI2S();

    for (;;)
    {
        uint64_t lastFrame = millis();

        g_Analyzer.RunSamplerPass();
        g_Analyzer.UpdatePeakData();
        g_Analyzer.DecayPeaks();

        // VURatio with a fadeout

        static float lastVU = 0.0;
        constexpr auto VU_DECAY_PER_SECOND = 4.0;
        if (g_Analyzer._VURatio > lastVU)
            lastVU = g_Analyzer._VURatio;
        else
            lastVU -= (millis() - lastFrame) / 1000.0 * VU_DECAY_PER_SECOND;
        lastVU = std::max(lastVU, 0.0f);
        lastVU = std::min(lastVU, 2.0f);
        g_Analyzer._VURatioFade = lastVU;

        // Instantaneous VURatio

        g_Analyzer._VURatio = (g_Analyzer._PeakVU == g_Analyzer._MinVU) ?
                                0.0 :
                                (g_Analyzer._VU - g_Analyzer._MinVU) / std::max(g_Analyzer._PeakVU - g_Analyzer._MinVU, (float) MIN_VU) * 2.0f;

        debugV("VURatio: %f\n", g_Analyzer._VURatio);

        // Delay enough time to yield 60fps max
        // We wait a minimum of 5ms even if busy so we don't Bogart the CPU

        unsigned long elapsed = millis() - lastFrame;
        const auto targetDelay = PERIOD_FROM_FREQ(60) * MILLIS_PER_SECOND / MICROS_PER_SECOND;
        delay(max(10.0, targetDelay - elapsed));

        g_Analyzer._AudioFPS = FPS(lastFrame, millis());
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



class VICESocketServer
{
private:

    int                         _port;
    int                         _server_fd;
    struct sockaddr_in          _address;
    std::unique_ptr<uint8_t []> _pBuffer;
    std::unique_ptr<uint8_t []> _abOutputBuffer;

    const int BUFFER_SIZE = 255;

public:

    VICESocketServer(int port) :
        _port(port),
        _server_fd(0)
    {
        _abOutputBuffer = std::make_unique<uint8_t []>(BUFFER_SIZE);
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
        _pBuffer = std::make_unique<uint8_t []>(BUFFER_SIZE);

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

    SoundAnalyzer Analyzer;

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
        data.vu = map(g_Analyzer._VURatioFade, 0, 2, 1, 16);           // Convert VU to a 1-16 value

        // We treat 0 as a NUL terminator and so we don't want to send it in-band.  Since a band has to be 2 before
        // it is displayed, this has no effect on the display

        for (int i = 0; i < 8; i++)
        {
            int iBand = map(i, 0, 7, 0, NUM_BANDS-2);
            uint8_t low   = g_Analyzer.g_peak2Decay[iBand] * MAXPET;
            uint8_t high  = g_Analyzer.g_peak2Decay[iBand+1] * MAXPET;
            data.peaks[i] = (high << 4) + low;
        }

        data.tail = 00;
        if (Serial2.availableForWrite())
        {
            Serial2.write((uint8_t *)&data, sizeof(data));
            //Serial2.flush(true);
            static int lastFrame = millis();
            g_Analyzer._serialFPS = FPS(lastFrame, millis());
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

#endif // ENABLE_AUDIOSERIAL

#endif


