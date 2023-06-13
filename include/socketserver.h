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
#pragma once
#include <unistd.h>
#include <stdio.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <string.h>
#include <memory>
#include <iostream>

#include "ledbuffer.h"

extern "C"
{
    #include "uzlib/src/uzlib.h"
}

#define STANDARD_DATA_HEADER_SIZE   24                                              // Size of the header for expanded data
#define COMPRESSED_HEADER_SIZE      16                                              // Size of the header for compressed data
#define LED_DATA_SIZE               sizeof(CRGB)                                    // Data size of an LED (24 bits or 3 bytes)

// We allocate whatever the max packet is, and use it to validate incoming packets, so right now it's set to the maxiumum
// LED data packet you could have (header plus 3 RGBs per NUM_LED)

#define MAXIMUM_PACKET_SIZE (STANDARD_DATA_HEADER_SIZE + LED_DATA_SIZE * NUM_LEDS) // Header plus 24 bits per actual LED
#define COMPRESSED_HEADER (0x44415645)                                              // asci "DAVE" as header

bool ProcessIncomingData(std::unique_ptr<uint8_t []> & payloadData, size_t payloadLength);

#if COLORDATA_SERVER_ENABLED

// SocketResponse
//
// Response data sent back to server ever time we receive a packet
struct SocketResponse
{
    uint32_t    size;              // 4
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
};

static_assert(sizeof(double) == 8);             // SocketResponse on wire uses 8 byte floats
static_assert(sizeof(float)  == 4);             // PeakData on wire uses 4 byte floats

// Two things must be true for this to work and interop with the C# side:  floats must be 8 bytes, not the default
// of 4 for Arduino.  So that must be set in 'platformio.ini', and you must ensure that you align things such that
// floats land on byte multiples of 8, otherwise you'll get packing bytes inserted.  Welcome to my world! Once upon
// a time, I ported about a billion lines of x86 'pragma_pack(1)' code to the MIPS (davepl)!

static_assert( sizeof(SocketResponse) == 64, "SocketResponse struct size is not what is expected - check alignment and float size" );

