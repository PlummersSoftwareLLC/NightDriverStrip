#!/usr/bin/env python3

# To use this script, it is recommended to create a virtual environment:
#
#   python3 -m venv .venv
#   source .venv/bin/activate
#   pip install requests Pillow pygame
#

import requests
import json
import argparse
import socket
import time
import struct
import threading
import queue

import os

class SocketReader(threading.Thread):
    def __init__(self, sock, data_queue, stop_event, verbose=False):
        super().__init__()
        self.sock = sock
        self.data_queue = data_queue
        self.stop_event = stop_event
        self.verbose = verbose
        self.daemon = True # Allow the main program to exit even if this thread is still running

    def run(self):
        while not self.stop_event.is_set():
            try:
                chunk = self.sock.recv(4096) # Read in larger chunks
                if not chunk:
                    # Connection closed by peer
                    if self.verbose: print("SocketReader: Connection closed by peer.")
                    break
                self.data_queue.put(chunk)
                if self.verbose: print(f"SocketReader: Put {len(chunk)} bytes into queue. Queue size: {self.data_queue.qsize()}")
            except socket.timeout:
                # No data available within the timeout, continue looping
                if self.verbose: print("SocketReader: Socket timeout.")
                continue
            except BlockingIOError:
                # No data available right now, continue looping
                time.sleep(0.001) # Small delay to prevent busy-waiting
                if self.verbose: print("SocketReader: BlockingIOError, sleeping.")
                continue
            except Exception as e:
                print(f"Error in SocketReader thread: {e}")
                break
        self.data_queue.put(None) # Signal that the reader has stopped
        if self.verbose: print("SocketReader: Thread stopped.")




class NightDriver:
    """
    A Python client for the NightDriverStrip REST API.
    """

    def __init__(self, host, port=12000):
        """
        Initializes the NightDriver client.

        Args:
            host: The IP address or hostname of the NightDriver device.
        """
        self.base_url = f"http://{host}:{port}"

    def _get(self, endpoint, params=None):
        """Helper for GET requests."""
        try:
            response = requests.get(f"{self.base_url}{endpoint}", params=params)
            response.raise_for_status()
            return response.json()
        except requests.exceptions.RequestException as e:
            print(f"Error making GET request to {endpoint}: {e}")
            return None

    def _post(self, endpoint, data=None):
        """Helper for POST requests."""
        try:
            response = requests.post(f"{self.base_url}{endpoint}", data=data)
            response.raise_for_status()
            # Return JSON if available, otherwise return the raw response
            try:
                return response.json()
            except json.JSONDecodeError:
                return response
        except requests.exceptions.RequestException as e:
            print(f"Error making POST request to {endpoint}: {e}")
            return None

    def get_effects(self):
        """Gets the list of effects from the device."""
        return self._get("/effects")

    def set_current_effect(self, effect_index, width=None, height=None):
        """Sets the current effect on the device."""
        data = {"currentEffectIndex": effect_index}
        if width is not None: data["width"] = width
        if height is not None: data["height"] = height
        return self._post("/currentEffect", data=data)

    def next_effect(self):
        """Switches to the next effect."""
        return self._post("/nextEffect")

    def previous_effect(self):
        """Switches to the previous effect."""
        return self._post("/previousEffect")

    def get_settings(self):
        """Gets the device settings."""
        return self._get("/settings")

    def set_settings(self, settings):
        """
        Sets device settings.

        Args:
            settings: A dictionary of settings to update.
        """
        return self._post("/settings", data=settings)

    def get_device_setting_specs(self):
        """Gets the specifications for device settings."""
        return self._get("/settings/specs")

    def get_effect_setting_specs(self, effect_index):
        """Gets the specifications for a specific effect's settings."""
        return self._get("/settings/effect/specs", params={"effectIndex": effect_index})

    def get_effect_settings(self, effect_index):
        """Gets the settings for a specific effect."""
        return self._get("/settings/effect", params={"effectIndex": effect_index})

    def set_effect_settings(self, effect_index, settings):
        """
        Sets settings for a specific effect.
        'effectIndex' must be in the settings dict.
        """
        data = {"effectIndex": effect_index}
        data.update(settings)
        return self._post("/settings/effect", data=data)


