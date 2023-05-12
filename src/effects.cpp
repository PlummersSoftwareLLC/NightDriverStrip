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

extern DRAM_ATTR std::shared_ptr<GFXBase> g_aptrDevices[NUM_CHANNELS];

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

std::vector<std::shared_ptr<LEDStripEffect>> CreateDefaultEffects()
{
    // The default effects table
    static const std::vector<std::shared_ptr<LEDStripEffect>> defaultEffects =
    {
    #if DEMO

        std::make_shared<RainbowFillEffect>(6, 2),

    #elif LASERLINE

        std::make_shared<LaserLineEffect>(500, 20),

    #elif CHIEFTAIN

        std::make_shared<LanternEffect>(),
        std::make_shared<PaletteEffect>(RainbowColors_p, 2.0f, 0.1, 0.0, 1.0, 0.0, LINEARBLEND, true, 1.0),
        std::make_shared<RainbowFillEffect>(10, 32),


    #elif LANTERN

        std::make_shared<LanternEffect>(),

    #elif MESMERIZER

//        new SplashLogoEffect(),

        std::make_shared<SpectrumAnalyzerEffect>("Spectrum",   NUM_BANDS,     spectrumBasicColors, 100, 0, 1.0, 1.0),
        std::make_shared<SpectrumAnalyzerEffect>("Spectrum 2",   32,            spectrumBasicColors, 100, 0, 1.25, 1.25),
        std::make_shared<SpectrumAnalyzerEffect>("Spectrum 3",   32,            spectrumBasicColors, 100, 0, 0.25, 1.25),

        std::make_shared<SpectrumAnalyzerEffect>("USA",        NUM_BANDS,     USAColors_p,         0),
        std::make_shared<SpectrumAnalyzerEffect>("AudioWave",  MATRIX_WIDTH,  CRGB(0,0,40),        0, 0, 1.25, 1.25),

        std::make_shared<SpectrumAnalyzerEffect>("Spectrum++", NUM_BANDS,     spectrumBasicColors, 0, 40, -1.0, 2.0),

        std::make_shared<PatternPongClock>(),
        std::make_shared<PatternSubscribers>(),
        std::make_shared<PatternWeather>(),

        // Animate a simple rainbow palette by using the palette effect on the built-in rainbow palette
        std::make_shared<GhostWave>("GhostWave", 0, 24, false),
        std::make_shared<WaveformEffect>("WaveIn", 8),
        std::make_shared<GhostWave>("WaveOut", 0, 0, true, 0),

        std::make_shared<WaveformEffect>("WaveForm", 8),
        std::make_shared<GhostWave>("GhostWave", 0, 0,  false),

        std::make_shared<PatternLife>(),
        std::make_shared<PatternRose>(),
        std::make_shared<PatternPinwheel>(),
        std::make_shared<PatternSunburst>(),

        std::make_shared<PatternFlowField>(),
        std::make_shared<PatternClock>(),
        std::make_shared<PatternAlienText>(),
        std::make_shared<PatternCircuit>(),

        std::make_shared<StarryNightEffect<MusicStar>>("Stars", RainbowColors_p, 2.0, 1, LINEARBLEND, 2.0, 0.5, 10.0),                                                // Rainbow Music Star

        std::make_shared<PatternPulsar>(),

        std::make_shared<PatternBounce>(),
        std::make_shared<PatternCube>(),
        std::make_shared<PatternSpiro>(),
        std::make_shared<PatternWave>(),
        std::make_shared<PatternSwirl>(),
        std::make_shared<PatternSerendipity>(),
        std::make_shared<PatternMandala>(),
        std::make_shared<PatternPaletteSmear>(),
        std::make_shared<PatternCurtain>(),
        std::make_shared<PatternGridLights>(),
        std::make_shared<PatternMunch>(),

        // std::make_shared<PatternInfinity>(),
        // std::make_shared<PatternQR>(),

    #elif UMBRELLA

        std::make_shared<FireEffect>("Calm Fire", NUM_LEDS, 2, 2, 75, 3, 10, true, false),
        std::make_shared<FireEffect>("Medium Fire", NUM_LEDS, 1, 5, 100, 3, 4, true, false),
        std::make_shared<MusicalPaletteFire>("Musical Red Fire", HeatColors_p, NUM_LEDS, 1, 8, 50, 1, 24, true, false),

        std::make_shared<MusicalPaletteFire>("Purple Fire", CRGBPalette16(CRGB::Black, CRGB::Purple, CRGB::MediumPurple, CRGB::LightPink), NUM_LEDS, 2, 3, 150, 3, 10, true, false),
        std::make_shared<MusicalPaletteFire>("Purple Fire", CRGBPalette16(CRGB::Black, CRGB::Purple, CRGB::MediumPurple, CRGB::LightPink), NUM_LEDS, 1, 7, 150, 3, 10, true, false),
        std::make_shared<MusicalPaletteFire>("Musical Purple Fire", CRGBPalette16(CRGB::Black, CRGB::Purple, CRGB::MediumPurple, CRGB::LightPink), NUM_LEDS, 1, 8, 50, 1, 24, true, false),

        std::make_shared<MusicalPaletteFire>("Blue Fire", CRGBPalette16(CRGB::Black, CRGB::DarkBlue, CRGB::Blue, CRGB::LightSkyBlue), NUM_LEDS, 2, 3, 150, 3, 10, true, false),
        std::make_shared<MusicalPaletteFire>("Blue Fire", CRGBPalette16(CRGB::Black, CRGB::DarkBlue, CRGB::Blue, CRGB::LightSkyBlue), NUM_LEDS, 1, 7, 150, 3, 10, true, false),
        std::make_shared<MusicalPaletteFire>("Musical Blue Fire", CRGBPalette16(CRGB::Black, CRGB::DarkBlue, CRGB::Blue, CRGB::LightSkyBlue), NUM_LEDS, 1, 8, 50, 1, 24, true, false),

        std::make_shared<MusicalPaletteFire>("Green Fire", CRGBPalette16(CRGB::Black, CRGB::DarkGreen, CRGB::Green, CRGB::LimeGreen), NUM_LEDS, 2, 3, 150, 3, 10, true, false),
        std::make_shared<MusicalPaletteFire>("Green Fire", CRGBPalette16(CRGB::Black, CRGB::DarkGreen, CRGB::Green, CRGB::LimeGreen), NUM_LEDS, 1, 7, 150, 3, 10, true, false),
        std::make_shared<MusicalPaletteFire>("Musical Green Fire", CRGBPalette16(CRGB::Black, CRGB::DarkGreen, CRGB::Green, CRGB::LimeGreen), NUM_LEDS, 1, 8, 50, 1, 24, true, false),

        std::make_shared<BouncingBallEffect>(),
        std::make_shared<DoublePaletteEffect>(),

        std::make_shared<MeteorEffect>(4, 4, 10, 2.0, 2.0),
        std::make_shared<MeteorEffect>(10, 1, 20, 1.5, 1.5),
        std::make_shared<MeteorEffect>(25, 1, 40, 1.0, 1.0),
        std::make_shared<MeteorEffect>(50, 1, 50, 0.5, 0.5),

        std::make_shared<StarryNightEffect<QuietStar>>("Rainbow Twinkle Stars", RainbowColors_p, STARRYNIGHT_PROBABILITY, 1, LINEARBLEND, 2.0, 0.0, STARRYNIGHT_MUSICFACTOR),       // Rainbow Twinkle
        std::make_shared<StarryNightEffect<MusicStar>>("RGB Music Blend Stars", RGBColors_p, 0.8, 1, NOBLEND, 15.0, 0.1, 10.0),                                                     // RGB Music Blur - Can You Hear Me Knockin'
        std::make_shared<StarryNightEffect<MusicStar>>("Rainbow Music Stars", RainbowColors_p, 2.0, 2, LINEARBLEND, 5.0, 0.0, 10.0),                                                // Rainbow Music Star
        std::make_shared<StarryNightEffect<BubblyStar>>("Little Blooming Rainbow Stars", BlueColors_p, STARRYNIGHT_PROBABILITY, 4, LINEARBLEND, 2.0, 0.0, STARRYNIGHT_MUSICFACTOR), // Blooming Little Rainbow Stars
        std::make_shared<StarryNightEffect<QuietStar>>("Green Twinkle Stars", GreenColors_p, STARRYNIGHT_PROBABILITY, 1, LINEARBLEND, 2.0, 0.0, STARRYNIGHT_MUSICFACTOR),           // Green Twinkle
        std::make_shared<StarryNightEffect<Star>>("Blue Sparkle Stars", BlueColors_p, STARRYNIGHT_PROBABILITY, 1, LINEARBLEND, 2.0, 0.0, STARRYNIGHT_MUSICFACTOR),                  // Blue Sparkle
        std::make_shared<StarryNightEffect<QuietStar>>("Red Twinkle Stars", RedColors_p, 1.0, 1, LINEARBLEND, 2.0),                                                                 // Red Twinkle
        std::make_shared<StarryNightEffect<Star>>("Lava Stars", LavaColors_p, STARRYNIGHT_PROBABILITY, 1, LINEARBLEND, 2.0, 0.0, STARRYNIGHT_MUSICFACTOR),                          // Lava Stars

        std::make_shared<PaletteEffect>(RainbowColors_p),
        std::make_shared<PaletteEffect>(RainbowColors_p, 1.0, 1.0),
        std::make_shared<PaletteEffect>(RainbowColors_p, .25),

    #elif TTGO

        // Animate a simple rainbow palette by using the palette effect on the built-in rainbow palette
        std::make_shared<SpectrumAnalyzerEffect>("Spectrum Fade", 12, spectrumBasicColors, 50, 70, -1.0, 3.0),

    #elif WROVERKIT

        // Animate a simple rainbow palette by using the palette effect on the built-in rainbow palette
        std::make_shared<PaletteEffect>(rainbowPalette, 256 / 16, .2, 0)

    #elif XMASTREES

        std::make_shared<ColorBeatOverRed>("ColorBeatOverRed"),

        //        std::make_shared<HueFireFanEffect>(NUM_LEDS, 1, 12, 200, 2, NUM_LEDS / 2, Sequential, false, true, false, HUE_GREEN),
        //        std::make_shared<HueFireFanEffect>(NUM_LEDS, 2, 10, 200, 2, NUM_LEDS / 2, Sequential, false, true, false, HUE_BLUE),

        std::make_shared<ColorCycleEffect>(BottomUp, 6),
        std::make_shared<ColorCycleEffect>(BottomUp, 2),

        std::make_shared<RainbowFillEffect>(48, 0),

        std::make_shared<ColorCycleEffect>(BottomUp, 3),
        std::make_shared<ColorCycleEffect>(BottomUp, 1),

        std::make_shared<StarryNightEffect<LongLifeSparkleStar>>("Green Sparkle Stars", GreenColors_p, 2.0, 1, LINEARBLEND, 2.0, 0.0, 0.0, CRGB(0, 128, 0)), // Blue Sparkle
        std::make_shared<StarryNightEffect<LongLifeSparkleStar>>("Red Sparkle Stars", GreenColors_p, 2.0, 1, LINEARBLEND, 2.0, 0.0, 0.0, CRGB::Red),         // Blue Sparkle
        std::make_shared<StarryNightEffect<LongLifeSparkleStar>>("Blue Sparkle Stars", GreenColors_p, 2.0, 1, LINEARBLEND, 2.0, 0.0, 0.0, CRGB::Blue),       // Blue Sparkle

        //        std::make_shared<VUFlameEffect>("Multicolor Sound Flame", VUFlameEffect::GREENX, 50, true),
        //        std::make_shared<VUFlameEffect>("Multicolor Sound Flame", VUFlameEffect::BLUEX, 50, true),
        //        std::make_shared<VUFlameEffect>("Multicolor Sound Flame", VUFlameEffect::REDX, 50, true),
        //       std::make_shared<VUFlameEffect>("Multicolor Sound Flame", VUFlameEffect::MULTICOLOR, 50, true),

        // std::make_shared<StarryNightEffect<LongLifeSparkleStar>>("Blue Sparkle Stars", BlueColors_p, 10.0, 1, LINEARBLEND, 2.0, 0.0, 0.0),        // Blue Sparkle

        // std::make_shared<StarryNightEffect<Star>>()
        /*
        std::make_shared<SparklySpinningMusicEffect>("SparklySpinningMusical", RainbowColors_p),
        std::make_shared<ColorBeatOverRed>("ColorBeatOnRedBkgnd"),
        std::make_shared<MoltenGlassOnVioletBkgnd>("MoltenGlassOnViolet", RainbowColors_p),
        std::make_shared<ColorBeatWithFlash>("ColorBeatWithFlash"),
        std::make_shared<MusicalHotWhiteInsulatorEffect>("MusicalHotWhite"),

        std::make_shared<SimpleInsulatorBeatEffect2>("SimpleInsulatorColorBeat"),
        std::make_shared<InsulatorSpectrumEffect>("InsulatorSpectrumEffect"),
        */
        std::make_shared<PaletteEffect>(rainbowPalette, 256 / 16, .2, 0)

    #elif INSULATORS

        //        std::make_shared<MusicFireEffect>(NUM_LEDS, 1, 10, 100, 0, NUM_LEDS),
        std::make_shared<InsulatorSpectrumEffect>("Spectrum Effect", RainbowColors_p),
        std::make_shared<NewMoltenGlassOnVioletBkgnd>("Molten Glass", RainbowColors_p),
        std::make_shared<StarryNightEffect<MusicStar>>("RGB Music Blend Stars", RGBColors_p, 0.8, 1, NOBLEND, 15.0, 0.1, 10.0),      // RGB Music Blur - Can You Hear Me Knockin'
        std::make_shared<StarryNightEffect<MusicStar>>("Rainbow Music Stars", RainbowColors_p, 2.0, 2, LINEARBLEND, 5.0, 0.0, 10.0), // Rainbow Music Star
        std::make_shared<PaletteReelEffect>("PaletteReelEffect"),
        std::make_shared<ColorBeatOverRed>("ColorBeatOverRed"),
        std::make_shared<TapeReelEffect>("TapeReelEffect"),

    // std::make_shared<SparklySpinningMusicEffect>(RainbowColors_p),
    // std::make_shared<SparklySpinningMusicEffect>("Blu Sprkl Spin", BlueColors_p),
    // std::make_shared<ColorBeatOverRed>("ColorBeatOverRed"),
    // std::make_shared<ColorBeatWithFlash>("ColorBeatFlash"),
    // std::make_shared<MusicalHotWhiteInsulatorEffect>("Hot White"),
    // std::make_shared<SimpleInsulatorBeatEffect2>("Simple Beat 2"),
    // std::make_shared<SparklySpinningMusicEffect>("Sparkle Spin", RainbowColors_p),

    #elif CUBE
        // Simple rainbow pallette
        std::make_shared<PaletteEffect>(rainbowPalette, 256 / 16, .2, 0),

        std::make_shared<SparklySpinningMusicEffect>("SparklySpinningMusical", RainbowColors_p),
        std::make_shared<ColorBeatOverRed>("ColorBeatOnRedBkgnd"),
        std::make_shared<SimpleInsulatorBeatEffect2>("SimpleInsulatorColorBeat"),
        std::make_shared<StarryNightEffect<MusicStar>>("Rainbow Music Stars", RainbowColors_p, 2.0, 2, LINEARBLEND, 5.0, 0.0, 10.0), // Rainbow Music Star

    #elif BELT

        // Yes, I made a sparkly LED belt and wore it to a party.  Batteries toO!
        std::make_shared<TwinkleEffect>(NUM_LEDS / 4, 10),

    #elif MAGICMIRROR

        std::make_shared<MoltenGlassOnVioletBkgnd>("MoltenGlass", RainbowColors_p),

    #elif SPECTRUM

        std::make_shared<SpectrumAnalyzerEffect>("Spectrum Standard", NUM_BANDS, spectrumAltColors, 0, 0, 0.5,  1.5),
        std::make_shared<SpectrumAnalyzerEffect>("Spectrum Standard", 24, spectrumAltColors, 0, 0, 1.25, 1.25),
        std::make_shared<SpectrumAnalyzerEffect>("Spectrum Standard", 24, spectrumAltColors, 0, 0, 0.25,  1.25),

        std::make_shared<SpectrumAnalyzerEffect>("Spectrum Standard", 16, spectrumAltColors, 0, 0, 1.0, 1.0),

        std::make_shared<SpectrumAnalyzerEffect>("Spectrum Standard", 48, CRGB(0,0,4), 0, 0, 1.25, 1.25),

        std::make_shared<GhostWave>("GhostWave", 0, 16, false, 40),
        std::make_shared<SpectrumAnalyzerEffect>("Spectrum USA", 16, USAColors_p, 0),
        std::make_shared<GhostWave>("GhostWave Rainbow", 8),
        std::make_shared<SpectrumAnalyzerEffect>("Spectrum Fade", 24, RainbowColors_p, 50, 70, -1.0, 2.0),
        std::make_shared<GhostWave>("GhostWave Blue", 0),
        std::make_shared<SpectrumAnalyzerEffect>("Spectrum Standard", 24, RainbowColors_p),
        std::make_shared<GhostWave>("GhostWave One", 4),

        //std::make_shared<GhostWave>("GhostWave Rainbow", &rainbowPalette),

    #elif ATOMLIGHT
        std::make_shared<ColorFillEffect>(CRGB::White, 1),
        // std::make_shared<FireFanEffect>(NUM_LEDS, 1, 15, 80, 2, 7, Sequential, true, false),
        // std::make_shared<FireFanEffect>(NUM_LEDS, 1, 15, 80, 2, 7, Sequential, true, false, true),
        // std::make_shared<HueFireFanEffect>(NUM_LEDS, 2, 5, 120, 1, 1, Sequential, true, false, false, HUE_BLUE),
        //  std::make_shared<HueFireFanEffect>(NUM_LEDS, 2, 3, 100, 1, 1, Sequential, true, false, false, HUE_GREEN),
        std::make_shared<RainbowFillEffect>(60, 0),
        std::make_shared<ColorCycleEffect>(Sequential),
        std::make_shared<PaletteEffect>(RainbowColors_p, 4, 0.1, 0.0, 1.0, 0.0),
        std::make_shared<BouncingBallEffect>(3, true, true, 1),

        std::make_shared<StarryNightEffect<BubblyStar>>("Little Blooming Rainbow Stars", BlueColors_p, 8.0, 4, LINEARBLEND, 2.0, 0.0, 4), // Blooming Little Rainbow Stars
        std::make_shared<StarryNightEffect<BubblyStar>>("Big Blooming Rainbow Stars", RainbowColors_p, 20, 12, LINEARBLEND, 1.0, 0.0, 2), // Blooming Rainbow Stars
                                                                                                                            //        std::make_shared<StarryNightEffect<FanStar>>("FanStars", RainbowColors_p, 8.0, 1.0, LINEARBLEND, 80.0, 0, 2.0),

        std::make_shared<MeteorEffect>(20, 1, 25, .15, .05),
        std::make_shared<MeteorEffect>(12, 1, 25, .15, .08),
        std::make_shared<MeteorEffect>(6, 1, 25, .15, .12),
        std::make_shared<MeteorEffect>(1, 1, 5, .15, .25),
        std::make_shared<MeteorEffect>(), // Rainbow palette

/* Commented out because no project definition sets the defines, and some sections refer to non-existent effects.
   Effect instantiations will have to be reviewed if this section is uncommented.

    #elif FIRESTICK

        std::make_shared<BouncingBallEffect>(),
        std::make_shared<VUFlameEffect>("Multicolor Sound Flame", VUFlameEffect::MULTICOLOR),
        std::make_shared<PaletteFlameEffect>("Smooth Red Fire", heatmap_pal),
        // std::make_shared<ClassicFireEffect>(),

        std::make_shared<StarryNightEffect<MusicStar>>("RGB Music Bubbles", RGBColors_p, 0.5, 1, NOBLEND, 15.0, 0.0, 75.0), // RGB Music Bubbles
        // std::make_shared<StarryNightEffect<MusicPulseStar>>("RGB Pulse", RainbowColors_p, 0.02, 20, NOBLEND, 5.0, 0.0, 75.0), // RGB Music Bubbles

        std::make_shared<PaletteFlameEffect>("Smooth Purple Fire", purpleflame_pal),

        std::make_shared<MeteorEffect>(),                                                                                                                                           // Our overlapping color meteors
        std::make_shared<StarryNightEffect<BubblyStar>>("Little Blooming Rainbow Stars", BlueColors_p, STARRYNIGHT_PROBABILITY, 4, LINEARBLEND, 2.0, 0.0, STARRYNIGHT_MUSICFACTOR), // Blooming Little Rainbow Stars
        std::make_shared<StarryNightEffect<BubblyStar>>("Big Blooming Rainbow Stars", RainbowColors_p, 2, 12, LINEARBLEND, 1.0),                                                    // Blooming Rainbow Stars
        std::make_shared<StarryNightEffect<BubblyStar>>("Neon Bars", RainbowColors_p, 0.5, 64, NOBLEND, 0),                                                                         // Neon Bars

        std::make_shared<StarryNightEffect<MusicStar>>("RGB Music Blend Stars", RGBColors_p, 0.8, 1, NOBLEND, 15.0, 0.1, 10.0),      // RGB Music Blur - Can You Hear Me Knockin'
        std::make_shared<StarryNightEffect<MusicStar>>("Rainbow Music Stars", RainbowColors_p, 2.0, 2, LINEARBLEND, 5.0, 0.0, 10.0), // Rainbow Music Star

        //        std::make_shared<VUFlameEffect("Sound Flame >(Green)",    VUFlameEffect::GREEN),
        //       std::make_shared<VUFlameEffect("Sound Flame >(Blue)",     VUFlameEffect::BLUE),

        std::make_shared<SimpleRainbowTestEffect>(8, 4),                                                                                                                  // Rainbow palette simple test of walking pixels
        std::make_shared<PaletteEffect>(RainbowColors_p),                                                                                                                 // Rainbow palette
        std::make_shared<StarryNightEffect<QuietStar>>("Green Twinkle Stars", GreenColors_p, STARRYNIGHT_PROBABILITY, 1, LINEARBLEND, 2.0, 0.0, STARRYNIGHT_MUSICFACTOR), // Green Twinkle
        std::make_shared<StarryNightEffect<Star>>("Blue Sparkle Stars", BlueColors_p, STARRYNIGHT_PROBABILITY, 1, LINEARBLEND, 2.0, 0.0, STARRYNIGHT_MUSICFACTOR),        // Blue Sparkle

        std::make_shared<StarryNightEffect<QuietStar>>("Red Twinkle Stars", RedColors_p, 1.0, 1, LINEARBLEND, 2.0, 0.0, STARRYNIGHT_MUSICFACTOR),                             // Red Twinkle
        std::make_shared<StarryNightEffect<Star>>("Lava Stars", LavaColors_p, STARRYNIGHT_PROBABILITY, 1, LINEARBLEND, 2.0, 0.0, STARRYNIGHT_MUSICFACTOR),                    // Lava Stars
        std::make_shared<StarryNightEffect<QuietStar>>("Rainbow Twinkle Stars", RainbowColors_p, STARRYNIGHT_PROBABILITY, 1, LINEARBLEND, 2.0, 0.0, STARRYNIGHT_MUSICFACTOR), // Rainbow Twinkle
        std::make_shared<DoublePaletteEffect>(),
        std::make_shared<VUEffect>()

    #elif FLAMEBULB
        std::make_shared<PaletteFlameEffect>("Smooth Red Fire", heatmap_pal, true, 4.5, 1, 1, 255, 4, false),
    #elif BIGMATRIX
        std::make_shared<SimpleRainbowTestEffect>(8, 1),  // Rainbow palette simple test of walking pixels
        std::make_shared<PaletteEffect>(RainbowColors_p), // Rainbow palette
        std::make_shared<RainbowFillEffect>(24, 0),
        std::make_shared<RainbowFillEffect>(),

        std::make_shared<StarryNightEffect<MusicStar>>("RGB Music Bubbles", RGBColors_p, 0.25, 1, NOBLEND, 3.0, 0.0, 75.0),                                                         // RGB Music Bubbles
        std::make_shared<StarryNightEffect<HotWhiteStar>>("Lava Stars", HeatColors_p, STARRYNIGHT_PROBABILITY, 1, LINEARBLEND, 2.0, 0.0, STARRYNIGHT_MUSICFACTOR),                  // Lava Stars
        std::make_shared<StarryNightEffect<BubblyStar>>("Little Blooming Rainbow Stars", BlueColors_p, STARRYNIGHT_PROBABILITY, 4, LINEARBLEND, 2.0, 0.0, STARRYNIGHT_MUSICFACTOR), // Blooming Little Rainbow Stars
        std::make_shared<StarryNightEffect<QuietStar>>("Green Twinkle Stars", GreenColors_p, STARRYNIGHT_PROBABILITY, 1, LINEARBLEND, 2.0, 0.0, STARRYNIGHT_MUSICFACTOR),           // Green Twinkle
        std::make_shared<StarryNightEffect<QuietStar>>("Red Twinkle Stars", RedColors_p, 1.0, 1, LINEARBLEND, 2.0),                                                                 // Red Twinkle
        std::make_shared<StarryNightEffect<QuietStar>>("Rainbow Twinkle Stars", RainbowColors_p, STARRYNIGHT_PROBABILITY, 1, LINEARBLEND, 2.0, 0.0, STARRYNIGHT_MUSICFACTOR),       // Rainbow Twinkle
        std::make_shared<BouncingBallEffect>(),
        std::make_shared<VUEffect>()

    #elif RINGSET
        std::make_shared<MusicalInsulatorEffect2>("Musical Effect 2"),
*/
    #elif FANSET

        std::make_shared<RainbowFillEffect>(24, 0),

        std::make_shared<ColorCycleEffect>(BottomUp),
        std::make_shared<ColorCycleEffect>(TopDown),
        std::make_shared<ColorCycleEffect>(LeftRight),
        std::make_shared<ColorCycleEffect>(RightLeft),

        std::make_shared<PaletteReelEffect>("PaletteReelEffect"),
        std::make_shared<MeteorEffect>(),
        std::make_shared<TapeReelEffect>("TapeReelEffect"),

        std::make_shared<StarryNightEffect<MusicStar>>("RGB Music Blend Stars", RGBColors_p, 0.8, 1, NOBLEND, 15.0, 0.1, 10.0),      // RGB Music Blur - Can You Hear Me Knockin'
        std::make_shared<StarryNightEffect<MusicStar>>("Rainbow Music Stars", RainbowColors_p, 2.0, 2, LINEARBLEND, 5.0, 0.0, 10.0), // Rainbow Music Star

        std::make_shared<FanBeatEffect>("FanBeat"),

        std::make_shared<StarryNightEffect<BubblyStar>>("Little Blooming Rainbow Stars", BlueColors_p, 8.0, 4, LINEARBLEND, 2.0, 0.0, 1.0), // Blooming Little Rainbow Stars
        std::make_shared<StarryNightEffect<BubblyStar>>("Big Blooming Rainbow Stars", RainbowColors_p, 2, 12, LINEARBLEND, 1.0),            // Blooming Rainbow Stars
        std::make_shared<StarryNightEffect<BubblyStar>>("Neon Bars", RainbowColors_p, 0.5, 64, NOBLEND, 0),                                 // Neon Bars

        std::make_shared<FireFanEffect>(GreenHeatColors_p, NUM_LEDS, 3, 7, 400, 2, NUM_LEDS / 2, Sequential, false, true),
        std::make_shared<FireFanEffect>(GreenHeatColors_p, NUM_LEDS, 3, 8, 600, 2, NUM_LEDS / 2, Sequential, false, true),
        std::make_shared<FireFanEffect>(GreenHeatColors_p, NUM_LEDS, 2, 10, 800, 2, NUM_LEDS / 2, Sequential, false, true),
        std::make_shared<FireFanEffect>(GreenHeatColors_p, NUM_LEDS, 1, 12, 1000, 2, NUM_LEDS / 2, Sequential, false, true),

        std::make_shared<FireFanEffect>(BlueHeatColors_p, NUM_LEDS, 3, 7, 400, 2, NUM_LEDS / 2, Sequential, false, true),
        std::make_shared<FireFanEffect>(BlueHeatColors_p, NUM_LEDS, 3, 8, 600, 2, NUM_LEDS / 2, Sequential, false, true),
        std::make_shared<FireFanEffect>(BlueHeatColors_p, NUM_LEDS, 2, 10, 800, 2, NUM_LEDS / 2, Sequential, false, true),
        std::make_shared<FireFanEffect>(BlueHeatColors_p, NUM_LEDS, 1, 12, 1000, 2, NUM_LEDS / 2, Sequential, false, true),

        std::make_shared<FireFanEffect>(HeatColors_p, NUM_LEDS, 3, 7, 400, 2, NUM_LEDS / 2, Sequential, false, true),
        std::make_shared<FireFanEffect>(HeatColors_p, NUM_LEDS, 3, 8, 600, 2, NUM_LEDS / 2, Sequential, false, true),
        std::make_shared<FireFanEffect>(HeatColors_p, NUM_LEDS, 2, 10, 800, 2, NUM_LEDS / 2, Sequential, false, true),
        std::make_shared<FireFanEffect>(HeatColors_p, NUM_LEDS, 1, 12, 1000, 2, NUM_LEDS / 2, Sequential, false, true),


    #elif BROOKLYNROOM

        std::make_shared<RainbowFillEffect>(24, 0),
        std::make_shared<RainbowFillEffect>(32, 1),
        std::make_shared<SimpleRainbowTestEffect>(8, 1),  // Rainbow palette simple test of walking pixels
        std::make_shared<SimpleRainbowTestEffect>(8, 4),  // Rainbow palette simple test of walking pixels
        std::make_shared<PaletteEffect>(MagentaColors_p), // Rainbow palette
        std::make_shared<DoublePaletteEffect>(),

        std::make_shared<MeteorEffect>(), // Our overlapping color meteors

        std::make_shared<StarryNightEffect<QuietStar>>("Magenta Twinkle Stars", GreenColors_p, STARRYNIGHT_PROBABILITY, 1, LINEARBLEND, 2.0, 0.0, STARRYNIGHT_MUSICFACTOR), // Green Twinkle
        std::make_shared<StarryNightEffect<Star>>("Blue Sparkle Stars", BlueColors_p, STARRYNIGHT_PROBABILITY, 1, LINEARBLEND, 2.0, 0.0, STARRYNIGHT_MUSICFACTOR),          // Blue Sparkle

        std::make_shared<StarryNightEffect<QuietStar>>("Red Twinkle Stars", MagentaColors_p, 1.0, 1, LINEARBLEND, 2.0),                                                       // Red Twinkle
        std::make_shared<StarryNightEffect<Star>>("Lava Stars", MagentaColors_p, STARRYNIGHT_PROBABILITY, 1, LINEARBLEND, 2.0, 0.0, STARRYNIGHT_MUSICFACTOR),                 // Lava Stars
        std::make_shared<StarryNightEffect<QuietStar>>("Rainbow Twinkle Stars", RainbowColors_p, STARRYNIGHT_PROBABILITY, 1, LINEARBLEND, 2.0, 0.0, STARRYNIGHT_MUSICFACTOR), // Rainbow Twinkle

        std::make_shared<StarryNightEffect<BubblyStar>>("Little Blooming Rainbow Stars", MagentaColors_p, STARRYNIGHT_PROBABILITY, 4, LINEARBLEND, 2.0, 0.0, STARRYNIGHT_MUSICFACTOR), // Blooming Little Rainbow Stars
        std::make_shared<StarryNightEffect<BubblyStar>>("Big Blooming Rainbow Stars", MagentaColors_p, 2, 12, LINEARBLEND, 1.0),                                                       // Blooming Rainbow Stars
        std::make_shared<StarryNightEffect<BubblyStar>>("Neon Bars", MagentaColors_p, 0.5, 64, NOBLEND, 0),                                                                            // Neon Bars

        std::make_shared<ClassicFireEffect>(true),

    #elif LEDSTRIP

        std::make_shared<StatusEffect>(CRGB::White)

/* Commented out because no project definition sets the define. Effect instantiations may need to be reviewed if
   this section is uncommented.

    #elif HOODORNAMENT

        std::make_shared<RainbowFillEffect>(24, 0),
        std::make_shared<RainbowFillEffect>(32, 1),
        std::make_shared<SimpleRainbowTestEffect>(8, 1),              // Rainbow palette simple test of walking pixels
        std::make_shared<PaletteEffect>(MagentaColors_p),             // Rainbow palette
        std::make_shared<DoublePaletteEffect>(),
*/
    #else

        std::make_shared<RainbowFillEffect>(6, 2),                    // Simple effect if not otherwise defined above

    #endif
    };

    // If this assert fires, you have not defined any effects in the table above.  If adding a new config, you need to
    // add the list of effects in this table as shown for the vaious other existing configs.  You MUST have at least
    // one effect even if it's the Status effect.
    static_assert(ARRAYSIZE(defaultEffects) > 0);
    return defaultEffects;
}

