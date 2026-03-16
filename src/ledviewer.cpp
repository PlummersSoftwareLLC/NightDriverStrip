//+--------------------------------------------------------------------------
//
// File:        ledviewer.cpp
//
// NightDriverStrip - (c) 2026 Plummer's Software LLC.  All Rights Reserved.
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
//    LED viewer socket server implementation.
//

#include "globals.h"
#include "ledviewer.h"
#include "nd_network.h"

#include <arpa/inet.h>
#include <cstring>
#include <sys/socket.h>
#include <sys/time.h>
#include <unistd.h>

LEDViewer::LEDViewer(int port) :
    _port(port),
    _server_fd(-1)
{
    memset(&_address, 0, sizeof(_address));
}

LEDViewer::~LEDViewer()
{
    release();
}

void LEDViewer::release()
{
    if (_server_fd >= 0)
    {
        close(_server_fd);
        _server_fd = -1;
    }
}

bool LEDViewer::begin()
{
    // Creating socket file descriptor

    if ((_server_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        debugE("Color data socket error\n");
        release();
        return false;
    }
    SetSocketBlockingEnabled(_server_fd, false);

    // When an error occurs and we close and reopen the port, we need to specify reuse flags
    // or it might be too soon to use the port again, since close doesn't actually close it
    // until the socket is no longer in use.

    int opt = 1;
    if (setsockopt(_server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)))
    {
        perror("setsockopt colordata");
        release();
        return false;
    }

    memset(&_address, 0, sizeof(_address));
    _address.sin_family = AF_INET;
    _address.sin_addr.s_addr = INADDR_ANY;
    _address.sin_port = htons(_port);

    if (bind(_server_fd, (struct sockaddr *)&_address, sizeof(_address)) < 0)       // Bind socket to port
    {
        perror("bind failed for color data\n");
        release();
        return false;
    }
    if (listen(_server_fd, 6) < 0)                                                  // Start listening for connections
    {
        perror("listen failed for color data\n");
        release();
        return false;
    }
    return true;
}

int LEDViewer::CheckForConnection()
{
    int new_socket = -1;
    // Accept a new incoming connection
    int addrlen = sizeof(_address);
    struct timeval to;
    to.tv_sec = 1;
    to.tv_usec = 0;
    if ((new_socket = accept(_server_fd, (struct sockaddr *)&_address, (socklen_t*)&addrlen)) < 0)
    {
        if (errno == EAGAIN || errno == EWOULDBLOCK)
            return -1;
        debugE("Error accepting color data connection: %s", strerror(errno));
        return -1;
    }
    if (setsockopt(new_socket, SOL_SOCKET, SO_SNDTIMEO, &to, sizeof(to)) < 0)
    {
        debugE("Unable to set send timeout on color data socket!");
        close(new_socket);
        return -1;
    }
    if (setsockopt(new_socket, SOL_SOCKET, SO_RCVTIMEO, &to, sizeof(to)) < 0)
    {
        debugE("Unable to set receive timeout on color data socket!");
        close(new_socket);
        return -1;
    }
    Serial.println("Accepted new ColorData Client!");
    return new_socket;
}

bool LEDViewer::SendPacket(int socket, void * pData, size_t cbSize)
{
    // Send data to the emulator's virtual serial port

    const byte * pb = (byte *)pData;
    debugV("Sending Packet:  %02X, %02X, %02X, %02X, %02X, %02X, %02X, %02X, %02X, %02X, %02X, %02X,...",
                            pb[0], pb[1], pb[2], pb[3], pb[4], pb[5], pb[6], pb[7], pb[8], pb[9], pb[10], pb[11]);

    if (cbSize != write(socket, pData, cbSize))
    {
        debugE("Could not write to color data socket\n");
        return false;
    }
    return true;
}
