# CLAUDE.md
This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## What This Project Is
NightDriverStrip is an open-source ESP32/Arduino firmware platform, built with PlatformIO, for turning addressable LEDs and RGB matrices into configurable, networkable visual displays. It supports several output transports and physical layouts: WS281x/WS2812B-style strips across up to eight channels, APA102-style clocked LEDs, SK6812 RGBW variants in selected builds, and HUB75 matrix panels through SmartMatrix. The same codebase is compiled into many project-specific firmware targets, including simple strip demos, WiFi strip receivers, M5Stick and M5Stack spectrum analyzers, Heltec and LilyGO display builds, Wrover and ESP32-S3 devices, the Mesmerizer HUB75 panel, lanterns, fans, trees, cubes, mirrors, hexagons, and other LED sculpture installations.

At its core, the project is a real-time graphics and effects engine. A shared `GFXBase` abstraction exposes drawing operations, while target-specific implementations such as `WS281xGFX`, APA102 output managers, and `HUB75GFX` handle the actual LED hardware. Visual behavior is organized around `LEDStripEffect` / `LEDMatrixEffect` subclasses and an `EffectManager`, which loads, rotates, configures, persists, enables, disables, copies, moves, and renders effects. The built-in catalog covers classic strip animations like fire, meteors, bouncing balls, twinkles, palettes, stars, fireworks, warm/cool white effects, and fan/ring patterns, plus matrix-oriented clocks, GIF/JPEG/QR rendering, weather and information panels, stock and subscriber views, noise fields, wave/swirl/spiro/radar/cube/life/maze patterns, and numerous audio-reactive spectrum or beat-driven effects.

NightDriverStrip can run autonomously, but its larger purpose is connected lighting. WiFi-enabled builds expose an embedded web UI and a REST-like API for selecting effects, changing order, enabling/disabling items, editing settings, reading device statistics, resetting configuration, and changing runtime device options. WebSockets can stream effect and color updates to browser clients. A raw TCP socket server can also receive timestamped LED frame packets, optionally LZ-compressed, so an external server or controller can deliver precise color data over the network. NTP synchronization keeps multiple ESP32 devices aligned, enabling coordinated shows across distributed strips or panels. When no remote frames arrive, the device falls back to local effects after a configured timeout.

The firmware is structured as a multitask embedded application. Separate tasks handle rendering, network maintenance, socket input, web serving, audio sampling and FFT/spectral analysis, status display updates, JSON persistence, telnet/serial debug console, optional IR remote control, and OTA updates. Device and effect configuration are stored in compact JSON on the ESP32 filesystem, while PlatformIO build flags define hardware topology, LED counts, pins, matrix dimensions, color order, power limits, WiFi, NTP, OTA, web UI, audio, and remote capabilities. In short, NightDriverStrip is a configurable ESP32 LED-display stack: visual-effects engine, audio visualizer, WiFi frame receiver, browser-managed device, and firmware framework for synchronized strips, matrices, rings, fans, and custom light sculptures.

## Core Coding Philosophy
**Produce tight, clean, small, and elegant code.**  
- **Small & Efficient**: Prefer minimal code size and memory usage. Embedded constraints matter — avoid unnecessary allocations, copies, or abstractions.  
- **Clean & Readable**: Code should be obvious at a glance to experienced embedded C++ developers.  
- **Elegant**: Simple, expressive solutions that avoid cleverness for its own sake.  
- **Well Commented**: Comment anything non-self-evident (e.g., why a particular algorithm or bit-twiddling trick was chosen, hardware timing quirks, trade-offs made for performance/size, or subtle FreeRTOS interactions). Do not comment obvious code.  
- **C++20 & STL**: Use modern C++20 features and STL containers/algorithms freely **when they improve clarity, safety, or performance without adding bloat or obfuscation**. Examples: `std::array`, `std::span`, `std::optional`, `constexpr`, structured bindings, range-based loops, and algorithms like `std::fill`, `std::copy`, `std::transform`. Avoid complex templates, heavy metaprogramming, or STL features that expand code size significantly on ESP32. Fall back to raw arrays/C-style constructs when they are clearer or smaller for less experienced readers.  
- **Consistency**: Match the style of the surrounding file/function. The codebase mixes camelCase, some Hungarian notation, and Win32 conventions — local consistency wins. Use `.clang-format` (Microsoft style) for formatting.  If you spot the same thing being done in two different ways in two places, try to unify them or at least call it out.

## Build System
The project uses **PlatformIO**. Build environments are defined in `platformio.ini`.

```sh
# List available build environments
python tools/show_envs.py
# Build a specific environment
pio run -e mesmerizer
# Build all environments
python tools/build_all.py
# Upload to device
pio run -e mesmerizer -t upload
# Monitor serial output
pio device monitor
```

