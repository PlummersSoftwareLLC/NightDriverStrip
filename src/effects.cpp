//+--------------------------------------------------------------------------
//
// File:        effects.cpp
//
// NightDriverStrip - (c) 2018 Plummer's Software LLC.  All Rights Reserved.
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
//    Main table of built-in effects and related constants and data
//
// History:     Jul-14-2021         Davepl      Split off from main.cpp
//---------------------------------------------------------------------------

#include "globals.h"
#include "SPIFFS.h"
#include "effectdependencies.h"
#include "systemcontainer.h"

// Palettes
//
// Palettes that are referenced by effects need to be instantiated first

const CRGBPalette16 BlueColors_p =
{
    CRGB::DarkBlue,
    CRGB::MediumBlue,
    CRGB::Blue,
    CRGB::MediumBlue,
    CRGB::DarkBlue,
    CRGB::MediumBlue,
    CRGB::Blue,
    CRGB::MediumBlue,
    CRGB::DarkBlue,
    CRGB::MediumBlue,
    CRGB::Blue,
    CRGB::MediumBlue,
    CRGB::DarkBlue,
    CRGB::MediumBlue,
    CRGB::Blue,
    CRGB::MediumBlue
};

const CRGBPalette16 RedColors_p =
{
    CRGB::Red,
    CRGB::DarkRed,
    CRGB::DarkRed,
    CRGB::DarkRed,

    CRGB::Red,
    CRGB::DarkRed,
    CRGB::DarkRed,
    CRGB::DarkRed,

    CRGB::Red,
    CRGB::DarkRed,
    CRGB::DarkRed,
    CRGB::DarkRed,

    CRGB::Red,
    CRGB::DarkRed,
    CRGB::DarkRed,
    CRGB::OrangeRed
};

const CRGBPalette16 GreenColors_p =
{
    CRGB::Green,
    CRGB::DarkGreen,
    CRGB::DarkGreen,
    CRGB::DarkGreen,

    CRGB::Green,
    CRGB::DarkGreen,
    CRGB::DarkGreen,
    CRGB::DarkGreen,

    CRGB::Green,
    CRGB::DarkGreen,
    CRGB::DarkGreen,
    CRGB::DarkGreen,

    CRGB::Green,
    CRGB::DarkGreen,
    CRGB::DarkGreen,
    CRGB::LimeGreen
};

const CRGBPalette16 PurpleColors_p =
{
    CRGB::Purple,
    CRGB::Maroon,
    CRGB::Violet,
    CRGB::DarkViolet,

    CRGB::Purple,
    CRGB::Maroon,
    CRGB::Violet,
    CRGB::DarkViolet,

    CRGB::Purple,
    CRGB::Maroon,
    CRGB::Violet,
    CRGB::DarkViolet,

    CRGB::Pink,
    CRGB::Maroon,
    CRGB::Violet,
    CRGB::DarkViolet,
};

const CRGBPalette16 RGBColors_p =
{
    CRGB::Red,
    CRGB::Green,
    CRGB::Blue,
    CRGB::Red,
    CRGB::Green,
    CRGB::Blue,
    CRGB::Red,
    CRGB::Green,
    CRGB::Blue,
    CRGB::Red,
    CRGB::Green,
    CRGB::Blue,
    CRGB::Red,
    CRGB::Green,
    CRGB::Blue,
    CRGB::Blue
};

const CRGBPalette16 MagentaColors_p =
{
    CRGB::Pink,
    CRGB::DeepPink,
    CRGB::HotPink,
    CRGB::LightPink,
    CRGB::LightCoral,
    CRGB::Purple,
    CRGB::MediumPurple,
    CRGB::Magenta,
    CRGB::DarkMagenta,
    CRGB::DarkSalmon,
    CRGB::MediumVioletRed,
    CRGB::Pink,
    CRGB::DeepPink,
    CRGB::HotPink,
    CRGB::LightPink,
    CRGB::Magenta};

const CRGBPalette16 spectrumBasicColors =
{
    CRGB(0xFD0E35), // Red
    CRGB(0xFF8833), // Orange
    CRGB(0xFFEB00), // Middle Yellow
    CRGB(0xAFE313), // Inchworm
    CRGB(0x3AA655), // Green
    CRGB(0x8DD9CC), // Middle Blue Green
    CRGB(0x0066FF), // Blue III
    CRGB(0xDB91EF), // Lilac
    CRGB(0xFD0E35), // Red
    CRGB(0xFF8833), // Orange
    CRGB(0xFFEB00), // Middle Yellow
    CRGB(0xAFE313), // Inchworm
    CRGB(0x3AA655), // Green
    CRGB(0x8DD9CC), // Middle Blue Green
    CRGB(0x0066FF), // Blue III
    CRGB(0xDB91EF)  // Lilac
};


