#pragma once

//+--------------------------------------------------------------------------
//
// File:        ledviewer.h
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
//    Allows a client to monitor the current state of the LED CRGB array
//
// History:     May-30-2023         Davepl      Created for NightDriverStrip
//
//---------------------------------------------------------------------------

#include "globals.h"
#include <netinet/in.h>
#include <sys/socket.h>
#define COLOR_DATA_PACKET_HEADER 0x434C5244

// Be careful of structure packing rules when adding elements to this structure
// The client will be expecting the data to be packed in a certain way (tight).
// If you add a field that is not a multiple of 4 bytes, you may need to add
// padding to keep the structure packed the way the client expects it.

typedef struct
{
    uint32_t  header;
    uint32_t  width;
    uint32_t  height;
    CRGB      colors[NUM_LEDS];  // Array of LED_COUNT CRGB values
} ColorDataPacket;

// LEDViewer
//
// LEDViewer is a class that listens on a TCP port for a connection a client
// and responds with the current state of the LED array.  This allows a client
// to monitor the state of the LED array in real time.

class LEDViewer
{
public:
    enum class SendResult
    {
        Sent,
        WouldBlock,
        Failed
    };

private:

    int                         _port;
    int                         _server_fd;
    struct sockaddr_in          _address;

public:

    explicit LEDViewer(int port);
    ~LEDViewer();

    void release();
    bool begin();
    int CheckForConnection();
    SendResult SendPacket(int socket, const void * pData, size_t cbSize);
};
