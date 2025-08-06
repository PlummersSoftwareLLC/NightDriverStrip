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

#include "effectsupport.h"

// Include the effect classes we'll need later

#include "effects/strip/fireeffect.h"           // fire effects
#include "effects/strip/paletteeffect.h"        // palette effects
#include "effects/strip/doublepaletteeffect.h"  // double palette effect
#include "effects/strip/meteoreffect.h"         // meteor blend effect
#include "effects/strip/stareffect.h"           // star effects
#include "effects/strip/bouncingballeffect.h"   // bouncing ball effectsenable+
#include "effects/strip/tempeffect.h"
#include "effects/strip/stareffect.h"
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

    #include "effects/matrix/PatternSMStrobeDiffusion.h"
    #include "effects/matrix/PatternSM2DDPR.h"

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
    #include "effects/matrix/PatternSMHypnosis.h"
    #include "effects/matrix/PatternSMRainbowTunnel.h"
    #include "effects/matrix/PatternSMRadialWave.h"
    #include "effects/matrix/PatternSMRadialFire.h"
    #include "effects/matrix/PatternSMSmoke.h"
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
    #include "effects/matrix/PatternPongClock.h"
    #include "effects/matrix/PatternBounce.h"
    #include "effects/matrix/PatternMandala.h"
    #include "effects/matrix/PatternSpin.h"
    #include "effects/matrix/PatternMisc.h"
    #include "effects/matrix/PatternNoiseSmearing.h"
    #include "effects/matrix/PatternQR.h"
    #include "effects/matrix/PatternAnimatedGIF.h"

  #if ENABLE_WIFI
    #include "effects/matrix/PatternSubscribers.h"
    #include "effects/matrix/PatternWeather.h"
    #include "effects/matrix/PatternStocks.h"
  #endif

#endif  // USE_HUB75

#ifdef USE_WS281X
    #include "ledstripgfx.h"
#endif

// Inform the linker which effects have setting specs, and in which class member

INIT_EFFECT_SETTING_SPECS(LEDStripEffect, _baseSettingSpecs);

#if USE_HUB75 && ENABLE_WIFI
    INIT_EFFECT_SETTING_SPECS(PatternSubscribers, mySettingSpecs);
    INIT_EFFECT_SETTING_SPECS(PatternStocks, mySettingSpecs);
#endif

