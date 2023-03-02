import cv2
from pytube import YouTube
import sys
import socket
import time
import struct

# YouTube video URL
url = "https://www.youtube.com/watch?v=dQw4w9WgXcQ"

# Download the video
yt = YouTube(url)
stream = yt.streams.filter(progressive=True, file_extension='mp4').order_by('resolution').asc().first()
stream.download()

# Open the video
cap = cv2.VideoCapture(stream.default_filename)
if not cap.isOpened():
    sys.exit("Could not open video.mp4")

# NightDriver ESP32 wifi address - update to your ESP32 WiFi

client = '192.168.8.219'      
sock = None

# Loop through each frame
cFrames = 0
t = time.time()

while True:

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
    resized = cv2.resize(rgb_frame, (64, 32))
    pixels = bytes(resized) # resized.reshape((-1, 1))
    #byte_list = [byte for rgb in pixels for byte in bytes(rgb)]

    cFrames = cFrames + 1
    
    # Compose and send the PEAKDATA packet to be sent to the ESP32 NightDriverStrip instance

    command = 3                                                         # WIFI_COMMAND_PIXELDATA64
    length32 = int(len(pixels) / 3)    
    seconds = int(time.time())
    micros  = time.perf_counter() - seconds

    
    commandData = (command).to_bytes(2, byteorder='little')             # Offset 0, command16
    channelData = (1).to_bytes(2, byteorder='little')                   # Offset 2, LED channelID
    lengthData  = (length32).to_bytes(4, byteorder='little')            # Offset 4, length32 
    secondsData = (0).to_bytes(8, byteorder='little')            # Offset 8, seconds
    microsData  = struct.pack('d', 0)                              # Offset 16, micros
    colorData   = bytes(pixels)
    
    complete_packet = commandData + channelData + lengthData + secondsData + microsData + colorData

    sys.write
    
    try:
        sock.send(complete_packet)
    except socket.error as e:
        print("Socket error!");
        sock.close()
        sock = None

# Print the frames matrix
print(cFrames)