class ColorClient:
    """
    A client for the NightDriver ColorServer.
    """
    MAX_INVALID_HEADER_MESSAGES = 3 # Limit the number of invalid header messages

    def __init__(self, host, port=12000, verbose=False):
        """
        Initializes the ColorClient.

        Args:
            host: The IP address or hostname of the NightDriver device.
            port: The port of the ColorServer (default 12000).
            verbose: Enable verbose output for debugging.
        """
        self.host = host
        self.port = port
        self.sock = None
        self.data_queue = queue.Queue()
        self.stop_event = threading.Event()
        self.reader_thread = None
        self.invalid_header_message_count = 0
        self.verbose = verbose
        self.frames_captured = 0
        self.frames_in_error = 0
        self.internal_buffer = b'' # RJL internal buffer for partial frames


    def __enter__(self):
        self.sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        self.sock.settimeout(5.0) # Increased timeout for initial read
        self.sock.connect((self.host, self.port))
        # RJL try to switch to a shorter timeout for SocketReader
        self.sock.settimeout(0.1)

        self.reader_thread = SocketReader(self.sock, self.data_queue, self.stop_event, verbose=self.verbose)
        self.reader_thread.start()
        return self

    def __exit__(self, exc_type, exc_val, exc_tb):
        self.stop_event.set() # Signal the reader thread to stop
        if self.reader_thread and self.reader_thread.is_alive():
            self.reader_thread.join(timeout=1.0) # Wait for the thread to finish
        if self.sock:
            self.sock.close()
    # RJL
    def OLD_get_data_from_queue(self, n_bytes, timeout_seconds=1.0):
        data = b''
        start_time = time.time()
        while len(data) < n_bytes:
            remaining_timeout = timeout_seconds - (time.time() - start_time)
            if remaining_timeout <= 0:
                if self.verbose: print(f"_get_data_from_queue: Timeout reached. Requested {n_bytes}, got {len(data)}")
                return None # Overall timeout
            try:
                # Try to get a chunk from the queue, waiting up to 0.1 seconds
                chunk = self.data_queue.get(timeout=min(remaining_timeout, 0.1))
                if chunk is None:
                    if self.verbose: print("_get_data_from_queue: Reader thread stopped (chunk is None).")
                    return None
                data += chunk
                if self.verbose: print(f"_get_data_from_queue: Received {len(chunk)} bytes. Total {len(data)}/{n_bytes}")
            except queue.Empty:
                if self.verbose: print("_get_data_from_queue: Queue empty, continuing.")
                continue
        return data

    def _get_data_from_queue(self, n_bytes, timeout_seconds=1.0):
        data = b''
        start_time = time.time()

        # Use any previously buffered data
        if self.internal_buffer:
            data += self.internal_buffer
            self.internal_buffer = b''

        while len(data) < n_bytes:
            remaining_timeout = timeout_seconds - (time.time() - start_time)
            if remaining_timeout <= 0:
                if self.verbose: print(f"_get_data_from_queue: Timeout. Needed {n_bytes}, got {len(data)}")
                return None

            try:
                chunk = self.data_queue.get(timeout=min(remaining_timeout, 0.1))
                if chunk is None:
                    if self.verbose: print("_get_data_from_queue: Reader thread stopped.")
                    return None
                data += chunk
                if self.verbose: print(f"_get_data_from_queue: Got {len(chunk)} bytes, total {len(data)}/{n_bytes}")
            except queue.Empty:
                if self.verbose: print("_get_data_from_queue: Queue empty, waiting...")
                continue

        # Save any extra bytes for future reads
        if len(data) > n_bytes:
            self.internal_buffer = data[n_bytes:]
            data = data[:n_bytes]

        return data


    def _find_header(self, timeout_seconds=None):
        header_bytes = b'DRLC'
        buffer = b''
        start_time = time.time()

        while True:
            if timeout_seconds is not None and (time.time() - start_time) > timeout_seconds:
                if self.verbose: print(f"_find_header: Timeout reached. Buffer size: {len(buffer)}")
                return None, buffer # Overall timeout

            # Try to get a chunk from the queue
            try:
                chunk = self.data_queue.get(timeout=0.1) # Small timeout for queue.get
                if chunk is None: # Reader thread has stopped
                    if self.verbose: print("_find_header: Reader thread stopped (chunk is None).")
                    return None, buffer
                buffer += chunk
                if self.verbose: print(f"_find_header: Received {len(chunk)} bytes. Current buffer size: {len(buffer)}")
            except queue.Empty:
                if self.verbose: print("_find_header: Queue empty, continuing.")
                continue # No data yet, try again

            # Search for the header in the buffer
            header_index = buffer.find(header_bytes)
            if header_index != -1:
                # Found the header, now extract the full 12 bytes (header + width + height)
                if len(buffer) - header_index >= 12:
                    full_header_data = buffer[header_index : header_index + 12]
                    remaining_buffer = buffer[header_index + 12:] # Consume the header from the buffer
                    self.invalid_header_message_count = 0 # Reset count on successful sync
                    if self.verbose: print(f"_find_header: Header found. Full header data size: {len(full_header_data)}, Remaining buffer size: {len(remaining_buffer)}")
                    return full_header_data, remaining_buffer
                else:
                    # Not enough bytes for the full header yet, keep what we have and wait for more
                    if self.verbose: print(f"_find_header: Partial header found. Waiting for more data. Current buffer size: {len(buffer)}")
                    buffer = buffer[header_index:]
                    # Continue to the outer loop to get more data
                    continue
            else:
                # Header not found, discard bytes before the last possible start of header
                # This prevents the buffer from growing indefinitely with junk data
                if len(buffer) > len(header_bytes) - 1:
                    if self.verbose: print(f"_find_header: Header not found. Discarding {len(buffer) - (len(header_bytes) - 1)} bytes. New buffer size: {len(header_bytes) - 1}")
                    buffer = buffer[-(len(header_bytes) - 1):]
                else:
                    if self.verbose: print(f"_find_header: Header not found. Buffer size: {len(buffer)}")
                # Continue to the outer loop to get more data
                continue

    def capture_frames(self, duration_seconds):
        """
        Captures frames for a specified duration.

        Args:
            duration_seconds: The number of seconds to capture.

        Returns:
            A list of frames, where each frame is a dictionary with 'width', 'height', and 'pixels'.
        """
        frames = []
        start_time = time.time()

        # Initial header synchronization
        remaining_time = duration_seconds - (time.time() - start_time)
        if remaining_time <= 0:
            if self.verbose: print("capture_frames: Initial remaining time <= 0.")
            return []

        header_data, current_buffer = self._find_header(remaining_time)
        if header_data is None:
            if self.verbose: print("capture_frames: Initial header not found or timeout.")
            return []

        header = struct.unpack('>I', header_data[0:4])[0]
        width, height = struct.unpack('<II', header_data[4:12])
        if header != 0x44524C43:
            if self.verbose: print(f"capture_frames: Initial header invalid: {header:08x}")
            return []

        num_leds = width * height
        pixel_data_size = num_leds * 3 # 3 bytes per RGB pixel
        if self.verbose: print(f"capture_frames: Initial frame dimensions: {width}x{height}, pixel data size: {pixel_data_size}")

        self.internal_buffer = b''  # Reset buffer before capture RJL

        while time.time() - start_time < duration_seconds:
            remaining_time = duration_seconds - (time.time() - start_time)
            if remaining_time <= 0:
                if self.verbose: print("capture_frames: Overall duration timeout reached.")
                break # Overall timeout

            try:
                # Pre-fill pixel_data with any remaining buffer from _find_header
                pixel_data = current_buffer
                bytes_needed = pixel_data_size - len(pixel_data)

                if bytes_needed > 0:
                    if self.verbose: print(f"capture_frames: Getting {bytes_needed} more bytes for pixel data.")
                    additional_pixel_data = self._get_data_from_queue(bytes_needed, remaining_time)
                    if additional_pixel_data is None:
                        if self.verbose: print("capture_frames: Not enough pixel data received or timeout.")
                        break
                    pixel_data += additional_pixel_data

                if len(pixel_data) < pixel_data_size:
                    if self.verbose: print(f"capture_frames: Incomplete pixel data. Expected {pixel_data_size}, got {len(pixel_data)}")
                    self.frames_in_error += 1
                    break # Incomplete frame

                pixels = [(pixel_data[i], pixel_data[i+1], pixel_data[i+2]) for i in range(0, pixel_data_size, 3)]
                frames.append({"width": width, "height": height, "pixels": pixels})
                self.frames_captured += 1
                if self.verbose: print(f"capture_frames: Frame {self.frames_captured} captured. Total frames in error: {self.frames_in_error}")

                remaining_time = duration_seconds - (time.time() - start_time)
                if remaining_time <= 0:
                    if self.verbose: print("capture_frames: Duration timeout after frame capture.")
                    break # Overall timeout

                next_header_data, current_buffer = self._find_header(remaining_time)
                if next_header_data is None:
                    if self.verbose: print("capture_frames: Next header not found or timeout.")
                    break

                next_header = struct.unpack('>I', next_header_data[0:4])[0]
                width, height = struct.unpack('<II', next_header_data[4:12])

                if next_header != 0x44524C43:
                    self.frames_in_error += 1
                    if self.invalid_header_message_count < self.MAX_INVALID_HEADER_MESSAGES:
                        if self.verbose: print(f"capture_frames: Invalid subsequent packet header: {next_header:08x}. Attempting to re-synchronize...")
                        self.invalid_header_message_count += 1
                    elif self.invalid_header_message_count == self.MAX_INVALID_HEADER_MESSAGES:
                        if self.verbose: print(f"capture_frames: Too many invalid packet headers. Suppressing further messages.")
                        self.invalid_header_message_count += 1

                    remaining_time = duration_seconds - (time.time() - start_time)
                    if remaining_time <= 0:
                        if self.verbose: print("capture_frames: Duration timeout during re-sync attempt.")
                        break # Overall timeout

                    re_sync_header_data, current_buffer = self._find_header(remaining_time)
                    if re_sync_header_data is None:
                        if self.verbose: print("capture_frames: Re-synchronization failed: header not found or timeout.")
                        break
                    header = struct.unpack('>I', re_sync_header_data[0:4])[0]
                    width, height = struct.unpack('<II', re_sync_header_data[4:12])
                    if header != 0x44524C43:
                        if self.verbose: print(f"capture_frames: Re-synchronization failed: {header:08x}")
                        break
                    num_leds = width * height
                    pixel_data_size = num_leds * 3
                    if self.verbose: print(f"capture_frames: Re-synchronized. New dimensions: {width}x{height}, pixel data size: {pixel_data_size}")

            except Exception as e:
                if self.verbose: print(f"capture_frames: Error during frame capture: {e}")
                self.frames_in_error += 1
                break

        if self.verbose: print(f"capture_frames: Exiting. Total frames captured: {self.frames_captured}, Total frames in error: {self.frames_in_error}")
        return frames