// Effect factories for the StarryNightEffect - one per star type
std::map<int, JSONEffectFactory> g_JsonStarryNightEffectFactories =
{
    { EFFECT_STAR,
        [](const JsonObjectConst& jsonObject)->std::shared_ptr<LEDStripEffect> { return make_shared_psram<StarryNightEffect<Star>>(jsonObject); } },
    { EFFECT_STAR_BUBBLY,
        [](const JsonObjectConst& jsonObject)->std::shared_ptr<LEDStripEffect> { return make_shared_psram<StarryNightEffect<BubblyStar>>(jsonObject); } },
    { EFFECT_STAR_HOT_WHITE,
        [](const JsonObjectConst& jsonObject)->std::shared_ptr<LEDStripEffect>  { return make_shared_psram<StarryNightEffect<HotWhiteStar>>(jsonObject); } },
    { EFFECT_STAR_LONG_LIFE_SPARKLE,
        [](const JsonObjectConst& jsonObject)->std::shared_ptr<LEDStripEffect>  { return make_shared_psram<StarryNightEffect<LongLifeSparkleStar>>(jsonObject); } },

#if ENABLE_AUDIO
    { EFFECT_STAR_MUSIC,
        [](const JsonObjectConst& jsonObject)->std::shared_ptr<LEDStripEffect>  { return make_shared_psram<StarryNightEffect<MusicStar>>(jsonObject); } },
#endif

    { EFFECT_STAR_QUIET,
        [](const JsonObjectConst& jsonObject)->std::shared_ptr<LEDStripEffect>  { return make_shared_psram<StarryNightEffect<QuietStar>>(jsonObject); } },
};

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

    // The EFFECT_SET_VERSION macro defines the "effect set version" for a project. This version
    // is persisted to JSON with the effect objects, and compared to it when the effects JSON file
    // is deserialized.
    //
    // If the persisted version and the one defined below don't match, the effects JSON is ignored
    // and the default set is loaded. This means that a "reset" of a project's effect set on the
    // boards running the project can be forced by bumping up the effect set version for that project.
    // As the user may have customized their effect set config or order, this should be done with
    // some hesitation - and increasingly so when the web UI starts offering more facilities for
    // customizing one's effect setup.
    //
    // The effect set version defaults to 1, so a project only needs to define it if it's different
    // than that; refer to MESMERIZER as an example. If the effect set version is defined to 0, the
    // default set will be loaded at every startup.
    //
    // The following line can be uncommented to override the per-project effect set version.

    // #define EFFECT_SET_VERSION   0

    #if __has_include ("custom_effects.h")
      #include "custom_effects.h"
    #endif

    // Fill effect factories using new effect set flags

    // === EFFECT SETS ===
    // These sections are shared by multiple projects

    #if defined(EFFECTS_MINIMAL)
        // Minimal effect set for projects with limited memory/space
        #ifndef EFFECT_SET_VERSION
            #define EFFECT_SET_VERSION  1
        #endif
        
        ADD_EFFECT(EFFECT_STRIP_STATUS, StatusEffect, CRGB::White);
        ADD_EFFECT(EFFECT_STRIP_RAINBOW_FILL, RainbowFillEffect, 6, 2);

    #elif defined(EFFECTS_SIMPLE)
        // Simple effect set for basic LED strip projects
        #ifndef EFFECT_SET_VERSION
            #define EFFECT_SET_VERSION  2
        #endif
        
        ADD_EFFECT(EFFECT_STRIP_FIRE, FireEffect, "Medium Fire", NUM_LEDS, 1, 3, 100, 3, 4, true, true);
        ADD_EFFECT(EFFECT_STRIP_BOUNCING_BALL, BouncingBallEffect, 3, true, true, 1);
        ADD_EFFECT(EFFECT_STRIP_METEOR, MeteorEffect, 4, 4, 10, 2.0, 2.0);
        ADD_STARRY_NIGHT_EFFECT(QuietStar, "Rainbow Twinkle Stars", RainbowColors_p, STARRYNIGHT_PROBABILITY, 1, LINEARBLEND, 2.0, 0.0, STARRYNIGHT_MUSICFACTOR);
        ADD_EFFECT(EFFECT_STRIP_PALETTE, PaletteEffect, RainbowColors_p);

    #elif defined(EFFECTS_PDPWOPR)
        // PDPWOPR project effects
        #ifndef EFFECT_SET_VERSION
            #define EFFECT_SET_VERSION  1
        #endif

        ADD_EFFECT(EFFECT_MATRIX_PDPCMX, PDPCMXEffect);
        ADD_EFFECT(EFFECT_MATRIX_PDPGRID, PDPGridEffect);
        #define DEFAULT_EFFECT_INTERVAL  0

    #elif defined(EFFECTS_DEMO)
        // Demo effect set for M5 demos and similar
        #ifndef EFFECT_SET_VERSION
            #define EFFECT_SET_VERSION  2
        #endif

        ADD_EFFECT(EFFECT_STRIP_FIRE, FireEffect, "Medium Fire", NUM_LEDS, 1, 3, 100, 3, 4, true, true);
        ADD_EFFECT(EFFECT_STRIP_BOUNCING_BALL, BouncingBallEffect, 3, true, true, 1);
        ADD_EFFECT(EFFECT_STRIP_BOUNCING_BALL, BouncingBallEffect, 8, true, true, 1);
        ADD_EFFECT(EFFECT_STRIP_METEOR, MeteorEffect, 4, 4, 10, 2.0, 2.0);
        ADD_EFFECT(EFFECT_STRIP_METEOR, MeteorEffect, 2, 4, 10, 2.0, 2.0);
        ADD_STARRY_NIGHT_EFFECT(QuietStar, "Red Twinkle Stars", RedColors_p, 1.0, 1, LINEARBLEND, 2.0);
        ADD_STARRY_NIGHT_EFFECT(QuietStar, "Green Twinkle Stars", GreenColors_p, STARRYNIGHT_PROBABILITY, 1, LINEARBLEND, 2.0, 0.0, STARRYNIGHT_MUSICFACTOR);
        ADD_STARRY_NIGHT_EFFECT(Star, "Blue Sparkle Stars", BlueColors_p, STARRYNIGHT_PROBABILITY, 1, LINEARBLEND, 2.0, 0.0, STARRYNIGHT_MUSICFACTOR);
        ADD_STARRY_NIGHT_EFFECT(QuietStar, "Rainbow Twinkle Stars", RainbowColors_p, STARRYNIGHT_PROBABILITY, 1, LINEARBLEND, 2.0, 0.0, STARRYNIGHT_MUSICFACTOR);
        ADD_STARRY_NIGHT_EFFECT(MusicStar, "RGB Music Blend Stars", RGBColors_p, 0.2, 1, NOBLEND, 5.0, 0.1, 2.0);
        ADD_EFFECT(EFFECT_STRIP_TWINKLE, TwinkleEffect, NUM_LEDS / 2, 20, 50);
        ADD_EFFECT(EFFECT_STRIP_PALETTE, PaletteEffect, RainbowColors_p, .25, 1, 0, 1.0, 0.0, LINEARBLEND, true, 1.0);
        ADD_EFFECT(EFFECT_STRIP_PALETTE, PaletteEffect, RainbowColors_p);
        ADD_STARRY_NIGHT_EFFECT(MusicStar, "Rainbow Twinkle Stars", RainbowColors_p, STARRYNIGHT_PROBABILITY, 1, LINEARBLEND, 0.0, 0.0, STARRYNIGHT_MUSICFACTOR);

    #elif defined(EFFECTS_FAN)
        // Fan-specific effects for fan projects
        #ifndef EFFECT_SET_VERSION
            #define EFFECT_SET_VERSION  4
        #endif

        ADD_EFFECT(EFFECT_STRIP_RAINBOW_FILL, RainbowFillEffect, 24, 0);
        ADD_EFFECT(EFFECT_STRIP_COLOR_CYCLE, ColorCycleEffect, BottomUp);
        ADD_EFFECT(EFFECT_STRIP_COLOR_CYCLE, ColorCycleEffect, TopDown);
        ADD_EFFECT(EFFECT_STRIP_COLOR_CYCLE, ColorCycleEffect, LeftRight);
        ADD_EFFECT(EFFECT_STRIP_COLOR_CYCLE, ColorCycleEffect, RightLeft);
        ADD_EFFECT(EFFECT_STRIP_PALETTE_REEL, PaletteReelEffect, "PaletteReelEffect");
        ADD_EFFECT(EFFECT_STRIP_METEOR, MeteorEffect);
        ADD_EFFECT(EFFECT_STRIP_TAPE_REEL, TapeReelEffect, "TapeReelEffect");
        ADD_STARRY_NIGHT_EFFECT(MusicStar, "RGB Music Blend Stars", RGBColors_p, 0.8, 1, NOBLEND, 15.0, 0.1, 10.0);
        ADD_STARRY_NIGHT_EFFECT(MusicStar, "Rainbow Music Stars", RainbowColors_p, 2.0, 2, LINEARBLEND, 5.0, 0.0, 10.0);
        ADD_EFFECT(EFFECT_STRIP_FAN_BEAT, FanBeatEffect, "FanBeat");
        ADD_STARRY_NIGHT_EFFECT(BubblyStar, "Little Blooming Rainbow Stars", BlueColors_p, 8.0, 4, LINEARBLEND, 2.0, 0.0, 1.0);
        ADD_STARRY_NIGHT_EFFECT(BubblyStar, "Big Blooming Rainbow Stars", RainbowColors_p, 2, 12, LINEARBLEND, 1.0);
        ADD_STARRY_NIGHT_EFFECT(BubblyStar, "Neon Bars", RainbowColors_p, 0.5, 64, NOBLEND, 0);
        ADD_EFFECT(EFFECT_STRIP_FIRE_FAN, FireFanEffect, GreenHeatColors_p, NUM_LEDS, 3, 7, 400, 2, NUM_LEDS / 2, Sequential, false, true);
        ADD_EFFECT(EFFECT_STRIP_FIRE_FAN, FireFanEffect, GreenHeatColors_p, NUM_LEDS, 3, 8, 600, 2, NUM_LEDS / 2, Sequential, false, true);
        ADD_EFFECT(EFFECT_STRIP_FIRE_FAN, FireFanEffect, GreenHeatColors_p, NUM_LEDS, 2, 10, 800, 2, NUM_LEDS / 2, Sequential, false, true);
        ADD_EFFECT(EFFECT_STRIP_FIRE_FAN, FireFanEffect, GreenHeatColors_p, NUM_LEDS, 1, 12, 1000, 2, NUM_LEDS / 2, Sequential, false, true);
        ADD_EFFECT(EFFECT_STRIP_FIRE_FAN, FireFanEffect, BlueHeatColors_p, NUM_LEDS, 3, 7, 400, 2, NUM_LEDS / 2, Sequential, false, true);
        ADD_EFFECT(EFFECT_STRIP_FIRE_FAN, FireFanEffect, BlueHeatColors_p, NUM_LEDS, 3, 8, 600, 2, NUM_LEDS / 2, Sequential, false, true);
        ADD_EFFECT(EFFECT_STRIP_FIRE_FAN, FireFanEffect, BlueHeatColors_p, NUM_LEDS, 2, 10, 800, 2, NUM_LEDS / 2, Sequential, false, true);
        ADD_EFFECT(EFFECT_STRIP_FIRE_FAN, FireFanEffect, BlueHeatColors_p, NUM_LEDS, 1, 12, 1000, 2, NUM_LEDS / 2, Sequential, false, true);
        ADD_EFFECT(EFFECT_STRIP_FIRE_FAN, FireFanEffect, HeatColors_p, NUM_LEDS, 3, 7, 400, 2, NUM_LEDS / 2, Sequential, false, true);
        ADD_EFFECT(EFFECT_STRIP_FIRE_FAN, FireFanEffect, HeatColors_p, NUM_LEDS, 3, 8, 600, 2, NUM_LEDS / 2, Sequential, false, true);
        ADD_EFFECT(EFFECT_STRIP_FIRE_FAN, FireFanEffect, HeatColors_p, NUM_LEDS, 2, 10, 800, 2, NUM_LEDS / 2, Sequential, false, true);
        ADD_EFFECT(EFFECT_STRIP_FIRE_FAN, FireFanEffect, HeatColors_p, NUM_LEDS, 1, 12, 1000, 2, NUM_LEDS / 2, Sequential, false, true);

    // === ADDITIONAL EFFECT SETS ===
    // These are effect sets for specific project types that need unique configurations

    #elif defined(EFFECTS_LASERLINE)
        // Laser line effect set
        #ifndef EFFECT_SET_VERSION
            #define EFFECT_SET_VERSION  1
        #endif
        
        ADD_EFFECT(EFFECT_STRIP_LASER_LINE, LaserLineEffect, 500, 20);

    #elif defined(EFFECTS_CHIEFTAIN)
        // Chieftain lantern effect set
        #ifndef EFFECT_SET_VERSION
            #define EFFECT_SET_VERSION  1
        #endif
        
        ADD_EFFECT(EFFECT_STRIP_LANTERN, LanternEffect);
        ADD_EFFECT(EFFECT_STRIP_PALETTE, PaletteEffect, RainbowColors_p, 2.0f, 0.1, 0.0, 1.0, 0.0, LINEARBLEND, true, 1.0);
        ADD_EFFECT(EFFECT_STRIP_RAINBOW_FILL, RainbowFillEffect, 10, 32);

    #elif defined(EFFECTS_PDPGRID)
        // PDP grid matrix effect set
        #ifndef EFFECT_SET_VERSION
            #define EFFECT_SET_VERSION  1
        #endif
        
        ADD_EFFECT(EFFECT_MATRIX_PDPCMX, PDPCMXEffect);
        ADD_EFFECT(EFFECT_MATRIX_PDPGRID, PDPGridEffect);

    #elif defined(EFFECTS_LANTERN)
        // Lantern effect set
        #ifndef EFFECT_SET_VERSION
            #define EFFECT_SET_VERSION  1
        #endif
        
        ADD_EFFECT(EFFECT_STRIP_FIRE, FireEffect, "Calm Fire", NUM_LEDS, 40, 5, 50, 3, 3, true, true);
        // ADD_EFFECT(EFFECT_STRIP_LANTERN, LanternEffect);

    #elif defined(EFFECTS_FULL)
        // Full matrix effect set for advanced displays (Mesmerizer, etc.)
        #ifndef EFFECT_SET_VERSION
            #define EFFECT_SET_VERSION  6
        #endif

        ADD_EFFECT(EFFECT_MATRIX_SPECTRUMBAR,       SpectrumBarEffect,      "Audiograph",  16, 4, 0);
        ADD_EFFECT(EFFECT_MATRIX_SPECTRUM_ANALYZER, SpectrumAnalyzerEffect, "Spectrum", NUM_BANDS, spectrumAltColors, false, 0, 0, 1.6,  1.6);
        ADD_EFFECT(EFFECT_MATRIX_SPECTRUM_ANALYZER, SpectrumAnalyzerEffect, "AudioWave",   MATRIX_WIDTH,  CRGB(0,0,40), 0, 1.25, 1.25, true);
        ADD_EFFECT(EFFECT_MATRIX_SMRADIAL_WAVE,     PatternSMRadialWave);
        ADD_EFFECT(EFFECT_MATRIX_ANIMATEDGIF,       PatternAnimatedGIF,     "Fire Log",    GIFIdentifier::Firelog);
        ADD_EFFECT(EFFECT_MATRIX_ANIMATEDGIF,       PatternAnimatedGIF,     "Pacman",      GIFIdentifier::Pacman);
        ADD_EFFECT(EFFECT_MATRIX_PONG_CLOCK,        PatternPongClock);
        ADD_EFFECT(EFFECT_MATRIX_ANIMATEDGIF,       PatternAnimatedGIF,     "Colorball",   GIFIdentifier::ColorSphere);
        ADD_EFFECT(EFFECT_MATRIX_SMFIRE2021,        PatternSMFire2021);
        ADD_EFFECT(EFFECT_MATRIX_GHOST_WAVE,        GhostWave,              "GhostWave",   0, 30, false,  10);
        ADD_EFFECT(EFFECT_MATRIX_SMGAMMA,           PatternSMGamma);
        ADD_EFFECT(EFFECT_MATRIX_ANIMATEDGIF,       PatternAnimatedGIF,     "Rings",       GIFIdentifier::ThreeRings);
        ADD_EFFECT(EFFECT_MATRIX_ANIMATEDGIF,       PatternAnimatedGIF,     "Atomic",      GIFIdentifier::Atomic);
        ADD_EFFECT(EFFECT_MATRIX_ANIMATEDGIF,       PatternAnimatedGIF,     "Bananaman",   GIFIdentifier::Banana, true, CRGB::DarkBlue);
        ADD_EFFECT(EFFECT_MATRIX_SMMETA_BALLS,      PatternSMMetaBalls);
        ADD_EFFECT(EFFECT_MATRIX_SMSUPERNOVA,       PatternSMSupernova);
        ADD_EFFECT(EFFECT_MATRIX_CUBE,              PatternCube);
        ADD_EFFECT(EFFECT_MATRIX_ANIMATEDGIF,       PatternAnimatedGIF,     "Tesseract",   GIFIdentifier::Tesseract);
        ADD_EFFECT(EFFECT_MATRIX_ANIMATEDGIF,       PatternAnimatedGIF,     "Nyancat",     GIFIdentifier::Nyancat);
        ADD_EFFECT(EFFECT_MATRIX_LIFE,              PatternLife);
        ADD_EFFECT(EFFECT_MATRIX_CIRCUIT,           PatternCircuit);
        ADD_EFFECT(EFFECT_MATRIX_SPECTRUM_ANALYZER, SpectrumAnalyzerEffect, "USA",         NUM_BANDS,     USAColors_p,         true,  0, 0, 0.75, 0.75);
        ADD_EFFECT(EFFECT_MATRIX_SPECTRUM_ANALYZER, SpectrumAnalyzerEffect, "Spectrum 2",  32,            spectrumBasicColors, false, 100, 0, 0.75, 0.75);
        ADD_EFFECT(EFFECT_MATRIX_SPECTRUM_ANALYZER, SpectrumAnalyzerEffect, "Spectrum++",  NUM_BANDS,     spectrumBasicColors, false, 0, 40, -1.0, 2.0);
        ADD_EFFECT(EFFECT_MATRIX_WAVEFORM,          WaveformEffect, "WaveIn", 8);
        ADD_EFFECT(EFFECT_MATRIX_GHOST_WAVE, GhostWave, "WaveOut", 0, 0, true, 0);
        ADD_STARRY_NIGHT_EFFECT(MusicStar, "Stars", RainbowColors_p, 1.0, 1, LINEARBLEND, 2.0, 0.5, 10.0);

      #if ENABLE_WIFI
        ADD_EFFECT(EFFECT_MATRIX_STOCKS,            PatternStocks);
        ADD_EFFECT(EFFECT_MATRIX_SUBSCRIBERS,       PatternSubscribers);
        ADD_EFFECT(EFFECT_MATRIX_WEATHER,           PatternWeather);
      #endif

        ADD_EFFECT(EFFECT_MATRIX_SMSMOKE,           PatternSMSmoke);
        ADD_EFFECT(EFFECT_MATRIX_GHOST_WAVE,        GhostWave, "PlasmaWave", 0, 255,  false);
        ADD_EFFECT(EFFECT_MATRIX_SMNOISE,           PatternSMNoise, "Shikon", PatternSMNoise::EffectType::Shikon_t);
        ADD_EFFECT(EFFECT_MATRIX_SMRADIAL_FIRE,     PatternSMRadialFire);
        ADD_EFFECT(EFFECT_MATRIX_SMFLOW_FIELDS,     PatternSMFlowFields);
        ADD_EFFECT(EFFECT_MATRIX_SMBLURRING_COLORS, PatternSMBlurringColors);
        ADD_EFFECT(EFFECT_MATRIX_SMWALKING_MACHINE, PatternSMWalkingMachine);
        ADD_EFFECT(EFFECT_MATRIX_SMHYPNOSIS,        PatternSMHypnosis);
        ADD_EFFECT(EFFECT_MATRIX_SMSTARDEEP,        PatternSMStarDeep);
        ADD_EFFECT(EFFECT_MATRIX_SM2DDPR,           PatternSM2DDPR);
        ADD_EFFECT(EFFECT_MATRIX_SMPICASSO3IN1,     PatternSMPicasso3in1, "Lines", 38);
        ADD_EFFECT(EFFECT_MATRIX_SMPICASSO3IN1,     PatternSMPicasso3in1, "Circles", 73);
        ADD_EFFECT(EFFECT_MATRIX_SMAMBERRAIN,       PatternSMAmberRain);
        ADD_EFFECT(EFFECT_MATRIX_SMSTROBE_DIFFUSION,PatternSMStrobeDiffusion);
        ADD_EFFECT(EFFECT_MATRIX_SMRAINBOW_TUNNEL,  PatternSMRainbowTunnel);
        ADD_EFFECT(EFFECT_MATRIX_SMSPIRO_PULSE,     PatternSMSpiroPulse);
        ADD_EFFECT(EFFECT_MATRIX_SMTWISTER,         PatternSMTwister);
        ADD_EFFECT(EFFECT_MATRIX_SMHOLIDAY_LIGHTS,  PatternSMHolidayLights);
        ADD_EFFECT(EFFECT_MATRIX_ROSE,              PatternRose);
        ADD_EFFECT(EFFECT_MATRIX_PINWHEEL,          PatternPinwheel);
        ADD_EFFECT(EFFECT_MATRIX_SUNBURST,          PatternSunburst);
        ADD_EFFECT(EFFECT_MATRIX_CLOCK,             PatternClock);
        ADD_EFFECT(EFFECT_MATRIX_ALIEN_TEXT,        PatternAlienText);
        ADD_EFFECT(EFFECT_MATRIX_PULSAR,            PatternPulsar);
        ADD_EFFECT(EFFECT_MATRIX_BOUNCE,            PatternBounce);
        ADD_EFFECT(EFFECT_MATRIX_WAVE,              PatternWave);
        ADD_EFFECT(EFFECT_MATRIX_SWIRL,             PatternSwirl);
        ADD_EFFECT(EFFECT_MATRIX_SERENDIPITY,       PatternSerendipity);
        ADD_EFFECT(EFFECT_MATRIX_MANDALA,           PatternMandala);
        ADD_EFFECT(EFFECT_MATRIX_MUNCH,             PatternMunch);
        ADD_EFFECT(EFFECT_MATRIX_MAZE,              PatternMaze);

    #elif defined(EFFECTS_UMBRELLA) || defined(UMBRELLA)
        // Umbrella-specific effects
        #ifndef EFFECT_SET_VERSION
            #define EFFECT_SET_VERSION  3
        #endif

        ADD_EFFECT(EFFECT_STRIP_FIRE, FireEffect, "Calm Fire", NUM_LEDS, 2, 2, 75, 3, 10, true, false);
        ADD_EFFECT(EFFECT_STRIP_FIRE, FireEffect, "Medium Fire", NUM_LEDS, 1, 5, 100, 3, 4, true, false);
        ADD_EFFECT(EFFECT_STRIP_MUSICAL_PALETTE_FIRE, MusicalPaletteFire, "Musical Red Fire", HeatColors_p, NUM_LEDS, 1, 8, 50, 1, 24, true, false);

        ADD_EFFECT(EFFECT_STRIP_MUSICAL_PALETTE_FIRE, MusicalPaletteFire, "Purple Fire", CRGBPalette16(CRGB::Black, CRGB::Purple, CRGB::MediumPurple, CRGB::LightPink), NUM_LEDS, 2, 3, 150, 3, 10, true, false);
        ADD_EFFECT(EFFECT_STRIP_MUSICAL_PALETTE_FIRE, MusicalPaletteFire, "Purple Fire", CRGBPalette16(CRGB::Black, CRGB::Purple, CRGB::MediumPurple, CRGB::LightPink), NUM_LEDS, 1, 7, 150, 3, 10, true, false);
        ADD_EFFECT(EFFECT_STRIP_MUSICAL_PALETTE_FIRE, MusicalPaletteFire, "Musical Purple Fire", CRGBPalette16(CRGB::Black, CRGB::Purple, CRGB::MediumPurple, CRGB::LightPink), NUM_LEDS, 1, 8, 50, 1, 24, true, false);

        ADD_EFFECT(EFFECT_STRIP_MUSICAL_PALETTE_FIRE, MusicalPaletteFire, "Blue Fire", CRGBPalette16(CRGB::Black, CRGB::DarkBlue, CRGB::Blue, CRGB::LightSkyBlue), NUM_LEDS, 2, 3, 150, 3, 10, true, false);
        ADD_EFFECT(EFFECT_STRIP_MUSICAL_PALETTE_FIRE, MusicalPaletteFire, "Blue Fire", CRGBPalette16(CRGB::Black, CRGB::DarkBlue, CRGB::Blue, CRGB::LightSkyBlue), NUM_LEDS, 1, 7, 150, 3, 10, true, false);
        ADD_EFFECT(EFFECT_STRIP_MUSICAL_PALETTE_FIRE, MusicalPaletteFire, "Musical Blue Fire", CRGBPalette16(CRGB::Black, CRGB::DarkBlue, CRGB::Blue, CRGB::LightSkyBlue), NUM_LEDS, 1, 8, 50, 1, 24, true, false);

        ADD_EFFECT(EFFECT_STRIP_MUSICAL_PALETTE_FIRE, MusicalPaletteFire, "Green Fire", CRGBPalette16(CRGB::Black, CRGB::DarkGreen, CRGB::Green, CRGB::LimeGreen), NUM_LEDS, 2, 3, 150, 3, 10, true, false);
        ADD_EFFECT(EFFECT_STRIP_MUSICAL_PALETTE_FIRE, MusicalPaletteFire, "Green Fire", CRGBPalette16(CRGB::Black, CRGB::DarkGreen, CRGB::Green, CRGB::LimeGreen), NUM_LEDS, 1, 7, 150, 3, 10, true, false);
        ADD_EFFECT(EFFECT_STRIP_MUSICAL_PALETTE_FIRE, MusicalPaletteFire, "Musical Green Fire", CRGBPalette16(CRGB::Black, CRGB::DarkGreen, CRGB::Green, CRGB::LimeGreen), NUM_LEDS, 1, 8, 50, 1, 24, true, false);

        ADD_EFFECT(EFFECT_STRIP_BOUNCING_BALL, BouncingBallEffect);
        ADD_EFFECT(EFFECT_STRIP_DOUBLE_PALETTE, DoublePaletteEffect);

        ADD_EFFECT(EFFECT_STRIP_METEOR, MeteorEffect, 4, 4, 10, 2.0, 2.0);
        ADD_EFFECT(EFFECT_STRIP_METEOR, MeteorEffect, 10, 1, 20, 1.5, 1.5);
        ADD_EFFECT(EFFECT_STRIP_METEOR, MeteorEffect, 25, 1, 40, 1.0, 1.0);
        ADD_EFFECT(EFFECT_STRIP_METEOR, MeteorEffect, 50, 1, 50, 0.5, 0.5);

        ADD_STARRY_NIGHT_EFFECT(QuietStar, "Rainbow Twinkle Stars", RainbowColors_p, STARRYNIGHT_PROBABILITY, 1, LINEARBLEND, 2.0, 0.0, STARRYNIGHT_MUSICFACTOR);       // Rainbow Twinkle
        ADD_STARRY_NIGHT_EFFECT(MusicStar, "RGB Music Blend Stars", RGBColors_p, 0.8, 1, NOBLEND, 15.0, 0.1, 10.0);                                                     // RGB Music Blur - Can You Hear Me Knockin'
        ADD_STARRY_NIGHT_EFFECT(MusicStar, "Rainbow Music Stars", RainbowColors_p, 2.0, 2, LINEARBLEND, 5.0, 0.0, 10.0);                                                // Rainbow Music Star
        ADD_STARRY_NIGHT_EFFECT(BubblyStar,"Little Blooming Rainbow Stars", BlueColors_p, STARRYNIGHT_PROBABILITY, 4, LINEARBLEND, 2.0, 0.0, STARRYNIGHT_MUSICFACTOR); // Blooming Little Rainbow Stars
        ADD_STARRY_NIGHT_EFFECT(QuietStar, "Green Twinkle Stars", GreenColors_p, STARRYNIGHT_PROBABILITY, 1, LINEARBLEND, 2.0, 0.0, STARRYNIGHT_MUSICFACTOR);           // Green Twinkle
        ADD_STARRY_NIGHT_EFFECT(Star, "Blue Sparkle Stars", BlueColors_p, STARRYNIGHT_PROBABILITY, 1, LINEARBLEND, 2.0, 0.0, STARRYNIGHT_MUSICFACTOR);                  // Blue Sparkle
        ADD_STARRY_NIGHT_EFFECT(QuietStar, "Red Twinkle Stars", RedColors_p, 1.0, 1, LINEARBLEND, 2.0);                                                                 // Red Twinkle
        ADD_STARRY_NIGHT_EFFECT(Star, "Lava Stars", LavaColors_p, STARRYNIGHT_PROBABILITY, 1, LINEARBLEND, 2.0, 0.0, STARRYNIGHT_MUSICFACTOR);                          // Lava Stars

        ADD_EFFECT(EFFECT_STRIP_PALETTE, PaletteEffect, RainbowColors_p);
        ADD_EFFECT(EFFECT_STRIP_PALETTE, PaletteEffect, RainbowColors_p, 1.0, 1.0);
        ADD_EFFECT(EFFECT_STRIP_PALETTE, PaletteEffect, RainbowColors_p, .25);

    #elif defined(EFFECTS_SPECTRUM)
        // Spectrum analyzer effect set
        #ifndef EFFECT_SET_VERSION
            #define EFFECT_SET_VERSION  1
        #endif
        
        ADD_EFFECT(EFFECT_MATRIX_SPECTRUM_ANALYZER, SpectrumAnalyzerEffect, "Spectrum Standard", NUM_BANDS, spectrumAltColors, false, 0, 0, 0.5,  1.5);
        ADD_EFFECT(EFFECT_MATRIX_SPECTRUM_ANALYZER, SpectrumAnalyzerEffect, "Spectrum Standard", 24,        spectrumAltColors, false, 0, 0, 1.25, 1.25);
        ADD_EFFECT(EFFECT_MATRIX_SPECTRUM_ANALYZER, SpectrumAnalyzerEffect, "Spectrum Standard", 24,        spectrumAltColors, false, 0, 0, 0.25, 1.25);
        ADD_EFFECT(EFFECT_MATRIX_SPECTRUM_ANALYZER, SpectrumAnalyzerEffect, "Spectrum Standard", 16,        spectrumAltColors, false, 0, 0, 1.0, 1.0);
        ADD_EFFECT(EFFECT_MATRIX_SPECTRUM_ANALYZER, SpectrumAnalyzerEffect, "Spectrum Standard", 48,        CRGB(0,0,4),              0, 1.25, 1.25);
        ADD_EFFECT(EFFECT_MATRIX_GHOST_WAVE, GhostWave, "GhostWave", 0, 16, false, 15);
        ADD_EFFECT(EFFECT_MATRIX_SPECTRUM_ANALYZER, SpectrumAnalyzerEffect, "Spectrum USA",      16,        USAColors_p,       true,  0);
        ADD_EFFECT(EFFECT_MATRIX_GHOST_WAVE, GhostWave, "GhostWave Rainbow", 8);
        ADD_EFFECT(EFFECT_MATRIX_SPECTRUM_ANALYZER, SpectrumAnalyzerEffect, "Spectrum Fade",     24,        RainbowColors_p,   false, 50, 70, -1.0, 2.0);
        ADD_EFFECT(EFFECT_MATRIX_GHOST_WAVE, GhostWave, "GhostWave Blue", 0);
        ADD_EFFECT(EFFECT_MATRIX_SPECTRUM_ANALYZER, SpectrumAnalyzerEffect, "Spectrum Standard", 24,        RainbowColors_p,   false);
        ADD_EFFECT(EFFECT_MATRIX_GHOST_WAVE, GhostWave, "GhostWave One", 4);

    #elif defined(EFFECTS_HELMET)
        // Helmet display effect set
        #ifndef EFFECT_SET_VERSION
            #define EFFECT_SET_VERSION  1
        #endif
        
        ADD_EFFECT(EFFECT_MATRIX_SILON, SilonEffect);
        ADD_EFFECT(EFFECT_MATRIX_SPECTRUM_ANALYZER, SpectrumAnalyzerEffect, "Spectrum Standard", NUM_BANDS, spectrumAltColors, false, 0, 0, 0.5,  1.5);

    #elif defined(EFFECTS_TTGO)
        // TTGO display effect set
        #ifndef EFFECT_SET_VERSION
            #define EFFECT_SET_VERSION  1
        #endif
        
        ADD_EFFECT(EFFECT_MATRIX_SPECTRUM_ANALYZER, SpectrumAnalyzerEffect, "Spectrum Fade", 12, spectrumBasicColors, false, 50, 70, -1.0, 3.0);

    #elif defined(EFFECTS_WROVERKIT)
        // Wrover Kit effect set
        #ifndef EFFECT_SET_VERSION
            #define EFFECT_SET_VERSION  1
        #endif
        
        ADD_EFFECT(EFFECT_STRIP_PALETTE, PaletteEffect, rainbowPalette, 256 / 16, .2, 0);

    #elif defined(EFFECTS_XMASTREES)
        // Christmas trees effect set
        #ifndef EFFECT_SET_VERSION
            #define EFFECT_SET_VERSION  1
        #endif
        
        ADD_EFFECT(EFFECT_STRIP_COLOR_BEAT_OVER_RED, ColorBeatOverRed, "ColorBeatOverRed");
        ADD_EFFECT(EFFECT_STRIP_COLOR_CYCLE, ColorCycleEffect, BottomUp, 6);
        ADD_EFFECT(EFFECT_STRIP_COLOR_CYCLE, ColorCycleEffect, BottomUp, 2);
        ADD_EFFECT(EFFECT_STRIP_RAINBOW_FILL, RainbowFillEffect, 48, 0);
        ADD_EFFECT(EFFECT_STRIP_COLOR_CYCLE, ColorCycleEffect, BottomUp, 3);
        ADD_EFFECT(EFFECT_STRIP_COLOR_CYCLE, ColorCycleEffect, BottomUp, 1);
        ADD_STARRY_NIGHT_EFFECT(LongLifeSparkleStar, "Green Sparkle Stars", GreenColors_p, 2.0, 1, LINEARBLEND, 2.0, 0.0, 0.0, CRGB(0, 128, 0));
        ADD_STARRY_NIGHT_EFFECT(LongLifeSparkleStar, "Red Sparkle Stars", GreenColors_p, 2.0, 1, LINEARBLEND, 2.0, 0.0, 0.0, CRGB::Red);
        ADD_STARRY_NIGHT_EFFECT(LongLifeSparkleStar, "Blue Sparkle Stars", GreenColors_p, 2.0, 1, LINEARBLEND, 2.0, 0.0, 0.0, CRGB::Blue);
        ADD_EFFECT(EFFECT_STRIP_PALETTE, PaletteEffect, rainbowPalette, 256 / 16, .2, 0);

    #elif defined(EFFECTS_INSULATORS)
        // Insulators effect set
        #ifndef EFFECT_SET_VERSION
            #define EFFECT_SET_VERSION  1
        #endif
        
        ADD_EFFECT(EFFECT_STRIP_RAINBOW_FILL, InsulatorSpectrumEffect, "Spectrum Effect", RainbowColors_p);
        ADD_EFFECT(EFFECT_STRIP_NEW_MOLTEN_GLASS_ON_VIOLET_BKGND, NewMoltenGlassOnVioletBkgnd, "Molten Glass", RainbowColors_p);
        ADD_STARRY_NIGHT_EFFECT(MusicStar, "RGB Music Blend Stars", RGBColors_p, 0.8, 1, NOBLEND, 15.0, 0.1, 10.0);
        ADD_STARRY_NIGHT_EFFECT(MusicStar, "Rainbow Music Stars", RainbowColors_p, 2.0, 2, LINEARBLEND, 5.0, 0.0, 10.0);
        ADD_EFFECT(EFFECT_STRIP_PALETTE_REEL, PaletteReelEffect, "PaletteReelEffect");
        ADD_EFFECT(EFFECT_STRIP_COLOR_BEAT_OVER_RED, ColorBeatOverRed, "ColorBeatOverRed");
        ADD_EFFECT(EFFECT_STRIP_TAPE_REEL, TapeReelEffect, "TapeReelEffect");

    #elif defined(EFFECTS_CUBE)
        // Cube display effect set
        #ifndef EFFECT_SET_VERSION
            #define EFFECT_SET_VERSION  1
        #endif
        
        ADD_EFFECT(EFFECT_STRIP_PALETTE, PaletteEffect, rainbowPalette, 256 / 16, .2, 0);
        ADD_EFFECT(EFFECT_STRIP_SPARKLY_SPINNING_MUSIC, SparklySpinningMusicEffect, "SparklySpinningMusical", RainbowColors_p);
        ADD_EFFECT(EFFECT_STRIP_COLOR_BEAT_OVER_RED, ColorBeatOverRed, "ColorBeatOnRedBkgnd");
        ADD_EFFECT(EFFECT_STRIP_SIMPLE_INSULATOR_BEAT2, SimpleInsulatorBeatEffect2, "SimpleInsulatorColorBeat");
        ADD_STARRY_NIGHT_EFFECT(MusicStar, "Rainbow Music Stars", RainbowColors_p, 2.0, 2, LINEARBLEND, 5.0, 0.0, 10.0);

    #elif defined(EFFECTS_BELT)
        // LED belt effect set
        #ifndef EFFECT_SET_VERSION
            #define EFFECT_SET_VERSION  1
        #endif
        
        ADD_EFFECT(EFFECT_TWINKLE, TwinkleEffect, NUM_LEDS / 4, 10);

    #elif defined(EFFECTS_MAGICMIRROR)
        // Magic mirror effect set
        #ifndef EFFECT_SET_VERSION
            #define EFFECT_SET_VERSION  1
        #endif
        
        ADD_EFFECT(EFFECT_STRIP_MOLTEN_GLASS_ON_VIOLET_BKGND, MoltenGlassOnVioletBkgnd, "MoltenGlass", RainbowColors_p);

    #elif defined(EFFECTS_ATOMLIGHT)
        // Atom light effect set
        #ifndef EFFECT_SET_VERSION
            #define EFFECT_SET_VERSION  2
        #endif

        ADD_EFFECT(EFFECT_STRIP_COLOR_FILL, ColorFillEffect, CRGB::White, 1);
        ADD_EFFECT(EFFECT_STRIP_FIRE_FAN, FireFanEffect, HeatColors_p, NUM_LEDS, 2, 2, 200, 2, 5, Sequential, true, false);
        ADD_EFFECT(EFFECT_STRIP_FIRE_FAN, FireFanEffect, HeatColors_p, NUM_LEDS, 1, 12, 400, 2, NUM_LEDS / 2, Sequential, true, false);
        ADD_EFFECT(EFFECT_STRIP_FIRE_FAN, FireFanEffect, GreenHeatColors_p, NUM_LEDS, 1, 10, 400, 2, NUM_LEDS / 2, Sequential, true, false);
        ADD_EFFECT(EFFECT_STRIP_FIRE_FAN, FireFanEffect, BlueHeatColors_p, NUM_LEDS, 1, 10, 400, 2, NUM_LEDS / 2, Sequential, true, false);
        ADD_EFFECT(EFFECT_STRIP_FIRE_FAN, FireFanEffect, RainbowColors_p, NUM_LEDS, 1, 10, 400, 2, NUM_LEDS / 2, Sequential, true, false);
        ADD_EFFECT(EFFECT_STRIP_FIRE_FAN, FireFanEffect, HeatColors_p, NUM_LEDS, 1, 10, 400, 2, NUM_LEDS / 2, Sequential, true, false, true);
        ADD_EFFECT(EFFECT_STRIP_BOUNCING_BALL, BouncingBallEffect, 3, true, true, 1);
        ADD_EFFECT(EFFECT_STRIP_RAINBOW_FILL, RainbowFillEffect, 60, 0);
        ADD_EFFECT(EFFECT_STRIP_COLOR_CYCLE, ColorCycleEffect, Sequential);
        ADD_EFFECT(EFFECT_STRIP_PALETTE, PaletteEffect, RainbowColors_p, 4, 0.1, 0.0, 1.0, 0.0);
        ADD_EFFECT(EFFECT_STRIP_METEOR, MeteorEffect, 20, 1, 25, .15, .05);
        ADD_EFFECT(EFFECT_STRIP_METEOR, MeteorEffect, 12, 1, 25, .15, .08);
        ADD_EFFECT(EFFECT_STRIP_METEOR, MeteorEffect, 6, 1, 25, .15, .12);
        ADD_EFFECT(EFFECT_STRIP_METEOR, MeteorEffect, 1, 1, 5, .15, .25);
        ADD_EFFECT(EFFECT_STRIP_METEOR, MeteorEffect); // Rainbow palette

    #elif defined(EFFECTS_PLATECOVER)
        // Plate cover effect set
        #ifndef EFFECT_SET_VERSION
            #define EFFECT_SET_VERSION  1
        #endif    

        ADD_EFFECT(EFFECT_STRIP_COLOR_FILL, ColorFillEffect, "Solid White", CRGB::White, 1);
        ADD_EFFECT(EFFECT_STRIP_COLOR_FILL, ColorFillEffect, "Solid Red",   CRGB::Red,   1);
        ADD_EFFECT(EFFECT_STRIP_COLOR_FILL, ColorFillEffect, "Solid Amber", CRGB(255, 50, 0), 1);
        ADD_EFFECT(EFFECT_STRIP_FIRE_FAN, FireFanEffect, HeatColors_p, NUM_LEDS, 4, 5.0, 200, 8, 8, Sequential, true, true, true, 90);
        ADD_EFFECT(EFFECT_STRIP_RAINBOW_FILL, RainbowFillEffect, 16, 3, true);
        ADD_EFFECT(EFFECT_STRIP_METEOR, MeteorEffect, 2, 1, 15, .75, .75);
        ADD_EFFECT(EFFECT_STRIP_COLOR_FILL, ColorFillEffect, "Off", CRGB::Black, 1);

    #elif defined(EFFECTS_SPIRALLAMP)
        // Spiral lamp effect set
        #ifndef EFFECT_SET_VERSION
            #define EFFECT_SET_VERSION  3
        #endif

        ADD_EFFECT(EFFECT_STRIP_VUMETER_VERTICAL, VUMeterVerticalEffect);
        ADD_EFFECT(EFFECT_STRIP_METEOR, MeteorEffect, 4, 4, 10, 1.0, 1.0);
        ADD_EFFECT(EFFECT_STRIP_COLOR_FILL, ColorFillEffect, "Solid White", CRGB::White, 1);
        ADD_EFFECT(EFFECT_STRIP_FIRE_FAN, FireFanEffect, HeatColors_p,      NUM_LEDS, 1, 2.5, 200, 2, 15, Sequential, true, false);
        ADD_EFFECT(EFFECT_STRIP_FIRE_FAN, FireFanEffect, GreenHeatColors_p, NUM_LEDS, 1, 2.5, 200, 2, 15, Sequential, true, false);
        ADD_EFFECT(EFFECT_STRIP_FIRE_FAN, FireFanEffect, BlueHeatColors_p,  NUM_LEDS, 1, 2.5, 200, 2, 15, Sequential, true, false);
        ADD_EFFECT(EFFECT_STRIP_FIRE_FAN, FireFanEffect, RainbowColors_p,   NUM_LEDS, 1, 2.5, 200, 2, 15, Sequential, true, false);
        ADD_EFFECT(EFFECT_STRIP_FIRE_FAN, FireFanEffect, HeatColors_p,      NUM_LEDS, 1, 2.5, 200, 2, 15, Sequential, true, false, true);
        ADD_EFFECT(EFFECT_STRIP_RAINBOW_FILL, RainbowFillEffect, 120, 0);
        ADD_EFFECT(EFFECT_STRIP_PALETTE, PaletteEffect, RainbowColors_p, 4, 0.1, 0.0, 1.0, 0.0);
        ADD_EFFECT(EFFECT_STRIP_BOUNCING_BALL, BouncingBallEffect, 3, true, true, 8);
        ADD_STARRY_NIGHT_EFFECT(MusicStar, "Rainbow Twinkle Stars", RainbowColors_p, STARRYNIGHT_PROBABILITY, 1, LINEARBLEND, 0.0, 0.0, STARRYNIGHT_MUSICFACTOR);
        ADD_STARRY_NIGHT_EFFECT(Star, "Rainbow Twinkle Stars", RainbowColors_p,  STARRYNIGHT_PROBABILITY, 1, LINEARBLEND, 2.0, 0.0, STARRYNIGHT_MUSICFACTOR);
        ADD_STARRY_NIGHT_EFFECT(Star,      "Red Sparkle Stars",   RedColors_p,   STARRYNIGHT_PROBABILITY, 1, LINEARBLEND, 2.0, 0.0, STARRYNIGHT_MUSICFACTOR);
        ADD_STARRY_NIGHT_EFFECT(MusicStar, "Red Stars",           RedColors_p,   STARRYNIGHT_PROBABILITY, 1, LINEARBLEND, 2.0, 0.0, STARRYNIGHT_MUSICFACTOR);
        ADD_STARRY_NIGHT_EFFECT(Star,      "Blue Sparkle Stars",  BlueColors_p,  STARRYNIGHT_PROBABILITY, 1, LINEARBLEND, 2.0, 0.0, STARRYNIGHT_MUSICFACTOR);
        ADD_STARRY_NIGHT_EFFECT(MusicStar, "Blue Stars",          BlueColors_p,  STARRYNIGHT_PROBABILITY, 1, LINEARBLEND, 2.0, 0.0, STARRYNIGHT_MUSICFACTOR);
        ADD_STARRY_NIGHT_EFFECT(Star,      "Green Sparkle Stars", GreenColors_p, STARRYNIGHT_PROBABILITY, 1, LINEARBLEND, 2.0, 0.0, STARRYNIGHT_MUSICFACTOR);
        ADD_STARRY_NIGHT_EFFECT(MusicStar, "Green Stars",         GreenColors_p, STARRYNIGHT_PROBABILITY, 1, LINEARBLEND, 2.0, 0.0, STARRYNIGHT_MUSICFACTOR);
        ADD_EFFECT(EFFECT_STRIP_TWINKLE, TwinkleEffect, NUM_LEDS / 2, 20, 50);
        ADD_EFFECT(EFFECT_STRIP_PALETTE, PaletteEffect, RainbowColors_p, .25, 1, 0, 1.0, 0.0, LINEARBLEND, true, 1.0);
        ADD_EFFECT(EFFECT_STRIP_PALETTE, PaletteEffect, RainbowColors_p);

    #elif defined(EFFECTS_HEXAGON)
        // Hexagon effect set
        #ifndef EFFECT_SET_VERSION
            #define EFFECT_SET_VERSION  1
        #endif
        
        ADD_EFFECT(EFFECT_HEXAGON_OUTER_RING, OuterHexRingEffect);

    #else
        // Default fallback - simple effect if not otherwise defined
        ADD_EFFECT(EFFECT_STRIP_RAINBOW_FILL, RainbowFillEffect, 6, 2);

    #endif

    // Set the effect set version to the default value of 1 if none was set yet
    #ifndef EFFECT_SET_VERSION
        #define EFFECT_SET_VERSION  1
    #endif

    // If this assert fires, you have not defined any effects in the table above.  If adding a new config, you need to
    // add the list of effects in this table as shown for the various other existing configs.  You MUST have at least
    // one effect even if it's the Status effect.
    assert(!g_ptrEffectFactories->IsEmpty());
}

