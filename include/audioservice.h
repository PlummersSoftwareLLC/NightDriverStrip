#pragma once

//+--------------------------------------------------------------------------
//
// File:        audioservice.h
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
// Description:
//
//    AudioService owns the audio sampler task and the I2S/ADC hardware
//    lifecycle. It can be started, stopped, restarted, and reconfigured at
//    runtime so changes to the audio input pin no longer require a reboot.
//
//    The compile-time audio mode (analog ADC, I2S external mic, M5 onboard)
//    is still selected at build time because the SoundAnalyzer template
//    binding and ESP-IDF driver paths are bound to that mode. The
//    runtime-mutable surface is the input pin and the enabled flag.
//    AudioConfig::FromCompileDefaults() reproduces the existing macro-driven
//    behavior so first boot looks exactly like before.
//
//    The global g_Analyzer instance remains the in-process source of audio
//    telemetry; AudioService just controls when its hardware is installed
//    and when its sampler task is running. When audio is stopped or
//    unavailable, AudioService::Source() returns a silent stub so effects
//    see VU/peaks/beats == zero rather than stale or undefined values.
//
// History:     May-04-2026         Davepl      Created
//
//---------------------------------------------------------------------------

#include "globals.h"

#include "itaskservice.h"   // includes iservice.h
#include "soundanalyzer.h"

// IAudioSource is a stable read-only interface effects can hold instead of
// reaching for the g_Analyzer global. It's a type alias of the existing
// ISoundAnalyzer interface so the migration is mechanical when we get to it.

using IAudioSource = ISoundAnalyzer;

// AudioConfig
//
// Runtime-mutable audio configuration. The mode/sampleRate/I2S pin fields
// are diagnostic and will not change live in this build target since the
// driver path is bound at compile time, but they're carried in the config
// so the SetupUI and Reconfigure() reflect a consistent picture.

struct AudioConfig
{
    enum class Mode
    {
        Disabled,           // ENABLE_AUDIO is 0 in this build
        AnalogADC,          // I2S-ADC built-in path (ESP32 only)
        I2SExternal,        // External digital mic (INMP441 etc.)
        M5Onboard           // M5Unified-managed mic
    };

    bool   enabled       = false;
    Mode   mode          = Mode::Disabled;
    int    inputPin      = -1;     // ADC pin, or I2S DIN pin in I2S mode. -1 disables hardware.
    int    bclkPin       = -1;     // I2S only
    int    wsPin         = -1;     // I2S only
    int    dataPin       = -1;     // I2S only (typically equal to inputPin)
    int    sampleRate    = 24000;  // SoundAnalyzerBase::SAMPLING_FREQUENCY

    // Build a config that mirrors the existing compile-time defaults.

    static AudioConfig FromCompileDefaults();

    // Overlay the persisted audio input pin (if a DeviceConfig is reachable)
    // onto a compile-defaults base. Used by Start() so the very first audio
    // session honors the user's last-saved pin without a manual Reconfigure.
    
    static AudioConfig FromCurrentSettings();

    // Human-readable mode name. Mirrors DeviceConfig::GetAudioInputModeName().
    
    const char* ModeName() const;
};

// AudioService
//
// Lifecycle manager for the audio engine. Construction is cheap; nothing is
// allocated or installed until Start() is called.
//
// Two complete class definitions live below, gated by ENABLE_AUDIO at file
// scope. The shape mirrors the SoundAnalyzer pattern in soundanalyzer.h: the
// public surface is identical between the two variants so callers don't need
// to #if their use sites, but each variant's method bodies are unconditional
// rather than dotted with inline preprocessor branches.

#if ENABLE_AUDIO

class AudioService : public ITaskService
{
  public:
    AudioService();
    ~AudioService() override;

    AudioService(const AudioService&) = delete;
    AudioService& operator=(const AudioService&) = delete;

    // IService::Name
    const char* Name() const override { return "AudioService"; }

    // Service-specific. Stops the audio engine if running, applies the new
    // config, and restarts if newConfig.enabled is true.
    bool Reconfigure(const AudioConfig& newConfig);

    // Audio-enabled builds always report available, regardless of running state.
    bool IsAvailable() const { return true; }

    const AudioConfig& Config() const { return _config; }

    // Read-only audio source. Always non-null. When audio is stopped or
    // unavailable, the returned source reports zero VU / silent peaks /
    // no beats so effects can run safely.
    IAudioSource& Source() const;

  protected:
    // ITaskService hooks
    TaskConfig GetTaskConfig() const override;
    bool OnBeforeStart() override;
    void Run() override;
    void OnAfterStop() override;

  private:
    AudioConfig _config{};
};

#else // !ENABLE_AUDIO

// Stub variant: same public surface, but every operation is a safe no-op and
// Source() returns the silent fallback. Callers that ask "is audio available?"
// get false; callers that try to Start get a clean false back rather than a
// runtime error.

class AudioService : public IService
{
  public:
    AudioService() = default;
    ~AudioService() override = default;

    AudioService(const AudioService&) = delete;
    AudioService& operator=(const AudioService&) = delete;

    bool Start() override;
    void Stop() override                          {}
    bool Restart() override                       { return false; }
    bool IsRunning() const override               { return false; }
    const char* Name() const override             { return "AudioService"; }

    bool Reconfigure(const AudioConfig& newConfig);

    bool IsAvailable() const                      { return false; }

    const AudioConfig& Config() const             { return _config; }

    IAudioSource& Source() const;

  private:
    AudioConfig _config{};
};

#endif // ENABLE_AUDIO