def create_animated_gif(frames, output_filename, frame_duration=100, scale=None, verbose=False):
    """
    Creates an animated GIF from a list of frames.

    Args:
        frames: A list of frames from capture_frames.
        output_filename: The name of the output GIF file.
        frame_duration: The duration of each frame in milliseconds.
        scale: The integer factor to scale the image by.
        verbose: Enable verbose output for debugging.
    """
    from PIL import Image

    if not frames:
        if verbose: print("create_animated_gif: No frames to create a GIF.")
        return

    # Smart scaling
    first_frame = frames[0]
    original_width = first_frame['width']
    original_height = first_frame['height']

    if scale is None:
        if original_width < 256 and original_height < 256:
            scale = 8 # Default scale factor for small images
            if verbose: print(f"create_animated_gif: Auto-scaling enabled with factor {scale}.")
        else:
            scale = 1 # No scaling for larger images

    if verbose: print(f"create_animated_gif: Creating GIF from {len(frames)} frames with scale factor {scale}.")
    images = []
    for i, frame in enumerate(frames):
        img = Image.new('RGB', (frame['width'], frame['height']))
        img.putdata([tuple(p) for p in frame['pixels']])

        if scale > 1:
            img = img.resize((frame['width'] * scale, frame['height'] * scale), Image.NEAREST)

        images.append(img)
        if verbose: print(f"create_animated_gif: Added frame {i+1}/{len(frames)} to GIF.")

    if images:
        images[0].save(
            output_filename,
            save_all=True,
            append_images=images[1:],
            optimize=False,
            duration=frame_duration,
            loop=0
        )
        if verbose: print(f"create_animated_gif: Saved animated GIF to {output_filename}")

