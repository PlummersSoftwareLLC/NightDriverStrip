##+--------------------------------------------------------------------------
##
## videoserver - (c) 2023 Dave Plummer.  All Rights Reserved.
##
## File:        videoserver2.py - Pushes video data to NightDriverStrip
##
## Description:
##
##    Pulls video from YouTube, downscales it, converts it to RGB data,
##    compresses it, and sends it over WiFi to NightDriverStrip
##
## History:     Jun-11-2023     davepl      Created
##
##---------------------------------------------------------------------------

# Dependencies

import cv2                          # python3 -m pip install opencv-python
from pytube import YouTube          # python3 -m pip install pytube
import sys
import socket
import time
import struct
import zlib
import datetime

# Constants

MATRIX_WIDTH = 64
MATRIX_HEIGHT = 32
FUTURE_DELAY = 5
URL = "https://www.youtube.com/watch?v=dQw4w9WgXcQ"
ESP32_WIFI_ADDRESS = '192.168.8.34'
PORT = 49152
WIFI_COMMAND_PIXELDATA64 = 3

# download_video
#
# Open the supplied YouTube URL in the video player and return the video stream

def download_video(url):
    print("Downloading Video Data...")
    yt = YouTube(url)
    stream = yt.streams.filter(progressive=True, file_extension='mp4').order_by('resolution').asc().first()
    stream.download()
    return stream

# connect_to_socket
#
# Connect to the NightDriverStrip ESP32 module via a socket to send the color send_video_data

def connect_to_socket():
    sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    address = (ESP32_WIFI_ADDRESS, PORT)
    sock.connect(address)
    return sock

# send_video_data
# 
# Take the supplied video stream and breaks each frame into a new colordata object to be sent to
# the NightDriverStrip module over WiFi

def send_video_data(stream):
    print("Sending Video Data...")
    cap = cv2.VideoCapture(stream.default_filename)
    if not cap.isOpened():
        sys.exit("Could not open video")

    sock = None
    future = datetime.datetime.now() + datetime.timedelta(seconds=FUTURE_DELAY)

    while True:
        # Connect to the socket if not already connected
        if sock is None:
            sock = connect_to_socket()

        # Read and process a frame
        ret, frame = cap.read()
        if not ret:
            break

        rgb_frame = cv2.cvtColor(frame, cv2.COLOR_BGR2RGB)
        resized = cv2.resize(rgb_frame, (MATRIX_WIDTH, MATRIX_HEIGHT))
        pixels = bytes(resized)

        # Advance timestamp by one frame's worth of time as we send each packet
        future += datetime.timedelta(seconds = 1.0 / stream.fps)
        seconds = int(future.timestamp())
        microseconds = future.microsecond

        # Compose and send the PIXELDATA packet
        header = build_header(seconds, microseconds, len(pixels) / 3)
        complete_packet = header + pixels

        compressed_packet = compress_packet(complete_packet)

        try:
            sock.send(compressed_packet)
        except socket.error:
            print("Socket error!")
            sock.close()
            sock = None

        time.sleep(1.0 / stream.fps / 2)

# build_header
#
# Compose the binary header that the socket server expects to receive, which describes the color data
# that is about to follow

def build_header(seconds, microseconds, length32):
    commandData = (WIFI_COMMAND_PIXELDATA64).to_bytes(2, byteorder='little')
    channelData = (1).to_bytes(2, byteorder='little')
    lengthData  = (length32).to_bytes(4, byteorder='little')
    secondsData = (seconds).to_bytes(8, byteorder='little')
    microsData  = (microseconds).to_bytes(8, byteorder='little')
    return commandData + channelData + lengthData + secondsData + microsData

# compress_packet
#
# Use zlib to lzcompress the packet and return it wrapped in the little header that indicates its 
# going to be a compressed packet

def compress_packet(complete_packet):
    compressed_data = zlib.compress(complete_packet)
    expandedSizeData = len(complete_packet).to_bytes(4, byteorder='little')
    compressedSizeData = len(compressed_data).to_bytes(4, byteorder='little')
    reservedData = (0x12345678).to_bytes(4, byteorder='little')
    return (0x44415645).to_bytes(4, byteorder='little') + compressedSizeData + expandedSizeData + reservedData + compressed_data

# main
#
# Entry point.  Downloads a YouTube video and sends it to the socket server on a NightDriverStrip
# mesmerizer project to be displayed on the matrix

if __name__ == "__main__":
    stream = download_video(URL)
    send_video_data(stream)