const CRGBPalette16 spectrumAltColors =
{
    CRGB::Red,
    CRGB::OrangeRed,
    CRGB::Orange,
    CRGB::Green,
    CRGB::ForestGreen,
    CRGB::Cyan,
    CRGB::Blue,
    CRGB::Indigo,
    CRGB::Red,
    CRGB::OrangeRed,
    CRGB::Orange,
    CRGB::Green,
    CRGB::ForestGreen,
    CRGB::Cyan,
    CRGB::Blue,
    CRGB::Indigo,
};

const CRGBPalette16 USAColors_p =
{
    CRGB::Blue,
    CRGB::Blue,
    CRGB::Blue,
    CRGB::Blue,
    CRGB::Blue,
    CRGB::Red,
    CRGB::White,
    CRGB::Red,
    CRGB::White,
    CRGB::Red,
    CRGB::White,
    CRGB::Red,
    CRGB::White,
    CRGB::Red,
    CRGB::White,
    CRGB::Red,
};

const CRGBPalette16 rainbowPalette(RainbowColors_p);

#if ENABLE_AUDIO

// GetSpectrumAnalyzer
//
// A little factory that makes colored spectrum analyzers to be used by the remote control
// colored buttons

std::shared_ptr<LEDStripEffect> GetSpectrumAnalyzer(CRGB color1, CRGB color2)
{
    auto object = make_shared_psram<SpectrumAnalyzerEffect>("Spectrum Clr", 24, CRGBPalette16(color1, color2));
    if (object->Init(g_ptrSystem->Devices()))
        return object;
    throw std::runtime_error("Could not initialize new spectrum analyzer, two color version!");
}

std::shared_ptr<LEDStripEffect> GetSpectrumAnalyzer(CRGB color)
{
    CHSV hueColor = rgb2hsv_approximate(color);
    CRGB color2 = CRGB(CHSV(hueColor.hue + 64, 255, 255));
    auto object = make_shared_psram<SpectrumAnalyzerEffect>("Spectrum Clr", 24, CRGBPalette16(color, color2));
    if (object->Init(g_ptrSystem->Devices()))
        return object;
    throw std::runtime_error("Could not initialize new spectrum analyzer, one color version!");
}

#endif

#define STARRYNIGHT_PROBABILITY 1.0
#define STARRYNIGHT_MUSICFACTOR 1.0

static DRAM_ATTR std::unique_ptr<EffectFactories> l_ptrEffectFactories = nullptr;
static std::map<int, JSONEffectFactory> l_JsonStarryNightEffectFactories =
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

std::shared_ptr<LEDStripEffect> CreateStarryNightEffectFromJSON(const JsonObjectConst& jsonObject)
{
    auto entry = l_JsonStarryNightEffectFactories.find(jsonObject[PTY_STARTYPENR]);

    return entry != l_JsonStarryNightEffectFactories.end()
        ? entry->second(jsonObject)
        : nullptr;
}

#define ADD_EFFECT(effectNumber, effectType, ...)   l_ptrEffectFactories->AddEffect(effectNumber, \
    []()                                 ->std::shared_ptr<LEDStripEffect> { return make_shared_psram<effectType>(__VA_ARGS__); }, \
    [](const JsonObjectConst& jsonObject)->std::shared_ptr<LEDStripEffect> { return make_shared_psram<effectType>(jsonObject); })

#define ADD_STARRY_NIGHT_EFFECT(starType, ...)      l_ptrEffectFactories->AddEffect(EFFECT_STRIP_STARRY_NIGHT, \
    []()                                 ->std::shared_ptr<LEDStripEffect> { return make_shared_psram<StarryNightEffect<starType>>(__VA_ARGS__); }, \
    [](const JsonObjectConst& jsonObject)->std::shared_ptr<LEDStripEffect> { return CreateStarryNightEffectFromJSON(jsonObject); })