def save_raw_frames(frames, output_filename, verbose=False):
    if verbose: print(f"save_raw_frames: Function entered. Verbose is: {verbose}")
    """
    Saves captured frames to a raw binary file for ledcat.

    Args:
        frames: A list of frames from capture_frames.
        output_filename: The name of the output .raw file.
        verbose: Enable verbose output for debugging.
    """
    import os

    if not frames:
        if verbose: print("save_raw_frames: No frames to save.")
        return

    if verbose: print(f"save_raw_frames: Saving {len(frames)} frames to {output_filename} (absolute path: {os.path.abspath(output_filename)})")

    all_pixel_data = bytearray()
    for i, frame in enumerate(frames):
        if verbose: print(f"save_raw_frames: Processing frame {i+1}/{len(frames)}. Frame pixels count: {len(frame['pixels'])}")
        for pixel_idx, pixel in enumerate(frame['pixels']):
            pixel_bytes = bytearray(pixel)
            all_pixel_data.extend(pixel_bytes)
            if verbose: print(f"save_raw_frames:   Added pixel {pixel_idx+1}. Current total bytes: {len(all_pixel_data)}")

    if verbose: print(f"save_raw_frames: Attempting to open file {output_filename} for writing. Full path: {os.path.abspath(output_filename)}")
    try:
        with open(output_filename, 'wb') as f:
            f.write(all_pixel_data)
            if verbose: print(f"save_raw_frames: Successfully wrote {len(all_pixel_data)} bytes to {output_filename}.")
    except IOError as e:
        print(f"Error writing raw frames to file: {e}")
        return

    if verbose: print(f"save_raw_frames: Saved raw frames to {output_filename}")

