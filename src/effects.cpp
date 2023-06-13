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

extern std::shared_ptr<GFXBase> g_aptrDevices[NUM_CHANNELS];

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
    CHSV hueColor = rgb2hsv_approximate(color1);
    auto object = std::make_shared<SpectrumAnalyzerEffect>("Spectrum Clr", 24, CRGBPalette16(color1, color2));
    if (object->Init(g_aptrDevices))
        return object;
    throw std::runtime_error("Could not initialize new spectrum analyzer, two color version!");
}

std::shared_ptr<LEDStripEffect> GetSpectrumAnalyzer(CRGB color)
{
    CHSV hueColor = rgb2hsv_approximate(color);
    CRGB color2 = CRGB(CHSV(hueColor.hue + 64, 255, 255));
    auto object = std::make_shared<SpectrumAnalyzerEffect>("Spectrum Clr", 24, CRGBPalette16(color, color2));
    if (object->Init(g_aptrDevices))
        return object;
    throw std::runtime_error("Could not initialize new spectrum analyzer, one color version!");
}

#endif

#define STARRYNIGHT_PROBABILITY 1.0
#define STARRYNIGHT_MUSICFACTOR 1.0

DRAM_ATTR EffectFactories g_EffectFactories;
std::map<int, JSONEffectFactory> g_JsonStarryNightEffectFactories =
{
    { EFFECT_STAR,
        [](const JsonObjectConst& jsonObject)->std::shared_ptr<LEDStripEffect> { return std::make_shared<StarryNightEffect<Star>>(jsonObject); } },
    { EFFECT_STAR_BUBBLY,
        [](const JsonObjectConst& jsonObject)->std::shared_ptr<LEDStripEffect> { return std::make_shared<StarryNightEffect<BubblyStar>>(jsonObject); } },
    { EFFECT_STAR_HOT_WHITE,
        [](const JsonObjectConst& jsonObject)->std::shared_ptr<LEDStripEffect>  { return std::make_shared<StarryNightEffect<HotWhiteStar>>(jsonObject); } },
    { EFFECT_STAR_LONG_LIFE_SPARKLE,
        [](const JsonObjectConst& jsonObject)->std::shared_ptr<LEDStripEffect>  { return std::make_shared<StarryNightEffect<LongLifeSparkleStar>>(jsonObject); } },

#if ENABLE_AUDIO
    { EFFECT_STAR_MUSIC,
        [](const JsonObjectConst& jsonObject)->std::shared_ptr<LEDStripEffect>  { return std::make_shared<StarryNightEffect<MusicStar>>(jsonObject); } },
#endif

    { EFFECT_STAR_QUIET,
        [](const JsonObjectConst& jsonObject)->std::shared_ptr<LEDStripEffect>  { return std::make_shared<StarryNightEffect<QuietStar>>(jsonObject); } },
};

std::shared_ptr<LEDStripEffect> CreateStarryNightEffectFromJSON(const JsonObjectConst& jsonObject)
{
    auto entry = g_JsonStarryNightEffectFactories.find(jsonObject[PTY_STARTYPENR]);

    return entry != g_JsonStarryNightEffectFactories.end()
        ? entry->second(jsonObject)
        : nullptr;
}

#define ADD_EFFECT(effectNumber, effectType, ...)   g_EffectFactories.AddEffect(effectNumber, \
    []()                                 ->std::shared_ptr<LEDStripEffect> { return std::make_shared<effectType>(__VA_ARGS__); }, \
    [](const JsonObjectConst& jsonObject)->std::shared_ptr<LEDStripEffect> { return std::make_shared<effectType>(jsonObject); })

#define ADD_STARRY_NIGHT_EFFECT(starType, ...)      g_EffectFactories.AddEffect(EFFECT_STRIP_STARRY_NIGHT, \
    []()                                 ->std::shared_ptr<LEDStripEffect> { return std::make_shared<StarryNightEffect<starType>>(__VA_ARGS__); }, \
    [](const JsonObjectConst& jsonObject)->std::shared_ptr<LEDStripEffect> { return CreateStarryNightEffectFromJSON(jsonObject); })