void LoadEffectFactories()
{
    // Check if the factories have already been loaded
    if (l_ptrEffectFactories)
        return;

    l_ptrEffectFactories = make_unique_psram<EffectFactories>();

    // Fill effect factories
    #if DEMO

        ADD_EFFECT(EFFECT_STRIP_RAINBOW_FILL, RainbowFillEffect, 6, 2);

    #elif LASERLINE

        ADD_EFFECT(EFFECT_STRIP_LASER_LINE, LaserLineEffect, 500, 20);

    #elif CHIEFTAIN

        ADD_EFFECT(EFFECT_STRIP_LANTERN, LanternEffect);
        ADD_EFFECT(EFFECT_STRIP_PALETTE, PaletteEffect, RainbowColors_p, 2.0f, 0.1, 0.0, 1.0, 0.0, LINEARBLEND, true, 1.0);
        ADD_EFFECT(EFFECT_STRIP_RAINBOW_FILL, RainbowFillEffect, 10, 32);

    #elif LANTERN

        ADD_EFFECT(EFFECT_STRIP_FIRE, FireEffect, "Calm Fire", NUM_LEDS, 40, 5, 50, 3, 3, true, true);
        // ADD_EFFECT(EFFECT_STRIP_LANTERN, LanternEffect);

    #elif MESMERIZER

        ADD_EFFECT(EFFECT_MATRIX_SPECTRUMBAR,       SpectrumBarEffect,      "Audiograph");
        ADD_EFFECT(EFFECT_MATRIX_GHOST_WAVE,        GhostWave, "GhostWave", 0, 30, false, 10);
        ADD_EFFECT(EFFECT_MATRIX_GHOST_WAVE,        GhostWave, "PlasmaWave", 0, 255,  false);
        ADD_EFFECT(EFFECT_MATRIX_SPECTRUM_ANALYZER, SpectrumAnalyzerEffect, "AudioWave",  MATRIX_WIDTH,  CRGB(0,0,40),        0, 0, 1.25, 1.25);

        ADD_EFFECT(EFFECT_MATRIX_SPECTRUM_ANALYZER, SpectrumAnalyzerEffect, "Spectrum",   NUM_BANDS,     spectrumBasicColors, 100, 0, 0.75, 0.75);
        ADD_EFFECT(EFFECT_MATRIX_SPECTRUM_ANALYZER, SpectrumAnalyzerEffect, "USA",        NUM_BANDS,     USAColors_p,           0, 0, 0.75, 0.75);

        ADD_EFFECT(EFFECT_MATRIX_SPECTRUM_ANALYZER, SpectrumAnalyzerEffect, "Spectrum 2", 32,            spectrumBasicColors, 100, 0, 0.75, 0.75);
        ADD_EFFECT(EFFECT_MATRIX_SPECTRUM_ANALYZER, SpectrumAnalyzerEffect, "Spectrum 3", 32,            spectrumBasicColors, 100, 0, 0.75, 0.75);


        ADD_EFFECT(EFFECT_MATRIX_SPECTRUM_ANALYZER, SpectrumAnalyzerEffect, "Spectrum++", NUM_BANDS,     spectrumBasicColors, 0, 40, -1.0, 2.0);

        ADD_EFFECT(EFFECT_MATRIX_WAVEFORM,          WaveformEffect, "WaveIn", 8);
        ADD_EFFECT(EFFECT_MATRIX_GHOST_WAVE,        GhostWave, "WaveOut", 0, 0, true, 0);

        ADD_STARRY_NIGHT_EFFECT(MusicStar, "Stars", RainbowColors_p, 1.0, 1, LINEARBLEND, 2.0, 0.5, 10.0); // Rainbow Music Star

        ADD_EFFECT(EFFECT_MATRIX_PONG_CLOCK,        PatternPongClock);
        ADD_EFFECT(EFFECT_MATRIX_SUBSCRIBERS,       PatternSubscribers);
        ADD_EFFECT(EFFECT_MATRIX_WEATHER,           PatternWeather);

        ADD_EFFECT(EFFECT_MATRIX_CUBE,              PatternCube);
        ADD_EFFECT(EFFECT_MATRIX_LIFE,              PatternLife);
        ADD_EFFECT(EFFECT_MATRIX_ROSE,              PatternRose);
        ADD_EFFECT(EFFECT_MATRIX_PINWHEEL,          PatternPinwheel);
        ADD_EFFECT(EFFECT_MATRIX_SUNBURST,          PatternSunburst);
        ADD_EFFECT(EFFECT_MATRIX_FLOW_FIELD,        PatternFlowField);
        ADD_EFFECT(EFFECT_MATRIX_CLOCK,             PatternClock);
        ADD_EFFECT(EFFECT_MATRIX_ALIEN_TEXT,        PatternAlienText);
        ADD_EFFECT(EFFECT_MATRIX_CIRCUIT,           PatternCircuit);
        ADD_EFFECT(EFFECT_MATRIX_PULSAR,            PatternPulsar);
        ADD_EFFECT(EFFECT_MATRIX_BOUNCE,            PatternBounce);
        ADD_EFFECT(EFFECT_MATRIX_SPIRO,             PatternSpiro);
        ADD_EFFECT(EFFECT_MATRIX_WAVE,              PatternWave);
        ADD_EFFECT(EFFECT_MATRIX_SWIRL,             PatternSwirl);
        ADD_EFFECT(EFFECT_MATRIX_SERENDIPITY,       PatternSerendipity);
        ADD_EFFECT(EFFECT_MATRIX_MANDALA,           PatternMandala);
        ADD_EFFECT(EFFECT_MATRIX_PALETTE_SMEAR,     PatternPaletteSmear);
        ADD_EFFECT(EFFECT_MATRIX_CURTAIN,           PatternCurtain);
        ADD_EFFECT(EFFECT_MATRIX_GRID_LIGHTS,       PatternGridLights);
        ADD_EFFECT(EFFECT_MATRIX_MUNCH,             PatternMunch);
        ADD_EFFECT(EFFECT_MATRIX_MAZE,              PatternMaze);

        // make_shared_psram<PatternInfinity>(),
        // make_shared_psram<PatternQR>(),

    #elif UMBRELLA

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

    #elif TTGO

        // Animate a simple rainbow palette by using the palette effect on the built-in rainbow palette
        ADD_EFFECT(EFFECT_MATRIX_SPECTRUM_ANALYZER, SpectrumAnalyzerEffect, "Spectrum Fade", 12, spectrumBasicColors, 50, 70, -1.0, 3.0);

    #elif WROVERKIT

        // Animate a simple rainbow palette by using the palette effect on the built-in rainbow palette
        ADD_EFFECT(EFFECT_STRIP_PALETTE, PaletteEffect, rainbowPalette, 256 / 16, .2, 0);

    #elif XMASTREES

        ADD_EFFECT(EFFECT_STRIP_COLOR_BEAT_OVER_RED, ColorBeatOverRed, "ColorBeatOverRed");

        ADD_EFFECT(EFFECT_STRIP_COLOR_CYCLE, ColorCycleEffect, BottomUp, 6);
        ADD_EFFECT(EFFECT_STRIP_COLOR_CYCLE, ColorCycleEffect, BottomUp, 2);

        ADD_EFFECT(EFFECT_STRIP_RAINBOW_FILL, RainbowFillEffect, 48, 0);

        ADD_EFFECT(EFFECT_STRIP_COLOR_CYCLE, ColorCycleEffect, BottomUp, 3);
        ADD_EFFECT(EFFECT_STRIP_COLOR_CYCLE, ColorCycleEffect, BottomUp, 1);

        ADD_STARRY_NIGHT_EFFECT(LongLifeSparkleStar, "Green Sparkle Stars", GreenColors_p, 2.0, 1, LINEARBLEND, 2.0, 0.0, 0.0, CRGB(0, 128, 0)); // Blue Sparkle
        ADD_STARRY_NIGHT_EFFECT(LongLifeSparkleStar, "Red Sparkle Stars", GreenColors_p, 2.0, 1, LINEARBLEND, 2.0, 0.0, 0.0, CRGB::Red);         // Blue Sparkle
        ADD_STARRY_NIGHT_EFFECT(LongLifeSparkleStar, "Blue Sparkle Stars", GreenColors_p, 2.0, 1, LINEARBLEND, 2.0, 0.0, 0.0, CRGB::Blue);       // Blue Sparkle

        ADD_EFFECT(EFFECT_STRIP_PALETTE, PaletteEffect, rainbowPalette, 256 / 16, .2, 0);

    #elif INSULATORS

        ADD_EFFECT(EFFECT_STRIP_RAINBOW_FILL, InsulatorSpectrumEffect, "Spectrum Effect", RainbowColors_p);
        ADD_EFFECT(EFFECT_STRIP_NEW_MOLTEN_GLASS_ON_VIOLET_BKGND, NewMoltenGlassOnVioletBkgnd, "Molten Glass", RainbowColors_p);
        ADD_STARRY_NIGHT_EFFECT(MusicStar, "RGB Music Blend Stars", RGBColors_p, 0.8, 1, NOBLEND, 15.0, 0.1, 10.0);      // RGB Music Blur - Can You Hear Me Knockin'
        ADD_STARRY_NIGHT_EFFECT(MusicStar, "Rainbow Music Stars", RainbowColors_p, 2.0, 2, LINEARBLEND, 5.0, 0.0, 10.0); // Rainbow Music Star
        ADD_EFFECT(EFFECT_STRIP_PALETTE_REEL, PaletteReelEffect, "PaletteReelEffect");
        ADD_EFFECT(EFFECT_STRIP_COLOR_BEAT_OVER_RED, ColorBeatOverRed, "ColorBeatOverRed");
        ADD_EFFECT(EFFECT_STRIP_TAPE_REEL, TapeReelEffect, "TapeReelEffect");

    #elif CUBE

        // Simple rainbow pallette
        ADD_EFFECT(EFFECT_STRIP_PALETTE, PaletteEffect, rainbowPalette, 256 / 16, .2, 0);

        ADD_EFFECT(EFFECT_STRIP_SPARKLY_SPINNING_MUSIC, SparklySpinningMusicEffect, "SparklySpinningMusical", RainbowColors_p);
        ADD_EFFECT(EFFECT_STRIP_COLOR_BEAT_OVER_RED, ColorBeatOverRed, "ColorBeatOnRedBkgnd");
        ADD_EFFECT(EFFECT_STRIP_SIMPLE_INSULATOR_BEAT2, SimpleInsulatorBeatEffect2, "SimpleInsulatorColorBeat");
        ADD_STARRY_NIGHT_EFFECT(MusicStar, "Rainbow Music Stars", RainbowColors_p, 2.0, 2, LINEARBLEND, 5.0, 0.0, 10.0); // Rainbow Music Star

    #elif BELT

        // Yes, I made a sparkly LED belt and wore it to a party.  Batteries toO!
        ADD_EFFECT(EFFECT_TWINKLE, TwinkleEffect, NUM_LEDS / 4, 10);

    #elif MAGICMIRROR

        ADD_EFFECT(EFFECT_STRIP_MOLTEN_GLASS_ON_VIOLET_BKGND, MoltenGlassOnVioletBkgnd, "MoltenGlass", RainbowColors_p);

    #elif SPECTRUM

        ADD_EFFECT(EFFECT_MATRIX_SPECTRUM_ANALYZER, SpectrumAnalyzerEffect, "Spectrum Standard", NUM_BANDS, spectrumAltColors, 0, 0, 0.5,  1.5);
        ADD_EFFECT(EFFECT_MATRIX_SPECTRUM_ANALYZER, SpectrumAnalyzerEffect, "Spectrum Standard", 24, spectrumAltColors, 0, 0, 1.25, 1.25);
        ADD_EFFECT(EFFECT_MATRIX_SPECTRUM_ANALYZER, SpectrumAnalyzerEffect, "Spectrum Standard", 24, spectrumAltColors, 0, 0, 0.25,  1.25);

        ADD_EFFECT(EFFECT_MATRIX_SPECTRUM_ANALYZER, SpectrumAnalyzerEffect, "Spectrum Standard", 16, spectrumAltColors, 0, 0, 1.0, 1.0);

        ADD_EFFECT(EFFECT_MATRIX_SPECTRUM_ANALYZER, SpectrumAnalyzerEffect, "Spectrum Standard", 48, CRGB(0,0,4), 0, 0, 1.25, 1.25);

        ADD_EFFECT(EFFECT_MATRIX_GHOST_WAVE, GhostWave, "GhostWave", 0, 16, false, 15);
        ADD_EFFECT(EFFECT_MATRIX_SPECTRUM_ANALYZER, SpectrumAnalyzerEffect, "Spectrum USA", 16, USAColors_p, 0);
        ADD_EFFECT(EFFECT_MATRIX_GHOST_WAVE, GhostWave, "GhostWave Rainbow", 8);
        ADD_EFFECT(EFFECT_MATRIX_SPECTRUM_ANALYZER, SpectrumAnalyzerEffect, "Spectrum Fade", 24, RainbowColors_p, 50, 70, -1.0, 2.0);
        ADD_EFFECT(EFFECT_MATRIX_GHOST_WAVE, GhostWave, "GhostWave Blue", 0);
        ADD_EFFECT(EFFECT_MATRIX_SPECTRUM_ANALYZER, SpectrumAnalyzerEffect, "Spectrum Standard", 24, RainbowColors_p);
        ADD_EFFECT(EFFECT_MATRIX_GHOST_WAVE, GhostWave, "GhostWave One", 4);

        //make_shared_psram<GhostWave>("GhostWave Rainbow", &rainbowPalette),

    #elif ATOMLIGHT

        ADD_EFFECT(EFFECT_STRIP_COLOR_FILL, ColorFillEffect, CRGB::White, 1);
        // make_shared_psram<FireFanEffect>(NUM_LEDS, 1, 15, 80, 2, 7, Sequential, true, false),
        // make_shared_psram<FireFanEffect>(NUM_LEDS, 1, 15, 80, 2, 7, Sequential, true, false, true),
        // make_shared_psram<HueFireFanEffect>(NUM_LEDS, 2, 5, 120, 1, 1, Sequential, true, false, false, HUE_BLUE),
        //  make_shared_psram<HueFireFanEffect>(NUM_LEDS, 2, 3, 100, 1, 1, Sequential, true, false, false, HUE_GREEN),
        ADD_EFFECT(EFFECT_STRIP_RAINBOW_FILL, RainbowFillEffect, 60, 0);
        ADD_EFFECT(EFFECT_STRIP_COLOR_CYCLE, ColorCycleEffect, Sequential);
        ADD_EFFECT(EFFECT_STRIP_PALETTE, PaletteEffect, RainbowColors_p, 4, 0.1, 0.0, 1.0, 0.0);
        ADD_EFFECT(EFFECT_STRIP_BOUNCING_BALL, BouncingBallEffect, 3, true, true, 1);

        ADD_STARRY_NIGHT_EFFECT(BubblyStar, "Little Blooming Rainbow Stars", BlueColors_p, 8.0, 4, LINEARBLEND, 2.0, 0.0, 4); // Blooming Little Rainbow Stars
        ADD_STARRY_NIGHT_EFFECT(BubblyStar, "Big Blooming Rainbow Stars", RainbowColors_p, 20, 12, LINEARBLEND, 1.0, 0.0, 2); // Blooming Rainbow Stars
        //        make_shared_psram<StarryNightEffect<FanStar>>("FanStars", RainbowColors_p, 8.0, 1.0, LINEARBLEND, 80.0, 0, 2.0),

        ADD_EFFECT(EFFECT_STRIP_METEOR, MeteorEffect, 20, 1, 25, .15, .05);
        ADD_EFFECT(EFFECT_STRIP_METEOR, MeteorEffect, 12, 1, 25, .15, .08);
        ADD_EFFECT(EFFECT_STRIP_METEOR, MeteorEffect, 6, 1, 25, .15, .12);
        ADD_EFFECT(EFFECT_STRIP_METEOR, MeteorEffect, 1, 1, 5, .15, .25);
        ADD_EFFECT(EFFECT_STRIP_METEOR, MeteorEffect); // Rainbow palette

    #elif FANSET

        ADD_EFFECT(EFFECT_STRIP_RAINBOW_FILL, RainbowFillEffect, 24, 0);

        ADD_EFFECT(EFFECT_STRIP_COLOR_CYCLE, ColorCycleEffect, BottomUp);
        ADD_EFFECT(EFFECT_STRIP_COLOR_CYCLE, ColorCycleEffect, TopDown);
        ADD_EFFECT(EFFECT_STRIP_COLOR_CYCLE, ColorCycleEffect, LeftRight);
        ADD_EFFECT(EFFECT_STRIP_COLOR_CYCLE, ColorCycleEffect, RightLeft);

        ADD_EFFECT(EFFECT_STRIP_PALETTE_REEL, PaletteReelEffect, "PaletteReelEffect");
        ADD_EFFECT(EFFECT_STRIP_METEOR, MeteorEffect);
        ADD_EFFECT(EFFECT_STRIP_TAPE_REEL, TapeReelEffect, "TapeReelEffect");

        ADD_STARRY_NIGHT_EFFECT(MusicStar, "RGB Music Blend Stars", RGBColors_p, 0.8, 1, NOBLEND, 15.0, 0.1, 10.0);      // RGB Music Blur - Can You Hear Me Knockin'
        ADD_STARRY_NIGHT_EFFECT(MusicStar, "Rainbow Music Stars", RainbowColors_p, 2.0, 2, LINEARBLEND, 5.0, 0.0, 10.0); // Rainbow Music Star

        ADD_EFFECT(EFFECT_STRIP_FAN_BEAT, FanBeatEffect, "FanBeat");

        ADD_STARRY_NIGHT_EFFECT(BubblyStar, "Little Blooming Rainbow Stars", BlueColors_p, 8.0, 4, LINEARBLEND, 2.0, 0.0, 1.0); // Blooming Little Rainbow Stars
        ADD_STARRY_NIGHT_EFFECT(BubblyStar, "Big Blooming Rainbow Stars", RainbowColors_p, 2, 12, LINEARBLEND, 1.0);            // Blooming Rainbow Stars
        ADD_STARRY_NIGHT_EFFECT(BubblyStar, "Neon Bars", RainbowColors_p, 0.5, 64, NOBLEND, 0);                                 // Neon Bars

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

    #elif HEXAGON

        ADD_EFFECT(EFFECT_HEXAGON_OUTER_RING, OuterHexRingEffect);
        //ADD_EFFECT(EFFECT_STRIP_RAINBOW_FILL, RainbowFillEffect, 24, 4);

    #elif LEDSTRIP

        ADD_EFFECT(EFFECT_STRIP_STATUS, StatusEffect, CRGB::White);

    #else

        ADD_EFFECT(EFFECT_STRIP_RAINBOW_FILL, RainbowFillEffect, 6, 2);                    // Simple effect if not otherwise defined above

    #endif

    // If this assert fires, you have not defined any effects in the table above.  If adding a new config, you need to
    // add the list of effects in this table as shown for the various other existing configs.  You MUST have at least
    // one effect even if it's the Status effect.
    assert(!l_ptrEffectFactories->IsEmpty());
}