Key environments: `mesmerizer`, `demo`, `ledstrip`, `spectrum`, `m5demo`, `heltecdemo`.

The web UI assets in `site/` are gzip-compressed and baked into flash before building — this happens automatically via a PlatformIO pre-build script that runs `tools/bake_site.py`.

## Linting & Code Quality
```sh
# C++ formatting (Microsoft style, defined in .clang-format)
clang-format -i <file>
# Frontend linting (ESLint, 4-space indent)
npx eslint site/app.js
# Python tools for code audit
python tools/pio_audit.py          # soft-fail
python tools/pio_audit.py --hard-fail  # strict
# Audit header include ordering
python tools/audit_include_rules.py
# Syntax check
python tools/check_syntax.py
```

## Testing
There is no unit test framework. Integration-level tests are run by the CI pipeline, which builds all PlatformIO environments. The CI matrix is driven by `.github/workflows/CI.yml` and `config/ci_exclude.json`.

For API testing, a Postman collection is in `tools/`.  
Topology validation: `python tools/test_topology.py`

## Code Architecture
### Core Abstraction Layers
- **Graphics layer** (`include/gfxbase.h`, `include/hub75gfx.h`): Base drawing primitives used by all effects. `GFXBase` wraps FastLED and provides coordinate-mapped pixel operations. `HUB75Gfx` extends this for matrix panels.
- **Effect base class** (`include/ledstripeffect.h`): All visual effects inherit from `LEDStripEffect` (strip) or `LEDMatrixEffect` (matrix). The key virtual method is `Draw()`, called each frame.
- **Effect manager** (`include/effectmanager.h`, `src/effectmanager*.cpp`): Manages the active effect lifecycle — selection, timing, transitions, and persistence. Effects are serialized to SPIFFS as JSON via ArduinoJson.
- **Device config** (`include/deviceconfig.h`, `src/deviceconfig*.cpp`): Persists device-level settings (brightness, wifi, etc.) as JSON. Uses the `DECLARE_DEVICE_SETTING` / `DEFINE_DEVICE_SETTING` macro pattern.
- **Interfaces** (`include/interfaces.h`): Abstract interfaces for services (audio, network, etc.) that effects can call — decouples effects from concrete implementations.
- **Globals** (`include/globals.h`): Feature flag defines, thread handles, shared state. Feature capabilities (WiFi, audio, screen, etc.) are enabled via `#define ENABLE_*` here or in `platformio.ini` `build_flags`.

### Effects
Effects live in:
- `include/effects/strip/` — LED strip effects
- `include/effects/matrix/` — HUB75 matrix effects

Effects are registered in `src/effects.cpp` via `LoadEffectFactories()`. Each effect class is instantiated by a factory lambda stored there. Effect state is auto-serialized; implement `SerializeToJSON()` / `DeserializeFromJSON()` only when needed for persistent settings.

### Threading Model
The firmware is multi-threaded (FreeRTOS). The main draw loop runs in `src/drawing.cpp`. Audio capture/FFT runs in `src/audio.cpp` / `src/audioservice.cpp`. Networking and the web server run on separate tasks. Shared state is protected via mutexes; `include/globals.h` defines the thread coordination primitives.

### Web UI
`site/app.js` is a vanilla JavaScript app (no framework, no build step required for editing). It talks to the device's REST API (documented in `REST_API.md`). The UI is served directly from the ESP32's flash.

### Configuration vs. Feature Flags
- **Feature flags** (`#define ENABLE_WIFI`, `ENABLE_AUDIO`, etc.) — compile-time inclusion of subsystems.
- **Runtime settings** — persisted to SPIFFS as JSON, managed through `DeviceConfig`.

## Adding a New Effect
1. Create a header in `include/effects/strip/` or `include/effects/matrix/`.
2. Derive from `LEDStripEffect` (or `LEDMatrixEffect`), implement `Draw()`.
3. Register a factory in `src/effects.cpp` inside `LoadEffectFactories()`.
4. Keep the implementation **tight and elegant**. Prefer `std::array`/`constexpr` where helpful. Add comments for non-obvious logic (e.g., palette math, performance tricks, or audio-reactive scaling).

## Key Reference Docs in the Repo
- `CODEBASE_INTRO.md` — architecture walkthrough
- `REST_API.md` — full REST API reference
- `CONTRIBUTING.md` — contribution guidelines and the "BlinkenPerBit" philosophy
- `UISpec.md` — web UI design specification

**When in doubt**: Make it smaller, cleaner, and more elegant. Favor clarity for the average embedded hobbyist while embracing modern C++ where it genuinely helps.