void LoadEffectFactories()
{
    // Check if the factories have already been loaded
    if (!g_EffectFactories.IsEmpty())
        return;

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

        ADD_EFFECT(EFFECT_STRIP_LANTERN, LanternEffect);

    #elif MESMERIZER

        ADD_EFFECT(EFFECT_MATRIX_SPECTRUM_ANALYZER, SpectrumAnalyzerEffect, "Spectrum",   NUM_BANDS,     spectrumBasicColors, 100, 0, 1.75, 1.75);
        ADD_EFFECT(EFFECT_MATRIX_SPECTRUM_ANALYZER, SpectrumAnalyzerEffect, "Spectrum 2", 32,            spectrumBasicColors, 100, 0, 1.25, 1.25);
        ADD_EFFECT(EFFECT_MATRIX_SPECTRUM_ANALYZER, SpectrumAnalyzerEffect, "Spectrum 3", 32,            spectrumBasicColors, 100, 0, 0.25, 1.25);

        ADD_EFFECT(EFFECT_MATRIX_SPECTRUM_ANALYZER, SpectrumAnalyzerEffect, "USA",        NUM_BANDS,     USAColors_p,         0);
        ADD_EFFECT(EFFECT_MATRIX_SPECTRUM_ANALYZER, SpectrumAnalyzerEffect, "AudioWave",  MATRIX_WIDTH,  CRGB(0,0,40),        0, 0, 1.25, 1.25);

        ADD_EFFECT(EFFECT_MATRIX_SPECTRUM_ANALYZER, SpectrumAnalyzerEffect, "Spectrum++", NUM_BANDS,     spectrumBasicColors, 0, 40, -1.0, 2.0);

        ADD_EFFECT(EFFECT_MATRIX_PONG_CLOCK, PatternPongClock);
        ADD_EFFECT(EFFECT_MATRIX_SUBSCRIBERS, PatternSubscribers);
        ADD_EFFECT(EFFECT_MATRIX_WEATHER, PatternWeather);

        ADD_EFFECT(EFFECT_MATRIX_GHOST_WAVE, GhostWave, "GhostWave", 0, 30, false, 2);
        ADD_EFFECT(EFFECT_MATRIX_WAVEFORM, WaveformEffect, "WaveIn", 8);
        ADD_EFFECT(EFFECT_MATRIX_GHOST_WAVE, GhostWave, "WaveOut", 0, 0, true, 0);

        ADD_EFFECT(EFFECT_MATRIX_WAVEFORM, WaveformEffect, "WaveForm", 8);
        ADD_EFFECT(EFFECT_MATRIX_GHOST_WAVE, GhostWave, "GhostWave", 0, 0,  false);

        ADD_EFFECT(EFFECT_MATRIX_LIFE, PatternLife);
        ADD_EFFECT(EFFECT_MATRIX_ROSE, PatternRose);
        ADD_EFFECT(EFFECT_MATRIX_PINWHEEL, PatternPinwheel);
        ADD_EFFECT(EFFECT_MATRIX_SUNBURST, PatternSunburst);

        ADD_EFFECT(EFFECT_MATRIX_FLOW_FIELD, PatternFlowField);
        ADD_EFFECT(EFFECT_MATRIX_CLOCK, PatternClock);
        ADD_EFFECT(EFFECT_MATRIX_ALIEN_TEXT, PatternAlienText);
        ADD_EFFECT(EFFECT_MATRIX_CIRCUIT, PatternCircuit);

        ADD_STARRY_NIGHT_EFFECT(MusicStar, "Stars", RainbowColors_p, 2.0, 1, LINEARBLEND, 2.0, 0.5, 10.0); // Rainbow Music Star

        ADD_EFFECT(EFFECT_MATRIX_PULSAR, PatternPulsar);

        ADD_EFFECT(EFFECT_MATRIX_BOUNCE, PatternBounce);
        ADD_EFFECT(EFFECT_MATRIX_CUBE, PatternCube);
        ADD_EFFECT(EFFECT_MATRIX_SPIRO, PatternSpiro);
        ADD_EFFECT(EFFECT_MATRIX_WAVE, PatternWave);
        ADD_EFFECT(EFFECT_MATRIX_SWIRL, PatternSwirl);
        ADD_EFFECT(EFFECT_MATRIX_SERENDIPITY, PatternSerendipity);
        ADD_EFFECT(EFFECT_MATRIX_MANDALA, PatternMandala);
        ADD_EFFECT(EFFECT_MATRIX_PALETTE_SMEAR, PatternPaletteSmear);
        ADD_EFFECT(EFFECT_MATRIX_CURTAIN, PatternCurtain);
        ADD_EFFECT(EFFECT_MATRIX_GRID_LIGHTS, PatternGridLights);
        ADD_EFFECT(EFFECT_MATRIX_MUNCH, PatternMunch);
        ADD_EFFECT(EFFECT_MATRIX_MAZE, PatternMaze);

        // std::make_shared<PatternInfinity>(),
        // std::make_shared<PatternQR>(),

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

        ADD_EFFECT(EFFECT_MATRIX_GHOST_WAVE, GhostWave, "GhostWave", 0, 16, false, 40);
        ADD_EFFECT(EFFECT_MATRIX_SPECTRUM_ANALYZER, SpectrumAnalyzerEffect, "Spectrum USA", 16, USAColors_p, 0);
        ADD_EFFECT(EFFECT_MATRIX_GHOST_WAVE, GhostWave, "GhostWave Rainbow", 8);
        ADD_EFFECT(EFFECT_MATRIX_SPECTRUM_ANALYZER, SpectrumAnalyzerEffect, "Spectrum Fade", 24, RainbowColors_p, 50, 70, -1.0, 2.0);
        ADD_EFFECT(EFFECT_MATRIX_GHOST_WAVE, GhostWave, "GhostWave Blue", 0);
        ADD_EFFECT(EFFECT_MATRIX_SPECTRUM_ANALYZER, SpectrumAnalyzerEffect, "Spectrum Standard", 24, RainbowColors_p);
        ADD_EFFECT(EFFECT_MATRIX_GHOST_WAVE, GhostWave, "GhostWave One", 4);

        //std::make_shared<GhostWave>("GhostWave Rainbow", &rainbowPalette),

    #elif ATOMLIGHT

        ADD_EFFECT(EFFECT_STRIP_COLOR_FILL, ColorFillEffect, CRGB::White, 1);
        // std::make_shared<FireFanEffect>(NUM_LEDS, 1, 15, 80, 2, 7, Sequential, true, false),
        // std::make_shared<FireFanEffect>(NUM_LEDS, 1, 15, 80, 2, 7, Sequential, true, false, true),
        // std::make_shared<HueFireFanEffect>(NUM_LEDS, 2, 5, 120, 1, 1, Sequential, true, false, false, HUE_BLUE),
        //  std::make_shared<HueFireFanEffect>(NUM_LEDS, 2, 3, 100, 1, 1, Sequential, true, false, false, HUE_GREEN),
        ADD_EFFECT(EFFECT_STRIP_RAINBOW_FILL, RainbowFillEffect, 60, 0);
        ADD_EFFECT(EFFECT_STRIP_COLOR_CYCLE, ColorCycleEffect, Sequential);
        ADD_EFFECT(EFFECT_STRIP_PALETTE, PaletteEffect, RainbowColors_p, 4, 0.1, 0.0, 1.0, 0.0);
        ADD_EFFECT(EFFECT_STRIP_BOUNCING_BALL, BouncingBallEffect, 3, true, true, 1);

        ADD_STARRY_NIGHT_EFFECT(BubblyStar, "Little Blooming Rainbow Stars", BlueColors_p, 8.0, 4, LINEARBLEND, 2.0, 0.0, 4); // Blooming Little Rainbow Stars
        ADD_STARRY_NIGHT_EFFECT(BubblyStar, "Big Blooming Rainbow Stars", RainbowColors_p, 20, 12, LINEARBLEND, 1.0, 0.0, 2); // Blooming Rainbow Stars
        //        std::make_shared<StarryNightEffect<FanStar>>("FanStars", RainbowColors_p, 8.0, 1.0, LINEARBLEND, 80.0, 0, 2.0),

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

    #elif LEDSTRIP

        ADD_EFFECT(EFFECT_STRIP_STATUS, StatusEffect, CRGB::White);

    #else

        ADD_EFFECT(EFFECT_STRIP_RAINBOW_FILL, RainbowFillEffect, 6, 2);                    // Simple effect if not otherwise defined above

    #endif

    // If this assert fires, you have not defined any effects in the table above.  If adding a new config, you need to
    // add the list of effects in this table as shown for the vaious other existing configs.  You MUST have at least
    // one effect even if it's the Status effect.
    assert(!g_EffectFactories.IsEmpty());
}