extern DRAM_ATTR std::unique_ptr<EffectManager<GFXBase>> g_ptrEffectManager;
DRAM_ATTR size_t g_EffectsManagerJSONBufferSize = 0;

#if USE_MATRIX

    void InitSplashEffectManager()
    {
        debugW("InitSplashEffectManager");

        g_ptrEffectManager = std::make_unique<EffectManager<GFXBase>>(std::make_shared<SplashLogoEffect>(), g_aptrDevices);
    }

#endif

// InitEffectsManager
//
// Initializes the effect manager.  Reboots on failure, since it's not optional

void InitEffectsManager()
{
    debugW("InitEffectsManager...");

    std::unique_ptr<AllocatedJsonDocument> pJsonDoc;

    if (LoadJSONFile(EFFECTS_CONFIG_FILE, g_EffectsManagerJSONBufferSize, pJsonDoc))
    {
        debugI("Creating EffectManager from JSON config");

        if (!g_ptrEffectManager)
            g_ptrEffectManager = std::make_unique<EffectManager<GFXBase>>(pJsonDoc->as<JsonObjectConst>(), g_aptrDevices);
        else
            g_ptrEffectManager->DeserializeFromJSON(pJsonDoc->as<JsonObjectConst>());

        if (g_ptrEffectManager->EffectCount() == 0)
        {
            debugW("JSON deserialization of EffectManager yielded no effects, so falling back to default list");
            std::vector<std::shared_ptr<LEDStripEffect>> vDefaultEffects = CreateDefaultEffects();
            g_ptrEffectManager->LoadEffects(vDefaultEffects);
        }
    }
    else
    {
        debugI("Creating EffectManager using default effects");

        std::vector<std::shared_ptr<LEDStripEffect>> effects = CreateDefaultEffects();

        if (!g_ptrEffectManager)
            g_ptrEffectManager = std::make_unique<EffectManager<GFXBase>>(effects, g_aptrDevices);
        else
            g_ptrEffectManager->LoadEffects(effects);
    }

    if (false == g_ptrEffectManager->Init())
        throw std::runtime_error("Could not initialize effect manager");
}

void SaveEffectManagerConfig()
{
    SaveToJSONFile(EFFECTS_CONFIG_FILE, g_EffectsManagerJSONBufferSize, *g_ptrEffectManager);
}

void RemoveEffectManagerConfig()
{
    RemoveJSONFile(EFFECTS_CONFIG_FILE);
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

