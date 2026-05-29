//+--------------------------------------------------------------------------
//
// File:        audioservice.cpp
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
//    Implementation of AudioService and AudioConfig. The file is structured
//    in three sections:
//
//      1. Shared support code (NullAudioSource, AudioConfig::ModeName) — no
//         preprocessor branches.
//      2. ENABLE_AUDIO == 1: real AudioService + the audio-enabled flavor of
//         AudioConfig::FromCompileDefaults / FromCurrentSettings.
//      3. ENABLE_AUDIO == 0: stub AudioService + minimal AudioConfig factories.
//
//    Method bodies inside each variant are unconditional, mirroring the
//    pattern used by SoundAnalyzer in soundanalyzer.h.
//
//---------------------------------------------------------------------------

#include "globals.h"

#include <algorithm>     // std::clamp, std::max — used in Run()
#include <cassert>       // assert()

#include "audioservice.h"
#include "deviceconfig.h"
#include "soundanalyzer.h"
#include "systemcontainer.h"
#include "taskmgr.h"     // AUDIO_STACK_SIZE / AUDIO_PRIORITY / AUDIO_CORE
#include "time_utils.h"  // FPS() — used by Run() to compute audio frame rate

// ===========================================================================
// 1. Shared support code (compiled in every build)
// ===========================================================================

namespace
{
    // The fallback source returned by Source() when audio is stopped or
    // unavailable. Implements ISoundAnalyzer so effects can call into it
    // harmlessly. The chosen "silent fallback" contract is:
    //
    //   VURatio / VURatioFade => 1.0f  (neutral — no scaling change)
    //   VU / PeakVU / MinVU   => 0.0f  (truly silent)
    //
    // This matches SoundAnalyzerBase::Reset(), which also sets the ratio
    // metrics to 1.0f. Effects that scale brightness/speed by VURatio treat
    // 1.0f as "no change", so both access paths (AudioService::Source() and
    // direct g_Analyzer reads) produce the same neutral behavior while audio
    // is stopped or being reconfigured.
    class NullAudioSource : public ISoundAnalyzer
    {
      public:
        // --- Core Processing ---
        void  RunSamplerPass() override {}
        void  SimulateBeatPass() override {}
        void  SetPeakDecayRates(float, float) override {}

        // --- Telemetry & Status ---
        int   AudioFPS() const override { return 0; }
        int   SerialFPS() const override { return 0; }
        bool  IsRemoteAudioActive() const override { return false; }

        // --- VU Metrics ---
        // VU/PeakVU/MinVU are zero (truly silent). VURatio and VURatioFade
        // are 1.0f (neutral) so effects that scale by these values are
        // unaffected, matching the behavior of SoundAnalyzerBase::Reset().
        float VU() const override { return 0.0f; }
        float VURatio() const override { return 1.0f; }
        float VURatioFade() const override { return 1.0f; }
        float PeakVU() const override { return 0.0f; }
        float MinVU() const override { return 0.0f; }

        // --- Spectral Data ---
        // The shared empty Peaks/Beat instances are static so every Null
        // source returns the same canonical empty value (and so the per-
        // instance footprint stays at zero).
        const PeakData& Peaks() const override { return EmptyPeaks(); }
        float Peak1Decay(int) const override { return 0.0f; }
        float Peak2Decay(int) const override { return 0.0f; }
        unsigned long LastPeak1Time(int) const override { return 0; }
        BeatInfo LastBeat() const override { return EmptyBeat(); }
        BeatInfo LastNearBeat() const override { return EmptyBeat(); }

        // --- Simulation & Testing ---
        void  SetSimulateBeat(bool) override {}
        void  SetSimulateBPM(int)  override {}
        bool  GetSimulateBeat() const override { return false; }
        int   GetSimulateBPM()  const override { return 0; }

      private:
        static const PeakData& EmptyPeaks()
        {
            static const PeakData s_empty{};
            return s_empty;
        }
        static const BeatInfo& EmptyBeat()
        {
            static const BeatInfo s_empty{};
            return s_empty;
        }
    };

    NullAudioSource& GetNullSource()
    {
        static NullAudioSource s_nullSource;
        return s_nullSource;
    }
}

// AudioConfig::ModeName is shared because the Mode enum is shared.
const char* AudioConfig::ModeName() const
{
    switch (mode)
    {
        case Mode::Disabled:    return "disabled";
        case Mode::AnalogADC:   return "adc";
        case Mode::I2SExternal: return "i2s";
        case Mode::M5Onboard:   return "m5_internal";
    }
    return "unknown";
}

