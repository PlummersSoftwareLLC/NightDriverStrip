//+--------------------------------------------------------------------------
//
// File:        socketserver.cpp
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
#include "byte_utils.h"
#include "ledbuffer.h"
#include "nd_network.h"
#include "socketserver.h"
#include "soundanalyzer.h"
#include "systemcontainer.h"
#include "taskmgr.h"   // SOCKET_STACK_SIZE / SOCKET_PRIORITY / SOCKET_CORE
#include "values.h"

#include <arpa/inet.h>
#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <unistd.h>

extern "C"
{
    #include "uzlib/src/uzlib.h"
}

#if INCOMING_WIFI_ENABLED

// SocketResponse
//
// Response data sent back to server every time we receive a packet

SocketServer::SocketServer(int port, int numLeds) :
    _port(port),
    _numLeds(numLeds),
    _cbReceived(0)
{
    _abOutputBuffer.reset( psram_allocator<uint8_t>().allocate(MAXIMUM_PACKET_SIZE+1) );        // +1 for uzlib one byte overreach bug
    memset(&_address, 0, sizeof(_address));
}

// ITaskService hooks
//
// Start/Stop/IsRunning are inherited final from ITaskService; this class only
// supplies the task config, the accept loop body, and the listening-socket
// shutdown nudge that breaks accept() out of its blocking call.

ITaskService::TaskConfig SocketServer::GetTaskConfig() const
{
    return TaskConfig {
        "Socket Server Loop",
        SOCKET_STACK_SIZE,
        SOCKET_PRIORITY,
        SOCKET_CORE,
        1500   // Stop timeout: accept() can block for ~1s before returning EBADF.
    };
}

void SocketServer::OnBeforeWaitForStop()
{
    // Closing the listening socket from this thread breaks the task out of
    // any blocking accept/read call so it can see ShouldShutdown() promptly.
    release();
}

// SocketServer::Run
//
// Repeatedly opens the socket and processes incoming connections. Runs until
// ShouldShutdown() is true; ITaskService::TaskEntryThunk handles the actual
// vTaskDelete and the running-state bookkeeping when this returns.
void SocketServer::Run()
{
    while (!ShouldShutdown())
    {
        if (nd_network::IsWiFiConnected())
        {
            release();
            if (ShouldShutdown())
                break;

            if (begin())
            {
                ProcessIncomingConnectionsLoop();
                debugV("Socket connection closed.  Retrying...");
            }
            else
            {
                debugE("Failed to start socket server, retrying in 5 seconds...");
                delay(5000);
            }
        }
        delay(500);
    }

    // Drop the listening socket if Stop() didn't already, so we leave the
    // service in a clean state regardless of which path we exited through.
    release();
}

void SocketServer::release()
{
    _pBuffer.reset();
    // Atomic exchange: only the caller that observed a non-negative fd does
    // the close(). The other (Run loop vs. OnBeforeWaitForStop) sees -1 and
    // becomes a no-op, eliminating a double-close race.
    int fd = _server_fd.exchange(-1);
    if (fd >= 0)
        close(fd);
}

bool SocketServer::begin()
{
    _pBuffer.reset( psram_allocator<uint8_t>().allocate(MAXIMUM_PACKET_SIZE) );
    _cbReceived = 0;

    // Build the socket on a local fd and only publish it into the atomic
    // _server_fd member after listen() succeeds. That keeps release() (which
    // can run on a different thread via OnBeforeWaitForStop) from closing a
    // half-configured descriptor or racing with bind/listen.

    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0)
    {
        debugE("socket error\n");
        _pBuffer.reset();
        return false;
    }

    nd_network::SetSocketBlockingEnabled(fd, false);

    // When an error occurs, and we close and reopen the port, we need to specify reuse flags
    // or it might be too soon to use the port again, since close doesn't actually close it
    // until the socket is no longer in use.

    int opt = 1;
    if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)))
    {
        debugE("setsockopt SO_REUSEADDR failed on socket %d: %s (%d)", fd, strerror(errno), errno);
        close(fd);
        _pBuffer.reset();
        return false;
    }

    memset(&_address, 0, sizeof(_address));
    _address.sin_family = AF_INET;
    _address.sin_addr.s_addr = INADDR_ANY;
    _address.sin_port = htons( _port );

    if (bind(fd, (struct sockaddr *)&_address, sizeof(_address)) < 0)       // Bind socket to port
    {
        debugE("bind failed on port %d, socket %d: %s (%d)", _port, fd, strerror(errno), errno);
        close(fd);
        _pBuffer.reset();
        return false;
    }
    if (listen(fd, 6) < 0)                                                  // Start listening for connections
    {
        debugE("listen failed on port %d, socket %d: %s (%d)", _port, fd, strerror(errno), errno);
        close(fd);
        _pBuffer.reset();
        return false;
    }

    _server_fd.store(fd);
    debugI("Socket server %d listening on port %d", fd, _port);
    return true;
}

