//+--------------------------------------------------------------------------
//
// File:        effectfactories.cpp
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
//    Lookup tables for effect factories that create effect instances from 
//    JSON objects, and support functions using them.
//
// History:     Apr-05-2023         Rbergen      Created for NightDriverStrip
//---------------------------------------------------------------------------

#include "effectdependencies.h"
#include <map>

typedef LEDStripEffect* (*JsonEffectFactory)(const JsonObjectConst&);

std::map<int, JsonEffectFactory> g_JsonStarryNightEffectFactories = 
{
    { EFFECT_STAR,
        [](const JsonObjectConst& jsonObject) -> LEDStripEffect* { return new StarryNightEffect<Star>(jsonObject); } },
    { EFFECT_STAR_BUBBLY, 
        [](const JsonObjectConst& jsonObject) -> LEDStripEffect* { return new StarryNightEffect<BubblyStar>(jsonObject); } },
    { EFFECT_STAR_HOT_WHITE,
        [](const JsonObjectConst& jsonObject) -> LEDStripEffect* { return new StarryNightEffect<HotWhiteStar>(jsonObject); } },
    { EFFECT_STAR_LONG_LIFE_SPARKLE,
        [](const JsonObjectConst& jsonObject) -> LEDStripEffect* { return new StarryNightEffect<LongLifeSparkleStar>(jsonObject); } },

#if ENABLE_AUDIO
    { EFFECT_STAR_MUSIC,
        [](const JsonObjectConst& jsonObject) -> LEDStripEffect* { return new StarryNightEffect<MusicStar>(jsonObject); } },
#endif

    { EFFECT_STAR_QUIET,
        [](const JsonObjectConst& jsonObject) -> LEDStripEffect* { return new StarryNightEffect<QuietStar>(jsonObject); } },
};

LEDStripEffect* CreateStarryNightEffectFromJSON(const JsonObjectConst& jsonObject)
{
    auto entry = g_JsonStarryNightEffectFactories.find(jsonObject[PTY_STARTYPENR].as<int>());

    return entry != g_JsonStarryNightEffectFactories.end() 
        ? entry->second(jsonObject)
        : nullptr;
}