extern DRAM_ATTR std::unique_ptr<EffectManager<GFXBase>> g_ptrEffectManager;
DRAM_ATTR size_t g_EffectsManagerJSONBufferSize = 0;
DRAM_ATTR size_t g_EffectsManagerJSONWriterIndex = std::numeric_limits<size_t>::max();
DRAM_ATTR size_t g_CurrentEffectWriterIndex = std::numeric_limits<size_t>::max();

#if USE_MATRIX

    void InitSplashEffectManager()
    {
        debugW("InitSplashEffectManager");

        g_ptrEffectManager = std::make_unique<EffectManager<GFXBase>>(std::make_shared<SplashLogoEffect>(), g_aptrDevices);
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

    g_EffectsManagerJSONWriterIndex = g_ptrJSONWriter->RegisterWriter(
        []() { SaveToJSONFile(EFFECTS_CONFIG_FILE, g_EffectsManagerJSONBufferSize, *g_ptrEffectManager); }
    );
    g_CurrentEffectWriterIndex = g_ptrJSONWriter->RegisterWriter(WriteCurrentEffectIndexFile);

    std::unique_ptr<AllocatedJsonDocument> pJsonDoc;

    if (LoadJSONFile(EFFECTS_CONFIG_FILE, g_EffectsManagerJSONBufferSize, pJsonDoc))
    {
        debugI("Creating EffectManager from JSON config");

        if (!g_ptrEffectManager)
            g_ptrEffectManager = std::make_unique<EffectManager<GFXBase>>(pJsonDoc->as<JsonObjectConst>(), g_aptrDevices);
        else
            g_ptrEffectManager->DeserializeFromJSON(pJsonDoc->as<JsonObjectConst>());
    }
    else
    {
        debugI("Creating EffectManager using default effects");

        if (!g_ptrEffectManager)
            g_ptrEffectManager = std::make_unique<EffectManager<GFXBase>>(g_aptrDevices);
        else
            g_ptrEffectManager->LoadDefaultEffects();
    }

    if (false == g_ptrEffectManager->Init())
        throw std::runtime_error("Could not initialize effect manager");


    // We won't need the default factories any more, so swipe them from memory
    g_EffectFactories.ClearDefaultFactories();
}

