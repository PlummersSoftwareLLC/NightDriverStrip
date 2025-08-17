//+--------------------------------------------------------------------------
//
// File:        effects.h
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
//
// Description:
//
//    Defines for effect (sub)types numbers and common JSON property names
//
// History:     Apr-05-2023         Rbergen      Created for NightDriverStrip
//              Aug-17-2025         Davepl       Converted to enums
//
//---------------------------------------------------------------------------
#pragma once

// Each effect class needs to have exactly one associated effect number defined in
// the below list. The effect numbers and their respective classes are linked in the
// effect factory definitions that are created in the LoadEffectFactories()
// function in effects.cpp. The link is used when the effect list is deserialized
// from the effects list JSON file on file storage, to determine which effect
// class to construct for a particular effect JSON object - which has the effect
// number persisted as one of the core properties.
//
// Amongst others, this means that an effect number that made it to the main
// codebase should not be renumbered or reused for another effect, as it will lead
// to a mismatch between effect JSON blobs for the "old" effect and the "new" effect
// class.

// Effect identifiers (preserve existing numeric values for JSON compatibility)
enum EffectId
{
	// Strip effects
	idStripBouncingBall                     = 1,
	idStripDoublePalette                    = 2,
	idStripPaletteSpin                      = 3,
	idStripColorCycle                       = 4,
	idStripFireFan                          = 5,
	idStripRingTest                         = 6,
	idStripLantern                          = 7,
	idStripFire                             = 8,
	idStripClassicFire                      = 9,
	idStripSmoothFire                       = 10,
	idStripBaseFire                         = 11,
	idStripLaserLine                        = 12,
	idStripMeteor                           = 13,
	idStripSimpleRainbowTest                = 14,
	idStripRainbowTwinkle                   = 15,
	idStripRainbowFill                      = 16,
	idStripColorFill                        = 17,
	idStripStatus                           = 18,
	idStripTwinkle                          = 19,
	idStripSimpleColorBeat                  = 20,
	idStripPalette                          = 21,
	idStripColorBeatWithFlash               = 22,
	idStripColorBeatOverRed                 = 23,
	idStripMoltenGlassOnVioletBkgnd         = 24,
	idStripNewMoltenGlassOnVioletBkgnd      = 25,
	idStripSparklySpinningMusic             = 26,
	idStripMusicalHotWhiteInsulator         = 27,
	idStripSnake                            = 28,
	idStripPaletteFlame                     = 29,
	idStripMusicalPaletteFire               = 30,
	idStripStarryNight                      = 31,
	idStripTwinkleStar                      = 32,
	idStripSimpleInsulatorBeat              = 33,
	idStripSimpleInsulatorBeat2             = 34,
	idStripPaletteReel                      = 35,
	idStripTapeReel                         = 36,
	idStripFanBeat                          = 37,
	idStripSplashLogo                       = 38,
	idStripVUMeter                          = 39,
	idStripVUMeterVertical                  = 40,
	idStripVUInsulators                     = 41,
	idStripCount                            = 42,
	idStripColorCycleBottomUp               = 43,
	idStripColorCycleTopDown                = 44,
	idStripColorCycleSequential             = 45,
	idStripColorCycleRightLeft              = 46,
	idStripColorCycleLeftRight              = 47,
	idStripFireFanBlue                      = 48,
	idStripFireFanGreen                     = 49,
	idStripRGBRollAround                    = 50,
	idStripHueTest                          = 51,

	// Matrix effects
	idMatrixAlienText                       = 101,
	idMatrixBounce                          = 102,
	idMatrixCircuit                         = 103,
	idMatrixClock                           = 104,
	idMatrixCube                            = 105,
	// 106 was Flow Field (unused)
	idMatrixLife                            = 107,
	idMatrixMandala                         = 108,
	idMatrixSunburst                        = 109,
	idMatrixRose                            = 110,
	idMatrixPinwheel                        = 111,
	idMatrixInfinity                        = 112,
	idMatrixMunch                           = 113,
	// 114 was Curtain (unused)
	// 115 was Grid Lights (unused)
	// 116 was Palette Smear (unused)
	idMatrixRainbowFlag                     = 117,
	idMatrixPongClock                       = 118,
	idMatrixPulse                           = 119,
	idMatrixPulsar                          = 120,
	idMatrixQR                              = 121,
	idMatrixRadar                           = 122,
	idMatrixSerendipity                     = 123,
	// 124 was Spark (unused)
	idMatrixSpin                            = 125,
	idMatrixSpiro                           = 126,
	idMatrixSubscribers                     = 127,
	idMatrixSwirl                           = 128,
	idMatrixWave                            = 129,
	idMatrixWeather                         = 130,
	idMatrixInsulatorSpectrum               = 131,
	idMatrixSpectrumAnalyzer                = 132,
	idMatrixWaveform                        = 133,
	idMatrixGhostWave                       = 134,
	idMatrixMaze                            = 135,
	idMatrixSpectrumBar                     = 136,