static DRAM_ATTR size_t l_EffectsManagerJSONBufferSize = 0;
static DRAM_ATTR size_t l_EffectsManagerJSONWriterIndex = std::numeric_limits<size_t>::max();
static DRAM_ATTR size_t l_CurrentEffectWriterIndex = std::numeric_limits<size_t>::max();

#if USE_HUB75

    void InitSplashEffectManager()
    {
        debugW("InitSplashEffectManager");

        g_ptrSystem->SetupEffectManager(make_shared_psram<SplashLogoEffect>(), g_ptrSystem->Devices());
    }

#endif

// Declare it here just so InitEffectsManager can refer to it. We define it a little further down

void WriteCurrentEffectIndexFile();

// InitEffectsManager
//
// Initializes the effect manager.  Reboots on failure, since it's not optional

void InitEffectsManager()
{
    debugW("InitEffectsManager...");

    LoadEffectFactories();

    l_EffectsManagerJSONWriterIndex = g_ptrSystem->JSONWriter().RegisterWriter(
        []() { SaveToJSONFile(EFFECTS_CONFIG_FILE, l_EffectsManagerJSONBufferSize, g_ptrSystem->EffectManager()); }
    );
    l_CurrentEffectWriterIndex = g_ptrSystem->JSONWriter().RegisterWriter(WriteCurrentEffectIndexFile);

    std::unique_ptr<AllocatedJsonDocument> pJsonDoc;

    if (LoadJSONFile(EFFECTS_CONFIG_FILE, l_EffectsManagerJSONBufferSize, pJsonDoc))
    {
        debugI("Creating EffectManager from JSON config");

        if (g_ptrSystem->HasEffectManager())
            g_ptrSystem->EffectManager().DeserializeFromJSON((pJsonDoc->as<JsonObjectConst>()));
        else
            g_ptrSystem->SetupEffectManager(pJsonDoc->as<JsonObjectConst>(), g_ptrSystem->Devices());
    }
    else
    {
        debugI("Creating EffectManager using default effects");

        if (g_ptrSystem->HasEffectManager())
            g_ptrSystem->EffectManager().LoadDefaultEffects();
        else
            g_ptrSystem->SetupEffectManager(g_ptrSystem->Devices());
    }

    if (false == g_ptrSystem->EffectManager().Init())
        throw std::runtime_error("Could not initialize effect manager");

    // We won't need the default factories anymore, so swipe them from memory
    l_ptrEffectFactories->ClearDefaultFactories();
}

