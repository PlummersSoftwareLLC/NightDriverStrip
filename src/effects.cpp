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
//    Main table of buil-in effects and related constants and data
//
// History:     Jul-14-2021         Davepl      Split off from main.cpp
//---------------------------------------------------------------------------

#include "globals.h"                            // CONFIG and global headers
#include "effects/effectmanager.h"              // manages all of the effects
#include "effects/fireeffect.h"                 // fire effects
#include "effects/paletteeffect.h"              // palette effects
#include "effects/doublepaletteeffect.h"        // double palette effect
#include "effects/meteoreffect.h"               // meteor blend effect
#include "effects/stareffect.h"                 // star effects
#include "effects/bouncingballeffect.h"         // bouincing ball effectsenable+
#include "effects/vueffect.h"                   // vu (sound) based effects
#include "effects/tempeffect.h"


#if ENABLE_AUDIO
#include "effects/spectrumeffects.h"            // Musis spectrum effects
#include "effects/musiceffect.h"                // Music based effects
#endif

#ifdef FAN_SIZE
    #include "effects/faneffects.h"             // Fan-based effects
#endif

//
// Externals
//

extern DRAM_ATTR shared_ptr<LEDMatrixGFX> g_pStrands[NUM_CHANNELS];

// Palettes
//
// Palettes that are referenced by effects need to be instantiated first


CRGBPalette256 BlueColors_p =
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

CRGBPalette256 RedColors_p =
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
    CRGB::OrangeRed,
};

CRGBPalette256 GreenColors_p =
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
    CRGB::LimeGreen,
};

CRGBPalette256 PurpleColors_p =
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

CRGBPalette256 RGBColors_p = 
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

CRGBPalette256 MagentaColors_p = 
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
    CRGB::Magenta
};

CRGBPalette256 spectrumBasicColors  =
{
	CRGB(0xFD0E35),                     // Red
	CRGB(0xFF8833),                     // Orange
	CRGB(0xFFEB00),                     // Middle Yellow
	CRGB(0xAFE313),                     // Inchworm
    CRGB(0x3AA655),                     // Green
    CRGB(0x8DD9CC),                     // Middle Blue Green
    CRGB(0x0066FF),                     // Blue III
    CRGB(0xDB91EF),                     // Lilac
    CRGB(0xFD0E35),                     // Red
	CRGB(0xFF8833),                     // Orange
	CRGB(0xFFEB00),                     // Middle Yellow
	CRGB(0xAFE313),                     // Inchworm
    CRGB(0x3AA655),                     // Green
    CRGB(0x8DD9CC),                     // Middle Blue Green
    CRGB(0x0066FF),                     // Blue III
    CRGB(0xDB91EF)                      // Lilac
};

CRGBPalette256 USAColors_p  =
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


// AllEffects
//
// The master effects table


CRGBPalette256 rainbowPalette(RainbowColors_p);
CRGBPalette256 blueSweep(CRGB::Blue, CRGB::Green);

CRGBPalette256 BlueStripes(CRGB::White, CRGB::Blue, CRGB::Blue, CRGB::Blue, CRGB::Blue, CRGB::White, CRGB::Black, CRGB::Black, 
                           CRGB::White, CRGB::Blue, CRGB::Blue, CRGB::Blue, CRGB::Blue, CRGB::White, CRGB::Black, CRGB::Black);

CRGBPalette256 MagentaStripes(CRGB::White, CRGB::Magenta, CRGB::Magenta, CRGB::Magenta, CRGB::Magenta, CRGB::White, CRGB::Black, CRGB::Black, 
                           CRGB::White, CRGB::Magenta, CRGB::Magenta, CRGB::Magenta, CRGB::Magenta, CRGB::White, CRGB::Black, CRGB::Black);

#if ENABLE_AUDIO
// GetSpectrumAnalyzer
//
// A little factory that makes colored spectrum analyzers to be used by the remote control
// colored buttons

unique_ptr<LEDStripEffect> GetSpectrumAnalyzer(CRGB color)
{
    CRGB colorDark = color;
    colorDark.fadeToBlackBy(64);
    CRGB colorLight = color;
    colorLight += CRGB(64,64,64);
    return make_unique<SpectrumAnalyzerEffect>("Spectrum Color", CRGBPalette256(colorLight, colorDark));    
}
#endif

