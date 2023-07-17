#!/usr/bin/python
##+--------------------------------------------------------------------------
##
## audioserver - (c) 2023 Dave Plummer.  All Rights Reserved.
##
## File:        audioserver.py - Pushes Audio FFT data to NightDriverStrip
##
## Description:
##
##    Samples the system audio, splits it into bands using an FFT, and then
##    sends is over WiFi to a NightDriverStrip instance
##
## History:     Feb-20-2023     davepl      Created
##
##---------------------------------------------------------------------------

import pyaudio
import numpy as np
import socket
import struct
import time
import sys

# NightDriver ESP32 wifi address - update to your ESP32 WiFi

client = '192.168.8.47'        

# Set up audio input stream. 512@24000 gives a nice framerate.  And 512
# is what I run on the ESP32 if connected via hardware mic, so at least it matches

chunk_size   = 512
sample_rate  = 44100
max_freq     = 20000
num_bands    = 16

# Correction I apply to get a mostly linear response across the bands.  

if num_bands == 16:
    band_scalars = [ 0.35, 0.20, 0.125, 0.1, 0.5, 1.2, 1.7, 2.0, 2.1, 2.75, 2.0, 8.0, 8.0, 8.0, 8.0, 8.0 ]
else:
    if num_bands == 12:
        band_scalars = [ 1.0, 1.0, 1.0, 1.0, 0.01, 0.01, 0.01, 0.1, 0.1, 0.1, 0.1, 1.0 ]

# Open the audio stream.  I'm reading from the mic here because I could not find a system independent
# way to read from the default output device, which would normally require a loopback instance be 
# installed and for simplicity sake, I don't want that.

p = pyaudio.PyAudio()

#
# Set up FFT parameters:
#

fft_size = 2**12  # Choose the size of the FFT (power of 2 for efficiency)

# Calculate the frequencies corresponding to each FFT bin.  

freqs = np.fft.rfftfreq(fft_size, d=1.0/sample_rate)  

# Divide the frequency range into frequency bands of equal logrithmic width
# `20` is the minimum frequency of human hearing, `max_freq` is the maximum frequency of interest, 
# and `num_bands` is the desired number of frequency bands.  

bands = np.logspace(np.log10(20), np.log10(max_freq), num_bands+1).astype(int)  

# Compute the width of each frequency band.  This returns the detla between band (n, n+1)) for each band

band_widths = np.diff(bands)  # Take the difference between adjacent frequency band limits

# The socket we will open to the ESP32 to send our band data to

print("Connect to " + client + "...")

sock = None

stream = p.open(format=pyaudio.paFloat32, channels=1, rate=sample_rate, input=True, frames_per_buffer=chunk_size)

# Loop to continuously sample audio and compute spectrum analyzer bands
while True:

    # Connect to the socket we will be sending to if its not already connected
    if sock == None:
        sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        address = (client, 49152)
        sock.connect(address)
        sock.setsockopt(socket.IPPROTO_TCP, socket.TCP_NODELAY, 1)
        sock.setblocking(True);

    # Read the raw audio data.  We ignore overflow exceptions from not accepting every bit, it's ok if
    # miss a few in betweeen samples
    audio_data = np.frombuffer(stream.read(chunk_size, exception_on_overflow=False), dtype=np.float32)
    
    # Compute the FFT to put the samples into frequency
    fft_data = np.abs(np.fft.rfft(audio_data, n=fft_size))

    # Compute band values
    band_values = []
    for i in range(len(bands)-1):
        band_start = np.searchsorted(freqs, bands[i])
        band_stop = np.searchsorted(freqs, bands[i+1])
        band_value = np.median(fft_data[band_start:band_stop])
        band_values.append(band_value)
    
    band_values = np.multiply(band_values, band_scalars)

    # Scale band values to [0, 1]
    band_values = np.clip(band_values, 0.000001, None)                  # Avoid div by zero
    max_band_value = np.max(band_values) + 0.000001;                    # Avoid zero maximumum
    if (max_band_value > 1):
        scaled_values = np.divide(band_values, max_band_value)
    else:
        scaled_values = band_values
        
    # Convert scaled values to ASCII bargraph
    bargraphs = []
    for scaled_value in scaled_values:
        asterisks = "*" * int(round(scaled_value * 8))
        bargraph = f"{asterisks:<8}"
        bargraphs.append(bargraph)

    # Print ASCII bargraphs
    print(bargraphs)
    sys.stdout.write('*')

    # Compose and send the PEAKDATA packet to be sent to the ESP32 NightDriverStrip instance

    packed_data = struct.pack('f' * len(scaled_values), *scaled_values)
    command = 4
    length32 = 4 * num_bands
    seconds = int(time.time())
    micros  = time.perf_counter() - seconds
    
    header1 = (command).to_bytes(2, byteorder='little')             # Offset 0, command16
    header2 = (num_bands).to_bytes(2, byteorder='little')           # Offset 2, num_bands
    header3 = (length32).to_bytes(4, byteorder='little')            # Offset 4, length32 
    header4 = (seconds).to_bytes(8, byteorder='little')             # Offset 8, seconds
    header5 = struct.pack('d', micros)                              # Offset 16, micros
   
    complete_packet = header1 + header2 + header3 + header4 + header5 + packed_data

    try:
        sock.send(complete_packet)
    except socket.error as e:
        print("Socket error!");
        sock.close()
        sock = None
    
    time.sleep(0.015);

sock.close()