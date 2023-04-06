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

#if USE_MATRIX
    volatile long PatternSubscribers::cSubscribers;
    volatile long PatternSubscribers::cViews;
#endif

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

size_t CreateDefaultEffects(std::unique_ptr<EffectPointerArray>& pEffectList) 
{
    // The default effects table
    LEDStripEffect *defaultEffects[] =
    {
    #if DEMO

        new RainbowFillEffect(6, 2),

    #elif LASERLINE

        new LaserLineEffect(500, 20),

    #elif CHIEFTAIN

        new LanternEffect(),
        new PaletteEffect(RainbowColors_p, 2.0f, 0.1, 0.0, 1.0, 0.0, LINEARBLEND, true, 1.0),
        new RainbowFillEffect(10, 32),


    #elif LANTERN

        new LanternEffect(),

    #elif MESMERIZER

        new SplashLogoEffect(),

        new SpectrumAnalyzerEffect("Spectrum",   NUM_BANDS,     spectrumBasicColors, 100, 0, 1.0, 1.0),
        new SpectrumAnalyzerEffect("Spectrum 2",   32,            spectrumBasicColors, 100, 0, 1.25, 1.25),
        new SpectrumAnalyzerEffect("Spectrum 3",   32,            spectrumBasicColors, 100, 0, 0.25, 1.25),

        new SpectrumAnalyzerEffect("USA",        NUM_BANDS,     USAColors_p,         0),
        new SpectrumAnalyzerEffect("AudioWave",  MATRIX_WIDTH,  CRGB(0,0,40),        0, 0, 1.25, 1.25),

        new SpectrumAnalyzerEffect("Spectrum++", NUM_BANDS,     spectrumBasicColors, 0, 40, -1.0, 2.0),

        new PatternPongClock(),
        new PatternSubscribers(),
        new PatternWeather(),
        
        // Animate a simple rainbow palette by using the palette effect on the built-in rainbow palette
        new GhostWave("GhostWave", 0, 24, false),
        new WaveformEffect("WaveIn", 8),     
        new GhostWave("WaveOut", 0, 0, false, 40),

        new WaveformEffect("WaveForm", 8),
        new GhostWave("GhostWave", 0, 0,  false),

        new PatternLife(),
        new PatternRose(),
        new PatternPinwheel(),
        new PatternSunburst(),

        new PatternInfinity(),
        new PatternFlowField(),
        new PatternClock(),        
        new PatternAlienText(),
        new PatternCircuit(),

        new StarryNightEffect<MusicStar>("Stars", RainbowColors_p, 2.0, 1, LINEARBLEND, 2.0, 0.5, 10.0),                                                // Rainbow Music Star

        new PatternPulsar(),

        new PatternBounce(),
        new PatternCube(),
        new PatternSpiro(),
        new PatternWave(),
        new PatternSwirl(),
        new PatternSerendipity(),
        new PatternMandala(),
        new PatternPaletteSmear(),
        new PatternCurtain(),
        new PatternGridLights(),
        new PatternMunch(),
        new PatternQR(),           
        
    #elif UMBRELLA

        new FireEffect("Calm Fire", NUM_LEDS, 2, 2, 75, 3, 10, true, false),
        new FireEffect("Medium Fire", NUM_LEDS, 1, 5, 100, 3, 4, true, false),
        new MusicalPaletteFire("Musical Red Fire", HeatColors_p, NUM_LEDS, 1, 8, 50, 1, 24, true, false),

        new MusicalPaletteFire("Purple Fire", CRGBPalette16(CRGB::Black, CRGB::Purple, CRGB::MediumPurple, CRGB::LightPink), NUM_LEDS, 2, 3, 150, 3, 10, true, false),
        new MusicalPaletteFire("Purple Fire", CRGBPalette16(CRGB::Black, CRGB::Purple, CRGB::MediumPurple, CRGB::LightPink), NUM_LEDS, 1, 7, 150, 3, 10, true, false),
        new MusicalPaletteFire("Musical Purple Fire", CRGBPalette16(CRGB::Black, CRGB::Purple, CRGB::MediumPurple, CRGB::LightPink), NUM_LEDS, 1, 8, 50, 1, 24, true, false),

        new MusicalPaletteFire("Blue Fire", CRGBPalette16(CRGB::Black, CRGB::DarkBlue, CRGB::Blue, CRGB::LightSkyBlue), NUM_LEDS, 2, 3, 150, 3, 10, true, false),
        new MusicalPaletteFire("Blue Fire", CRGBPalette16(CRGB::Black, CRGB::DarkBlue, CRGB::Blue, CRGB::LightSkyBlue), NUM_LEDS, 1, 7, 150, 3, 10, true, false),
        new MusicalPaletteFire("Musical Blue Fire", CRGBPalette16(CRGB::Black, CRGB::DarkBlue, CRGB::Blue, CRGB::LightSkyBlue), NUM_LEDS, 1, 8, 50, 1, 24, true, false),

        new MusicalPaletteFire("Green Fire", CRGBPalette16(CRGB::Black, CRGB::DarkGreen, CRGB::Green, CRGB::LimeGreen), NUM_LEDS, 2, 3, 150, 3, 10, true, false),
        new MusicalPaletteFire("Green Fire", CRGBPalette16(CRGB::Black, CRGB::DarkGreen, CRGB::Green, CRGB::LimeGreen), NUM_LEDS, 1, 7, 150, 3, 10, true, false),
        new MusicalPaletteFire("Musical Green Fire", CRGBPalette16(CRGB::Black, CRGB::DarkGreen, CRGB::Green, CRGB::LimeGreen), NUM_LEDS, 1, 8, 50, 1, 24, true, false),

        new BouncingBallEffect(),
        new DoublePaletteEffect(),

        new MeteorEffect(4, 4, 10, 2.0, 2.0),
        new MeteorEffect(10, 1, 20, 1.5, 1.5),
        new MeteorEffect(25, 1, 40, 1.0, 1.0),
        new MeteorEffect(50, 1, 50, 0.5, 0.5),

        new StarryNightEffect<QuietStar>("Rainbow Twinkle Stars", RainbowColors_p, STARRYNIGHT_PROBABILITY, 1, LINEARBLEND, 2.0, 0.0, STARRYNIGHT_MUSICFACTOR),       // Rainbow Twinkle
        new StarryNightEffect<MusicStar>("RGB Music Blend Stars", RGBColors_p, 0.8, 1, NOBLEND, 15.0, 0.1, 10.0),                                                     // RGB Music Blur - Can You Hear Me Knockin'
        new StarryNightEffect<MusicStar>("Rainbow Music Stars", RainbowColors_p, 2.0, 2, LINEARBLEND, 5.0, 0.0, 10.0),                                                // Rainbow Music Star
        new StarryNightEffect<BubblyStar>("Little Blooming Rainbow Stars", BlueColors_p, STARRYNIGHT_PROBABILITY, 4, LINEARBLEND, 2.0, 0.0, STARRYNIGHT_MUSICFACTOR), // Blooming Little Rainbow Stars
        new StarryNightEffect<QuietStar>("Green Twinkle Stars", GreenColors_p, STARRYNIGHT_PROBABILITY, 1, LINEARBLEND, 2.0, 0.0, STARRYNIGHT_MUSICFACTOR),           // Green Twinkle
        new StarryNightEffect<Star>("Blue Sparkle Stars", BlueColors_p, STARRYNIGHT_PROBABILITY, 1, LINEARBLEND, 2.0, 0.0, STARRYNIGHT_MUSICFACTOR),                  // Blue Sparkle
        new StarryNightEffect<QuietStar>("Red Twinkle Stars", RedColors_p, 1.0, 1, LINEARBLEND, 2.0),                                                                 // Red Twinkle
        new StarryNightEffect<Star>("Lava Stars", LavaColors_p, STARRYNIGHT_PROBABILITY, 1, LINEARBLEND, 2.0, 0.0, STARRYNIGHT_MUSICFACTOR),                          // Lava Stars

        new PaletteEffect(RainbowColors_p),
        new PaletteEffect(RainbowColors_p, 1.0, 1.0),
        new PaletteEffect(RainbowColors_p, .25),

    #elif TTGO

        // Animate a simple rainbow palette by using the palette effect on the built-in rainbow palette
        new SpectrumAnalyzerEffect("Spectrum Fade", 12, spectrumBasicColors, 50, 70, -1.0, 3.0),

    #elif WROVERKIT

        // Animate a simple rainbow palette by using the palette effect on the built-in rainbow palette
        new PaletteEffect(rainbowPalette, 256 / 16, .2, 0)

    #elif XMASTREES

        new ColorBeatOverRed("ColorBeatOverRed"),

        //        new HueFireFanEffect(NUM_LEDS, 1, 12, 200, 2, NUM_LEDS / 2, Sequential, false, true, false, HUE_GREEN),
        //        new HueFireFanEffect(NUM_LEDS, 2, 10, 200, 2, NUM_LEDS / 2, Sequential, false, true, false, HUE_BLUE),

        new ColorCycleEffect(BottomUp, 6),
        new ColorCycleEffect(BottomUp, 2),

        new RainbowFillEffect(48, 0),

        new ColorCycleEffect(BottomUp, 3),
        new ColorCycleEffect(BottomUp, 1),

        new StarryNightEffect<LongLifeSparkleStar>("Green Sparkle Stars", GreenColors_p, 2.0, 1, LINEARBLEND, 2.0, 0.0, 0.0, CRGB(0, 128, 0)), // Blue Sparkle
        new StarryNightEffect<LongLifeSparkleStar>("Red Sparkle Stars", GreenColors_p, 2.0, 1, LINEARBLEND, 2.0, 0.0, 0.0, CRGB::Red),         // Blue Sparkle
        new StarryNightEffect<LongLifeSparkleStar>("Blue Sparkle Stars", GreenColors_p, 2.0, 1, LINEARBLEND, 2.0, 0.0, 0.0, CRGB::Blue),       // Blue Sparkle

        //        new VUFlameEffect("Multicolor Sound Flame", VUFlameEffect::GREENX, 50, true),
        //        new VUFlameEffect("Multicolor Sound Flame", VUFlameEffect::BLUEX, 50, true),
        //        new VUFlameEffect("Multicolor Sound Flame", VUFlameEffect::REDX, 50, true),
        //       new VUFlameEffect("Multicolor Sound Flame", VUFlameEffect::MULTICOLOR, 50, true),

        // new StarryNightEffect<LongLifeSparkleStar>("Blue Sparkle Stars", BlueColors_p, 10.0, 1, LINEARBLEND, 2.0, 0.0, 0.0),        // Blue Sparkle

        // new StarryNightEffect<Star>()
        /*
        new SparklySpinningMusicEffect("SparklySpinningMusical", RainbowColors_p),
        new ColorBeatOverRed("ColorBeatOnRedBkgnd"),
        new MoltenGlassOnVioletBkgnd("MoltenGlassOnViolet", RainbowColors_p),
        new ColorBeatWithFlash("ColorBeatWithFlash"),
        new MusicalHotWhiteInsulatorEffect("MusicalHotWhite"),

        new SimpleInsulatorBeatEffect2("SimpleInsulatorColorBeat"),
        new InsulatorSpectrumEffect("InsulatorSpectrumEffect"),
        */
        new PaletteEffect(rainbowPalette, 256 / 16, .2, 0)

    #elif INSULATORS

        //        new MusicFireEffect(NUM_LEDS, 1, 10, 100, 0, NUM_LEDS),
        new InsulatorSpectrumEffect("Spectrum Effect", RainbowColors_p),
        new NewMoltenGlassOnVioletBkgnd("Molten Glass", RainbowColors_p),
        new StarryNightEffect<MusicStar>("RGB Music Blend Stars", RGBColors_p, 0.8, 1, NOBLEND, 15.0, 0.1, 10.0),      // RGB Music Blur - Can You Hear Me Knockin'
        new StarryNightEffect<MusicStar>("Rainbow Music Stars", RainbowColors_p, 2.0, 2, LINEARBLEND, 5.0, 0.0, 10.0), // Rainbow Music Star
        new PaletteReelEffect("PaletteReelEffect"),
        new ColorBeatOverRed("ColorBeatOverRed"),
        new TapeReelEffect("TapeReelEffect"),

    // new SparklySpinningMusicEffect(RainbowColors_p),
    // new SparklySpinningMusicEffect("Blu Sprkl Spin", BlueColors_p),
    // new ColorBeatOverRed("ColorBeatOverRed"),
    // new ColorBeatWithFlash("ColorBeatFlash"),
    // new MusicalHotWhiteInsulatorEffect("Hot White"),
    // new SimpleInsulatorBeatEffect2("Simple Beat 2"),
    // new SparklySpinningMusicEffect("Sparkle Spin", RainbowColors_p),

    #elif CUBE
        // Simple rainbow pallette
        new PaletteEffect(rainbowPalette, 256 / 16, .2, 0),

        new SparklySpinningMusicEffect("SparklySpinningMusical", RainbowColors_p),
        new ColorBeatOverRed("ColorBeatOnRedBkgnd"),
        new SimpleInsulatorBeatEffect2("SimpleInsulatorColorBeat"),
        new StarryNightEffect<MusicStar>("Rainbow Music Stars", RainbowColors_p, 2.0, 2, LINEARBLEND, 5.0, 0.0, 10.0), // Rainbow Music Star

    #elif BELT

        // Yes, I made a sparkly LED belt and wore it to a party.  Batteries toO!
        new TwinkleEffect(NUM_LEDS / 4, 10),

    #elif MAGICMIRROR

        new MoltenGlassOnVioletBkgnd("MoltenGlass", RainbowColors_p),

    #elif SPECTRUM

        new SpectrumAnalyzerEffect("Spectrum Standard", NUM_BANDS, spectrumAltColors, 0, 0, 0.5,  1.5),
        new SpectrumAnalyzerEffect("Spectrum Standard", 24, spectrumAltColors, 0, 0, 1.25, 1.25),
        new SpectrumAnalyzerEffect("Spectrum Standard", 24, spectrumAltColors, 0, 0, 0.25,  1.25),

        new SpectrumAnalyzerEffect("Spectrum Standard", 16, spectrumAltColors, 0, 0, 1.0, 1.0),

        new SpectrumAnalyzerEffect("Spectrum Standard", 48, CRGB(0,0,4), 0, 0, 1.25, 1.25),
        
        new GhostWave("GhostWave", 0, 16, false, 40),
        new SpectrumAnalyzerEffect("Spectrum USA", 16, USAColors_p, 0),
        new GhostWave("GhostWave Rainbow", 8),
        new SpectrumAnalyzerEffect("Spectrum Fade", 24, RainbowColors_p, 50, 70, -1.0, 2.0),
        new GhostWave("GhostWave Blue", 0),
        new SpectrumAnalyzerEffect("Spectrum Standard", 24, RainbowColors_p),
        new GhostWave("GhostWave One", 4),

        //new GhostWave("GhostWave Rainbow", &rainbowPalette),

    #elif ATOMLIGHT
        new ColorFillEffect(CRGB::White, 1),
        // new FireFanEffect(NUM_LEDS, 1, 15, 80, 2, 7, Sequential, true, false),
        // new FireFanEffect(NUM_LEDS, 1, 15, 80, 2, 7, Sequential, true, false, true),
        // new HueFireFanEffect(NUM_LEDS, 2, 5, 120, 1, 1, Sequential, true, false, false, HUE_BLUE),
        //  new HueFireFanEffect(NUM_LEDS, 2, 3, 100, 1, 1, Sequential, true, false, false, HUE_GREEN),
        new RainbowFillEffect(60, 0),
        new ColorCycleEffect(Sequential),
        new PaletteEffect(RainbowColors_p, 4, 0.1, 0.0, 1.0, 0.0),
        new BouncingBallEffect(3, true, true, 1),

        new StarryNightEffect<BubblyStar>("Little Blooming Rainbow Stars", BlueColors_p, 8.0, 4, LINEARBLEND, 2.0, 0.0, 4), // Blooming Little Rainbow Stars
        new StarryNightEffect<BubblyStar>("Big Blooming Rainbow Stars", RainbowColors_p, 20, 12, LINEARBLEND, 1.0, 0.0, 2), // Blooming Rainbow Stars
                                                                                                                            //        new StarryNightEffect<FanStar>("FanStars", RainbowColors_p, 8.0, 1.0, LINEARBLEND, 80.0, 0, 2.0),

        new MeteorEffect(20, 1, 25, .15, .05),
        new MeteorEffect(12, 1, 25, .15, .08),
        new MeteorEffect(6, 1, 25, .15, .12),
        new MeteorEffect(1, 1, 5, .15, .25),
        new MeteorEffect(), // Rainbow palette

    #elif FIRESTICK

        new BouncingBallEffect(),
        new VUFlameEffect("Multicolor Sound Flame", VUFlameEffect::MULTICOLOR),
        new PaletteFlameEffect("Smooth Red Fire", heatmap_pal),
        // new ClassicFireEffect(),

        new StarryNightEffect<MusicStar>("RGB Music Bubbles", RGBColors_p, 0.5, 1, NOBLEND, 15.0, 0.0, 75.0), // RGB Music Bubbles
        // new StarryNightEffect<MusicPulseStar>("RGB Pulse", RainbowColors_p, 0.02, 20, NOBLEND, 5.0, 0.0, 75.0), // RGB Music Bubbles

        new PaletteFlameEffect("Smooth Purple Fire", purpleflame_pal),

        new MeteorEffect(),                                                                                                                                           // Our overlapping color meteors
        new StarryNightEffect<BubblyStar>("Little Blooming Rainbow Stars", BlueColors_p, STARRYNIGHT_PROBABILITY, 4, LINEARBLEND, 2.0, 0.0, STARRYNIGHT_MUSICFACTOR), // Blooming Little Rainbow Stars
        new StarryNightEffect<BubblyStar>("Big Blooming Rainbow Stars", RainbowColors_p, 2, 12, LINEARBLEND, 1.0),                                                    // Blooming Rainbow Stars
        new StarryNightEffect<BubblyStar>("Neon Bars", RainbowColors_p, 0.5, 64, NOBLEND, 0),                                                                         // Neon Bars

        new StarryNightEffect<MusicStar>("RGB Music Blend Stars", RGBColors_p, 0.8, 1, NOBLEND, 15.0, 0.1, 10.0),      // RGB Music Blur - Can You Hear Me Knockin'
        new StarryNightEffect<MusicStar>("Rainbow Music Stars", RainbowColors_p, 2.0, 2, LINEARBLEND, 5.0, 0.0, 10.0), // Rainbow Music Star

        //        new VUFlameEffect("Sound Flame (Green)",    VUFlameEffect::GREEN),
        //       new VUFlameEffect("Sound Flame (Blue)",     VUFlameEffect::BLUE),

        new SimpleRainbowTestEffect(8, 4),                                                                                                                  // Rainbow palette simple test of walking pixels
        new PaletteEffect(RainbowColors_p),                                                                                                                 // Rainbow palette
        new StarryNightEffect<QuietStar>("Green Twinkle Stars", GreenColors_p, STARRYNIGHT_PROBABILITY, 1, LINEARBLEND, 2.0, 0.0, STARRYNIGHT_MUSICFACTOR), // Green Twinkle
        new StarryNightEffect<Star>("Blue Sparkle Stars", BlueColors_p, STARRYNIGHT_PROBABILITY, 1, LINEARBLEND, 2.0, 0.0, STARRYNIGHT_MUSICFACTOR),        // Blue Sparkle

        new StarryNightEffect<QuietStar>("Red Twinkle Stars", RedColors_p, 1.0, 1, LINEARBLEND, 2.0, 0.0, STARRYNIGHT_MUSICFACTOR),                             // Red Twinkle
        new StarryNightEffect<Star>("Lava Stars", LavaColors_p, STARRYNIGHT_PROBABILITY, 1, LINEARBLEND, 2.0, 0.0, STARRYNIGHT_MUSICFACTOR),                    // Lava Stars
        new StarryNightEffect<QuietStar>("Rainbow Twinkle Stars", RainbowColors_p, STARRYNIGHT_PROBABILITY, 1, LINEARBLEND, 2.0, 0.0, STARRYNIGHT_MUSICFACTOR), // Rainbow Twinkle
        new DoublePaletteEffect(),
        new VUEffect()

    #elif FLAMEBULB
        new PaletteFlameEffect("Smooth Red Fire", heatmap_pal, true, 4.5, 1, 1, 255, 4, false),
    #elif BIGMATRIX
        new SimpleRainbowTestEffect(8, 1),  // Rainbow palette simple test of walking pixels
        new PaletteEffect(RainbowColors_p), // Rainbow palette
        new RainbowFillEffect(24, 0),
        new RainbowFillEffect(),

        new StarryNightEffect<MusicStar>("RGB Music Bubbles", RGBColors_p, 0.25, 1, NOBLEND, 3.0, 0.0, 75.0),                                                         // RGB Music Bubbles
        new StarryNightEffect<HotWhiteStar>("Lava Stars", HeatColors_p, STARRYNIGHT_PROBABILITY, 1, LINEARBLEND, 2.0, 0.0, STARRYNIGHT_MUSICFACTOR),                  // Lava Stars
        new StarryNightEffect<BubblyStar>("Little Blooming Rainbow Stars", BlueColors_p, STARRYNIGHT_PROBABILITY, 4, LINEARBLEND, 2.0, 0.0, STARRYNIGHT_MUSICFACTOR), // Blooming Little Rainbow Stars
        new StarryNightEffect<QuietStar>("Green Twinkle Stars", GreenColors_p, STARRYNIGHT_PROBABILITY, 1, LINEARBLEND, 2.0, 0.0, STARRYNIGHT_MUSICFACTOR),           // Green Twinkle
        new StarryNightEffect<QuietStar>("Red Twinkle Stars", RedColors_p, 1.0, 1, LINEARBLEND, 2.0),                                                                 // Red Twinkle
        new StarryNightEffect<QuietStar>("Rainbow Twinkle Stars", RainbowColors_p, STARRYNIGHT_PROBABILITY, 1, LINEARBLEND, 2.0, 0.0, STARRYNIGHT_MUSICFACTOR),       // Rainbow Twinkle
        new BouncingBallEffect(),
        new VUEffect()

    #elif RINGSET
        new MusicalInsulatorEffect2("Musical Effect 2"),

    #elif FANSET

        new RainbowFillEffect(24, 0),

        new ColorCycleEffect(BottomUp),
        new ColorCycleEffect(TopDown),
        new ColorCycleEffect(LeftRight),
        new ColorCycleEffect(RightLeft),

        new PaletteReelEffect("PaletteReelEffect"),
        new MeteorEffect(),
        new TapeReelEffect("TapeReelEffect"),

        new StarryNightEffect<MusicStar>("RGB Music Blend Stars", RGBColors_p, 0.8, 1, NOBLEND, 15.0, 0.1, 10.0),      // RGB Music Blur - Can You Hear Me Knockin'
        new StarryNightEffect<MusicStar>("Rainbow Music Stars", RainbowColors_p, 2.0, 2, LINEARBLEND, 5.0, 0.0, 10.0), // Rainbow Music Star

        new FanBeatEffect("FanBeat"),

        new StarryNightEffect<BubblyStar>("Little Blooming Rainbow Stars", BlueColors_p, 8.0, 4, LINEARBLEND, 2.0, 0.0, 1.0), // Blooming Little Rainbow Stars
        new StarryNightEffect<BubblyStar>("Big Blooming Rainbow Stars", RainbowColors_p, 2, 12, LINEARBLEND, 1.0),            // Blooming Rainbow Stars
        new StarryNightEffect<BubblyStar>("Neon Bars", RainbowColors_p, 0.5, 64, NOBLEND, 0),                                 // Neon Bars

        new FireFanEffect(GreenHeatColors_p, NUM_LEDS, 3, 7, 400, 2, NUM_LEDS / 2, Sequential, false, true),
        new FireFanEffect(GreenHeatColors_p, NUM_LEDS, 3, 8, 600, 2, NUM_LEDS / 2, Sequential, false, true),
        new FireFanEffect(GreenHeatColors_p, NUM_LEDS, 2, 10, 800, 2, NUM_LEDS / 2, Sequential, false, true),
        new FireFanEffect(GreenHeatColors_p, NUM_LEDS, 1, 12, 1000, 2, NUM_LEDS / 2, Sequential, false, true),

        new FireFanEffect(BlueHeatColors_p, NUM_LEDS, 3, 7, 400, 2, NUM_LEDS / 2, Sequential, false, true),
        new FireFanEffect(BlueHeatColors_p, NUM_LEDS, 3, 8, 600, 2, NUM_LEDS / 2, Sequential, false, true),
        new FireFanEffect(BlueHeatColors_p, NUM_LEDS, 2, 10, 800, 2, NUM_LEDS / 2, Sequential, false, true),
        new FireFanEffect(BlueHeatColors_p, NUM_LEDS, 1, 12, 1000, 2, NUM_LEDS / 2, Sequential, false, true),

        new FireFanEffect(HeatColors_p, NUM_LEDS, 3, 7, 400, 2, NUM_LEDS / 2, Sequential, false, true),
        new FireFanEffect(HeatColors_p, NUM_LEDS, 3, 8, 600, 2, NUM_LEDS / 2, Sequential, false, true),
        new FireFanEffect(HeatColors_p, NUM_LEDS, 2, 10, 800, 2, NUM_LEDS / 2, Sequential, false, true),
        new FireFanEffect(HeatColors_p, NUM_LEDS, 1, 12, 1000, 2, NUM_LEDS / 2, Sequential, false, true),


    #elif BROOKLYNROOM

        new RainbowFillEffect(24, 0),
        new RainbowFillEffect(32, 1),
        new SimpleRainbowTestEffect(8, 1),  // Rainbow palette simple test of walking pixels
        new SimpleRainbowTestEffect(8, 4),  // Rainbow palette simple test of walking pixels
        new PaletteEffect(MagentaColors_p), // Rainbow palette
        new DoublePaletteEffect(),

        new MeteorEffect(), // Our overlapping color meteors

        new StarryNightEffect<QuietStar>("Magenta Twinkle Stars", GreenColors_p, STARRYNIGHT_PROBABILITY, 1, LINEARBLEND, 2.0, 0.0, STARRYNIGHT_MUSICFACTOR), // Green Twinkle
        new StarryNightEffect<Star>("Blue Sparkle Stars", BlueColors_p, STARRYNIGHT_PROBABILITY, 1, LINEARBLEND, 2.0, 0.0, STARRYNIGHT_MUSICFACTOR),          // Blue Sparkle

        new StarryNightEffect<QuietStar>("Red Twinkle Stars", MagentaColors_p, 1.0, 1, LINEARBLEND, 2.0),                                                       // Red Twinkle
        new StarryNightEffect<Star>("Lava Stars", MagentaColors_p, STARRYNIGHT_PROBABILITY, 1, LINEARBLEND, 2.0, 0.0, STARRYNIGHT_MUSICFACTOR),                 // Lava Stars
        new StarryNightEffect<QuietStar>("Rainbow Twinkle Stars", RainbowColors_p, STARRYNIGHT_PROBABILITY, 1, LINEARBLEND, 2.0, 0.0, STARRYNIGHT_MUSICFACTOR), // Rainbow Twinkle

        new StarryNightEffect<BubblyStar>("Little Blooming Rainbow Stars", MagentaColors_p, STARRYNIGHT_PROBABILITY, 4, LINEARBLEND, 2.0, 0.0, STARRYNIGHT_MUSICFACTOR), // Blooming Little Rainbow Stars
        new StarryNightEffect<BubblyStar>("Big Blooming Rainbow Stars", MagentaColors_p, 2, 12, LINEARBLEND, 1.0),                                                       // Blooming Rainbow Stars
        new StarryNightEffect<BubblyStar>("Neon Bars", MagentaColors_p, 0.5, 64, NOBLEND, 0),                                                                            // Neon Bars

        new ClassicFireEffect(true),

    #elif LEDSTRIP

        new StatusEffect(CRGB::White)
        
    #elif HOODORNAMENT

        new RainbowFillEffect(24, 0),
        new RainbowFillEffect(32, 1),
        new SimpleRainbowTestEffect(8, 1),              // Rainbow palette simple test of walking pixels
        new PaletteEffect(MagentaColors_p),             // Rainbow palette
        new DoublePaletteEffect(),

    #else                                                                   

        new RainbowFillEffect(6, 2),                    // Simple effect if not otherwise defined above

    #endif
    };

    // If this assert fires, you have not defined any effects in the table above.  If adding a new config, you need to 
    // add the list of effects in this table as shown for the vaious other existing configs.  You MUST have at least
    // one effect even if it's the Status effect.
    static_assert(ARRAYSIZE(defaultEffects) > 0);

    pEffectList = std::make_unique<EffectPointerArray>(ARRAYSIZE(defaultEffects));
    std::copy(std::begin(defaultEffects), std::end(defaultEffects), pEffectList.get());

    return ARRAYSIZE(defaultEffects);
}