// ===========================================================================
// 2. Audio-enabled variant
// ===========================================================================

#if ENABLE_AUDIO

// AudioConfig::FromCompileDefaults
//
// Translate the existing compile-time audio macros into a runtime AudioConfig
// so first boot looks exactly like before this refactor.
AudioConfig AudioConfig::FromCompileDefaults()
{
    AudioConfig cfg{};
    cfg.enabled    = true;
    cfg.sampleRate = 24000; // matches SoundAnalyzerBase::SAMPLING_FREQUENCY
    cfg.audioInputPin   = AUDIO_INPUT_PIN;

    #if USE_M5
        cfg.mode = AudioConfig::Mode::M5Onboard;
    #elif USE_I2S_AUDIO || ELECROW
        cfg.mode    = AudioConfig::Mode::I2SExternal;
        #ifdef I2S_BCLK_PIN
        cfg.bclkPin = I2S_BCLK_PIN;
        #endif
        #ifdef I2S_WS_PIN
        cfg.wsPin   = I2S_WS_PIN;
        #endif
        #ifdef I2S_DATA_PIN
        cfg.dataPin = I2S_DATA_PIN;
        #else
        cfg.dataPin = AUDIO_INPUT_PIN;
        #endif
    #else
        cfg.mode = AudioConfig::Mode::AnalogADC;
    #endif
    return cfg;
}

AudioConfig AudioConfig::FromCurrentSettings()
{
    AudioConfig cfg = FromCompileDefaults();

    // Overlay any persisted audio input pin from DeviceConfig if one is
    // available. -1 is the sentinel meaning "use the compile default";
    // anything >= 0 is a real GPIO and overrides.
    if (g_ptrSystem && g_ptrSystem->HasDeviceConfig())
    {
        const int persistedPin = g_ptrSystem->GetDeviceConfig().GetAudioInputPin();
        if (persistedPin >= 0)
            cfg.audioInputPin = persistedPin;
    }
    return cfg;
}

AudioService::AudioService()
{
    // Construction is metadata-only per the IService contract, but the config
    // needs to be in a sensible state from the start so callers can read
    // Config() before the first Start/Reconfigure.
    _config = AudioConfig::FromCompileDefaults();
}

AudioService::~AudioService()
{
    Stop();
}

IAudioSource& AudioService::Source() const
{
    if (IsRunning())
        return g_Analyzer;
    return GetNullSource();
}

// ---- ITaskService hooks ----

ITaskService::TaskConfig AudioService::GetTaskConfig() const
{
    return TaskConfig {
        "Audio Sampler Loop",
        AUDIO_STACK_SIZE,
        AUDIO_PRIORITY,
        AUDIO_CORE,
        750    // Stop timeout: I2S read uses a 100ms timeout, plus loop work.
    };
}

// OnBeforeStart - validate the config and emit start diagnostics. Returning
// false here aborts the launch (e.g. when audio is disabled in this run or
// the pin is not configured). The config itself is no longer mutated here:
// callers wanting the "current settings" semantics should call
// Reconfigure(AudioConfig::FromCurrentSettings()) before Start().
bool AudioService::OnBeforeStart()
{
    if (!_config.enabled)
    {
        debugI("Audio: Start() called but config marks audio disabled (pin=%d mode=%s)",
               _config.audioInputPin, _config.ModeName());
        return false;
    }

    debugI("Audio: starting (pin=%d mode=%s rate=%d)",
           _config.audioInputPin, _config.ModeName(), _config.sampleRate);
    return true;
}

// OnAfterStop - release the audio hardware and zero analyzer telemetry so
// direct readers of g_Analyzer see safe defaults during a reconfigure.
// Idempotent in both calls (TeardownAudioInput's _hardwareInstalled guard
// and Reset's unconditional zero-out).
void AudioService::OnAfterStop()
{
    g_Analyzer.TeardownAudioInput();
    g_Analyzer.Reset();
}

