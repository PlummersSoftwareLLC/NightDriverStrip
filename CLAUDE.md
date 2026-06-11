# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## What This Project Is

NightDriverStrip is open-source ESP32 firmware for driving LED strips and HUB75 LED matrices with visual effects. It supports audio-reactive effects (FFT analysis), WiFi remote control, a web UI, REST API, and OTA updates. The project targets various ESP32 hardware variants, with the Mesmerizer (HUB75 matrix) as a primary target.

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
python tools/pio_audit.py              # soft-fail
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

**Graphics layer** (`include/gfxbase.h`, `include/hub75gfx.h`): Base drawing primitives used by all effects. `GFXBase` wraps FastLED and provides coordinate-mapped pixel operations. `HUB75Gfx` extends this for matrix panels.

**Effect base class** (`include/ledstripeffect.h`): All visual effects inherit from `LEDStripEffect` (strip) or `LEDMatrixEffect` (matrix). The key virtual method is `Draw()`, called each frame.

**Effect manager** (`include/effectmanager.h`, `src/effectmanager*.cpp`): Manages the active effect lifecycle — selection, timing, transitions, and persistence. Effects are serialized to SPIFFS as JSON via ArduinoJson.

**Device config** (`include/deviceconfig.h`, `src/deviceconfig*.cpp`): Persists device-level settings (brightness, wifi, etc.) as JSON. Uses the `DECLARE_DEVICE_SETTING` / `DEFINE_DEVICE_SETTING` macro pattern.

**Interfaces** (`include/interfaces.h`): Abstract interfaces for services (audio, network, etc.) that effects can call — decouples effects from concrete implementations.

**Globals** (`include/globals.h`): Feature flag defines, thread handles, shared state. Feature capabilities (WiFi, audio, screen, etc.) are enabled via `#define ENABLE_*` here or in `platformio.ini` `build_flags`.

### Effects

Effects live in:

- `include/effects/strip/` — LED strip effects (~30 files: fire, music, particles, meteors, etc.)
- `include/effects/matrix/` — HUB75 matrix effects (~40 files: patterns, GIFs, clocks, QR codes, etc.)

Effects are registered in `src/effects.cpp` via `LoadEffectFactories()`. Each effect class is instantiated by a factory lambda stored there.

### Threading Model

The firmware is multi-threaded (FreeRTOS). The main draw loop runs in `src/drawing.cpp`. Audio capture/FFT runs in `src/audio.cpp` / `src/audioservice.cpp`. Networking and the web server run on separate tasks. Shared state is protected via mutexes; `include/globals.h` defines the thread coordination primitives.

### Web UI

`site/app.js` is a vanilla JavaScript app (no framework, no build step required for editing). It talks to the device's REST API (documented in `REST_API.md`). The UI is served directly from the ESP32's flash.

### Configuration vs. Feature Flags

- **Feature flags** (`#define ENABLE_WIFI`, `ENABLE_AUDIO`, etc.) in `globals.h` or per-environment `build_flags` in `platformio.ini` — control compile-time inclusion of subsystems.
- **Runtime settings** — persisted to SPIFFS as JSON, managed through `DeviceConfig`.

## Adding a New Effect

1. Create a header in `include/effects/strip/` or `include/effects/matrix/`.
2. Derive from `LEDStripEffect` (or `LEDMatrixEffect`), implement `Draw()`.
3. Register a factory in `src/effects.cpp` inside `LoadEffectFactories()`.
4. Effect state is auto-serialized; implement `SerializeToJSON()` / `DeserializeFromJSON()` if the effect has persistent settings.

## Code Style

Match the style of the file or function you are modifying. The codebase mixes camelCase, Hungarian notation, and Win32 naming conventions across different files — consistency within a file takes priority over global uniformity. C++20 features are available (`-std=c++2a`).

## Key Reference Docs in the Repo

- `CODEBASE_INTRO.md` — architecture walkthrough
- `REST_API.md` — full REST API reference
- `CONTRIBUTING.md` — contribution guidelines and the "BlinkenPerBit" philosophy
- `UISpec.md` — web UI design specification