// AllEffects
// 
// A list of internal effects, if any.  
DRAM_ATTR LEDStripEffect * AllEffects[] =
{
  #if DEMO

    // Animate a simple rainbow palette by using the palette effect on the built-in rainbow palette
    new PaletteEffect(rainbowPalette, 256/16, .2, 0)

  #elif TREESET

  // Animate a simple rainbow palette by using the palette effect on the built-in rainbow palette
  
    new ColorCycleEffect(BottomUp),
    new StarryNightEffect<LongLifeSparkleStar>("Blue Sparkle Stars", GreenColors_p, 2.0, 1, LINEARBLEND, 2.0, 0.0, 0.0, CRGB::Blue),        // Blue Sparkle

    //new PaletteSpinEffect("BlueStripeSpin", CRGBPalette256(blueSweep), false, 0, 0.1)

    //new StarryNightEffect<LongLifeSparkleStar>("Blue Sparkle Stars", BlueColors_p, 10.0, 1, LINEARBLEND, 2.0, 0.0, 0.0),        // Blue Sparkle

    
    //new PaletteSpinEffect("CycleStripeSpin", CRGBPalette256(MagentaStripes), true),
    
    //new StarryNightEffect<Star>()
    /*
    new SparklySpinningMusicEffect("SparklySpinningMusical", RainbowColors_p), 
    new ColorBeatOverRedBkgnd("ColorBeatOnRedBkgnd"),
    new MoltenGlassOnVioletBkgnd("MoltenGlassOnViolet", RainbowColors_p),
    new ColorBeatWithFlash("ColorBeatWithFlash"),
    new MusicalHotWhiteInsulatorEffect("MusicalHotWhite"),

    new SimpleInsulatorBeatEffect2("SimpleInsulatorColorBeat"),
    new InsulatorSpectrumEffect("InsulatorSpectrumEffect"),
    */
    new PaletteEffect(rainbowPalette, 256/16, .2, 0)

  #elif INSULATORS

    new SparklySpinningMusicEffect(RainbowColors_p), 
    /*
    new ColorBeatOverRedBkgnd(),
    new MoltenGlassOnVioletBkgnd(RainbowColors_p),
    new ColorBeatWithFlash(),
    new MusicalHotWhiteInsulatorEffect(),

    new ColorBeatOverRedBkgnd(),              // Color beat over red background

    new SimpleInsulatorBeatEffect2(),
    new ColorBeatOverRedBkgnd(),              // Color beat over red background
    new InsulatorSpectrumEffect(),

    new ColorBeatOverRedBkgnd(),              // Color beat over red background
    new SparklySpinningMusicEffect(RainbowColors_p), 
    new MoltenGlassOnVioletBkgnd(RainbowColors_p),
*/
          

  #elif BELT

    // Yes, I made a sparkly LED belt and wore it to a party.  Batteries toO!
    new TwinkleEffect(NUM_LEDS/4, 10),

  #elif MAGICMIRROR

    new MoltenGlassOnVioletBkgnd(RainbowColors_p),

  #elif SPECTRUM

    new GhostWave("GhostWave One", new CRGBPalette256(CRGBPalette16(CRGB::Blue,  CRGB::Green, CRGB::Yellow, CRGB::Red)), 4),
    new SpectrumAnalyzerEffect("Spectrum Standard", spectrumBasicColors),
    new GhostWave("GhostWave Rainbow", &rainbowPalette, 8),
    new SpectrumAnalyzerEffect("Spectrum USA", USAColors_p, 0),
    new SpectrumAnalyzerEffect("Spectrum Fade", spectrumBasicColors, 50, 70, -1.0, 3.0),
    new GhostWave("GhostWave Blue", new CRGBPalette256(CRGBPalette16(CRGB::DarkBlue, CRGB::Blue, CRGB::Blue, CRGB::White)), 0),
    new GhostWave("GhostWave Rainbow", &rainbowPalette),
   
  #elif UMBRELLA
  
    new PaletteFlameEffect("Smooth Red Fire", heatmap_pal),
    new ClassicFireEffect(),
    new StarryNightEffect<MusicStar>("RGB Music Bubbles", RGBColors_p, 0.5, 1, NOBLEND, 15.0, 0.0, 75.0),   // RGB Music Bubbles
    new StarryNightEffect<MusicPulseStar>("RGB Pulse", RainbowColors_p, 0.02, 20, NOBLEND, 5.0, 0.0, 75.0), // RGB Music Bubbles
    new PaletteFlameEffect("Smooth Purple Fire", purpleflame_pal),
    new VUFlameEffect("Multicolor Sound Flame", VUFlameEffect::MULTICOLOR),
    new BouncingBallEffect(),
    new MeteorEffect(),                                                                                                     // Our overlapping color meteors
    new StarryNightEffect<BubblyStar>("Little Blooming Rainbow Stars", BlueColors_p, prob, 4, LINEARBLEND, 2.0, 0.0, mult), // Blooming Little Rainbow Stars
    new StarryNightEffect<BubblyStar>("Big Blooming Rainbow Stars", RainbowColors_p, 2, 12, LINEARBLEND, 1.0),              // Blooming Rainbow Stars
    new StarryNightEffect<BubblyStar>("Neon Bars", RainbowColors_p, 0.5, 64, NOBLEND, 0),                                   // Neon Bars
    new StarryNightEffect<MusicStar>("RGB Music Blend Stars", RGBColors_p, 0.8, 1, NOBLEND, 15.0, 0.1, 10.0),      // RGB Music Blur - Can You Hear Me Knockin'
    new StarryNightEffect<MusicStar>("Rainbow Music Stars", RainbowColors_p, 2.0, 2, LINEARBLEND, 5.0, 0.0, 10.0), // Rainbow Music Star

    //        new VUFlameEffect("Sound Flame (Green)",    VUFlameEffect::GREEN),
    //       new VUFlameEffect("Sound Flame (Blue)",     VUFlameEffect::BLUE),

    new SimpleRainbowTestEffect(8, 4),                                                                            // Rainbow palette simple test of walking pixels
    new PaletteEffect(RainbowColors_p),                                                                           // Rainbow palette
    new StarryNightEffect<QuietStar>("Green Twinkle Stars", GreenColors_p, prob, 1, LINEARBLEND, 2.0, 0.0, mult), // Green Twinkle
    new StarryNightEffect<Star>("Blue Sparkle Stars", BlueColors_p, prob, 1, LINEARBLEND, 2.0, 0.0, mult),        // Blue Sparkle

    new StarryNightEffect<QuietStar>("Red Twinkle Stars", RedColors_p, 1.0, 1, LINEARBLEND, 2.0),                     // Red Twinkle
    new StarryNightEffect<Star>("Lava Stars", LavaColors_p, prob, 1, LINEARBLEND, 2.0, 0.0, mult),                    // Lava Stars
    new StarryNightEffect<QuietStar>("Rainbow Twinkle Stars", RainbowColors_p, prob, 1, LINEARBLEND, 2.0, 0.0, mult), // Rainbow Twinkle
    new DoublePaletteEffect(),
#elif ATOMLIGHT
    new ColorFillEffect(CRGB::White, 1),
    new FireFanEffect(NUM_LEDS,      1, 15, 80, 2, 7, Sequential, true, false),
    new FireFanEffect(NUM_LEDS,      1, 15, 80, 2, 7, Sequential, true, false, true),
    new BlueFireFanEffect(NUM_LEDS,      2, 5, 120, 1, 1, Sequential, true, false),
    new GreenFireFanEffect(NUM_LEDS,      2, 3, 100, 1, 1, Sequential, true, false),
    new RainbowFillEffect(60, 0),
    new ColorCycleEffect(Sequential),
    new PaletteEffect(RainbowColors_p, 4, 0.1, 0.0, 1.0, 0.0),
    new BouncingBallEffect(3, true, true, 1),


    new ChannelBeatEffect("ChannelBeat"),

    new StarryNightEffect<BubblyStar>("Little Blooming Rainbow Stars", BlueColors_p, 8.0, 4, LINEARBLEND, 2.0, 0.0, 4), // Blooming Little Rainbow Stars
    new StarryNightEffect<BubblyStar>("Big Blooming Rainbow Stars", RainbowColors_p, 20, 12, LINEARBLEND, 1.0, 0.0, 2),              // Blooming Rainbow Stars
    new StarryNightEffect<FanStar>("FanStars", RainbowColors_p, 8.0, 1.0, LINEARBLEND, 80.0, 0, 2.0),

    new MeteorEffect(20, 1, 25, .15, .05),
    new MeteorEffect(12, 1, 25, .15, .08),
    new MeteorEffect(6, 1, 25, .15, .12),
    new MeteorEffect(1, 1, 5, .15, .25),
    new MeteorEffect(), // Rainbow palette

    new VUEffect(),
#elif FIRESTICK

    new BouncingBallEffect(),
    new VUFlameEffect("Multicolor Sound Flame", VUFlameEffect::MULTICOLOR),
    new PaletteFlameEffect("Smooth Red Fire", heatmap_pal),
    //new ClassicFireEffect(),

    new StarryNightEffect<MusicStar>("RGB Music Bubbles", RGBColors_p, 0.5, 1, NOBLEND, 15.0, 0.0, 75.0), // RGB Music Bubbles
    //new StarryNightEffect<MusicPulseStar>("RGB Pulse", RainbowColors_p, 0.02, 20, NOBLEND, 5.0, 0.0, 75.0), // RGB Music Bubbles

    new PaletteFlameEffect("Smooth Purple Fire", purpleflame_pal),

    new MeteorEffect(),                                                                                                     // Our overlapping color meteors
    new StarryNightEffect<BubblyStar>("Little Blooming Rainbow Stars", BlueColors_p, prob, 4, LINEARBLEND, 2.0, 0.0, mult), // Blooming Little Rainbow Stars
    new StarryNightEffect<BubblyStar>("Big Blooming Rainbow Stars", RainbowColors_p, 2, 12, LINEARBLEND, 1.0),              // Blooming Rainbow Stars
    new StarryNightEffect<BubblyStar>("Neon Bars", RainbowColors_p, 0.5, 64, NOBLEND, 0),                                   // Neon Bars

    new StarryNightEffect<MusicStar>("RGB Music Blend Stars", RGBColors_p, 0.8, 1, NOBLEND, 15.0, 0.1, 10.0),      // RGB Music Blur - Can You Hear Me Knockin'
    new StarryNightEffect<MusicStar>("Rainbow Music Stars", RainbowColors_p, 2.0, 2, LINEARBLEND, 5.0, 0.0, 10.0), // Rainbow Music Star

    //        new VUFlameEffect("Sound Flame (Green)",    VUFlameEffect::GREEN),
    //       new VUFlameEffect("Sound Flame (Blue)",     VUFlameEffect::BLUE),

    new SimpleRainbowTestEffect(8, 4),                                                                            // Rainbow palette simple test of walking pixels
    new PaletteEffect(RainbowColors_p),                                                                           // Rainbow palette
    new StarryNightEffect<QuietStar>("Green Twinkle Stars", GreenColors_p, prob, 1, LINEARBLEND, 2.0, 0.0, mult), // Green Twinkle
    new StarryNightEffect<Star>("Blue Sparkle Stars", BlueColors_p, prob, 1, LINEARBLEND, 2.0, 0.0, mult),        // Blue Sparkle

    new StarryNightEffect<QuietStar>("Red Twinkle Stars", RedColors_p, 1.0, 1, LINEARBLEND, 2.0, 0.0, mult),          // Red Twinkle
    new StarryNightEffect<Star>("Lava Stars", LavaColors_p, prob, 1, LINEARBLEND, 2.0, 0.0, mult),                    // Lava Stars
    new StarryNightEffect<QuietStar>("Rainbow Twinkle Stars", RainbowColors_p, prob, 1, LINEARBLEND, 2.0, 0.0, mult), // Rainbow Twinkle
    new DoublePaletteEffect(),
    new VUEffect()

#elif FLAMEBULB
    new PaletteFlameEffect("Smooth Red Fire", heatmap_pal, true, 4.5, 1, 1, 255, 4, false),
#elif BIGMATRIX
    new SimpleRainbowTestEffect(8, 1),  // Rainbow palette simple test of walking pixels
    new PaletteEffect(RainbowColors_p), // Rainbow palette
    new RainbowFillEffect(24, 0),
    new RainbowFillEffect(),

    new StarryNightEffect<MusicStar>("RGB Music Bubbles", RGBColors_p, 0.25, 1, NOBLEND, 3.0, 0.0, 75.0),                   // RGB Music Bubbles
    new StarryNightEffect<HotWhiteStar>("Lava Stars", HeatColors_p, prob, 1, LINEARBLEND, 2.0, 0.0, mult),                  // Lava Stars
    new StarryNightEffect<BubblyStar>("Little Blooming Rainbow Stars", BlueColors_p, prob, 4, LINEARBLEND, 2.0, 0.0, mult), // Blooming Little Rainbow Stars
    new StarryNightEffect<QuietStar>("Green Twinkle Stars", GreenColors_p, prob, 1, LINEARBLEND, 2.0, 0.0, mult),           // Green Twinkle
    new StarryNightEffect<QuietStar>("Red Twinkle Stars", RedColors_p, 1.0, 1, LINEARBLEND, 2.0),                           // Red Twinkle
    new StarryNightEffect<QuietStar>("Rainbow Twinkle Stars", RainbowColors_p, prob, 1, LINEARBLEND, 2.0, 0.0, mult),       // Rainbow Twinkle
    new BouncingBallEffect(),
    new VUEffect()

#elif STRAND || ATOMISTRING

    new PaletteEffect(RainbowStripeColors_p, 8.0, .125, 0, 1, 0), // Rainbow palette
    new TwinkleEffect(NUM_LEDS/2, 10),
    new SimpleRainbowTestEffect(8, 1),  // Rainbow palette simple test of walking pixels
    new TwinkleEffect(NUM_LEDS/2, 0, 50),
    new RainbowFillEffect(),

#elif RINGSET
    new MusicalInsulatorEffect2("Musical Effect 2"),   

#elif FANSET

    new RainbowFillEffect(24, 0),
    new ColorCycleEffect(BottomUp),
    new ColorCycleEffect(TopDown),
    new ColorCycleEffect(LeftRight),
    new ColorCycleEffect(RightLeft),

    // /* 8 */ new StarryNightEffect<FanStar>("FanStars", RainbowColors_p, 4.0, 1.0, LINEARBLEND, 80.0, 0, 4.0),
    /* 6 */ new StarryNightEffect<BubblyStar>("Little Blooming Rainbow Stars", BlueColors_p, 8.0, 4, LINEARBLEND, 2.0, 0.0, 1.0), // Blooming Little Rainbow Stars
    /* 7 */ new StarryNightEffect<BubblyStar>("Big Blooming Rainbow Stars", RainbowColors_p, 2, 12, LINEARBLEND, 1.0),              // Blooming Rainbow Stars
    /* 6 */ new StarryNightEffect<BubblyStar>("Neon Bars", RainbowColors_p, 0.5, 64, NOBLEND, 0),                                   // Neon Bars

    new FireFanEffect(NUM_LEDS,      4, 7, 200, 2, NUM_LEDS / 2, Sequential, false, true),
    new FireFanEffect(NUM_LEDS,      3, 8, 200, 2, NUM_LEDS / 2, Sequential, false, true),
    new FireFanEffect(NUM_LEDS,      2, 10, 200, 2, NUM_LEDS / 2, Sequential, false, true),
    new FireFanEffect(NUM_LEDS,      1, 12, 200, 2, NUM_LEDS / 2, Sequential, false, true),

    new BlueFireFanEffect(NUM_LEDS,      4, 7, 200, 2, NUM_LEDS / 2, Sequential, false, true),
    new BlueFireFanEffect(NUM_LEDS,      3, 8, 200, 2, NUM_LEDS / 2, Sequential, false, true),
    new BlueFireFanEffect(NUM_LEDS,      2, 10, 200, 2, NUM_LEDS / 2, Sequential, false, true),
    new BlueFireFanEffect(NUM_LEDS,      1, 12, 200, 2, NUM_LEDS / 2, Sequential, false, true),
    
    new GreenFireFanEffect(NUM_LEDS,      4, 7, 200, 2, NUM_LEDS / 2, Sequential, false, true),
    new GreenFireFanEffect(NUM_LEDS,      3, 8, 200, 2, NUM_LEDS / 2, Sequential, false, true),
    new GreenFireFanEffect(NUM_LEDS,      2, 10, 200, 2, NUM_LEDS / 2, Sequential, false, true),
    new GreenFireFanEffect(NUM_LEDS,      1, 12, 200, 2, NUM_LEDS / 2, Sequential, false, true),

    #if ENABLE_AUDIO
      
        new MusicFireEffect(NUM_LEDS, 1, 10, 100, 0, NUM_LEDS),
        
        new StarryNightEffect<MusicStar>("RGB Music Blend Stars", RGBColors_p, 0.8, 1, NOBLEND, 15.0, 0.1, 10.0),      // RGB Music Blur - Can You Hear Me Knockin'
        new StarryNightEffect<MusicStar>("Rainbow Music Stars", RainbowColors_p, 2.0, 2, LINEARBLEND, 5.0, 0.0, 10.0), // Rainbow Music Star

        new FanBeatEffect("FanBeat"),
        new PaletteReelEffect("PaletteReelEffect"),
        new MeteorEffect(),   
        new TapeReelEffect("TapeReelEffect"),
        new VUFlameEffect("Multicolor Sound Flame", VUFlameEffect::MULTICOLOR, 50, true),
    #endif

#elif BROOKLYNROOM

    new RainbowFillEffect(24, 0),
    new RainbowFillEffect(32, 1),
    new SimpleRainbowTestEffect(8, 1),  // Rainbow palette simple test of walking pixels
    new SimpleRainbowTestEffect(8, 4),                                                                            // Rainbow palette simple test of walking pixels
    new PaletteEffect(MagentaColors_p),                                                                           // Rainbow palette
    new DoublePaletteEffect(),

    new MeteorEffect(),                                                                                                     // Our overlapping color meteors

    new StarryNightEffect<QuietStar>("Magenta Twinkle Stars", GreenColors_p, prob, 1, LINEARBLEND, 2.0, 0.0, mult), // Green Twinkle
    new StarryNightEffect<Star>("Blue Sparkle Stars", BlueColors_p, prob, 1, LINEARBLEND, 2.0, 0.0, mult),        // Blue Sparkle

    new StarryNightEffect<QuietStar>("Red Twinkle Stars", MagentaColors_p, 1.0, 1, LINEARBLEND, 2.0),                     // Red Twinkle
    new StarryNightEffect<Star>("Lava Stars", MagentaColors_p, prob, 1, LINEARBLEND, 2.0, 0.0, mult),                    // Lava Stars
    new StarryNightEffect<QuietStar>("Rainbow Twinkle Stars", RainbowColors_p, prob, 1, LINEARBLEND, 2.0, 0.0, mult), // Rainbow Twinkle

    new StarryNightEffect<BubblyStar>("Little Blooming Rainbow Stars", MagentaColors_p, prob, 4, LINEARBLEND, 2.0, 0.0, mult), // Blooming Little Rainbow Stars
    new StarryNightEffect<BubblyStar>("Big Blooming Rainbow Stars", MagentaColors_p, 2, 12, LINEARBLEND, 1.0),              // Blooming Rainbow Stars
    new StarryNightEffect<BubblyStar>("Neon Bars", MagentaColors_p, 0.5, 64, NOBLEND, 0),                                   // Neon Bars
/*
    new StarryNightEffect<MusicStar>("RGB Music Blend Stars", RGBColors_p, 0.8, 1, NOBLEND, 15.0, 0.1, 10.0),      // RGB Music Blur - Can You Hear Me Knockin'
    new StarryNightEffect<MusicStar>("Rainbow Music Stars", RainbowColors_p, 2.0, 2, LINEARBLEND, 5.0, 0.0, 10.0), // Rainbow Music Star
    new StarryNightEffect<MusicStar>("RGB Music Bubbles", RGBColors_p, 0.5, 1, NOBLEND, 15.0, 0.0, 75.0),   // RGB Music Bubbles
    new StarryNightEffect<MusicPulseStar>("RGB Pulse", RainbowColors_p, 0.02, 20, NOBLEND, 5.0, 0.0, 75.0), // RGB Music Bubbles
*/
    new VUEffect(),
    new VUEffect(1),
    new VUEffect(8),
    new VUEffect(128),

    new ClassicFireEffect(true),
    new VUFlameEffect("Sound Flame (Red)",      VUFlameEffect::REDX,       50, true),
    new VUFlameEffect("Sound Flame (Green)",    VUFlameEffect::GREENX,     50, true),
    new VUFlameEffect("Sound Flame (Blue)",     VUFlameEffect::BLUEX,      50, true),
    new VUFlameEffect("Multicolor Sound Flame", VUFlameEffect::MULTICOLOR, 50, true),


#elif LEDSTRIP
    // new PaletteEffect(RainbowStripeColors_p, 8.0, .125, 0, 5, 1), // Rainbow palette
    new StatusEffect(CRGB::Blue)

#elif HOODORNAMENT

    new RainbowFillEffect(24, 0),
    new RainbowFillEffect(32, 1),
    new SimpleRainbowTestEffect(8, 1),  // Rainbow palette simple test of walking pixels
    new PaletteEffect(MagentaColors_p),                                                                           // Rainbow palette
    new DoublePaletteEffect(),        

#endif

};

// InitEffectsManager
//
// Initializes the effect manager.  Reboots on failure, since it's not optional

void InitEffectsManager()
{
    g_pEffectManager = make_unique<EffectManager>(AllEffects, ARRAYSIZE(AllEffects), g_pStrands);
    if (false == g_pEffectManager->Init())
        throw runtime_error("Could not initialize effect manager");
}