void SaveEffectManagerConfig()
{
    debugV("Saving effect manager config...");
    // Default value for writer index is max value for size_t, so nothing will happen if writer has not yet been registered
    g_ptrJSONWriter->FlagWriter(g_EffectsManagerJSONWriterIndex);
}

void RemoveEffectManagerConfig()
{
    RemoveJSONFile(EFFECTS_CONFIG_FILE);
    // We take the liberty of also removing the file with the current effect config index
    SPIFFS.remove(CURRENT_EFFECT_CONFIG_FILE);
}

void SaveCurrentEffectIndex()
{
    if (g_ptrDeviceConfig->RememberCurrentEffect())
        // Default value for writer index is max value for size_t, so nothing will happen if writer has not yet been registered
        g_ptrJSONWriter->FlagWriter(g_CurrentEffectWriterIndex);
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

    auto bytesWritten = file.print(g_ptrEffectManager->GetCurrentEffectIndex());
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

// Dirty hack to support FastLED, which calls out of band to get the pixel index for "the" array, without
// any indication of which array or who's asking, so we assume the first matrix.  If you have trouble with
// more than one matrix and some FastLED functions like blur2d, this would be why.

uint16_t XY(uint8_t x, uint8_t y)
{
    // Have a drink on me!
    return (*g_ptrEffectManager)[0]->xy(x, y);
}

// btimap_output
//
// Output function for the jpeg library.  It doesn't provide any mechanism for passing a this pointer or other context,
// so it has to assume that it will be drawing to the main channel 0.

bool bitmap_output(int16_t x, int16_t y, uint16_t w, uint16_t h, uint16_t *bitmap)
{
    auto pgfx = g_ptrEffectManager->g();
    pgfx->drawRGBBitmap(x, y, bitmap, w, h);
    return true;
}