def live_view(host, layout="flat"):
    """
    Displays a live, real-time view of the device output.
    """
    import pygame
    import math

    PIXEL_GAP = 1
    DEFAULT_PIXEL_SCALE = 10

    with ColorClient(host) as client:
        # Get the first frame to determine the matrix size
        print("Waiting for first frame to determine matrix size...")
        frames = client.capture_frames(duration_seconds=1) # Capture a short moment to get a frame
        if not frames:
            print("Could not get a frame from the device.")
            return

        first_frame = frames[0]
        matrix_width = first_frame['width']
        matrix_height = first_frame['height']

        is_hex_display = (matrix_width * matrix_height == 271)
        hex_n = 10 # For 271 pixels, the hexagon side length is 10

        pygame.init()

        if is_hex_display:
            HEX_RADIUS = 20  # Adjust this to change the size of the hexagons

            # Generate axial coordinates for a hexagon of radius n-1
            hex_coords = []
            R = hex_n - 1
            for q in range(-R, R + 1):
                r1 = max(-R, -q - R)
                r2 = min(R, -q + R)
                for r in range(r1, r2 + 1):
                    hex_coords.append((q, r))

            if layout == "pointy":
                # Sort coordinates to match the "upper-left" start order (flat-top)
                # Sort by y (2r+q), then by x (q)
                hex_coords.sort(key=lambda c: (2 * c[1] + c[0], c[0]))

                # Function to get screen coordinates from axial coordinates (flat-top)
                def axial_to_screen(q, r, radius):
                    x = radius * 3/2 * q
                    y = radius * math.sqrt(3) * (r + q/2)
                    return (x, y)

                # Function to draw a flat-top hexagon
                def get_hex_points(x, y, radius):
                    return [
                        (x + radius * math.cos(math.radians(angle)), y + radius * math.sin(math.radians(angle)))
                        for angle in range(0, 360, 60)
                    ]
            else: # flat layout (default)
                # Sort coordinates for a pointy-top layout.
                # This sorts by row (r), then by column (q).
                hex_coords.sort(key=lambda c: (c[1], c[0]))

                # Function to get screen coordinates from axial coordinates (pointy-top)
                def axial_to_screen(q, r, radius):
                    x = radius * math.sqrt(3) * (q + r / 2.0)
                    y = radius * 3.0/2.0 * r
                    return (x, y)

                # Function to draw a pointy-top hexagon
                def get_hex_points(x, y, radius):
                    return [
                        (x + radius * math.cos(math.radians(angle)), y + radius * math.sin(math.radians(angle)))
                        for angle in range(30, 390, 60)
                    ]

            # Calculate screen size needed
            screen_coords = [axial_to_screen(q, r, HEX_RADIUS) for q, r in hex_coords]
            min_x = min(c[0] for c in screen_coords)
            max_x = max(c[0] for c in screen_coords)
            min_y = min(c[1] for c in screen_coords)
            max_y = max(c[1] for c in screen_coords)

            screen_width = int(max_x - min_x + 2.5 * HEX_RADIUS)
            screen_height = int(max_y - min_y + 2.5 * HEX_RADIUS)

            offset_x = -min_x + 1.5 * HEX_RADIUS
            offset_y = -min_y + 1.5 * HEX_RADIUS

        else: # Original rectangular layout logic
            display_info = pygame.display.Info()
            max_screen_width = display_info.current_w - 50 # 50px buffer

            pixel_scale_x = DEFAULT_PIXEL_SCALE
            if matrix_width * (pixel_scale_x + PIXEL_GAP) > max_screen_width:
                pixel_scale_x = int(max_screen_width / matrix_width) - PIXEL_GAP
                if pixel_scale_x < 1: pixel_scale_x = 1

            pixel_scale_y = pixel_scale_x
            if matrix_height == 1:
                pixel_scale_y = 100

            screen_width = matrix_width * (pixel_scale_x + PIXEL_GAP) - PIXEL_GAP
            screen_height = matrix_height * (pixel_scale_y + PIXEL_GAP) - PIXEL_GAP

        screen = pygame.display.set_mode((screen_width, screen_height))
        pygame.display.set_caption("NightDriver Live View")
        clock = pygame.time.Clock()

        running = True
        while running:
            for event in pygame.event.get():
                if event.type == pygame.QUIT:
                    running = False

            latest_frames = client.capture_frames(duration_seconds=0.1)
            if not latest_frames:
                continue

            frame = latest_frames[-1]
            screen.fill((0, 0, 0))

            if is_hex_display:
                if len(frame['pixels']) == 271:
                    for i, (q, r) in enumerate(hex_coords):
                        color = frame['pixels'][i]
                        x, y = axial_to_screen(q, r, HEX_RADIUS)
                        points = get_hex_points(x + offset_x, y + offset_y, HEX_RADIUS - PIXEL_GAP)
                        pygame.draw.polygon(screen, color, points)
            else:
                for y in range(matrix_height):
                    for x in range(matrix_width):
                        pixel_index = y * matrix_width + x
                        if pixel_index < len(frame['pixels']):
                            color = frame['pixels'][pixel_index]
                            rect = pygame.Rect(
                                x * (pixel_scale_x + PIXEL_GAP),
                                y * (pixel_scale_y + PIXEL_GAP),
                                pixel_scale_x,
                                pixel_scale_y
                            )
                            pygame.draw.rect(screen, color, rect)

            pygame.display.flip()
            clock.tick(60)

    pygame.quit()

