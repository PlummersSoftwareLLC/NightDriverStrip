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

extern "C" 
{
    #include "uzlib/src/uzlib.h"
}

using namespace std;

#define EXPANDED_DATA_HEADER_SIZE 24                                                // Size of the header for expanded data
#define COMPRESSED_DATA_HEADER_SIZE 16                                              // Size of the header for compressed data
#define LED_DATA_SIZE     3                                                         // Data size of an LED (24 bits or 3 bytes)
#define EXPECTED_EXPANDED_PACKET_SIZE \
            (EXPANDED_DATA_HEADER_SIZE + LED_DATA_SIZE * NUM_LEDS)                  // Header plus 24 bits per actual LED
#define COMPRESSED_HEADER (0x44415645)                                              // asci "DAVE" as header 
bool ProcessIncomingData(uint8_t * payloadData, size_t payloadLength);              // In main file

#if INCOMING_WIFI_ENABLED

typedef struct
{
    uint32_t Size;
    uint32_t Version;
    int64_t  ClockSeconds;
    int64_t  ClockMicros;
} SocketStatsResponse;


// SocketServer
//
// Handles incoming connections from the server and pass the data that comes in 

class SocketServer
{
private:

    int                    _port;
    int                    _numLeds;
    int                    _server_fd;
    struct sockaddr_in     _address; 
    unique_ptr<uint8_t []> _pBuffer;
    unique_ptr<uint8_t []> _abOutputBuffer;

public:

    size_t              _cbReceived;