// Load the effects JSON file and check if it's appropriate to use
std::optional<JsonObjectConst> LoadEffectsJSONFile(JsonDocument& jsonDoc)
{
    // If the effect set version is defined to 0, we ignore whatever is persisted
    if (EFFECT_SET_VERSION == 0)
        return {};

    if (!LoadJSONFile(EFFECTS_CONFIG_FILE, jsonDoc))
        return {};

    auto jsonObject = jsonDoc.as<JsonObjectConst>();

    // Ignore JSON if it was persisted for a different project
    if (jsonObject[PTY_PROJECT].is<String>()
        && jsonObject[PTY_PROJECT].as<String>() != PROJECT_NAME)
    {
        return {};
    }

    // Default to 1 if no effect set version was persisted
    int jsonVersion = jsonObject[PTY_EFFECTSETVER].is<int>() ? jsonObject[PTY_EFFECTSETVER] : 1;

    // Only return the JSON object if the persistent version matches the current one
    if (jsonVersion == EFFECT_SET_VERSION)
        return jsonObject;

    return {};
}

// Load the default effect set. It's defined here because it uses EFFECT_SET_VERSION.
void EffectManager::LoadDefaultEffects()
{
    _effectSetVersion = EFFECT_SET_VERSION;

    for (const auto &numberedFactory : g_ptrEffectFactories->GetDefaultFactories())
        ProduceAndLoadDefaultEffect(numberedFactory);

    SetInterval(DEFAULT_EFFECT_INTERVAL, true);

    construct(true);
}