def backup_configuration(client, output_filename):
    """
    Saves the device configuration to a JSON file.
    """
    print(f"Backing up configuration to {output_filename}...")

    config = {}
    config['settings'] = client.get_settings()

    effects_data = client.get_effects()
    if effects_data and 'Effects' in effects_data:
        config['effects'] = effects_data['Effects']
        for i, effect in enumerate(config['effects']):
            effect['settings'] = client.get_effect_settings(i)

    with open(output_filename, 'w') as f:
        json.dump(config, f, indent=2)

    print("Backup complete.")

def restore_configuration(client, input_filename):
    """
    Restores the device configuration from a JSON file.
    """
    print(f"Restoring configuration from {input_filename}...")

    try:
        with open(input_filename, 'r') as f:
            config = json.load(f)
    except FileNotFoundError:
        print(f"Error: Backup file not found at {input_filename}")
        return
    except json.JSONDecodeError:
        print(f"Error: Could not decode JSON from {input_filename}")
        return

    if 'settings' in config:
        print("Restoring device settings...")
        client.set_settings(config['settings'])

    if 'effects' in config:
        print("Restoring effects...")
        # This is a simplified restore. A more robust implementation might need to handle
        # deleting, adding, and reordering effects to match the backup perfectly.
        for i, effect_config in enumerate(config['effects']):
            if 'settings' in effect_config:
                print(f"  Restoring settings for effect {i}...")
                client.set_effect_settings(i, effect_config['settings'])

    print("Restore complete.")