extern AppTime g_AppTime;
extern std::unique_ptr<LEDBufferManager> g_aptrBufferManager[NUM_CHANNELS];
extern uint32_t g_FPS;
extern float g_Brite;
extern uint32_t g_Watts;

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

    SocketServer(int port, int numLeds) :
        _port(port),
        _numLeds(numLeds),
        _server_fd(-1),
        _cbReceived(0)
    {
        _abOutputBuffer.reset( psram_allocator<uint8_t>().allocate(MAXIMUM_PACKET_SIZE+1) );        // +1 for uzlib one byte overreach bug
        memset(&_address, 0, sizeof(_address));
    }

    void release()
    {
        _pBuffer.release();
        _pBuffer = nullptr;

        if (_server_fd >= 0)
        {
            close(_server_fd);
            _server_fd = -1;
        }
    }

    bool begin()
    {
        _pBuffer.reset( psram_allocator<uint8_t>().allocate(MAXIMUM_PACKET_SIZE) );
        _cbReceived = 0;

        // Creating socket file descriptor
        if ((_server_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
        {
            debugW("socket error\n");
            release();
            return false;
        }

        // When an error occurs and we close and reopen the port, we need to specify reuse flags
        // or it might be too soon to use the port again, since close doesn't actually close it
        // until the socket is no longer in use.

        int opt = 1;
        if (setsockopt(_server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)))
        {
            perror("setsockopt");
            release();
            return false;
        }

        memset(&_address, 0, sizeof(_address));
        _address.sin_family = AF_INET;
        _address.sin_addr.s_addr = INADDR_ANY;
        _address.sin_port = htons( _port );

        if (bind(_server_fd, (struct sockaddr *)&_address, sizeof(_address)) < 0)       // Bind socket to port
        {
            perror("bind failed\n");
            release();
            return false;
        }
        if (listen(_server_fd, 6) < 0)                                                  // Start listening for connections
        {
            perror("listen failed\n");
            release();
            return false;
        }
        return true;
    }


    void ResetReadBuffer()
    {
        _cbReceived = 0;
    }

    // ReadUntilNBytesReceived
    //
    // Read from the socket until the buffer contains at least cbNeeded bytes

    bool ReadUntilNBytesReceived(size_t socket, size_t cbNeeded)
    {
        if (cbNeeded <= _cbReceived)                            // If we already have that many bytes, we're already done
        {
            debugV("Already had enough data to satisfy read: requested %d, had %d", cbNeeded, _cbReceived);
            return true;
        }

        // This test caps maximum packet size as a full buffer read of LED data.  If other packets wind up being longer,
        // the buffer itself and this test might need to change

        if (cbNeeded > MAXIMUM_PACKET_SIZE)
        {
            debugW("Unexpected request for %d bytes in ReadUntilNBytesReceived\n", cbNeeded);
            return false;
        }

        do
        {
            // If we're reading at a point in the buffer more than just the header, we're actually transferring data, so light up the LED

            // Read data from the socket until we have _bcNeeded bytes in the buffer

            int cbRead = read(socket, _pBuffer.get() + _cbReceived, cbNeeded - _cbReceived);

            // Restore the old state

            if (cbRead > 0)
            {
                _cbReceived += cbRead;
            }
            else
            {
                debugW("ERROR: %d bytes read in ReadUntilNBytesReceived trying to read %d\n", cbRead, cbNeeded-_cbReceived);
                return false;
            }
        } while (_cbReceived < cbNeeded);
        return true;
    }

    // ProcessIncomingConnectionsLoop
    //
    // Socket server main ProcessIncomingConnectionsLoop - accepts new connections and reads from them, dispatching
    // data packets into our buffer and closing the socket if anything goes weird.

    int ProcessIncomingConnectionsLoop()
    {
        if (0 == _server_fd)
        {
            debugW("No _server_fd, returning.");
            return false;
        }

        int new_socket = 0;

        // Accept a new incoming connnection
        int addrlen = sizeof(_address);
        if ((new_socket = accept(_server_fd, (struct sockaddr *)&_address, (socklen_t*)&addrlen))<0)
        {
            debugW("Error accepting data!");
            return false;
        }

        // Report where this connection is coming from

        struct sockaddr_in addr;
        socklen_t addr_size = sizeof(struct sockaddr_in);
        getpeername(new_socket, (struct sockaddr *)&addr, &addr_size);
        debugV("Incoming connection from: %s", inet_ntoa(addr.sin_addr));

        // Set a timeout of 3 seconds on the socket so we don't permanently hang on a corrupt or partial packet

        struct timeval to;
        to.tv_sec = 3;
        to.tv_usec = 0;
        if (setsockopt(new_socket,SOL_SOCKET,SO_RCVTIMEO,&to,sizeof(to)) < 0)
        {
            debugW("Unable to set read timeout on socket!");
            close(new_socket);
            ResetReadBuffer();
            return false;
        }

        do
        {
            bool bSendResponsePacket = false;

             // Read until we have at least enough for the data header

            if (false == ReadUntilNBytesReceived(new_socket, STANDARD_DATA_HEADER_SIZE))
            {
                debugW("Read error in getting header.\n");
                close(new_socket);
                ResetReadBuffer();
                return false;
            }

            // Now that we have the header we can see how much more data is expected to follow

            const uint32_t header  = _pBuffer[3] << 24  | _pBuffer[2] << 16  | _pBuffer[1] << 8  | _pBuffer[0];
            if (header == COMPRESSED_HEADER)
            {
                uint32_t compressedSize = _pBuffer[7] << 24  | _pBuffer[6] << 16  | _pBuffer[5] << 8  | _pBuffer[4];
                uint32_t expandedSize   = _pBuffer[11] << 24 | _pBuffer[10] << 16 | _pBuffer[9] << 8  | _pBuffer[8];
                uint32_t reserved       = _pBuffer[15] << 24 | _pBuffer[14] << 16 | _pBuffer[13] << 8 | _pBuffer[12];
                debugV("Compressed Header: compressedSize: %u, expandedSize: %u, reserved: %u", compressedSize, expandedSize, reserved);

                if (expandedSize > MAXIMUM_PACKET_SIZE)
                {
                    debugE("Expanded packet would be %d but buffer is only %d !!!!\n", expandedSize, MAXIMUM_PACKET_SIZE);
                    break;
                }

                if (false == ReadUntilNBytesReceived(new_socket, COMPRESSED_HEADER_SIZE + compressedSize))
                {
                    debugW("Could not read compressed data from stream\n");
                    break;
                }
                debugV("Successfuly read %u bytes", COMPRESSED_HEADER_SIZE + compressedSize);

                // If our buffer is in PSRAM it would be expensive to decompress in place, as the SPIRAM doesn't like
                // non-linear access from what I can tell.  I bet it must send addr+len to request each unique read, so
                // one big read one time would work best, and we use that to copy it to a regular RAM buffer.

                #if USE_PSRAM
                    std::unique_ptr<uint8_t []> _abTempBuffer = std::make_unique<uint8_t []>(MAXIMUM_PACKET_SIZE);
                    memcpy(_abTempBuffer.get(), _pBuffer.get(), MAXIMUM_PACKET_SIZE);
                    auto pSourceBuffer = &_abTempBuffer[COMPRESSED_HEADER_SIZE];
                #else
                    auto pSourceBuffer = &_pBuffer[COMPRESSED_HEADER_SIZE];
                #endif

                if (!DecompressBuffer(pSourceBuffer, compressedSize, _abOutputBuffer.get(), expandedSize))
                {
                    debugW("Error decompressing data\n");
                    break;
                }

                if (false == ProcessIncomingData(_abOutputBuffer, expandedSize))
                {
                    debugW("Error processing data\n");
                    break;

                }
                ResetReadBuffer();
            }
            else
            {
                // Read the rest of the data
                uint16_t command16   = WORDFromMemory(&_pBuffer.get()[0]);

                if (command16 == WIFI_COMMAND_PEAKDATA)
                {
                    #if ENABLE_AUDIO

                        uint16_t numbands  = WORDFromMemory(&_pBuffer.get()[2]);
                        uint32_t length32  = DWORDFromMemory(&_pBuffer.get()[4]);
                        uint64_t seconds   = ULONGFromMemory(&_pBuffer.get()[8]);
                        uint64_t micros    = ULONGFromMemory(&_pBuffer.get()[16]);

                        size_t totalExpected = STANDARD_DATA_HEADER_SIZE + length32;

                        debugV("PeakData Header: numbands=%u, length=%u, seconds=%llu, micro=%llu", numbands, length32, seconds, micros);

                        if (numbands != NUM_BANDS)
                        {
                            debugE("Expecting %d bands but received %d", NUM_BANDS, numbands);
                            break;
                        }

                        if (length32 != numbands * sizeof(float))
                        {
                            debugE("Expecting %d bytes for %d audio bands, but received %d.  Ensure float size and endianness matches between sender and receiver systems.", totalExpected, NUM_BANDS, _cbReceived);
                            break;
                        }

                        if (false == ReadUntilNBytesReceived(new_socket, totalExpected))
                        {
                            debugE("Error in getting peak data from wifi, could not read the %d bytes", totalExpected);
                            break;
                        }

                        if (false == ProcessIncomingData(_pBuffer, totalExpected))
                            break;

                        // Consume the data by resetting the buffer
                        debugV("Consuming the data as WIFI_COMMAND_PEAKDATA by setting _cbReceived to from %d down 0.", _cbReceived);

                    #endif
                    ResetReadBuffer();

                }
                else if (command16 == WIFI_COMMAND_PIXELDATA64)
                {
                    // We know it's pixel data, so we do some validation before calling Process.

                    uint16_t channel16 = WORDFromMemory(&_pBuffer.get()[2]);
                    uint32_t length32  = DWORDFromMemory(&_pBuffer.get()[4]);
                    uint64_t seconds   = ULONGFromMemory(&_pBuffer.get()[8]);
                    uint64_t micros    = ULONGFromMemory(&_pBuffer.get()[16]);

                    debugW("Uncompressed Header: channel16=%u, length=%u, seconds=%llu, micro=%llu", channel16, length32, seconds, micros);

                    size_t totalExpected = STANDARD_DATA_HEADER_SIZE + length32 * LED_DATA_SIZE;
                    if (totalExpected > MAXIMUM_PACKET_SIZE)
                    {
                        debugW("Too many bytes promised (%u) - more than we can use for our LEDs at max packet (%u)\n", totalExpected, MAXIMUM_PACKET_SIZE);
                        break;
                    }

                    debugV("Expecting %d total bytes", totalExpected);
                    if (false == ReadUntilNBytesReceived(new_socket, totalExpected))
                    {
                        debugW("Error in getting pixel data from wifi\n");
                        break;
                    }

                    // Add it to the buffer ring

                    if (false == ProcessIncomingData(_pBuffer, totalExpected))
                    {
                        break;
                    }

                    // Consume the data by resetting the buffer
                    debugV("Consuming the data as WIFI_COMMAND_PIXELDATA64 by setting _cbReceived to from %d down 0.", _cbReceived);
                    ResetReadBuffer();

                    bSendResponsePacket = true;
                }
                else
                {
                    debugW("Unknown command in packet received: %d\n", command16);
                    break;
                }
            }

            // If we make it to this point, it should be success, so we consume

            ResetReadBuffer();

            if (bSendResponsePacket)
            {
                SocketResponse response = {
                                            .size = sizeof(SocketResponse),
                                            .flashVersion = FLASH_VERSION,
                                            .currentClock = g_AppTime.CurrentTime(),
                                            .oldestPacket = g_aptrBufferManager[0]->AgeOfOldestBuffer(),
                                            .newestPacket = g_aptrBufferManager[0]->AgeOfNewestBuffer(),
                                            .brightness   = g_Brite,
                                            .wifiSignal   = (float) WiFi.RSSI(),
                                            .bufferSize   = g_aptrBufferManager[0]->BufferCount(),
                                            .bufferPos    = g_aptrBufferManager[0]->Depth(),
                                            .fpsDrawing   = g_FPS,
                                            .watts        = g_Watts
                                        };

                // I dont think this is fatal, and doesn't affect the read buffer, so content to ignore for now if it happens
                if (sizeof(response) != write(new_socket, &response, sizeof(response)))
                    debugW("Unable to send response back to server.");
            }
        } while (true);

        close(new_socket);
        ResetReadBuffer();
        return false;
    }

    // DecompressBuffer
    //
    // Use unzlib to decompress a memory buffer

    bool DecompressBuffer(const uint8_t * pBuffer, size_t cBuffer, uint8_t * pOutput, size_t expectedOutputSize) const
    {
        debugV("Compressed Data: %02X %02X %02X %02X...", pBuffer[0], pBuffer[1], pBuffer[2], pBuffer[3]);

        struct uzlib_uncomp d = { 0 };
        uzlib_uncompress_init(&d, NULL, 0);

        d.source         = pBuffer;
        d.source_limit   = pBuffer + cBuffer;
        d.source_read_cb = nullptr;
        d.dest_start     = pOutput;
        d.dest           = pOutput;

        // There's an "off by one" bug/feature in uzlib that reaches one byte past the end.  Took forever
        // to find it...
        d.dest_limit     = pOutput + expectedOutputSize + 1;

        int res = uzlib_zlib_parse_header(&d);
        if (res < 0)
        {
            debugE("ERROR: Cannot parse zlib data header\n");
            return false;
        }

        res = uzlib_uncompress_chksum(&d);                                          // Expand the data

        if (res != TINF_DONE) {
            debugE("Error during decompression after producing %d bytes: %d\n", d.dest - pOutput, res);
            return false;
        }

        if (d.dest - pOutput != expectedOutputSize)
        {
            debugE("Exepcted it to to decompress to %d but got %d instead\n", expectedOutputSize, d.dest - pOutput);
            return false;
        }

        return true;
    }
};

#endif