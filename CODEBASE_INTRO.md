# Introduction to the NightDriverStrip codebase: The Mesmerizer Edition <!-- omit in toc -->

## Table of contents <!-- omit in toc -->

- [Welcome!](#welcome)
- [Key components to focus on](#key-components-to-focus-on)
  - [The core: `GFXBase`](#the-core-gfxbase)
  - [`LEDMatrixGfx`](#ledmatrixgfx)
  - [`EffectManager`](#effectmanager)
  - [`LEDStripEffect`](#ledstripeffect)
  - [Device configuration](#device-configuration)
  - [Networking](#networking)
  - [Remote Control](#remote-control)
- [Major threads within the application](#major-threads-within-the-application)
- [Main process flow](#main-process-flow)
  - [Code Startup](#code-startup)
  - [Event loop](#event-loop)
  - [Main drawing loop](#main-drawing-loop)
- [Memory management and performance](#memory-management-and-performance)
- [Testing and validation](#testing-and-validation)
- [Collaboration and documentation](#collaboration-and-documentation)
- [Further reading](#further-reading)

<!-- markdownlint-disable-next-line MD026 no-trailing-punctuation -->
## Welcome!

Welcome to the NightDriverStrip codebase, a project dedicated to creating visually stunning LED displays. You can read this introduction if you're diving into this codebase with the intent to enhance, modify, or simply understand the effects encapsulated within the Mesmerizer project configuration.

This introduction aims to provide an overview of the key components, design decisions, and architectural characteristics of the project. This will help you quickly develop an understanding of the NightDriverStrip codebase.

As mentioned, the guide takes the Mesmerizer project configuration as the starting point. The reason is that it's the configuration with most features enabled and the largest number of effects. Those effects further include the most complex and visually appealing ones. That said, it does not mean this introduction can't be used if you're working on or with other project configurations. It does mean not all of the contents of this document may be relevant to you.

The text in this document regularly mentions "LED display" as the visualization device. In the case of Mesmerizer that translates to a HUB75 LED panel. In case of other devices/projects, the actual device will be a WS2812-variant LED strip or matrix.

## Key components to focus on

### The core: `GFXBase`

- Located in: [gfxbase.h](./include/gfxbase.h)
- Role: Base class for all graphical operations
- Key functionalities: Drawing pixels, setting colors, managing graphical objects chain.
- Explanation: The `GFXBase` class lies at the heart of the codebase. It provides the basic functionalities required for any graphical operation, and is thus the basis for all graphical classes and effects. Familiarizing yourself with its methods and properties will give you a solid understanding of the basic operations like drawing pixels, setting colors, and managing the chain of graphical objects.

### `LEDMatrixGfx`

- Located in: [ledmatrixgfx.h](./include/ledmatrixgfx.h) and [ledmatrixgfx.cpp](./src/ledmatrixgfx.cpp)
- Role: Extends `GFXBase` for matrix-style LED arrangements.
- Key functionalities: Matrix-specific drawing, dimension management.
- Explanation: This class extends the functionalities of [`GFXBase`](#the-core-gfxbase) to cater to matrix-style LED arrangements. It provides methods to draw on a matrix, manage its dimensions, and handle matrix-specific effects.

### `EffectManager`

- Located in: [effectmanager.h](./include/effectmanager.h) and [effectmanager.cpp](./src/effectmanager.cpp)
- Role: Handles the creation and management of visual effects on the LED display.
- Key functionalities: Creating effects, managing effect transitions.

### `LEDStripEffect`

- Located in: [ledstripeffect.h](./include/ledstripeffect.h) and [ledstripeffect.cpp](./src/ledstripeffect.cpp)
- Role: Base class for all effects.
- Key functionalities: Base facilities for dictating LED behavior.
- Explanation: This class is the base class for all visual effects that are displayed on the LED display. Each effect is a combination of algorithms that dictate how individual LEDs light up, change color, and transition. Understanding the methods in this class is crucial if you aim to add new effects or modify existing ones.

### Device configuration

- Located in: [deviceconfig.h](./include/deviceconfig.h) and [deviceconfig.cpp](./src/deviceconfig.cpp)
- Role: Holds device-wide configuration settings.
- Key functionalities: Loading, saving, and accessing configurations.
- Explanation: The `DeviceConfig` class holds essential configuration settings for the device. It's important to understand how configurations are loaded, saved, and accessed as they influence how certain effects are rendered.

### Networking

- Located in: [network.h](./include/network.h),  [network.cpp](./src/network.cpp), [socketserver.h](./include/socketserver.h), [socketserver.cpp](./src/socketserver.cpp), [webserver.h](./include/webserver.h) and [webserver.cpp](./src/webserver.cpp)
- Role: Handling networking functionalities.
- Key functionalities: Data transmission and reception.
- Explanation: The codebase provides functionalities for outgoing and incoming data via a WiFI network, using raw and web protocols. Familiarize yourself with the mentioned files to understand how data is transmitted, received, and processed.

### Remote Control

- Located in: [remotecontrol.h](./include/remotecontrol.h) and [remotecontrol.cpp](./src/remotecontrol.cpp)
- Role: Enables real-time user interaction with the LED display.
- Key functionalities: Processing remote commands.
- Explanation: Support is included for IR remote controls, allowing users to interact with the LED display in real-time.

## Major threads within the application

Note that the following list is not exhaustive; only the key threads are discussed here.

- **Main thread** (from [main.cpp](./src/main.cpp)): Initializes the hardware and software components. Starts the drawing, audio, network, and other related threads.
  Enters the main `loop()` function, which logs the system status at regular intervals.

- **Drawing thread** (from [drawing.cpp](./src/drawing.cpp)): Enters the main drawing loop. This manages the continuous updates to the LED display, by whatever is the currently active effect. Also handles the visualizations related to effect transitions.

- **Audio thread** (from [audio.cpp](./src/audio.cpp)): Responsible for audio processing.
  Captures audio input, processes it, and then uses the processed data to influence the LED animations.

- **Network thread** (from [network.cpp](./src/network.cpp)): Handles core network-related tasks.
  Sets up and monitors the WiFi connection. Runs the main network loop that executes network tasks that are scheduled by effects or other functionalities.

- **Remote Control thread** (from [remotecontrol.cpp](./src/remotecontrol.cpp)): Manages remote control functionalities.
  Receives commands from a remote control device and processes them to control the LED animations or other functionalities.

- **Socket Server thread** (from [socketserver.cpp](./src/socketserver.cpp)): Manages incoming socket connections.
  Listens for incoming socket connections and processes data received from them.

- **Web Server thread** (from [webserver.cpp](./src/webserver.cpp)): Manages the web server functionalities.
  Serves web pages, listens for HTTP requests, and processes them to control the LED animations or provide information.

## Main process flow

### Code Startup

When the device initializes, within `setup()` in [main.cpp](./src/main.cpp), the major tasks are:

- **Hardware initialization**: The code starts by initializing the necessary hardware components. This includes setting up the LED strips, configuring GPIO pins, and ensuring that all connected devices (LED display and otherwise) are ready for operation.

- **Software component setup**: Memory allocations for various data structures and buffers are performed.
Configuration files and settings are read where available to determine the initial state or behavior of the application.

- **Thread creation**: Multiple threads are spun up to handle different functionalities concurrently. This includes the threads discussed in [the previous section](#major-threads-within-the-application).

- **Network setup**: If the application has networking capabilities enabled, it will initialize the network components. This includes connecting to WiFi and can involve setting up a socket server, initializing a web server, and establishing other network-related services.

- **Audio system initialization**: The audio system is set up to capture and process audio input. This is crucial for animations that react to sound or music.

- **Remote Control setup**: If the application has remote control support enabled, it will initialize the necessary components to listen for and process remote control commands.

- **Graphics system initialization**: The graphics system, responsible for driving the LED animations, is initialized. This involves setting up the necessary data structures, algorithms, and configurations to start drawing on the LED display.

- **Initial effect setup**: The first visual effect is set up and started. This ensures that as soon as the system is ready, there's immediate visual feedback on the LED display.

- **Loading the full effect set**: The complete effects list is loaded, either from a JSON file stored on the device's SPIFFS partition or the default set. The default set is determined by the effect factories that are added in the `LoadEffectFactories()` function in [effects.cpp](./src/effects.cpp).

### Event loop

Once all the initializations are complete, the application enters its main event loop. _**Note** that the event loop is not an actual function or separate thread._ Instead, it's implemented by the combination of threads that take in and process different types of input. In that, it continuously listens for events (like user inputs, network commands, or audio signals) and updates the LED animations accordingly.

### Main drawing loop

The `EffectManager` and the drawing loop work together to render the effects that have been enabled within the application.

- **`EffectManager`**: The `EffectManager` is a class that is present in the [effectmanager.h](./include/effectmanager.h), [effectmanager.cpp](./src/effectmanager.cpp) and, for a few methods and support functions, [effects.cpp](./src/effects.cpp) files. It  manages the LED effects by holding the list of effect objects that represent individual effects that can be shown on the LED display.
  The `EffectManager` has a method named `Update()`, which is responsible for drawing the current effect on the LED display. This method is called within the main draw loop to render the effect.

- **Drawing loop**: The drawing loop is present in the [drawing.cpp](./src/drawing.cpp) file. It's a continuous loop that updates the LED display with the current effect.
Within this loop, the `EffectManager`'s `Update()` method is called, which in turn calls the `Draw()` method of the current effect (object). This ensures that the effect is rendered on the LED display.

- **Effect classes**: Each of the effect classes, as present in the various files in the [include/effects](./include/effects/) subfolders, represents a single effect that can be applied to the LED display. All effect classes, including those targeting LED matrices, eventually derive from the `LEDStripEffect` class as found in [ledstripeffect.h](./include/ledstripeffect.h) and [ledstripeffect.cpp](./src/ledstripeffect.cpp).
Each effect has its own implementation of the `Draw()` method, which defines how the effect should be rendered on the LED display. This method is called by the `EffectManager` when it's time to render the effect.

## Memory management and performance

Given the real-time nature of LED displays, performance is paramount. The codebase makes extensive use of dynamic memory allocation, with a preference for PSRAM memory when available.
Be cautious when making changes that can impact memory usage - particularly when it concerns regular RAM. There just isn't that much of it available.

## Testing and validation

Before introducing new effects or making changes, it's important to test your modifications thoroughly. Given the visual nature of the project, testing on actual LED hardware is almost always a crucial step.

A good way to "smoke test" any change is compiling your version of the code for all project configurations. This can be done by running the following command from the project root directory:

```shell
python tools/build_all.py
```

## Collaboration and documentation

The codebase is a collaborative effort. As you navigate through it, you'll find comments and documentation that provide insights into design decisions, algorithm explanations, and usage guidelines. When making changes or additions, ensure you follow the existing coding conventions and document your work. This will make it easier for other developers to understand and build upon your contributions.

You may also find that some existing documentation is somewhat incomplete or no longer entirely accurate. In that case, please feel free to extend or correct such documentation. In the end, all improvements to documentation are valuable to the project.

## Further reading

Now that you've been introduced to the NightDriverStrip codebase, the next best thing to spend some time on reading is the NightDriverStrip source code! Browse through the source files mentioned in this document, and don't hold back on inspecting others as well. If you come across things that are hard to follow or don't seem to make sense, then please open a Q&A post in the [Discussions section](https://github.com/PlummersSoftwareLLC/NightDriverStrip/discussions) in this repository. The questions asked there will help others when answered, and can also provide pointers to gaps in the documentation that's currently in place.

Once you feel sufficiently comfortable with the code you're looking at, you can continue your journey with the [Getting started with the source code](./README.md#getting-started-with-the-source-code) section in the main README.