def generate_gallery(gif_files=None):
    """
    Generates an HTML gallery from captured GIF files.
    If gif_files is provided, it uses that list. Otherwise, it scans the current directory.
    """
    import os
    print("Generating effect gallery...")

    if gif_files is None:
        print("Scanning for all .gif files in the current directory...")
        gif_files = [f for f in os.listdir('.') if f.endswith('.gif')]
    else:
        print(f"Using {len(gif_files)} freshly captured GIFs for the gallery...")

    if not gif_files:
        print("No GIF files found to generate a gallery.")
        return

    html_content = """
    <!DOCTYPE html>
    <html lang=\"en\">
    <head>
        <meta charset=\"UTF-8\">
        <meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\">
        <title>NightDriver Effect Gallery</title>
        <style>
            body { font-family: sans-serif; background-color: #f0f0f0; margin: 20px; }
            h1 { text-align: center; color: #333; }
            .gallery { display: flex; flex-wrap: wrap; justify-content: center; gap: 20px; }
            .effect { background-color: #fff; border: 1px solid #ddd; border-radius: 8px; padding: 15px; text-align: center; width: 300px; box-shadow: 0 2px 4px rgba(0,0,0,0.1); }
            .effect h2 { margin: 0 0 10px 0; font-size: 1.2em; color: #555; }
            .effect img { min-height: 40px; max-width: 100%; height: auto; border-radius: 4px; }
        </style>
    </head>
    <body>
        <h1>NightDriver Effect Gallery</h1>
        <div class=\"gallery\">
    """

    for gif_file in sorted(gif_files):
        effect_name = os.path.splitext(gif_file)[0].replace('_', ' ')
        html_content += f"""
            <div class=\"effect\">
                <h2>{effect_name}</h2>
                <img src=\"{gif_file}\" alt=\"{effect_name}\">
            </div>
        """

    html_content += """
        </div>
    </body>
    </html>
    """

    with open("index.html", 'w') as f:
        f.write(html_content)

    print("Gallery created successfully. Open index.html in your browser.")


