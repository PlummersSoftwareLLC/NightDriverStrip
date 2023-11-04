##+--------------------------------------------------------------------------
##
## videoserver - (c) 2023 Dave Plummer.  All Rights Reserved.
##
## File:        pcstats.py - Pushes text about CPU use, etc, to LED matrix
##
## Description:
##
##    Creates a bitmap of text that shows CPU usage, memory usage, etc
##    compresses it, and sends it over WiFi to NightDriverStrip
##
## History:     Oct-21-2023     davepl      Created
##
##---------------------------------------------------------------------------

# Dependencies

from PIL import Image, ImageDraw, ImageFont     # python3 -m pip install pillow
import socket
import time
import zlib
import datetime

# Constants

MATRIX_WIDTH = 64
MATRIX_HEIGHT = 32
FUTURE_DELAY = 3
ESP32_WIFI_ADDRESS = '192.168.8.86'
PORT = 49152
WIFI_COMMAND_PIXELDATA64 = 3
FPS = 20

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

def send_video_data():

    sock = None
    future = datetime.datetime.now() + datetime.timedelta(seconds=FUTURE_DELAY)

    while True:
        # Connect to the socket if not already connected
        if sock is None:
            sock = connect_to_socket()

        contentimage = DrawFrame()

        pixels = contentimage.tobytes()

        # Advance timestamp by one frame's worth of time as we send each packet
        future += datetime.timedelta(seconds = 1.0 / FPS)
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

        time.sleep(1.0 / FPS)

# build_header
#
# Compose the binary header that the socket server expects to receive, which describes the color data
# that is about to follow

def build_header(seconds, microseconds, length32):
    commandData = (WIFI_COMMAND_PIXELDATA64).to_bytes(2, byteorder='little')
    channelData = (1).to_bytes(2, byteorder='little')
    lengthData  = (int(length32)).to_bytes(4, byteorder='little')
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

def DrawFrame():
        DrawFrame.frame += 1

        img = Image.new('RGB', (MATRIX_WIDTH * 1, MATRIX_HEIGHT * 1), color = 'darkblue');
        d = ImageDraw.Draw(img)
        # use a truetype font
        font = ImageFont.load_default()
        d.text((0,0), "Hello World", font=font, fill=(255,255,255))
        d.text((0,10), "Pass: " + str(DrawFrame.frame), font=font, fill=(255,255,255))
        return img
DrawFrame.frame = 0

# main
#
# Entry point.

if __name__ == "__main__":

    send_video_data()