void SocketServer::ResetReadBuffer()
{
    _cbReceived = 0;
    memset(_pBuffer.get(), 0, MAXIMUM_PACKET_SIZE);
}

// ReadUntilNBytesReceived
//
// Read from the socket until the buffer contains at least cbNeeded bytes

bool SocketServer::ReadUntilNBytesReceived(size_t socket, size_t cbNeeded)
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
        debugE("Unexpected request for %d bytes in ReadUntilNBytesReceived\n", cbNeeded);
        return false;
    }

    do
    {
        // If we're reading at a point in the buffer more than just the header, we're actually transferring data, so light up the LED

        // Read data from the socket until we have _bcNeeded bytes in the buffer

        int cbRead = 0;
        do
        {
            cbRead = read(socket, (uint8_t *) _pBuffer.get() + _cbReceived, cbNeeded - _cbReceived);
        } while (cbRead < 0 && errno == EINTR);

        // Restore the old state

        if (cbRead > 0)
        {
            _cbReceived += cbRead;
        }
        else
        {
            debugE("ERROR: %d bytes read in ReadUntilNBytesReceived trying to read %d\n", cbRead, cbNeeded-_cbReceived);
            return false;
        }
    } while (_cbReceived < cbNeeded);
    return true;
}

// DecompressBuffer
//
// Use unzlib to decompress a memory buffer

bool SocketServer::DecompressBuffer(const uint8_t * pBuffer, size_t cBuffer, uint8_t * pOutput, size_t expectedOutputSize)
{
    debugV("Compressed Data: %02X %02X %02X %02X...", pBuffer[0], pBuffer[1], pBuffer[2], pBuffer[3]);

    struct uzlib_uncomp d = { 0 };
    uzlib_uncompress_init(&d, nullptr, 0);

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
        debugE("Expected it to to decompress to %d but got %d instead\n", expectedOutputSize, d.dest - pOutput);
        return false;
    }

    return true;
}

// ProcessIncomingConnectionsLoop
//
// Socket server main ProcessIncomingConnectionsLoop - accepts new connections and reads from them, dispatching
// data packets into our buffer and closing the socket if anything goes weird.