void SaveEffectManagerConfig()
{
    debugV("Saving effect manager config...");
    // Default value for writer index is max value for size_t, so nothing will happen if writer has not yet been registered
    g_ptrSystem->JSONWriter().FlagWriter(l_EffectsManagerJSONWriterIndex);
}

void RemoveEffectManagerConfig()
{
    RemoveJSONFile(EFFECTS_CONFIG_FILE);
    // We take the liberty of also removing the file with the current effect config index
    SPIFFS.remove(CURRENT_EFFECT_CONFIG_FILE);
}

void SaveCurrentEffectIndex()
{
    if (g_ptrSystem->DeviceConfig().RememberCurrentEffect())
        // Default value for writer index is max value for size_t, so nothing will happen if writer has not yet been registered
        g_ptrSystem->JSONWriter().FlagWriter(l_CurrentEffectWriterIndex);
}

void WriteCurrentEffectIndexFile()
{
    SPIFFS.remove(CURRENT_EFFECT_CONFIG_FILE);

    File file = SPIFFS.open(CURRENT_EFFECT_CONFIG_FILE, FILE_WRITE);

    if (!file)
    {
        debugE("Unable to open file %s for writing!", CURRENT_EFFECT_CONFIG_FILE);
        return;
    }

    auto bytesWritten = file.print(g_ptrSystem->EffectManager().GetCurrentEffectIndex());
    debugI("Number of bytes written to file %s: %zu", CURRENT_EFFECT_CONFIG_FILE, bytesWritten);

    file.flush();
    file.close();

    if (bytesWritten == 0)
    {
        debugE("Unable to write to file %s!", CURRENT_EFFECT_CONFIG_FILE);
        SPIFFS.remove(CURRENT_EFFECT_CONFIG_FILE);
    }
}