std::map<int, JsonEffectFactory> g_JsonEffectFactories = 
{
    { EFFECT_STRIP_BOUNCING_BALL,
        [](const JsonObjectConst& jsonObject) -> LEDStripEffect* { return new BouncingBallEffect(jsonObject); } },
    { EFFECT_STRIP_CLASSIC_FIRE,
        [](const JsonObjectConst& jsonObject) -> LEDStripEffect* { return new ClassicFireEffect(jsonObject); } },
    { EFFECT_STRIP_COLOR_CYCLE,
        [](const JsonObjectConst& jsonObject) -> LEDStripEffect* { return new ColorCycleEffect(jsonObject); } },
    { EFFECT_STRIP_COLOR_FILL,
        [](const JsonObjectConst& jsonObject) -> LEDStripEffect* { return new ColorFillEffect(jsonObject); } },
    { EFFECT_STRIP_DOUBLE_PALETTE,
        [](const JsonObjectConst& jsonObject) -> LEDStripEffect* { return new DoublePaletteEffect(jsonObject); } },
    { EFFECT_STRIP_FIRE,
        [](const JsonObjectConst& jsonObject) -> LEDStripEffect* { return new FireEffect(jsonObject); } },
    { EFFECT_STRIP_LANTERN,
        [](const JsonObjectConst& jsonObject) -> LEDStripEffect* { return new LanternEffect(jsonObject); } },
    { EFFECT_STRIP_LASER_LINE,
        [](const JsonObjectConst& jsonObject) -> LEDStripEffect* { return new LaserLineEffect(jsonObject); } },
    { EFFECT_STRIP_METEOR,
        [](const JsonObjectConst& jsonObject) -> LEDStripEffect* { return new MeteorEffect(jsonObject); } },
    { EFFECT_STRIP_PALETTE,
        [](const JsonObjectConst& jsonObject) -> LEDStripEffect* { return new PaletteEffect(jsonObject); } },
    { EFFECT_STRIP_PALETTE_FLAME,
        [](const JsonObjectConst& jsonObject) -> LEDStripEffect* { return new PaletteFlameEffect(jsonObject); } },
    { EFFECT_STRIP_RAINBOW_FILL,
        [](const JsonObjectConst& jsonObject) -> LEDStripEffect* { return new RainbowFillEffect(jsonObject); } },
    { EFFECT_STRIP_SIMPLE_RAINBOW_TEST,
        [](const JsonObjectConst& jsonObject) -> LEDStripEffect* { return new SimpleRainbowTestEffect(jsonObject); } },
    { EFFECT_STRIP_STARRY_NIGHT,
        [](const JsonObjectConst& jsonObject) -> LEDStripEffect* { return CreateStarryNightEffectFromJSON(jsonObject); } },
    { EFFECT_STRIP_STATUS,
        [](const JsonObjectConst& jsonObject) -> LEDStripEffect* { return new StatusEffect(jsonObject); } },
    { EFFECT_STRIP_TWINKLE,
        [](const JsonObjectConst& jsonObject) -> LEDStripEffect* { return new TwinkleEffect(jsonObject); } },

#if ENABLE_AUDIO
    { EFFECT_STRIP_COLOR_BEAT_OVER_RED,
        [](const JsonObjectConst& jsonObject) -> LEDStripEffect* { return new ColorBeatOverRed(jsonObject); } },
    { EFFECT_STRIP_NEW_MOLTEN_GLASS_ON_VIOLET_BKGND,
        [](const JsonObjectConst& jsonObject) -> LEDStripEffect* { return new NewMoltenGlassOnVioletBkgnd(jsonObject); } },
    { EFFECT_STRIP_MUSICAL_PALETTE_FIRE,
        [](const JsonObjectConst& jsonObject) -> LEDStripEffect* { return new MusicalPaletteFire(jsonObject); } },
    { EFFECT_STRIP_SIMPLE_INSULATOR_BEAT2,
        [](const JsonObjectConst& jsonObject) -> LEDStripEffect* { return new SimpleInsulatorBeatEffect2(jsonObject); } },
    { EFFECT_STRIP_SPARKLY_SPINNING_MUSIC,
        [](const JsonObjectConst& jsonObject) -> LEDStripEffect* { return new SparklySpinningMusicEffect(jsonObject); } },
    { EFFECT_MATRIX_INSULATOR_SPECTRUM,
        [](const JsonObjectConst& jsonObject) -> LEDStripEffect* { return new InsulatorSpectrumEffect(jsonObject); } },
    { EFFECT_MATRIX_SPECTRUM_ANALYZER,
        [](const JsonObjectConst& jsonObject) -> LEDStripEffect* { return new SpectrumAnalyzerEffect(jsonObject); } },
    { EFFECT_MATRIX_WAVEFORM,
        [](const JsonObjectConst& jsonObject) -> LEDStripEffect* { return new WaveformEffect(jsonObject); } },
    { EFFECT_MATRIX_GHOST_WAVE,
        [](const JsonObjectConst& jsonObject) -> LEDStripEffect* { return new GhostWave(jsonObject); } },
#endif

#if FAN_SIZE
    { EFFECT_STRIP_FAN_BEAT,
        [](const JsonObjectConst& jsonObject) -> LEDStripEffect* { return new FanBeatEffect(jsonObject); } },
    { EFFECT_STRIP_FIRE_FAN,
        [](const JsonObjectConst& jsonObject) -> LEDStripEffect* { return new FireFanEffect(jsonObject); } },
    { EFFECT_STRIP_PALETTE_REEL,
        [](const JsonObjectConst& jsonObject) -> LEDStripEffect* { return new PaletteReelEffect(jsonObject); } },
    { EFFECT_STRIP_PALETTE_REEL,
        [](const JsonObjectConst& jsonObject) -> LEDStripEffect* { return new PaletteReelEffect(jsonObject); } },
    { EFFECT_STRIP_TAPE_REEL,
        [](const JsonObjectConst& jsonObject) -> LEDStripEffect* { return new TapeReelEffect(jsonObject); } },
#endif

#if USE_MATRIX
    { EFFECT_MATRIX_ALIEN_TEXT,
        [](const JsonObjectConst& jsonObject) -> LEDStripEffect* { return new PatternAlienText(jsonObject); } },
    { EFFECT_MATRIX_BOUNCE,
        [](const JsonObjectConst& jsonObject) -> LEDStripEffect* { return new PatternBounce(jsonObject); } },
    { EFFECT_MATRIX_CIRCUIT,
        [](const JsonObjectConst& jsonObject) -> LEDStripEffect* { return new PatternCircuit(jsonObject); } },
    { EFFECT_MATRIX_CLOCK,
        [](const JsonObjectConst& jsonObject) -> LEDStripEffect* { return new PatternClock(jsonObject); } },
    { EFFECT_MATRIX_CUBE,
        [](const JsonObjectConst& jsonObject) -> LEDStripEffect* { return new PatternCube(jsonObject); } },
    { EFFECT_MATRIX_FLOW_FIELD,
        [](const JsonObjectConst& jsonObject) -> LEDStripEffect* { return new PatternFlowField(jsonObject); } },
    { EFFECT_MATRIX_INFINITY,
        [](const JsonObjectConst& jsonObject) -> LEDStripEffect* { return new PatternInfinity(jsonObject); } },
    { EFFECT_MATRIX_LIFE,
        [](const JsonObjectConst& jsonObject) -> LEDStripEffect* { return new PatternLife(jsonObject); } },
    { EFFECT_MATRIX_MANDALA,
        [](const JsonObjectConst& jsonObject) -> LEDStripEffect* { return new PatternMandala(jsonObject); } },
    { EFFECT_MATRIX_PINWHEEL,
        [](const JsonObjectConst& jsonObject) -> LEDStripEffect* { return new PatternPinwheel(jsonObject); } },
    { EFFECT_MATRIX_PONG_CLOCK,
        [](const JsonObjectConst& jsonObject) -> LEDStripEffect* { return new PatternPongClock(jsonObject); } },
    { EFFECT_MATRIX_PULSAR,
        [](const JsonObjectConst& jsonObject) -> LEDStripEffect* { return new PatternPulsar(jsonObject); } },
    { EFFECT_MATRIX_ROSE,
        [](const JsonObjectConst& jsonObject) -> LEDStripEffect* { return new PatternRose(jsonObject); } },
    { EFFECT_MATRIX_SERENDIPITY,
        [](const JsonObjectConst& jsonObject) -> LEDStripEffect* { return new PatternSerendipity(jsonObject); } },
    { EFFECT_MATRIX_SPIRO,
        [](const JsonObjectConst& jsonObject) -> LEDStripEffect* { return new PatternSpiro(jsonObject); } },
    { EFFECT_MATRIX_SUBSCRIBERS,
        [](const JsonObjectConst& jsonObject) -> LEDStripEffect* { return new PatternSubscribers(jsonObject); } },
    { EFFECT_MATRIX_SUNBURST,
        [](const JsonObjectConst& jsonObject) -> LEDStripEffect* { return new PatternSunburst(jsonObject); } },
    { EFFECT_MATRIX_SWIRL,
        [](const JsonObjectConst& jsonObject) -> LEDStripEffect* { return new PatternSwirl(jsonObject); } },
    { EFFECT_MATRIX_WAVE,
        [](const JsonObjectConst& jsonObject) -> LEDStripEffect* { return new PatternWave(jsonObject); } },
    { EFFECT_MATRIX_WEATHER,
        [](const JsonObjectConst& jsonObject) -> LEDStripEffect* { return new PatternWeather(jsonObject); } },
#endif
};

LEDStripEffect* CreateEffectFromJSON(const JsonObjectConst& jsonObject)
{
    auto entry = g_JsonEffectFactories.find(jsonObject[PTY_EFFECTNR].as<int>());

    return entry != g_JsonEffectFactories.end() 
        ? entry->second(jsonObject)
        : nullptr;
}