    SocketServer(int port, int numLeds) :
        _port(port),
        _numLeds(numLeds),
        _server_fd(0),
        _cbReceived(0)
    {
        _abOutputBuffer = make_unique<uint8_t []>(EXPECTED_EXPANDED_PACKET_SIZE);
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
        _pBuffer = make_unique<uint8_t []>(EXPECTED_EXPANDED_PACKET_SIZE);

        _cbReceived = 0;
        
        // Creating socket file descriptor 

        if ((_server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) 
        { 
            debugW("socket error\n");
            release();
            return false;
        } 

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

        if (cbNeeded > EXPECTED_EXPANDED_PACKET_SIZE)
        {
            debugW("Unexpected request for %d bytes in ReadUntilNBytesReceived\n", cbNeeded);
            return false;
        }

        do
        {
            // If we're reading at a point in the buffer more than just the header, we're actually transferring data, so light up the LED

            auto oldState = digitalRead(BUILTIN_LED_PIN);
            if (cbNeeded > EXPANDED_DATA_HEADER_SIZE)
                digitalWrite(BUILTIN_LED_PIN, 1);

            // Read data from the socket until we have _bcNeeded bytes in the buffer

            int cbRead = read(socket, _pBuffer.get() + _cbReceived, cbNeeded - _cbReceived);

            // Restore the old state

            if (cbNeeded > EXPANDED_DATA_HEADER_SIZE)
                digitalWrite(BUILTIN_LED_PIN, oldState);

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

    // SendResponseToServer
    //
    // After successfully processing a packet of color data, sends a response back to the server with stats and results

    bool SendResponseToServer(int socket, void * pData, size_t cbSize)
    {
        // Send a response back to the server 
        if (cbSize != write(socket, pData, cbSize))
        {
            debugW("Could not write to socket\n");
            return false;
        }
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

        // Set a timeout of 3 seconds on the socket so we don't permanently hang on a corrupt or partial packet
               
        struct timeval to;
        to.tv_sec = 3;
        to.tv_usec = 0;
        if (setsockopt(new_socket,SOL_SOCKET,SO_RCVTIMEO,&to,sizeof(to)) < 0)
        {
            debugW("Unable to set read timeout on socket!");
            close(new_socket);
            return false;
        }
        
        do
        {
             // Read until we have at least enough for the data header 

            if (false == ReadUntilNBytesReceived(new_socket, COMPRESSED_DATA_HEADER_SIZE))
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

                if (expandedSize > EXPECTED_EXPANDED_PACKET_SIZE)
                {
                    debugE("Expanded packet would be %d but buffer is only %d !!!!\n", expandedSize, EXPECTED_EXPANDED_PACKET_SIZE);
                    close(new_socket);
                    ResetReadBuffer();
                    return false;
                }

                if (false == ReadUntilNBytesReceived(new_socket, COMPRESSED_DATA_HEADER_SIZE + compressedSize))
                {
                    debugW("Could not read compressed data from stream\n");
                    close(new_socket);
                    ResetReadBuffer();
                    return false;
                }
                debugV("Successfuly read %u bytes", COMPRESSED_DATA_HEADER_SIZE + compressedSize);

                if (expandedSize > EXPECTED_EXPANDED_PACKET_SIZE)
                {
                    debugE("Expanded data size of %d would overflow buffer of %d\n", expandedSize, sizeof(_abOutputBuffer));
                    ResetReadBuffer();
                    return false;
                }

                if (!DecompressBuffer(&_pBuffer[COMPRESSED_DATA_HEADER_SIZE], compressedSize, _abOutputBuffer.get(), expandedSize))
                {
                    close(new_socket);
                    debugW("Error decompressing data\n");
                    ResetReadBuffer();
                    return false;
                }

                if (false == ProcessIncomingData(_abOutputBuffer.get(), expandedSize))
                {
                    close(new_socket);
                    debugW("Error processing data\n");
                    ResetReadBuffer();
                    return false;                    
                }
                ResetReadBuffer();
            }
            else
            {
                // Read the rest of the data
                uint16_t command16   = WORDFromMemory(&_pBuffer.get()[0]);

                if (command16 == WIFI_COMMAND_VU)
                {
                    if (false == ReadUntilNBytesReceived(new_socket, WIFI_COMMAND_VU_SIZE))
                    {
                        debugW("Error in getting data for VU command\n");
                        close(new_socket);
                        ResetReadBuffer();
                        return false;
                    }
                    if (true == ProcessIncomingData(_pBuffer.get(), WIFI_COMMAND_VU_SIZE))
                    {
                        assert(WIFI_COMMAND_VU_SIZE == _cbReceived);
                        ResetReadBuffer();
                    }
                    else
                        return false;
                }
                else if (command16 == WIFI_COMMAND_CLOCK)
                {
                    if (false == ReadUntilNBytesReceived(new_socket, WIFI_COMMAND_CLOCK_SIZE))
                    {
                        debugW("Error in getting data for CLOCK command\n");
                        close(new_socket);
                        ResetReadBuffer();
                        return false;
                    }
                    if (true == ProcessIncomingData(_pBuffer.get(), WIFI_COMMAND_CLOCK_SIZE))
                    {
                        assert(_cbReceived == WIFI_COMMAND_CLOCK_SIZE);
                        ResetReadBuffer();
                    }
                    else
                        return false;   
                }
                else if (command16 == WIFI_COMMAND_PIXELDATA64)
                {
                    // We know it's pixel data, so we do some validation before calling Process.

                    uint16_t channel16 = WORDFromMemory(&_pBuffer.get()[2]);
                    uint32_t length32  = DWORDFromMemory(&_pBuffer.get()[4]);
                    uint64_t seconds   = ULONGFromMemory(&_pBuffer.get()[8]);
                    uint64_t micros    = ULONGFromMemory(&_pBuffer.get()[16]);

                    debugV("Uncompressed Header: channel16=%u, length=%u, seconds=%llu, micro=%llu", channel16, length32, seconds, micros);

                    size_t totalExpected = EXPANDED_DATA_HEADER_SIZE + length32 * LED_DATA_SIZE;
                    if (totalExpected > EXPECTED_EXPANDED_PACKET_SIZE)
                    {
                        debugW("Too many bytes promised (%u) - more than we can use for our LEDs at max packet (%u)\n", totalExpected, EXPECTED_EXPANDED_PACKET_SIZE);
                        close(new_socket);
                        ResetReadBuffer();
                        return false;
                    }
                    debugV("Expecting %d total bytes", totalExpected);
                    if (false == ReadUntilNBytesReceived(new_socket, totalExpected))
                    {
                        debugW("Error in getting data\n");
                        close(new_socket);
                        ResetReadBuffer();
                        return false;
                    }
                
                    // Add it to the buffer ring
                    
                    if (false == ProcessIncomingData(_pBuffer.get(), totalExpected))
                    {
                        debugW("Error processing incoming data\n");
                        close(new_socket);
                        ResetReadBuffer();
                        return false;
                    }

                    // Consume the data by resetting the buffer 
                    debugV("Consuming the data as WIFI_COMMAND_PIXELDATA64 by setting _cbReceived to from %d down 0.", _cbReceived);
                    ResetReadBuffer();
                }
            }

            // The fall through cases are either success above or an unrecognized command, or garbage. Either way, we consume
            // everything read so far up to this point as "completed".

            ResetReadBuffer();
            delay(1);
            
        } while (true);
    }    

    #define OUT_CHUNK_SIZE 1

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
            delete pOutput;
            return false;
        }
        //printf("Returning good result");
        return true;
    }
};

#endif