bool ReadCurrentEffectIndex(size_t& index)
{
    File file = SPIFFS.open(CURRENT_EFFECT_CONFIG_FILE);
    bool readIndex = false;

    if (file)
    {
        if (file.size() > 0)
        {
            debugI("Attempting to read file %s", CURRENT_EFFECT_CONFIG_FILE);

            auto valueString = file.readString();

            if (!valueString.isEmpty())
            {
                index = strtoul(valueString.c_str(), NULL, 10);
                readIndex = true;
            }
        }

        file.close();
    }

    return readIndex;
}

void EffectManager::LoadJSONAndMissingEffects(const JsonArrayConst& effectsArray)
{
    std::set<int> loadedEffectNumbers;

    // Create effects from JSON objects, using the respective factories in g_EffectFactories
    auto& jsonFactories = l_ptrEffectFactories->GetJSONFactories();

    for (auto effectObject : effectsArray)
    {
        int effectNumber = effectObject[PTY_EFFECTNR];
        auto factoryEntry = jsonFactories.find(effectNumber);

        if (factoryEntry == jsonFactories.end())
            continue;

        auto pEffect = factoryEntry->second(effectObject);
        if (pEffect)
        {
            if (effectObject[PTY_COREEFFECT].as<int>())
                pEffect->MarkAsCoreEffect();

            _vEffects.push_back(pEffect);
            loadedEffectNumbers.insert(effectNumber);
        }
    }

    // Now add missing effects from the default factory list
    auto &defaultFactories = l_ptrEffectFactories->GetDefaultFactories();

    // We iterate manually, so we can use where we are as the starting point for a later inner loop
    for (auto iter = defaultFactories.begin(); iter != defaultFactories.end(); iter++)
    {
        int effectNumber = iter->EffectNumber;

        // If we've already loaded this effect (number) from JSON, we can move on to check the next one
        if (loadedEffectNumbers.count(effectNumber))
            continue;

        // We found an effect (number) in the default list that we have not yet loaded from JSON.
        //   So, we go through the rest of the default factory list to create and add to our effects
        //   list all instances of this effect.
        std::for_each(iter, defaultFactories.end(), [&](const EffectFactories::NumberedFactory& numberedFactory)
            {
                if (numberedFactory.EffectNumber != effectNumber)
                    return;

                auto pEffect = numberedFactory.Factory();
                if (pEffect)
                {
                    // Effects in the default list are core effects. These can be disabled but not deleted.
                    pEffect->MarkAsCoreEffect();
                    _vEffects.push_back(pEffect);
                }
            }
        );

        // Register that we added this effect number, so we don't add the respective effects more than once
        loadedEffectNumbers.insert(effectNumber);
    }
}

