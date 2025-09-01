//+--------------------------------------------------------------------------
//
// File:        effects.cpp
//
// NightDriverStrip - (c) 2023 Plummer's Software LLC.  All Rights Reserved.
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
//    Initializer/loader and support functions/macros for effects
//
// History:     Jul-14-2021         Davepl      Split off from main.cpp
//              Sep-26-2023         Rbergen     Extracted EffectManager stuff
//---------------------------------------------------------------------------

// Ensure Adafruit font types are seen before any potential LGFX aliasing from M5Unified
#include <gfxfont.h>
#include <Adafruit_GFX.h>

#include "effectsupport.h"

// Include the effect classes we'll need later

#include "effects/strip/fireeffect.h"           // fire effects
#include "effects/strip/paletteeffect.h"        // palette effects
#include "effects/strip/doublepaletteeffect.h"  // double palette effect
#include "effects/strip/meteoreffect.h"         // meteor blend effect
#include "effects/strip/stareffect.h"           // star effects
#include "effects/strip/bouncingballeffect.h"   // bouncing ball effects
#include "effects/strip/tempeffect.h"
#include "effects/strip/laserline.h"
#include "effects/strip/misceffects.h"
#include "effects/matrix/PatternClock.h"        // No matrix dependencies

#if ENABLE_AUDIO
    #include "effects/matrix/spectrumeffects.h" // Musis spectrum effects
    #include "effects/strip/musiceffect.h"      // Music based effects
#endif

#if FAN_SIZE
    #include "effects/strip/faneffects.h"       // Fan-based effects
#endif

//
// Externals
//

#if USE_HUB75
    #include "ledmatrixgfx.h"
    #include "effects/matrix/PatternMandala.h"    
    // These effects require LEDMatrixGFX::getPolarMap()
    #include "effects/matrix/PatternSMHypnosis.h"
    #include "effects/matrix/PatternSMRainbowTunnel.h"
    #include "effects/matrix/PatternSMRadialWave.h"
    #include "effects/matrix/PatternSMRadialFire.h"

    #if USE_NOISE
        #include "effects/matrix/PatternNoiseSmearing.h"
        #include "effects/matrix/PatternSMSmoke.h"
    #endif

#endif

#if USE_MATRIX

    #if ENABLE_WIFI
        #include "effects/matrix/PatternSubscribers.h"
        #include "effects/matrix/PatternWeather.h"
        #include "effects/matrix/PatternStocks.h"
    #endif

    #include "effects/matrix/PatternPongClock.h"
    #include "effects/matrix/PatternAnimatedGIF.h"  
    #include "effects/matrix/PatternSMStarDeep.h"
    #include "effects/matrix/PatternSMAmberRain.h"
    #include "effects/matrix/PatternSMBlurringColors.h"
    #include "effects/matrix/PatternSMFire2021.h"
    #include "effects/matrix/PatternSMNoise.h"
    #include "effects/matrix/PatternSMPicasso3in1.h"
    #include "effects/matrix/PatternSMSpiroPulse.h"
    #include "effects/matrix/PatternSMTwister.h"
    #include "effects/matrix/PatternSMMetaBalls.h"
    #include "effects/matrix/PatternSMHolidayLights.h"
    #include "effects/matrix/PatternSMGamma.h"
    #include "effects/matrix/PatternSMFlowFields.h"
    #include "effects/matrix/PatternSMSupernova.h"
    #include "effects/matrix/PatternSMWalkingMachine.h"
    #include "effects/matrix/PatternPongClock.h"
    #include "effects/matrix/PatternMandala.h"
    #include "effects/matrix/PatternQR.h"
    #include "effects/matrix/PatternSM2DDPR.h"
    #include "effects/matrix/PatternSMStrobeDiffusion.h"
    #include "effects/matrix/PatternSerendipity.h"
    #include "effects/matrix/PatternSwirl.h"
    #include "effects/matrix/PatternPulse.h"
    #include "effects/matrix/PatternWave.h"
    #include "effects/matrix/PatternMaze.h"
    #include "effects/matrix/PatternLife.h"
    #include "effects/matrix/PatternSpiro.h"
    #include "effects/matrix/PatternCube.h"
    #include "effects/matrix/PatternCircuit.h"
    #include "effects/matrix/PatternAlienText.h"
    #include "effects/matrix/PatternRadar.h"
    #include "effects/matrix/PatternBounce.h"
    #include "effects/matrix/PatternSpin.h"
    #include "effects/matrix/PatternMisc.h"
#endif


#if USE_WS281X
    #include "ledstripgfx.h"
#endif

// Global effect set version

#define EFFECT_SET_VERSION 6

// Inform the linker which effects have setting specs, and in which class member

INIT_EFFECT_SETTING_SPECS(LEDStripEffect, _baseSettingSpecs);

//#if USE_HUB75 && ENABLE_WIFI
#if (USE_MATRIX) && ENABLE_WIFI
    INIT_EFFECT_SETTING_SPECS(PatternSubscribers, mySettingSpecs);
    INIT_EFFECT_SETTING_SPECS(PatternStocks, mySettingSpecs);