// AudioService::Run - the audio sampler loop. Initializes the I2S/ADC input
// on the task's own thread (some IDF drivers require that), then samples,
// processes peaks, and updates VU/ratio telemetry until ShouldShutdown()
// is true. ITaskService::TaskEntryThunk handles the post-Run vTaskDelete.
void AudioService::Run()
{
    debugI(">>> Sampler Task Started");

    // M5 boards sample through M5.Mic/M5Unified, not the generic AUDIO_INPUT_PIN
    // path. Only configure the input pin for the external mic configurations
    // that actually consume it.
    #if !USE_M5
    const auto audioInputPin = g_ptrSystem->GetConfiguredAudioInputPin();
    if (audioInputPin >= 0)
        pinMode(audioInputPin, INPUT);
    #endif

    g_Analyzer.InitAudioInput();

    auto lastVU = 0.0f;
    auto frameDurationSeconds = 0.016;
    constexpr auto VU_DECAY_PER_SECOND = 9.00;
    constexpr auto kMaxFPS = 60;

    while (!ShouldShutdown())
    {
        auto lastFrame = millis();

        g_Analyzer.RunSamplerPass();
        g_Analyzer.UpdatePeakData();
        g_Analyzer.DecayPeaks();

        // Fade out the VURatio
        if (g_Analyzer.VURatio() > lastVU)
            lastVU = g_Analyzer.VURatio();
        else
            lastVU -= frameDurationSeconds * VU_DECAY_PER_SECOND;

        g_Analyzer.SetVURatioFade(std::clamp(lastVU, 0.0f, 2.0f));

        // Instantaneous VURatio
        assert(g_Analyzer.PeakVU() >= g_Analyzer.MinVU());
        g_Analyzer.SetVURatio((g_Analyzer.PeakVU() == g_Analyzer.MinVU())
            ? 0.0f
            : (g_Analyzer.VU() - g_Analyzer.MinVU())
              / std::max(g_Analyzer.PeakVU() - g_Analyzer.MinVU(), (float) MIN_VU)
              * 2.0f);

        // Yield to share the CPU. We always wait at least kMinFrameDelay so
        // we don't bogart the core even when sampling is fast.
        const auto targetDelay = PERIOD_FROM_FREQ(kMaxFPS) * MILLIS_PER_SECOND / MICROS_PER_SECOND;
        delay(std::max(1.0, targetDelay - (millis() - lastFrame)));

        const auto duration = millis() - lastFrame;
        frameDurationSeconds = duration / 1000.0;
        g_Analyzer.SetAudioFPS(FPS(duration));
    }
}

// AudioService::Reconfigure - service-specific. Stops if running, applies the
// new config, and restarts when newConfig.enabled is true. The Start/Stop
// inherited from ITaskService own all the lifecycle discipline. Short-
// circuits when the requested config matches the current one and the
// service is already in the desired running state, so an unrelated
// settings PUT doesn't bounce a running sampler.

bool AudioService::Reconfigure(const AudioConfig& newConfig)
{
    debugI("Audio: reconfigure requested (pin=%d mode=%s enabled=%d)",
           newConfig.audioInputPin, newConfig.ModeName(), (int)newConfig.enabled);

    auto sameConfig = [](const AudioConfig& a, const AudioConfig& b)
    {
        return a.enabled    == b.enabled
            && a.mode       == b.mode
            && a.audioInputPin   == b.audioInputPin
            && a.bclkPin    == b.bclkPin
            && a.wsPin      == b.wsPin
            && a.dataPin    == b.dataPin
            && a.sampleRate == b.sampleRate;
    };

    if (sameConfig(_config, newConfig) && IsRunning() == newConfig.enabled)
    {
        debugI("Audio: reconfigure is a no-op (config and running state unchanged)");
        return true;
    }

    const bool wasRunning = IsRunning();
    if (wasRunning)
        Stop();

    _config = newConfig;

    if (!_config.enabled)
    {
        debugI("Audio: reconfigure leaves audio disabled");
        return true;
    }

    if (!Start())
    {
        debugE("Audio: reconfigure failed to start with new settings");
        return false;
    }

    debugI("Audio: reconfigure completed");
    return true;
}

// ===========================================================================
// 3. Audio-disabled stub variant
// ===========================================================================

#else // !ENABLE_AUDIO

AudioConfig AudioConfig::FromCompileDefaults()
{
    AudioConfig cfg{};
    cfg.enabled = false;
    cfg.mode    = AudioConfig::Mode::Disabled;
    return cfg;
}

AudioConfig AudioConfig::FromCurrentSettings()
{
    return FromCompileDefaults();
}

bool AudioService::Start()
{
    debugI("Audio: build has ENABLE_AUDIO=0, AudioService::Start is a no-op");
    return false;
}

bool AudioService::Reconfigure(const AudioConfig&)
{
    debugI("Audio: Reconfigure ignored, ENABLE_AUDIO=0");
    return false;
}

IAudioSource& AudioService::Source() const
{
    return GetNullSource();
}

#endif // ENABLE_AUDIO