extern DRAM_ATTR std::unique_ptr<EffectManager<GFXBase>> g_aptrEffectManager;
DRAM_ATTR size_t g_EffectsManagerJSONBufferSize = 0;

// InitEffectsManager
//
// Initializes the effect manager.  Reboots on failure, since it's not optional

void InitEffectsManager()
{
    debugW("InitEffectsManager...");

    bool jsonReadSuccessful = false;

    File file = SPIFFS.open(EFFECTS_CONFIG_FILE);
    
    std::unique_ptr<DynamicJsonDocument> pJsonDoc(nullptr);

    if (file) 
    {
        debugI("Attempting to read EffectManager config from JSON file");

        if (g_EffectsManagerJSONBufferSize == 0)
            g_EffectsManagerJSONBufferSize = file.size();

        // Loop is here to deal with out of memory conditions
        while(true)
        {
            pJsonDoc.reset(new DynamicJsonDocument(g_EffectsManagerJSONBufferSize));
            DeserializationError error = deserializeJson(*pJsonDoc, file);

            if (error == DeserializationError::NoMemory)
            {
                debugW("Out of memory reading EffectManager config - increasing buffer");

                pJsonDoc.reset(nullptr);
                g_EffectsManagerJSONBufferSize += JSON_BUFFER_INCREMENT;
            }
            else if (error == DeserializationError::Ok)
            {
                jsonReadSuccessful = true;
                break;
            }
            else 
                break;
        }

        file.close();
    }

    if (jsonReadSuccessful)
    {
        debugI("Creating EffectManager from JSON config");

        g_aptrEffectManager = std::make_unique<EffectManager<GFXBase>>(pJsonDoc->as<JsonObjectConst>(), g_aptrDevices);
    }
    else
    {
        debugI("Creating EffectManager using default effects");
        
        std::unique_ptr<EffectPointerArray> defaultEffects;
        size_t effectCount = CreateDefaultEffects(defaultEffects);

        g_aptrEffectManager = std::make_unique<EffectManager<GFXBase>>(defaultEffects, effectCount, g_aptrDevices);
    }

    if (false == g_aptrEffectManager->Init())
        throw std::runtime_error("Could not initialize effect manager");
}