void EffectManager::LoadDefaultEffects()
{
    for (auto &numberedFactory : l_ptrEffectFactories->GetDefaultFactories())
    {
        auto pEffect = numberedFactory.Factory();
        if (pEffect)
        {
            // Effects in the default list are core effects. These can be disabled but not deleted.
            pEffect->MarkAsCoreEffect();
            _vEffects.push_back(pEffect);
        }
    }

    for (int i = 0; i < _vEffects.size(); i++)
        EnableEffect(i, true);

    SetInterval(DEFAULT_EFFECT_INTERVAL, true);

    construct(true);
}

std::shared_ptr<LEDStripEffect> EffectManager::CopyEffect(size_t index)
{
    if (index >= _vEffects.size())
    {
        debugW("Invalid index for CopyEffect");
        return nullptr;
    }

    static size_t jsonBufferSize = JSON_BUFFER_BASE_SIZE;

    auto& sourceEffect = _vEffects[index];

    std::unique_ptr<AllocatedJsonDocument> ptrJsonDoc = nullptr;

    SerializeWithBufferSize(ptrJsonDoc, jsonBufferSize,
        [&sourceEffect](JsonObject &jsonObject) { return sourceEffect->SerializeToJSON(jsonObject); });

    auto jsonEffectFactories = l_ptrEffectFactories->GetJSONFactories();
    auto factoryEntry = jsonEffectFactories.find(sourceEffect->EffectNumber());

    if (factoryEntry == jsonEffectFactories.end())
        return nullptr;

    auto copiedEffect = factoryEntry->second(ptrJsonDoc->as<JsonObjectConst>());

    if (!copiedEffect)
        return nullptr;

    copiedEffect->SetEnabled(false);

    return copiedEffect;
}