	idMatrixSM2DDPR                         = 137,
	idMatrixSMAmberRain                     = 138,
	idMatrixSMBlurringColors                = 139,
	idMatrixSMFire2021                      = 140,
	idMatrixSMFlowFields                    = 141,
	idMatrixSMGamma                         = 142,
	idMatrixSMHolidayLights                 = 143,
	idMatrixSMHypnosis                      = 144,
	idMatrixSMMetaBalls                     = 145,
	idMatrixSMNoise                         = 146,
	idMatrixSMPicasso3in1                   = 147,
	idMatrixSMRadialFire                    = 148,
	idMatrixSMRadialWave                    = 149,
	idMatrixSMRainbowTunnel                 = 150,
	idMatrixSMSmoke                         = 151,
	idMatrixSMSpiroPulse                    = 152,
	idMatrixSMStarDeep                      = 153,
	idMatrixSMStrobeDiffusion               = 154,
	idMatrixSMSupernova                     = 155,
	idMatrixSMTwister                       = 156,
	idMatrixSMWalkingMachine                = 157,
	idMatrixAnimatedGIF                     = 158,
	idMatrixStocks                          = 159,
	idMatrixSilon                           = 160,
	idMatrixPDPGrid                         = 161,
	idMatrixAudioSpike                      = 162,
	idMatrixPDPCMX                          = 163,

	// Hexagon effects
	idHexagonOuterRing                      = 201
};

// Starry Night star variations
enum StarId
{
	idStar                                  = 1,
	idStarRandomPaletteColor                = 2,
	idStarLongLifeSparkle                   = 3,
	idStarColor                             = 4,
	idStarMusic                             = 5,
	idStarMusicPulse                        = 6,
	idStarQuiet                             = 7,
	idStarBubbly                            = 8,
	idStarFlash                             = 9,
	idStarColorCycle                        = 10,
	idStarMultiColor                        = 11,
	idStarChristmas                         = 12,
	idStarHotWhite                          = 13
};

// Legacy EFFECT_* macro aliases were removed after migration to enum-based ids.

// Some common JSON properties to prevent typos. By project convention JSON properties
// at the LEDStripEffect level have a length of 2 characters, and JSON properties
// of actual effects (i.e. LEDStripEffect subclasses) a length of 3. The purpose of this
// is keeping the effect list JSON as compact as possible. As the effect list JSON is not
// intended for "human consumption", legibility of the overall list is not much of a concern.
// As each effect instantiation has its own JSON object, clashes between
// JSON property names only have to be prevented within the scope of an individual effect
// (class).

#define PTY_EFFECTNR        "en"
#define PTY_COREEFFECT      "ce"
#define PTY_REVERSED        "rvr"
#define PTY_MIRORRED        "mir"
#define PTY_ERASE           "ers"
#define PTY_SPARKS          "spc"
#define PTY_SPARKING        "spg"
#define PTY_SPARKHEIGHT     "sph"
#define PTY_SPARKTEMP       "spt"
#define PTY_COOLING         "clg"
#define PTY_PALETTE         "plt"
#define PTY_ORDER           "ord"
#define PTY_CELLSPERLED     "cpl"
#define PTY_LEDCOUNT        "ldc"
#define PTY_SIZE            "sze"
#define PTY_SPEED           "spd"
#define PTY_MINSPEED        "mns"
#define PTY_MAXSPEED        "mxs"
#define PTY_SPEEDDIVISOR    "sdd"
#define PTY_DELTAHUE        "dth"
#define PTY_MIRRORED        "mrd"
#define PTY_EVERYNTH        "ent"
#define PTY_COLOR           "clr"
#define PTY_BLEND           "bld"
#define PTY_STARTYPENR      "stt"
#define PTY_BLUR            "blr"
#define PTY_FADE            "fde"
#define PTY_VERSION         "ver"
#define PTY_HUESTEP         "hst"
#define PTY_MULTICOLOR      "mcl"
#define PTY_EFFECT          "eft"
#define PTY_SCALE           "scl"
#define PTY_EFFECTSETVER    "esv"
#define PTY_PROJECT         "prj"
#define PTY_GIFINDEX        "gij"
#define PTY_BKCOLOR         "bkg"
#define PTY_FPS             "fps"
#define PTY_PRECLEAR        "prc"
#define PTY_IGNOREGLOBALCOLOR   "igc"

#define EFFECTS_CONFIG_FILE "/effects.cfg"
