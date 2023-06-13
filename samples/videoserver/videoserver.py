##+--------------------------------------------------------------------------
##
## videoserver - (c) 2023 Dave Plummer.  All Rights Reserved.
##
## File:        videoserver.py - Pushes video data to NightDriverStrip
##
## Description:
##
##    Pulls video from YouTube, downscales it, converts it to RGB data,
##    compresses it, and sends it over WiFi to NightDriverStrip
##
## History:     Mar-2-2023     davepl      Created
##
##---------------------------------------------------------------------------

import cv2                      # python3 -m pip install opencv-python
from pytube import YouTube      # python3 -m pip install pytube
import sys
import socket
import time
import struct
import zlib
import datetime

matrix_width  = 64
matrix_height = 32
future_delay  = 5

# YouTube video URL
# url = "https://youtu.be/7eMpKGIQ6RM"
url = "https://www.youtube.com/watch?v=dQw4w9WgXcQ"
# url = "https://www.youtube.com/watch?v=iF7lo0vU_WI"
# url = "https://www.youtube.com/watch?v=_gzWsIJQTKY"

print("Downloading Video Data...")

# Download the video
yt = YouTube(url)
stream = yt.streams.filter(progressive=True, file_extension='mp4').order_by('resolution').asc().first()
stream.download()

# Open the video
cap = cv2.VideoCapture(stream.default_filename)
if not cap.isOpened():
    sys.exit("Could not open video")

# NightDriver ESP32 wifi address - update to your ESP32 WiFi

client = '192.168.8.34'      
sock = None

# Get a timestamp slightly into the future for buffering

now = datetime.datetime.now()
future = now + datetime.timedelta(seconds=future_delay)

print("Sending Video Data...")

while True:

    startTime = datetime.datetime.now()

    # Connect to the socket we will be sending to if its not already connected
    if sock == None:
        sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        address = (client, 49152)
        sock.connect(address)
        #sock.setsockopt(socket.IPPROTO_TCP, socket.TCP_NODELAY, 1)
        #sock.setblocking(True);

    # Read a frame
    ret, frame = cap.read()
    rgb_frame = cv2.cvtColor(frame, cv2.COLOR_BGR2RGB)

    # If there are no more frames, break out of the loop
    if not ret:
        break

    # Resize the frame match the matrix
    resized = cv2.resize(rgb_frame, (matrix_width, matrix_height))
    pixels = bytes(resized) # resized.reshape((-1, 1))

    # Timestamp for when this frame shoud be shown, such as 2 seconds from now.  Advance the clock
    # by one frame's worth of time as we send each packet
    
    future = future + datetime.timedelta(seconds = 1.0 / stream.fps)
    seconds = int(future.timestamp())
    microseconds = future.microsecond    
    
    # Compose and send the PIXELDATA packet to be sent to the ESP32 NightDriverStrip instance

    command = 3                                                         # WIFI_COMMAND_PIXELDATA64 == 3
    length32    = int(len(pixels) / 3)                                  # Number of pixels being set
    commandData = (command).to_bytes(2, byteorder='little')             # Offset 0, command16
    channelData = (1).to_bytes(2, byteorder='little')                   # Offset 2, LED channelID
    lengthData  = (length32).to_bytes(4, byteorder='little')            # Offset 4, length32 
    secondsData = (seconds).to_bytes(8, byteorder='little')             # Offset 8, seconds
    microsData  = (microseconds).to_bytes(8, byteorder='little')        # struct.pack('d', microseconds)                        # Offset 16, micros
    colorData   = bytes(pixels)
    
    header = commandData + channelData + lengthData + secondsData + microsData
    complete_packet = header + colorData

    # A compressed packet is made of up the tag 0x4415645 (DAVE) followed by the raw lz-compressed bits of the original packet.
    
    compressed_data = zlib.compress(complete_packet);
    expandedSize = len(complete_packet);
    expandedSizeData = expandedSize.to_bytes(4, byteorder='little')
    compressedSize = len(compressed_data)
    compressedSizeData = compressedSize.to_bytes(4, byteorder='little')
    reservedData = (0x12345678).to_bytes(4, byteorder='little')
    
    compressed_packet = (0x44415645).to_bytes(4, byteorder='little') + compressedSizeData + expandedSizeData + reservedData + compressed_data
    
    try:
        sock.send(compressed_packet)
    except socket.error as e:
        print("Socket error!");
        sock.close()
        sock = None

    time.sleep(1.0 / stream.fps / 2)                                        # Div by two is a manual hack to get timing closer