void SaveEffectManagerConfig()
{
    if (g_EffectsManagerJSONBufferSize == 0)
        g_EffectsManagerJSONBufferSize = JSON_BUFFER_BASE_SIZE;

    JsonObject jsonObject;

    // Loop is here to deal with out of memory conditions
    while(true)
    {
        DynamicJsonDocument jsonDoc(g_EffectsManagerJSONBufferSize);
        jsonObject = jsonDoc.to<JsonObject>();

        if (g_aptrEffectManager->SerializeToJSON(jsonObject))
            break;

        g_EffectsManagerJSONBufferSize += JSON_BUFFER_INCREMENT;
    }

    SPIFFS.remove(EFFECTS_CONFIG_FILE);

    File file = SPIFFS.open(EFFECTS_CONFIG_FILE, FILE_WRITE);
    
    if (!file)
    {
        debugE("Unable to open file to write EffectManager config!");
        return;
    }

    size_t bytesWritten = serializeJson(jsonObject, file);

    file.close();

    if (bytesWritten == 0)
    {
        debugE("Unable to write EffectManager config to file!");
        SPIFFS.remove(EFFECTS_CONFIG_FILE);
        return;
    }
}

// Dirty hack to support FastLED, which calls out of band to get the pixel index for "the" array, without
// any indication of which array or who's asking, so we assume the first matrix.  If you have trouble with
// more than one matrix and some FastLED functions like blur2d, this would be why.

uint16_t XY(uint8_t x, uint8_t y)
{
    // Have a drink on me!
    return (*g_aptrEffectManager)[0].get()->xy(x, y);
}

// btimap_output
//
// Output function for the jpeg library.  It doesn't provide any mechanism for passing a this pointer or other context,
// so it has to assume that it will be drawing to the main channel 0.

bool bitmap_output(int16_t x, int16_t y, uint16_t w, uint16_t h, uint16_t *bitmap)
{
    auto pgfx = (*g_aptrEffectManager)[0].get();
    pgfx->drawRGBBitmap(x, y, bitmap, w, h);
    return true;
}