bool SocketServer::ProcessIncomingConnectionsLoop()
{
    int listen_fd = _server_fd.load();
    if (0 >= listen_fd)
    {
        debugE("No _server_fd, returning.");
        return false;
    }

    int new_socket = -1;

    // Accept loop: wait for an incoming connection, sleeping between polls to avoid busy-spinning
    socklen_t addrlen = sizeof(_address);
    while (new_socket < 0)
    {
        // Re-read each iteration so a release() from another thread (Stop)
        // is observed as -1 and we exit promptly via EBADF/-1 fall-through.
        listen_fd = _server_fd.load();
        if (listen_fd < 0)
            return false;
        new_socket = accept(listen_fd, (struct sockaddr *)&_address, &addrlen);
        if (new_socket < 0)
        {
            if (errno == EAGAIN || errno == EWOULDBLOCK)
            {
                delay(100); // No connection yet, yield and retry
                continue;
            }
            debugE("Socket server %d failed to accept connection: %s (%d)", listen_fd, strerror(errno), errno);
            return false;
        }
    }

    // Report where this connection is coming from

    struct sockaddr_in addr;
    socklen_t addr_size = sizeof(struct sockaddr_in);
    if (0 != getpeername(new_socket, (struct sockaddr *)&addr, &addr_size))
    {
        close(new_socket);
        ResetReadBuffer();
        return false;
    }

    debugV("Incoming connection from: %s", inet_ntoa(addr.sin_addr));

    // Set a timeout of 3 seconds on the socket so we don't permanently hang on a corrupt or partial packet

    struct timeval to;
    to.tv_sec = 3;
    to.tv_usec = 0;
    if (setsockopt(new_socket,SOL_SOCKET,SO_RCVTIMEO,&to,sizeof(to)) < 0)
    {
        debugE("Unable to set read timeout on socket!");
        close(new_socket);
        ResetReadBuffer();
        return false;
    }

    if (_pBuffer == nullptr)
    {
        debugE("Buffer not allocated!");
        close(new_socket);
        ResetReadBuffer();
        return false;
    }

    // Ensure the new_socket is valid
    if (new_socket < 0) {
        debugE("Invalid socket!");
        ResetReadBuffer();
        return false;
    }

    do
    {
        bool bSendResponsePacket = false;

            // Read until we have at least enough for the data header

        if (false == ReadUntilNBytesReceived(new_socket, STANDARD_DATA_HEADER_SIZE))
        {
            debugE("Read error in getting header.\n");
            break;
        }

        // Now that we have the header we can see how much more data is expected to follow

        const uint32_t header  = _pBuffer[3] << 24  | _pBuffer[2] << 16  | _pBuffer[1] << 8  | _pBuffer[0];
        if (header == COMPRESSED_HEADER)
        {
            uint32_t compressedSize = _pBuffer[7] << 24  | _pBuffer[6] << 16  | _pBuffer[5] << 8  | _pBuffer[4];
            uint32_t expandedSize   = _pBuffer[11] << 24 | _pBuffer[10] << 16 | _pBuffer[9] << 8  | _pBuffer[8];
            uint32_t reserved       = _pBuffer[15] << 24 | _pBuffer[14] << 16 | _pBuffer[13] << 8 | _pBuffer[12];
            debugV("Compressed Header: compressedSize: %lu, expandedSize: %lu, reserved: %lu", (unsigned long)compressedSize, (unsigned long)expandedSize, (unsigned long)reserved);

            if (expandedSize > MAXIMUM_PACKET_SIZE)
            {
                debugE("Expanded packet would be %lu but buffer is only %lu !!!!\n", (unsigned long)expandedSize, (unsigned long)MAXIMUM_PACKET_SIZE);
                break;
            }

            if (false == ReadUntilNBytesReceived(new_socket, COMPRESSED_HEADER_SIZE + compressedSize))
            {
                debugE("Could not read compressed data from stream\n");
                break;
            }
            debugV("Successfully read %zu bytes", (size_t)(COMPRESSED_HEADER_SIZE + compressedSize));

            // If our buffer is in PSRAM it would be expensive to decompress in place, as the SPIRAM doesn't like
            // non-linear access from what I can tell.  I bet it must send addr+len to request each unique read, so
            // one big read one time would work best, and we use that to copy it to a regular RAM buffer.

            #if USE_PSRAM
                std::unique_ptr<uint8_t []> _abTempBuffer = std::make_unique<uint8_t []>(MAXIMUM_PACKET_SIZE+1);    // Plus one for uzlib buffer overreach bug
                memcpy(_abTempBuffer.get(), _pBuffer.get(), MAXIMUM_PACKET_SIZE);
                auto pSourceBuffer = &_abTempBuffer[COMPRESSED_HEADER_SIZE];
            #else
                auto pSourceBuffer = &_pBuffer[COMPRESSED_HEADER_SIZE];
            #endif

            if (!DecompressBuffer(pSourceBuffer, compressedSize, _abOutputBuffer.get(), expandedSize))
            {
                debugE("Error decompressing data\n");
                break;
            }

            if (false == ProcessIncomingData(_abOutputBuffer, expandedSize))
            {
                debugE("Error processing data\n");
                break;

            }
            ResetReadBuffer();
            bSendResponsePacket = true;
        }
        else
        {
            // Read the rest of the data
            uint16_t command16   = WORDFromMemory(&_pBuffer.get()[0]);

            if (command16 == WIFI_COMMAND_PEAKDATA)
            {
                #if ENABLE_AUDIO
                {
                    uint16_t numbands  = WORDFromMemory(&_pBuffer.get()[2]);
                    uint32_t length32  = DWORDFromMemory(&_pBuffer.get()[4]);
                    uint64_t seconds   = ULONGFromMemory(&_pBuffer.get()[8]);
                    uint64_t micros    = ULONGFromMemory(&_pBuffer.get()[16]);

                    size_t totalExpected = STANDARD_DATA_HEADER_SIZE + length32;

                    debugV("PeakData Header: numbands=%u, length=%lu, seconds=%llu, micro=%llu", numbands, (unsigned long)length32, seconds, micros);

                    if (numbands != NUM_BANDS)
                    {
                        debugE("Expecting %d bands but received %d", NUM_BANDS, numbands);
                        break;
                    }

                    if (length32 != numbands * sizeof(float))
                    {
                        debugE("Expecting %zu bytes for %d audio bands, but received %zu.  Ensure float size and endianness matches between sender and receiver systems.", (size_t)totalExpected, (int)NUM_BANDS, (size_t)_cbReceived);
                        break;
                    }

                    if (false == ReadUntilNBytesReceived(new_socket, totalExpected))
                    {
                        debugE("Error in getting peak data from wifi, could not read the %zu bytes", (size_t)totalExpected);
                        break;
                    }

                    if (false == ProcessIncomingData(_pBuffer, totalExpected))
                        break;

                    // Consume the data by resetting the buffer
                    debugV("Consuming the data as WIFI_COMMAND_PEAKDATA by setting _cbReceived to from %zu down 0.", (size_t)_cbReceived);
                }
                #else
                    // Audio disabled: consume any declared payload to keep stream in sync, then ignore it
                    uint32_t length32  = DWORDFromMemory(&_pBuffer.get()[4]);
                    size_t totalExpected = STANDARD_DATA_HEADER_SIZE + length32;
                    if (!ReadUntilNBytesReceived(new_socket, totalExpected))
                    {
                        debugE("Audio disabled, failed to skip PEAKDATA payload of %zu bytes", (size_t)totalExpected);
                        break;
                    }
                    debugV("Audio disabled; skipped PEAKDATA payload (%zu bytes)", (size_t)totalExpected);
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

                debugV("Uncompressed Header: channel16=%u, length=%lu, seconds=%llu, micro=%llu", channel16, (unsigned long)length32, seconds, micros);

                size_t totalExpected = STANDARD_DATA_HEADER_SIZE + length32 * LED_DATA_SIZE;
                if (totalExpected > MAXIMUM_PACKET_SIZE)
                {
                    debugE("Too many bytes promised (%zu) - more than we can use for our LEDs at max packet (%lu)\n", (size_t)totalExpected, (unsigned long)MAXIMUM_PACKET_SIZE);
                    break;
                }

                debugV("Expecting %zu total bytes", (size_t)totalExpected);
                if (false == ReadUntilNBytesReceived(new_socket, totalExpected))
                {
                    debugE("Error in getting pixel data from wifi\n");
                    break;
                }

                // Add it to the buffer ring

                if (false == ProcessIncomingData(_pBuffer, totalExpected))
                {
                    debugE("Error in processing pixel data from wifi\n");
                    break;
                }

                // Consume the data by resetting the buffer
                debugV("Consuming the data as WIFI_COMMAND_PIXELDATA64 by setting _cbReceived to from %zu down 0.", (size_t)_cbReceived);
                ResetReadBuffer();

                bSendResponsePacket = true;
            }
            else
            {
                debugE("Unknown command in packet received: %u\n", command16);
                break;
            }
        }

        // If we make it to this point, it should be success, so we consume

        ResetReadBuffer();

        if (bSendResponsePacket)
        {
            static uint64_t sequence = 0;

            debugV("Sending Response Packet from Socket Server");
            auto& bufferManager = g_ptrSystem->GetBufferManagers()[0];

            SocketResponse response = {
                                        .size = sizeof(SocketResponse),
                                        .sequence     = sequence++,
                                        .flashVersion = FLASH_VERSION,
                                        .currentClock = g_Values.AppTime.CurrentTime(),
                                        .oldestPacket = bufferManager.AgeOfOldestBuffer(),
                                        .newestPacket = bufferManager.AgeOfNewestBuffer(),
                                        .brightness   = g_Values.Brite,
                                        .wifiSignal   = (float) nd_network::GetWiFiRSSI(),
                                        .bufferSize   = bufferManager.BufferCount(),
                                        .bufferPos    = bufferManager.Depth(),
                                        .fpsDrawing   = g_Values.FPS,
                                        .watts        = g_Values.Watts
                                    };

            // I dont think this is fatal, and doesn't affect the read buffer, so content to ignore for now if it happens
            if (sizeof(response) != write(new_socket, &response, sizeof(response)))
                debugE("Unable to send response back to server.");
        }

        delay(1);

    } while (true);

    close(new_socket);
    ResetReadBuffer();
    return false;
}

#endif
