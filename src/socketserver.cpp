#include "globals.h"
#include "systemcontainer.h"

#if INCOMING_WIFI_ENABLED

int SocketServer::ProcessIncomingConnectionsLoop()
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
            break;
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
                debugE("Expanded packet would be %u but buffer is only %u !!!!\n", expandedSize, MAXIMUM_PACKET_SIZE);
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
                std::unique_ptr<uint8_t []> _abTempBuffer = std::make_unique<uint8_t []>(MAXIMUM_PACKET_SIZE+1);    // Plus one for uzlib buffer overreach bug
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
            bSendResponsePacket = true;
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
                        debugE("Expecting %zu bytes for %d audio bands, but received %zu.  Ensure float size and endianness matches between sender and receiver systems.", totalExpected, NUM_BANDS, _cbReceived);
                        break;
                    }

                    if (false == ReadUntilNBytesReceived(new_socket, totalExpected))
                    {
                        debugE("Error in getting peak data from wifi, could not read the %zu bytes", totalExpected);
                        break;
                    }

                    if (false == ProcessIncomingData(_pBuffer, totalExpected))
                        break;

                    // Consume the data by resetting the buffer
                    debugV("Consuming the data as WIFI_COMMAND_PEAKDATA by setting _cbReceived to from %zu down 0.", _cbReceived);

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

                debugV("Uncompressed Header: channel16=%u, length=%u, seconds=%llu, micro=%llu", channel16, length32, seconds, micros);

                size_t totalExpected = STANDARD_DATA_HEADER_SIZE + length32 * LED_DATA_SIZE;
                if (totalExpected > MAXIMUM_PACKET_SIZE)
                {
                    debugW("Too many bytes promised (%zu) - more than we can use for our LEDs at max packet (%u)\n", totalExpected, MAXIMUM_PACKET_SIZE);
                    break;
                }

                debugV("Expecting %zu total bytes", totalExpected);
                if (false == ReadUntilNBytesReceived(new_socket, totalExpected))
                {
                    debugW("Error in getting pixel data from wifi\n");
                    break;
                }

                // Add it to the buffer ring

                if (false == ProcessIncomingData(_pBuffer, totalExpected))
                {
                    debugW("Error in processing pixel data from wifi\n");
                    break;
                }

                // Consume the data by resetting the buffer
                debugV("Consuming the data as WIFI_COMMAND_PIXELDATA64 by setting _cbReceived to from %zu down 0.", _cbReceived);
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
            debugV("Sending Response Packet from Socket Server");
            auto& bufferManager = g_ptrSystem->BufferManagers()[0];

            SocketResponse response = {
                                        .size = sizeof(SocketResponse),
                                        .flashVersion = FLASH_VERSION,
                                        .currentClock = g_Values.AppTime.CurrentTime(),
                                        .oldestPacket = bufferManager.AgeOfOldestBuffer(),
                                        .newestPacket = bufferManager.AgeOfNewestBuffer(),
                                        .brightness   = g_Values.Brite,
                                        .wifiSignal   = (float) WiFi.RSSI(),
                                        .bufferSize   = bufferManager.BufferCount(),
                                        .bufferPos    = bufferManager.Depth(),
                                        .fpsDrawing   = g_Values.FPS,
                                        .watts        = g_Values.Watts
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

#endif
