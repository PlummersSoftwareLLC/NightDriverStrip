#pragma once

//+--------------------------------------------------------------------------
//
// File:        SocketServer.h
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
//    Hosts a socket server on port 49152 to receive LED data from the master
//
// History:     Oct-26-2018     Davepl      Created
//---------------------------------------------------------------------------

#include "globals.h"

#include <memory>
#include <netinet/in.h>
#include <sys/socket.h>

#define STANDARD_DATA_HEADER_SIZE   24                                             // Size of the header for expanded data
#define COMPRESSED_HEADER_SIZE      16                                             // Size of the header for compressed data
#define LED_DATA_SIZE               sizeof(CRGB)                                   // Data size of an LED (24 bits or 3 bytes)

// We allocate whatever the max packet is, and use it to validate incoming packets, so right now it's set to the maximum
// LED data packet you could have (header plus 3 RGBs per NUM_LED)

#define MAXIMUM_PACKET_SIZE (STANDARD_DATA_HEADER_SIZE + LED_DATA_SIZE * NUM_LEDS) // Header plus 24 bits per actual LED
#define COMPRESSED_HEADER (0x44415645)                                             // ASCII "DAVE" as header

bool ProcessIncomingData(std::unique_ptr<uint8_t []> & payloadData, size_t payloadLength);

#if INCOMING_WIFI_ENABLED

// SocketResponse
//
// Response data sent back to server every time we receive a packet
struct SocketResponse
{
    uint32_t    size;              // 4
    uint64_t    sequence;          // 8
    uint32_t    flashVersion;      // 4
    double      currentClock;      // 8
    double      oldestPacket;      // 8
    double      newestPacket;      // 8
    double      brightness;        // 8
    double      wifiSignal;        // 8
    uint32_t    bufferSize;        // 4
    uint32_t    bufferPos;         // 4
    uint32_t    fpsDrawing;        // 4
    uint32_t    watts;             // 4
} __attribute__((packed));

static_assert(sizeof(double) == 8);             // SocketResponse on wire uses 8 byte doubles
static_assert(sizeof(float)  == 4);             // PeakData on wire uses 4 byte floats

// Two things must be true for this to work and interop with the C# side:  floats must be 8 bytes, not the default
// of 4 for Arduino.  So that must be set in 'platformio.ini', and you must ensure that you align things such that
// floats land on byte multiples of 8, otherwise you'll get packing bytes inserted.  Welcome to my world! Once upon
// a time, I ported about a billion lines of x86 'pragma_pack(1)' code to the MIPS (davepl)!

static_assert( sizeof(SocketResponse) == 72, "SocketResponse struct size is not what is expected - check alignment and float size" );

// SocketServer
//
// Handles incoming connections from the server and pass the data that comes in

class SocketServer
{
private:

    int                         _port;
    int                         _numLeds;
    int                         _server_fd;
    struct sockaddr_in          _address;
    std::unique_ptr<uint8_t []> _pBuffer;
    std::unique_ptr<uint8_t []> _abOutputBuffer;

public:

    size_t                      _cbReceived;

    SocketServer(int port, int numLeds);
    void release();
    bool begin();
    void ResetReadBuffer();

    // ReadUntilNBytesReceived
    //
    // Read from the socket until the buffer contains at least cbNeeded bytes

    bool ReadUntilNBytesReceived(size_t socket, size_t cbNeeded);

    // ProcessIncomingConnectionsLoop
    //
    // Socket server main ProcessIncomingConnectionsLoop - accepts new connections and reads from them, dispatching
    // data packets into our buffer and closing the socket if anything goes weird.

    bool ProcessIncomingConnectionsLoop();

    // DecompressBuffer
    //
    // Use unzlib to decompress a memory buffer

    static bool DecompressBuffer(const uint8_t * pBuffer, size_t cBuffer, uint8_t * pOutput, size_t expectedOutputSize);
};

#endif
