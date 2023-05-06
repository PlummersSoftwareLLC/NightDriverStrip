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

typedef std::shared_ptr<LEDStripEffect> (*JsonEffectFactory)(const JsonObjectConst&);

std::map<int, JsonEffectFactory> g_JsonStarryNightEffectFactories = 
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

std::map<int, JsonEffectFactory> g_JsonEffectFactories = 
{
    { EFFECT_STRIP_BOUNCING_BALL,
        [](const JsonObjectConst& jsonObject)->std::shared_ptr<LEDStripEffect> { return std::make_shared<BouncingBallEffect>(jsonObject); } },
    { EFFECT_STRIP_CLASSIC_FIRE,
        [](const JsonObjectConst& jsonObject)->std::shared_ptr<LEDStripEffect> { return std::make_shared<ClassicFireEffect>(jsonObject); } },
    { EFFECT_STRIP_COLOR_CYCLE,
        [](const JsonObjectConst& jsonObject)->std::shared_ptr<LEDStripEffect> { return std::make_shared<ColorCycleEffect>(jsonObject); } },
    { EFFECT_STRIP_COLOR_FILL,
        [](const JsonObjectConst& jsonObject)->std::shared_ptr<LEDStripEffect> { return std::make_shared<ColorFillEffect>(jsonObject); } },
    { EFFECT_STRIP_DOUBLE_PALETTE,
        [](const JsonObjectConst& jsonObject)->std::shared_ptr<LEDStripEffect> { return std::make_shared<DoublePaletteEffect>(jsonObject); } },
    { EFFECT_STRIP_FIRE,
        [](const JsonObjectConst& jsonObject)->std::shared_ptr<LEDStripEffect> { return std::make_shared<FireEffect>(jsonObject); } },
    { EFFECT_STRIP_LANTERN,
        [](const JsonObjectConst& jsonObject)->std::shared_ptr<LEDStripEffect> { return std::make_shared<LanternEffect>(jsonObject); } },
    { EFFECT_STRIP_LASER_LINE,
        [](const JsonObjectConst& jsonObject)->std::shared_ptr<LEDStripEffect> { return std::make_shared<LaserLineEffect>(jsonObject); } },
    { EFFECT_STRIP_METEOR,
        [](const JsonObjectConst& jsonObject)->std::shared_ptr<LEDStripEffect> { return std::make_shared<MeteorEffect>(jsonObject); } },
    { EFFECT_STRIP_PALETTE,
        [](const JsonObjectConst& jsonObject)->std::shared_ptr<LEDStripEffect> { return std::make_shared<PaletteEffect>(jsonObject); } },
    { EFFECT_STRIP_PALETTE_FLAME,
        [](const JsonObjectConst& jsonObject)->std::shared_ptr<LEDStripEffect> { return std::make_shared<PaletteFlameEffect>(jsonObject); } },
    { EFFECT_STRIP_RAINBOW_FILL,
        [](const JsonObjectConst& jsonObject)->std::shared_ptr<LEDStripEffect> { return std::make_shared<RainbowFillEffect>(jsonObject); } },
    { EFFECT_STRIP_SIMPLE_RAINBOW_TEST,
        [](const JsonObjectConst& jsonObject)->std::shared_ptr<LEDStripEffect> { return std::make_shared<SimpleRainbowTestEffect>(jsonObject); } },
    { EFFECT_STRIP_STARRY_NIGHT,
        [](const JsonObjectConst& jsonObject)->std::shared_ptr<LEDStripEffect> { return CreateStarryNightEffectFromJSON(jsonObject); } },
    { EFFECT_STRIP_STATUS,
        [](const JsonObjectConst& jsonObject)->std::shared_ptr<LEDStripEffect> { return std::make_shared<StatusEffect>(jsonObject); } },
    { EFFECT_STRIP_TWINKLE,
        [](const JsonObjectConst& jsonObject)->std::shared_ptr<LEDStripEffect> { return std::make_shared<TwinkleEffect>(jsonObject); } },
    { EFFECT_STRIP_SPLASH_LOGO,
        [](const JsonObjectConst& jsonObject)->std::shared_ptr<LEDStripEffect> { return std::make_shared<SplashLogoEffect>(jsonObject); } },

#if ENABLE_AUDIO
    { EFFECT_STRIP_COLOR_BEAT_OVER_RED,
        [](const JsonObjectConst& jsonObject)->std::shared_ptr<LEDStripEffect> { return std::make_shared<ColorBeatOverRed>(jsonObject); } },
    { EFFECT_STRIP_NEW_MOLTEN_GLASS_ON_VIOLET_BKGND,
        [](const JsonObjectConst& jsonObject)->std::shared_ptr<LEDStripEffect> { return std::make_shared<NewMoltenGlassOnVioletBkgnd>(jsonObject); } },
    { EFFECT_STRIP_MUSICAL_PALETTE_FIRE,
        [](const JsonObjectConst& jsonObject)->std::shared_ptr<LEDStripEffect> { return std::make_shared<MusicalPaletteFire>(jsonObject); } },
    { EFFECT_STRIP_SIMPLE_INSULATOR_BEAT2,
        [](const JsonObjectConst& jsonObject)->std::shared_ptr<LEDStripEffect> { return std::make_shared<SimpleInsulatorBeatEffect2>(jsonObject); } },
    { EFFECT_STRIP_SPARKLY_SPINNING_MUSIC,
        [](const JsonObjectConst& jsonObject)->std::shared_ptr<LEDStripEffect> { return std::make_shared<SparklySpinningMusicEffect>(jsonObject); } },
    { EFFECT_MATRIX_INSULATOR_SPECTRUM,
        [](const JsonObjectConst& jsonObject)->std::shared_ptr<LEDStripEffect> { return std::make_shared<InsulatorSpectrumEffect>(jsonObject); } },
    { EFFECT_MATRIX_SPECTRUM_ANALYZER,
        [](const JsonObjectConst& jsonObject)->std::shared_ptr<LEDStripEffect> { return std::make_shared<SpectrumAnalyzerEffect>(jsonObject); } },
    { EFFECT_MATRIX_WAVEFORM,
        [](const JsonObjectConst& jsonObject)->std::shared_ptr<LEDStripEffect> { return std::make_shared<WaveformEffect>(jsonObject); } },
    { EFFECT_MATRIX_GHOST_WAVE,
        [](const JsonObjectConst& jsonObject)->std::shared_ptr<LEDStripEffect> { return std::make_shared<GhostWave>(jsonObject); } },
#endif

#if FAN_SIZE
    { EFFECT_STRIP_FAN_BEAT,
        [](const JsonObjectConst& jsonObject)->std::shared_ptr<LEDStripEffect> { return std::make_shared<FanBeatEffect>(jsonObject); } },
    { EFFECT_STRIP_FIRE_FAN,
        [](const JsonObjectConst& jsonObject)->std::shared_ptr<LEDStripEffect> { return std::make_shared<FireFanEffect>(jsonObject); } },
    { EFFECT_STRIP_PALETTE_REEL,
        [](const JsonObjectConst& jsonObject)->std::shared_ptr<LEDStripEffect> { return std::make_shared<PaletteReelEffect>(jsonObject); } },
    { EFFECT_STRIP_PALETTE_REEL,
        [](const JsonObjectConst& jsonObject)->std::shared_ptr<LEDStripEffect> { return std::make_shared<PaletteReelEffect>(jsonObject); } },
    { EFFECT_STRIP_TAPE_REEL,
        [](const JsonObjectConst& jsonObject)->std::shared_ptr<LEDStripEffect> { return std::make_shared<TapeReelEffect>(jsonObject); } },
#endif

#if USE_MATRIX
    { EFFECT_MATRIX_ALIEN_TEXT,
        [](const JsonObjectConst& jsonObject)->std::shared_ptr<LEDStripEffect> { return std::make_shared<PatternAlienText>(jsonObject); } },
    { EFFECT_MATRIX_BOUNCE,
        [](const JsonObjectConst& jsonObject)->std::shared_ptr<LEDStripEffect> { return std::make_shared<PatternBounce>(jsonObject); } },
    { EFFECT_MATRIX_CIRCUIT,
        [](const JsonObjectConst& jsonObject)->std::shared_ptr<LEDStripEffect> { return std::make_shared<PatternCircuit>(jsonObject); } },
    { EFFECT_MATRIX_CLOCK,
        [](const JsonObjectConst& jsonObject)->std::shared_ptr<LEDStripEffect> { return std::make_shared<PatternClock>(jsonObject); } },
    { EFFECT_MATRIX_CUBE,
        [](const JsonObjectConst& jsonObject)->std::shared_ptr<LEDStripEffect> { return std::make_shared<PatternCube>(jsonObject); } },
    { EFFECT_MATRIX_CURTAIN,
        [](const JsonObjectConst& jsonObject)->std::shared_ptr<LEDStripEffect> { return std::make_shared<PatternCurtain>(jsonObject); } },
    { EFFECT_MATRIX_FLOW_FIELD,
        [](const JsonObjectConst& jsonObject)->std::shared_ptr<LEDStripEffect> { return std::make_shared<PatternFlowField>(jsonObject); } },
    { EFFECT_MATRIX_GRID_LIGHTS,
        [](const JsonObjectConst& jsonObject)->std::shared_ptr<LEDStripEffect> { return std::make_shared<PatternGridLights>(jsonObject); } },
    { EFFECT_MATRIX_INFINITY,
        [](const JsonObjectConst& jsonObject)->std::shared_ptr<LEDStripEffect> { return std::make_shared<PatternInfinity>(jsonObject); } },
    { EFFECT_MATRIX_LIFE,
        [](const JsonObjectConst& jsonObject)->std::shared_ptr<LEDStripEffect> { return std::make_shared<PatternLife>(jsonObject); } },
    { EFFECT_MATRIX_MANDALA,
        [](const JsonObjectConst& jsonObject)->std::shared_ptr<LEDStripEffect> { return std::make_shared<PatternMandala>(jsonObject); } },
    { EFFECT_MATRIX_MUNCH,
        [](const JsonObjectConst& jsonObject)->std::shared_ptr<LEDStripEffect> { return std::make_shared<PatternMunch>(jsonObject); } },
    { EFFECT_MATRIX_PALETTE_SMEAR,
        [](const JsonObjectConst& jsonObject)->std::shared_ptr<LEDStripEffect> { return std::make_shared<PatternPaletteSmear>(jsonObject); } },
    { EFFECT_MATRIX_PINWHEEL,
        [](const JsonObjectConst& jsonObject)->std::shared_ptr<LEDStripEffect> { return std::make_shared<PatternPinwheel>(jsonObject); } },
    { EFFECT_MATRIX_PONG_CLOCK,
        [](const JsonObjectConst& jsonObject)->std::shared_ptr<LEDStripEffect> { return std::make_shared<PatternPongClock>(jsonObject); } },
    { EFFECT_MATRIX_PULSE,
        [](const JsonObjectConst& jsonObject)->std::shared_ptr<LEDStripEffect> { return std::make_shared<PatternPulse>(jsonObject); } },
    { EFFECT_MATRIX_PULSAR,
        [](const JsonObjectConst& jsonObject)->std::shared_ptr<LEDStripEffect> { return std::make_shared<PatternPulsar>(jsonObject); } },
    { EFFECT_MATRIX_QR,
        [](const JsonObjectConst& jsonObject)->std::shared_ptr<LEDStripEffect> { return std::make_shared<PatternQR>(jsonObject); } },
    { EFFECT_MATRIX_RADAR,
        [](const JsonObjectConst& jsonObject)->std::shared_ptr<LEDStripEffect> { return std::make_shared<PatternRadar>(jsonObject); } },
    { EFFECT_MATRIX_ROSE,
        [](const JsonObjectConst& jsonObject)->std::shared_ptr<LEDStripEffect> { return std::make_shared<PatternRose>(jsonObject); } },
    { EFFECT_MATRIX_SERENDIPITY,
        [](const JsonObjectConst& jsonObject)->std::shared_ptr<LEDStripEffect> { return std::make_shared<PatternSerendipity>(jsonObject); } },
    { EFFECT_MATRIX_SPIN,
        [](const JsonObjectConst& jsonObject)->std::shared_ptr<LEDStripEffect> { return std::make_shared<PatternSpin>(jsonObject); } },
    { EFFECT_MATRIX_SPIRO,
        [](const JsonObjectConst& jsonObject)->std::shared_ptr<LEDStripEffect> { return std::make_shared<PatternSpiro>(jsonObject); } },
    { EFFECT_MATRIX_SUBSCRIBERS,
        [](const JsonObjectConst& jsonObject)->std::shared_ptr<LEDStripEffect> { return std::make_shared<PatternSubscribers>(jsonObject); } },
    { EFFECT_MATRIX_SUNBURST,
        [](const JsonObjectConst& jsonObject)->std::shared_ptr<LEDStripEffect> { return std::make_shared<PatternSunburst>(jsonObject); } },
    { EFFECT_MATRIX_SWIRL,
        [](const JsonObjectConst& jsonObject)->std::shared_ptr<LEDStripEffect> { return std::make_shared<PatternSwirl>(jsonObject); } },
    { EFFECT_MATRIX_WAVE,
        [](const JsonObjectConst& jsonObject)->std::shared_ptr<LEDStripEffect> { return std::make_shared<PatternWave>(jsonObject); } },
    { EFFECT_MATRIX_WEATHER,
        [](const JsonObjectConst& jsonObject)->std::shared_ptr<LEDStripEffect> { return std::make_shared<PatternWeather>(jsonObject); } },
#endif
};

std::shared_ptr<LEDStripEffect> CreateEffectFromJSON(const JsonObjectConst& jsonObject)
{
    auto entry = g_JsonEffectFactories.find(jsonObject[PTY_EFFECTNR]);

    return entry != g_JsonEffectFactories.end()
        ? entry->second(jsonObject)
        : nullptr;
}