def main():
    print(f"main: Current working directory: {os.getcwd()}")
    parser = argparse.ArgumentParser(description="A command-line client for NightDriver devices.")
    parser.add_argument("host", help="The IP address or hostname of the NightDriver device.")
    parser.add_argument("--rest-port", type=int, default=80, help="The port of the NightDriver REST API (default: 80).")
    parser.add_argument("--effects", action="store_true", help="Get the list of effects.")
    parser.add_argument("--settings", action="store_true", help="Get the current device settings.")
    parser.add_argument("--next", action="store_true", help="Switch to the next effect.")
    parser.add_argument("--prev", action="store_true", help="Switch to the previous effect.")
    parser.add_argument("--set-effect", type=int, metavar="INDEX", help="Set the current effect to the specified index.")
    parser.add_argument("--set-brightness", type=int, metavar="VALUE", help="Set the device brightness (0-255).")
    parser.add_argument("--capture", type=str, metavar="EFFECT_INDEX_OR_NAME", help="Capture an effect and save it as a GIF.")
    parser.add_argument("--duration", type=int, default=5, metavar="SECONDS", help="Duration to capture the effect in seconds (default: 5).")
    parser.add_argument("--output", default="effect_capture.gif", metavar="FILENAME", help="Output filename for the captured GIF (default: effect_capture.gif).")
    parser.add_argument("--scale", type=int, metavar="FACTOR", help="Scale factor for the output GIF (e.g., 8 for 8x). Default: auto-scale if width or height < 256.")
    parser.add_argument("--capture-all", action="store_true", help="Capture all effects and save them as GIFs.")
    parser.add_argument("--live-view", action="store_true", help="Display a live, real-time view of the device output.")
    parser.add_argument("--hex-layout", type=str, default="flat", choices=["pointy", "flat"], help="Specify the hexagon layout for live view (pointy or flat). Default: flat.")
    parser.add_argument("--backup", metavar="FILENAME", help="Save the device configuration to a JSON file.")
    parser.add_argument("--restore", metavar="FILENAME", help="Restore the device configuration from a JSON file.")
    parser.add_argument("--generate-gallery", action="store_true", help="Generate an HTML gallery from captured GIFs.")
    parser.add_argument("--verbose", action="store_true", help="Enable verbose output for debugging.")


    args = parser.parse_args()

    client = NightDriver(args.host, port=args.rest_port)

    captured_files = []

    if args.effects:
        print("Getting effects...")
        effects = client.get_effects()
        if effects:
            print(json.dumps(effects, indent=2))

    if args.settings:
        print("Getting settings...")
        settings = client.get_settings()
        if settings:
            print(json.dumps(settings, indent=2))

    if args.next:
        print("Switching to next effect...")
        client.next_effect()

    if args.prev:
        print("Switching to previous effect...")
        client.previous_effect()

    if args.set_effect is not None:
        print(f"Setting effect to index {args.set_effect}...")
        client.set_current_effect(args.set_effect, width=16, height=16)

    if args.capture is not None:
        effect_to_capture = None
        try:
            effect_index = int(args.capture)
            effect_to_capture = effect_index
        except ValueError:
            # Not a number, so it must be a name
            effects_data = client.get_effects()
            if effects_data and 'Effects' in effects_data:
                for i, effect in enumerate(effects_data['Effects']):
                    if args.capture.lower() in effect.get('name', '').lower():
                        effect_to_capture = i
                        break
            if effect_to_capture is None:
                print(f"Error: Could not find an effect with the name '{args.capture}'")

        if effect_to_capture is not None:
            print(f"Capturing effect {effect_to_capture} to {args.output} for {args.duration} seconds...")
            client.set_current_effect(effect_to_capture, width=16, height=16)
            with ColorClient(args.host, verbose=args.verbose) as color_client:
                frames = color_client.capture_frames(args.duration)

            if frames:
                create_animated_gif(frames, args.output, scale=args.scale, verbose=args.verbose)
                raw_filename = args.output.replace('.gif', '.raw')
                save_raw_frames(frames, raw_filename, verbose=args.verbose)
                captured_files.append(args.output)

    elif args.capture_all:
        print("Capturing all effects...")
        effects_data = client.get_effects()
        if effects_data and 'Effects' in effects_data:
            for i, effect in enumerate(effects_data['Effects']):
                effect_name = effect.get('name', f'effect_{i}')
                sanitized_name = "".join(c if c.isalnum() else '_' for c in effect_name)
                output_filename = f"{sanitized_name}.gif"

                print(f"Capturing effect: {effect_name} to {output_filename}")
                client.set_current_effect(i, width=16, height=16)

                with ColorClient(args.host, verbose=args.verbose) as color_client:
                    frames = color_client.capture_frames(args.duration)

                if frames:
                    create_animated_gif(frames, output_filename, scale=args.scale, verbose=args.verbose)
                    raw_filename = output_filename.replace('.gif', '.raw')
                    save_raw_frames(frames, raw_filename, verbose=args.verbose)
                    captured_files.append(output_filename)

    if args.live_view:
        live_view(args.host, args.hex_layout)

    if args.backup:
        backup_configuration(client, args.backup)

    if args.restore:
        restore_configuration(client, args.restore)

    if args.generate_gallery:
        generate_gallery(captured_files if captured_files else None)

if __name__ == '__main__':
    # This script requires the 'requests' and 'Pillow' libraries.
    # You can install them with: pip install requests Pillow
    main()