#endif

// Apple5x7 font definition - needed for WiFi-enabled matrix patterns using Adafruit-style fonts
// Define this unconditionally (guarded by ENABLE_WIFI) so the symbol is always available regardless of M5/LGFX usage.
#if ENABLE_WIFI && USE_MATRIX
#include <FontGfx_apple5x7.h>           // Requires the SmartMatrix dependency to pick up this font
#endif

// Default and JSON factory functions + decoration for effects
DRAM_ATTR std::unique_ptr<EffectFactories> g_ptrEffectFactories = nullptr;

// This function sets up the effect factories for the effects for whatever project is being built. The ADD_EFFECT macro variations
//   are provided and used for convenience.
void LoadEffectFactories()
{
    // Check if the factories have already been loaded
    if (g_ptrEffectFactories)
        return;

    g_ptrEffectFactories = make_unique_psram<EffectFactories>();

    // Include custom effects header if available - it overrides whatever the effect set flags
    // would otherwise include.

    #if __has_include ("custom_effects.h")
      #include "custom_effects.h"
    #endif

    // Fill effect factories using new effect set flags

    // === EFFECT SETS ===
    // These sections are shared by multiple projects

    #if defined(EFFECTS_MINIMAL)
        // Minimal effect set for projects with limited memory/space
        RegisterAll(*g_ptrEffectFactories,
            Effect<StatusEffect>(CRGB::White),
            Effect<RainbowFillEffect>(6, 2)
        );
    #endif

    #if defined(EFFECTS_SIMPLE)
        // Simple effect set for basic LED strip projects
        RegisterAll(*g_ptrEffectFactories,
            Effect<FireEffect>("Medium Fire", NUM_LEDS, 1, 3, 100, 3, 4, true, true),
            Effect<BouncingBallEffect>(3, true, true, 1),
            Effect<MeteorEffect>(4, 4, 10, 2.0, 2.0),
            Effect<StarEffect<QuietStar>>("Rainbow Twinkle Stars", RainbowColors_p, kStarEffectProbability, 1, LINEARBLEND, 2.0, 0.0, kStarEffectMusicFactor),
            Effect<PaletteEffect>(RainbowColors_p)
        );
    #endif

    #if defined(EFFECTS_PDPWOPR)
        // PDPWOPR project effects
        RegisterAll(*g_ptrEffectFactories,
            Effect<PDPCMXEffect>(),
            Effect<PDPGridEffect>()
        );
    #endif

    #if defined(EFFECTS_DEMO)
        // Demo effect set for M5 demos and similar
        RegisterAll(*g_ptrEffectFactories,
            Effect<FireEffect>("Medium Fire", NUM_LEDS, 1, 3, 100, 3, 4, true, true),
            Effect<BouncingBallEffect>(3, true, true, 1),
            Effect<BouncingBallEffect>(8, true, true, 1),
            Effect<MeteorEffect>(4, 4, 10, 2.0, 2.0),
            Effect<MeteorEffect>(2, 4, 10, 2.0, 2.0),
            Effect<StarEffect<QuietStar>>("Red Twinkle Stars", RedColors_p,   1.0, 1, LINEARBLEND, 2.0),
            Effect<StarEffect<QuietStar>>("Green Twinkle Stars", GreenColors_p, kStarEffectProbability, 1, LINEARBLEND, 2.0, 0.0, kStarEffectMusicFactor),
            Effect<StarEffect<Star>>("Blue Sparkle Stars", BlueColors_p,  kStarEffectProbability, 1, LINEARBLEND, 2.0, 0.0, kStarEffectMusicFactor),
            Effect<StarEffect<QuietStar>>("Rainbow Twinkle Stars", RainbowColors_p, kStarEffectProbability, 1, LINEARBLEND, 2.0, 0.0, kStarEffectMusicFactor),
            Effect<TwinkleEffect>(NUM_LEDS / 2, 20, 50),
            Effect<PaletteEffect>(RainbowColors_p, .25, 1, 0, 1.0, 0.0, LINEARBLEND, true, 1.0),
            Effect<PaletteEffect>(RainbowColors_p)
        );

        #if ENABLE_AUDIO
        RegisterAll(*g_ptrEffectFactories,
            Effect<StarEffect<MusicStar>>("RGB Music Blend Stars", RGBColors_p, 0.2, 1, NOBLEND, 5.0, 0.1, 2.0),
            Effect<StarEffect<MusicStar>>("Rainbow Twinkle Stars", RainbowColors_p, kStarEffectProbability, 1, LINEARBLEND, 0.0, 0.0, kStarEffectMusicFactor)
        );
        #endif
    #endif

    #if defined(EFFECTS_FAN)
        // Fan-specific effects for fan projects
        RegisterAll(*g_ptrEffectFactories,
            Effect<RainbowFillEffect>(24, 0),
            Effect<ColorCycleEffect>(BottomUp),
            Effect<ColorCycleEffect>(TopDown),
            Effect<ColorCycleEffect>(LeftRight),
            Effect<ColorCycleEffect>(RightLeft),
            Effect<PaletteReelEffect>("PaletteReelEffect"),
            Effect<MeteorEffect>(),
            Effect<TapeReelEffect>("TapeReelEffect"),
            Effect<StarEffect<MusicStar>>("RGB Music Blend Stars", RGBColors_p, 0.8, 1, NOBLEND, 15.0, 0.1, 10.0),
            Effect<StarEffect<MusicStar>>("Rainbow Music Stars", RainbowColors_p, 2.0, 2, LINEARBLEND, 5.0, 0.0, 10.0),
            Effect<FanBeatEffect>("FanBeat"),
            Effect<StarEffect<BubblyStar>>("Little Blooming Rainbow Stars", BlueColors_p, 8.0, 4, LINEARBLEND, 2.0, 0.0, 1.0),
            Effect<StarEffect<BubblyStar>>("Big Blooming Rainbow Stars", RainbowColors_p, 2, 12, LINEARBLEND, 1.0),
            Effect<StarEffect<BubblyStar>>("Neon Bars", RainbowColors_p, 0.5, 64, NOBLEND, 0),
            Effect<FireFanEffect>(GreenHeatColors_p, NUM_LEDS, 3, 7, 400, 2, NUM_LEDS / 2, Sequential, false, true),
            Effect<FireFanEffect>(GreenHeatColors_p, NUM_LEDS, 3, 8, 600, 2, NUM_LEDS / 2, Sequential, false, true),
            Effect<FireFanEffect>(GreenHeatColors_p, NUM_LEDS, 2, 10, 800, 2, NUM_LEDS / 2, Sequential, false, true),
            Effect<FireFanEffect>(GreenHeatColors_p, NUM_LEDS, 1, 12, 1000, 2, NUM_LEDS / 2, Sequential, false, true),
            Effect<FireFanEffect>(BlueHeatColors_p,  NUM_LEDS, 3, 7, 400, 2, NUM_LEDS / 2, Sequential, false, true),
            Effect<FireFanEffect>(BlueHeatColors_p,  NUM_LEDS, 3, 8, 600, 2, NUM_LEDS / 2, Sequential, false, true),
            Effect<FireFanEffect>(BlueHeatColors_p,  NUM_LEDS, 2, 10, 800, 2, NUM_LEDS / 2, Sequential, false, true),
            Effect<FireFanEffect>(BlueHeatColors_p,  NUM_LEDS, 1, 12, 1000, 2, NUM_LEDS / 2, Sequential, false, true),
            Effect<FireFanEffect>(HeatColors_p,      NUM_LEDS, 3, 7, 400, 2, NUM_LEDS / 2, Sequential, false, true),
            Effect<FireFanEffect>(HeatColors_p,      NUM_LEDS, 3, 8, 600, 2, NUM_LEDS / 2, Sequential, false, true),
            Effect<FireFanEffect>(HeatColors_p,      NUM_LEDS, 2, 10, 800, 2, NUM_LEDS / 2, Sequential, false, true),
            Effect<FireFanEffect>(HeatColors_p,      NUM_LEDS, 1, 12, 1000, 2, NUM_LEDS / 2, Sequential, false, true)
        );
    #endif

    #if defined(EFFECTS_LASERLINE)
        // Laser line effect set
        RegisterAll(*g_ptrEffectFactories,
            Effect<LaserLineEffect>(500, 20)
        );
    #endif

    #if defined(EFFECTS_CHIEFTAIN)
        // Chieftain lantern effect set
        RegisterAll(*g_ptrEffectFactories,
            Effect<LanternEffect>(),
            Effect<PaletteEffect>(RainbowColors_p, 2.0f, 0.1, 0.0, 1.0, 0.0, LINEARBLEND, true, 1.0),
            Effect<RainbowFillEffect>(10, 32)
        );
    #endif

    #if defined(EFFECTS_LANTERN)
        // Lantern effect set
        RegisterAll(*g_ptrEffectFactories,
            Effect<FireEffect>("Calm Fire", NUM_LEDS, 40, 5, 50, 3, 3, true, true)
        );
    #endif

    #if defined(EFFECTS_STACKDEMO)
        RegisterAll(*g_ptrEffectFactories,
            Effect<PatternPongClock>(),
            Effect<PatternStocks>(),
            Effect<PatternSubscribers>(),
            Effect<PatternWeather>(),
            Effect<SpectrumBarEffect>("Audiograph", 16, 4, 0),
            Effect<SpectrumAnalyzerEffect>("Spectrum", NUM_BANDS, spectrumAltColors, false, 0, 0, 1.6, 1.6),
            Effect<SpectrumAnalyzerEffect>("AudioWave", MATRIX_WIDTH, CRGB(0,0,40), 0, 1.25, 1.25, true),
            Effect<PatternAnimatedGIF>("Rings", GIFIdentifier::ThreeRings),
            Effect<PatternAnimatedGIF>("Fire Log", GIFIdentifier::Firelog),
            Effect<PatternAnimatedGIF>("Nyancat", GIFIdentifier::Nyancat),
            Effect<PatternAnimatedGIF>("Pacman", GIFIdentifier::Pacman),
            Effect<PatternAnimatedGIF>("Atomic", GIFIdentifier::Atomic),
            Effect<PatternAnimatedGIF>("Banana", GIFIdentifier::Banana, true, CRGB::DarkBlue),
            Effect<PatternSMFire2021>(),
            Effect<GhostWave>("GhostWave", 0, 30, false, 10),
            Effect<PatternSMGamma>(),
            Effect<PatternSMMetaBalls>(),
            Effect<PatternSMSupernova>(),
            Effect<PatternCube>(),
            Effect<PatternLife>(),
            Effect<PatternCircuit>(),
            Effect<SpectrumAnalyzerEffect>("USA", NUM_BANDS, USAColors_p, true, 0, 0, 0.75, 0.75),
            Effect<SpectrumAnalyzerEffect>("Spectrum 2", 32, spectrumBasicColors, false, 100, 0, 0.75, 0.75),
            Effect<SpectrumAnalyzerEffect>("Spectrum++", NUM_BANDS, spectrumBasicColors, false, 0, 40, -1.0, 2.0),
            Effect<WaveformEffect>("WaveIn", 8),
            Effect<GhostWave>("WaveOut", 0, 0, true, 0),
            Effect<StarEffect<MusicStar>>("Stars", RainbowColors_p, 1.0, 1, LINEARBLEND, 2.0, 0.5, 10.0),
            Effect<GhostWave>("PlasmaWave", 0, 255, false),
            Effect<PatternSMNoise>("Shikon", PatternSMNoise::EffectType::Shikon_t),
            Effect<PatternSMFlowFields>(),
            Effect<PatternSMBlurringColors>(),
            Effect<PatternSMWalkingMachine>(),
            Effect<PatternSMStarDeep>(),
            Effect<PatternSM2DDPR>(),
            Effect<PatternSMPicasso3in1>("Lines", 38),
            Effect<PatternSMPicasso3in1>("Circles", 73),
            Effect<PatternSMAmberRain>(),
            Effect<PatternSMStrobeDiffusion>(),
            Effect<PatternSMSpiroPulse>(),
            Effect<PatternSMTwister>(),
            Effect<PatternSMHolidayLights>(),
            Effect<PatternRose>(),
            Effect<PatternPinwheel>(),
            Effect<PatternSunburst>(),
            Effect<PatternClock>(),
            Effect<PatternAlienText>(),
            Effect<PatternPulsar>(),
            Effect<PatternBounce>(),
            Effect<PatternWave>(),
            Effect<PatternSwirl>(),
            Effect<PatternSerendipity>(),
            Effect<PatternMunch>(),
            Effect<PatternMaze>()
        );
    #endif

    #if defined(EFFECTS_FULLMATRIX)
        // Full matrix effect set for advanced displays (Mesmerizer, etc.)
        RegisterAll(*g_ptrEffectFactories,
            Effect<SpectrumBarEffect>("Audiograph", 16, 4, 0),
            Effect<SpectrumAnalyzerEffect>("Spectrum", NUM_BANDS, spectrumAltColors, false, 0, 0, 1.6, 1.6),
            Effect<SpectrumAnalyzerEffect>("AudioWave", MATRIX_WIDTH, CRGB(0,0,40), 0, 1.25, 1.25, true),
            Effect<PatternSMRadialWave>(),
            Effect<PatternAnimatedGIF>("Fire Log", GIFIdentifier::Firelog),
            Effect<PatternAnimatedGIF>("Pacman", GIFIdentifier::Pacman),
            Effect<PatternPongClock>(),
            Effect<PatternAnimatedGIF>("Colorball", GIFIdentifier::ColorSphere),
            Effect<PatternSMFire2021>(),
            Effect<GhostWave>("GhostWave", 0, 30, false, 10),
            Effect<PatternSMGamma>(),
            Effect<PatternAnimatedGIF>("Rings", GIFIdentifier::ThreeRings),
            Effect<PatternAnimatedGIF>("Atomic", GIFIdentifier::Atomic),
            Effect<PatternAnimatedGIF>("Bananaman", GIFIdentifier::Banana, true, CRGB::DarkBlue),
            Effect<PatternSMMetaBalls>(),
            Effect<PatternSMSupernova>(),
            Effect<PatternCube>(),
            Effect<PatternAnimatedGIF>("Tesseract", GIFIdentifier::Tesseract),
            Effect<PatternAnimatedGIF>("Nyancat", GIFIdentifier::Nyancat),
            Effect<PatternLife>(),
            Effect<PatternCircuit>(),
            Effect<SpectrumAnalyzerEffect>("USA", NUM_BANDS, USAColors_p, true, 0, 0, 0.75, 0.75),
            Effect<SpectrumAnalyzerEffect>("Spectrum 2", 32, spectrumBasicColors, false, 100, 0, 0.75, 0.75),
            Effect<SpectrumAnalyzerEffect>("Spectrum++", NUM_BANDS, spectrumBasicColors, false, 0, 40, -1.0, 2.0),
            Effect<WaveformEffect>("WaveIn", 8),
            Effect<GhostWave>("WaveOut", 0, 0, true, 0),
            Effect<StarEffect<MusicStar>>("Stars", RainbowColors_p, 1.0, 1, LINEARBLEND, 2.0, 0.5, 10.0)
        );

        #if ENABLE_WIFI
        RegisterAll(*g_ptrEffectFactories,
            Effect<PatternStocks>(),
            Effect<PatternSubscribers>(),
            Effect<PatternWeather>()
        );
        #endif

        RegisterAll(*g_ptrEffectFactories,
            Effect<PatternSMSmoke>(),
            Effect<GhostWave>("PlasmaWave", 0, 255, false),
            Effect<PatternSMNoise>("Shikon", PatternSMNoise::EffectType::Shikon_t),
            Effect<PatternSMRadialFire>(),
            Effect<PatternSMFlowFields>(),
            Effect<PatternSMBlurringColors>(),
            Effect<PatternSMWalkingMachine>(),
            Effect<PatternSMHypnosis>(),
            Effect<PatternSMStarDeep>(),
            Effect<PatternSM2DDPR>(),
            Effect<PatternSMPicasso3in1>("Lines", 38),
            Effect<PatternSMPicasso3in1>("Circles", 73),
            Effect<PatternSMAmberRain>(),
            Effect<PatternSMStrobeDiffusion>(),
            Effect<PatternSMRainbowTunnel>(),
            Effect<PatternSMSpiroPulse>(),
            Effect<PatternSMTwister>(),
            Effect<PatternSMHolidayLights>(),
            Effect<PatternRose>(),
            Effect<PatternPinwheel>(),
            Effect<PatternSunburst>(),
            Effect<PatternClock>(),
            Effect<PatternAlienText>(),
            Effect<PatternPulsar>(),
            Effect<PatternBounce>(),
            Effect<PatternWave>(),
            Effect<PatternSwirl>(),
            Effect<PatternSerendipity>(),
            Effect<PatternMandala>(),
            Effect<PatternMunch>(),
            Effect<PatternMaze>()
        );
    #endif

    #if defined(EFFECTS_UMBRELLA)
        // Umbrella-specific effects
        RegisterAll(*g_ptrEffectFactories,
            Effect<FireEffect>("Calm Fire", NUM_LEDS, 2, 2, 75, 3, 10, true, false),
            Effect<FireEffect>("Medium Fire", NUM_LEDS, 1, 5, 100, 3, 4, true, false),
            Effect<MusicalPaletteFire>("Musical Red Fire", HeatColors_p, NUM_LEDS, 1, 8, 50, 1, 24, true, false),
            Effect<MusicalPaletteFire>("Purple Fire", CRGBPalette16(CRGB::Black, CRGB::Purple, CRGB::MediumPurple, CRGB::LightPink), NUM_LEDS, 2, 3, 150, 3, 10, true, false),
            Effect<MusicalPaletteFire>("Purple Fire", CRGBPalette16(CRGB::Black, CRGB::Purple, CRGB::MediumPurple, CRGB::LightPink), NUM_LEDS, 1, 7, 150, 3, 10, true, false),
            Effect<MusicalPaletteFire>("Musical Purple Fire", CRGBPalette16(CRGB::Black, CRGB::Purple, CRGB::MediumPurple, CRGB::LightPink), NUM_LEDS, 1, 8, 50, 1, 24, true, false),
            Effect<MusicalPaletteFire>("Blue Fire", CRGBPalette16(CRGB::Black, CRGB::DarkBlue, CRGB::Blue, CRGB::LightSkyBlue), NUM_LEDS, 2, 3, 150, 3, 10, true, false),
            Effect<MusicalPaletteFire>("Blue Fire", CRGBPalette16(CRGB::Black, CRGB::DarkBlue, CRGB::Blue, CRGB::LightSkyBlue), NUM_LEDS, 1, 7, 150, 3, 10, true, false),
            Effect<MusicalPaletteFire>("Musical Blue Fire", CRGBPalette16(CRGB::Black, CRGB::DarkBlue, CRGB::Blue, CRGB::LightSkyBlue), NUM_LEDS, 1, 8, 50, 1, 24, true, false),
            Effect<MusicalPaletteFire>("Green Fire", CRGBPalette16(CRGB::Black, CRGB::DarkGreen, CRGB::Green, CRGB::LimeGreen), NUM_LEDS, 2, 3, 150, 3, 10, true, false),
            Effect<MusicalPaletteFire>("Green Fire", CRGBPalette16(CRGB::Black, CRGB::DarkGreen, CRGB::Green, CRGB::LimeGreen), NUM_LEDS, 1, 7, 150, 3, 10, true, false),
            Effect<MusicalPaletteFire>("Musical Green Fire", CRGBPalette16(CRGB::Black, CRGB::DarkGreen, CRGB::Green, CRGB::LimeGreen), NUM_LEDS, 1, 8, 50, 1, 24, true, false),
            Effect<BouncingBallEffect>(),
            Effect<DoublePaletteEffect>(),
            Effect<MeteorEffect>(4, 4, 10, 2.0, 2.0),
            Effect<MeteorEffect>(10, 1, 20, 1.5, 1.5),
            Effect<MeteorEffect>(25, 1, 40, 1.0, 1.0),
            Effect<MeteorEffect>(50, 1, 50, 0.5, 0.5),
            Effect<StarEffect<QuietStar>>("Rainbow Twinkle Stars", RainbowColors_p, kStarEffectProbability, 1, LINEARBLEND, 2.0, 0.0, kStarEffectMusicFactor),
            Effect<StarEffect<MusicStar>>("RGB Music Blend Stars", RGBColors_p, 0.8, 1, NOBLEND, 15.0, 0.1, 10.0),
            Effect<StarEffect<MusicStar>>("Rainbow Music Stars", RainbowColors_p, 2.0, 2, LINEARBLEND, 5.0, 0.0, 10.0),
            Effect<StarEffect<BubblyStar>>("Little Blooming Rainbow Stars", BlueColors_p, kStarEffectProbability, 4, LINEARBLEND, 2.0, 0.0, kStarEffectMusicFactor),
            Effect<StarEffect<QuietStar>>("Green Twinkle Stars", GreenColors_p, kStarEffectProbability, 1, LINEARBLEND, 2.0, 0.0, kStarEffectMusicFactor),
            Effect<StarEffect<Star>>("Blue Sparkle Stars", BlueColors_p, kStarEffectProbability, 1, LINEARBLEND, 2.0, 0.0, kStarEffectMusicFactor),
            Effect<StarEffect<QuietStar>>("Red Twinkle Stars", RedColors_p, 1.0, 1, LINEARBLEND, 2.0),
            Effect<StarEffect<Star>>("Lava Stars", LavaColors_p, kStarEffectProbability, 1, LINEARBLEND, 2.0, 0.0, kStarEffectMusicFactor),
            Effect<PaletteEffect>(RainbowColors_p),
            Effect<PaletteEffect>(RainbowColors_p, 1.0, 1.0),
            Effect<PaletteEffect>(RainbowColors_p, .25)
        );
    #endif

    #if defined(EFFECTS_SPECTRUM)
        // Spectrum analyzer effect set
        RegisterAll(*g_ptrEffectFactories,
            Effect<SpectrumAnalyzerEffect>("Spectrum Standard", NUM_BANDS, spectrumAltColors, false, 0, 0, 0.5, 1.5),
            Effect<SpectrumAnalyzerEffect>("Spectrum Standard", 24, spectrumAltColors, false, 0, 0, 1.25, 1.25),
            Effect<SpectrumAnalyzerEffect>("Spectrum Standard", 24, spectrumAltColors, false, 0, 0, 0.25, 1.25),
            Effect<SpectrumAnalyzerEffect>("Spectrum Standard", 16, spectrumAltColors, false, 0, 0, 1.0, 1.0),
            Effect<SpectrumAnalyzerEffect>("Spectrum Standard", 48, CRGB(0,0,4), 0, 1.25, 1.25),
            Effect<GhostWave>("GhostWave", 0, 16, false, 15),
            Effect<SpectrumAnalyzerEffect>("Spectrum USA", 16, USAColors_p, true, 0),
            Effect<GhostWave>("GhostWave Rainbow", 8),
            Effect<SpectrumAnalyzerEffect>("Spectrum Fade", 24, RainbowColors_p, false, 50, 70, -1.0, 2.0),
            Effect<GhostWave>("GhostWave Blue", 0),
            Effect<SpectrumAnalyzerEffect>("Spectrum Standard", 24, RainbowColors_p, false),
            Effect<GhostWave>("GhostWave One", 4)
        );
    #endif

    #if defined(EFFECTS_HELMET)
        // Helmet display effect set
        RegisterAll(*g_ptrEffectFactories,
            Effect<SilonEffect>(),
            Effect<SpectrumAnalyzerEffect>("Spectrum Standard", NUM_BANDS, spectrumAltColors, false, 0, 0, 0.5, 1.5)
        );
    #endif

    #if defined(EFFECTS_TTGO)
        // TTGO display effect set
        RegisterAll(*g_ptrEffectFactories,
            Effect<SpectrumAnalyzerEffect>("Spectrum Fade", 12, spectrumBasicColors, false, 50, 70, -1.0, 3.0)
        );
    #endif

    #if defined(EFFECTS_WROVERKIT)
        // Wrover Kit effect set
        RegisterAll(*g_ptrEffectFactories,
            Effect<PaletteEffect>(rainbowPalette, 256 / 16, .2, 0)
        );
    #endif

    #if defined(EFFECTS_XMASTREES)
        // Christmas trees effect set
        RegisterAll(*g_ptrEffectFactories,
            Effect<ColorBeatOverRed>("ColorBeatOverRed"),
            Effect<ColorCycleEffect>(BottomUp, 6),
            Effect<ColorCycleEffect>(BottomUp, 2),
            Effect<RainbowFillEffect>(48, 0),
            Effect<ColorCycleEffect>(BottomUp, 3),
            Effect<ColorCycleEffect>(BottomUp, 1),
            Effect<StarEffect<LongLifeSparkleStar>>("Green Sparkle Stars", GreenColors_p, 2.0, 1, LINEARBLEND, 2.0, 0.0, 0.0, CRGB(0, 128, 0)),
            Effect<StarEffect<LongLifeSparkleStar>>("Red Sparkle Stars",   GreenColors_p, 2.0, 1, LINEARBLEND, 2.0, 0.0, 0.0, CRGB::Red),
            Effect<StarEffect<LongLifeSparkleStar>>("Blue Sparkle Stars",  GreenColors_p, 2.0, 1, LINEARBLEND, 2.0, 0.0, 0.0, CRGB::Blue),
            Effect<PaletteEffect>(rainbowPalette, 256 / 16, .2, 0)
        );
    #endif

    #if defined(EFFECTS_INSULATORS)
        // Insulators effect set
        RegisterAll(*g_ptrEffectFactories,
            Effect<InsulatorSpectrumEffect>("Spectrum Effect", RainbowColors_p),
            Effect<NewMoltenGlassOnVioletBkgnd>("Molten Glass", RainbowColors_p),
            Effect<StarEffect<MusicStar>>("RGB Music Blend Stars", RGBColors_p, 0.8, 1, NOBLEND, 15.0, 0.1, 10.0),
            Effect<StarEffect<MusicStar>>("Rainbow Music Stars",   RainbowColors_p, 2.0, 2, LINEARBLEND, 5.0, 0.0, 10.0),
            Effect<PaletteReelEffect>("PaletteReelEffect"),
            Effect<ColorBeatOverRed>("ColorBeatOverRed"),
            Effect<TapeReelEffect>("TapeReelEffect")
        );
    #endif

    #if defined(EFFECTS_CUBE)
        // Cube display effect set
        RegisterAll(*g_ptrEffectFactories,
            Effect<PaletteEffect>(rainbowPalette, 256 / 16, .2, 0),
            Effect<SparklySpinningMusicEffect>("SparklySpinningMusical", RainbowColors_p),
            Effect<ColorBeatOverRed>("ColorBeatOnRedBkgnd"),
            Effect<SimpleInsulatorBeatEffect2>("SimpleInsulatorColorBeat"),
            Effect<StarEffect<MusicStar>>("Rainbow Music Stars", RainbowColors_p, 2.0, 2, LINEARBLEND, 5.0, 0.0, 10.0)
        );
    #endif

    #if defined(EFFECTS_MAGICMIRROR)
        // Magic mirror effect set
        RegisterAll(*g_ptrEffectFactories,
            Effect<MoltenGlassOnVioletBkgnd>("MoltenGlass", RainbowColors_p)
        );
    #endif

    #if defined(EFFECTS_ATOMLIGHT)
        // Atom light effect set
        RegisterAll(*g_ptrEffectFactories,
            Effect<ColorFillEffect>(CRGB::White, 1),
            Effect<FireFanEffect>(HeatColors_p, NUM_LEDS, 2, 2, 200, 2, 5, Sequential, true, false),
            Effect<FireFanEffect>(HeatColors_p, NUM_LEDS, 1, 12, 400, 2, NUM_LEDS / 2, Sequential, true, false),
            Effect<FireFanEffect>(GreenHeatColors_p, NUM_LEDS, 1, 10, 400, 2, NUM_LEDS / 2, Sequential, true, false),
            Effect<FireFanEffect>(BlueHeatColors_p, NUM_LEDS, 1, 10, 400, 2, NUM_LEDS / 2, Sequential, true, false),
            Effect<FireFanEffect>(RainbowColors_p, NUM_LEDS, 1, 10, 400, 2, NUM_LEDS / 2, Sequential, true, false),
            Effect<FireFanEffect>(HeatColors_p, NUM_LEDS, 1, 10, 400, 2, NUM_LEDS / 2, Sequential, true, false, true),
            Effect<BouncingBallEffect>(3, true, true, 1),
            Effect<RainbowFillEffect>(60, 0),
            Effect<ColorCycleEffect>(Sequential),
            Effect<PaletteEffect>(RainbowColors_p, 4, 0.1, 0.0, 1.0, 0.0),
            Effect<MeteorEffect>(20, 1, 25, .15, .05),
            Effect<MeteorEffect>(12, 1, 25, .15, .08),
            Effect<MeteorEffect>(6, 1, 25, .15, .12),
            Effect<MeteorEffect>(1, 1, 5, .15, .25),
            Effect<MeteorEffect>()
        );
    #endif

    #if defined(EFFECTS_PLATECOVER)
        // Plate cover effect set
        RegisterAll(*g_ptrEffectFactories,
            Effect<ColorFillEffect>("Solid White", CRGB::White, 1),
            Effect<ColorFillEffect>("Solid Red",   CRGB::Red,   1),
            Effect<ColorFillEffect>("Solid Amber", CRGB(255, 50, 0), 1),
            Effect<FireFanEffect>(HeatColors_p, NUM_LEDS, 4, 5.0, 200, 8, 8, Sequential, true, true, true, 90),
            Effect<RainbowFillEffect>(16, 3, true),
            Effect<MeteorEffect>(2, 1, 15, .75, .75),
            Effect<ColorFillEffect>("Off", CRGB::Black, 1)
        );
    #endif

    #if defined(EFFECTS_SPIRALLAMP)
        // Spiral lamp effect set
        RegisterAll(*g_ptrEffectFactories,
            Effect<VUMeterVerticalEffect>(),
            Effect<MeteorEffect>(4, 4, 10, 1.0, 1.0),
            Effect<ColorFillEffect>("Solid White", CRGB::White, 1),
            Effect<FireFanEffect>(HeatColors_p,      NUM_LEDS, 1, 2.5, 200, 2, 15, Sequential, true, false),
            Effect<FireFanEffect>(GreenHeatColors_p, NUM_LEDS, 1, 2.5, 200, 2, 15, Sequential, true, false),
            Effect<FireFanEffect>(BlueHeatColors_p,  NUM_LEDS, 1, 2.5, 200, 2, 15, Sequential, true, false),
            Effect<FireFanEffect>(RainbowColors_p,   NUM_LEDS, 1, 2.5, 200, 2, 15, Sequential, true, false),
            Effect<FireFanEffect>(HeatColors_p,      NUM_LEDS, 1, 2.5, 200, 2, 15, Sequential, true, false, true),
            Effect<RainbowFillEffect>(120, 0),
            Effect<PaletteEffect>(RainbowColors_p, 4, 0.1, 0.0, 1.0, 0.0),
            Effect<BouncingBallEffect>(3, true, true, 8),
            Effect<StarEffect<MusicStar>>("Rainbow Twinkle Stars", RainbowColors_p, kStarEffectProbability, 1, LINEARBLEND, 0.0, 0.0, kStarEffectMusicFactor),
            Effect<StarEffect<Star>>("Rainbow Twinkle Stars", RainbowColors_p, kStarEffectProbability, 1, LINEARBLEND, 2.0, 0.0, kStarEffectMusicFactor),
            Effect<StarEffect<Star>>("Red Sparkle Stars", RedColors_p,   kStarEffectProbability, 1, LINEARBLEND, 2.0, 0.0, kStarEffectMusicFactor),
            Effect<StarEffect<MusicStar>>("Red Stars", RedColors_p,   kStarEffectProbability, 1, LINEARBLEND, 2.0, 0.0, kStarEffectMusicFactor),
            Effect<StarEffect<Star>>("Blue Sparkle Stars", BlueColors_p,  kStarEffectProbability, 1, LINEARBLEND, 2.0, 0.0, kStarEffectMusicFactor),
            Effect<StarEffect<MusicStar>>("Blue Stars", BlueColors_p,  kStarEffectProbability, 1, LINEARBLEND, 2.0, 0.0, kStarEffectMusicFactor),
            Effect<StarEffect<Star>>("Green Sparkle Stars", GreenColors_p, kStarEffectProbability, 1, LINEARBLEND, 2.0, 0.0, kStarEffectMusicFactor),
            Effect<StarEffect<MusicStar>>("Green Stars", GreenColors_p, kStarEffectProbability, 1, LINEARBLEND, 2.0, 0.0, kStarEffectMusicFactor),
            Effect<TwinkleEffect>(NUM_LEDS / 2, 20, 50),
            Effect<PaletteEffect>(RainbowColors_p, .25, 1, 0, 1.0, 0.0, LINEARBLEND, true, 1.0),
            Effect<PaletteEffect>(RainbowColors_p)
        );
    #endif

    #if defined(EFFECTS_HEXAGON)

        // Hexagon effect set
        RegisterAll(*g_ptrEffectFactories,
            Effect<OuterHexRingEffect>()
        );

    #endif

    // Default fallback if no set contributed any effect
    if (g_ptrEffectFactories->IsEmpty())
    {
        RegisterAll(*g_ptrEffectFactories,
            Effect<RainbowFillEffect>(6, 2)
        );
    }

    // If this assert fires, you have not defined any effects in the table above.  If adding a new config, you need to
    // add the list of effects in this table as shown for the various other existing configs.  You MUST have at least
    // one effect even if it's the Status effect.

    assert(!g_ptrEffectFactories->IsEmpty());

    auto factoriesHashString = fnv1a::hash_to_string(fnv1a::hash<uint64_t>(g_ptrEffectFactories->FactoryIDs()));
    g_ptrEffectFactories->HashString(factoriesHashString);
}