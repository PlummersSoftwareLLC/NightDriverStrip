//+--------------------------------------------------------------------------
//
// File:        effectdependencies.h
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
//    Effective effect includes based on global.h
//
// History:     Apr-05-2023         Rbergen      Created for NightDriverStrip
//
//---------------------------------------------------------------------------

#pragma once

#include "globals.h"

#include "effects/strip/fireeffect.h"          // fire effects
#include "effects/strip/paletteeffect.h"       // palette effects
#include "effects/strip/doublepaletteeffect.h" // double palette effect
#include "effects/strip/meteoreffect.h"        // meteor blend effect
#include "effects/strip/stareffect.h"          // star effects
#include "effects/strip/bouncingballeffect.h"  // bouincing ball effectsenable+
#include "effects/strip/tempeffect.h"
#include "effects/strip/stareffect.h"
#include "effects/strip/laserline.h"
#include "effects/matrix/PatternClock.h"       // No matrix dependencies

#if ENABLE_AUDIO
    #include "effects/matrix/spectrumeffects.h"    // Musis spectrum effects
    #include "effects/strip/musiceffect.h"         // Music based effects
#endif

#if FAN_SIZE
    #include "effects/strip/faneffects.h" // Fan-based effects
#endif

//
// Externals
//

#define BOOGER 0
#if USE_MATRIX
    #include "ledmatrixgfx.h"
#if BOOGER
    #include "effects/strip/misceffects.h"
    #include "effects/matrix/PatternBalls.h"
    #include "effects/matrix/PatternSMStrobeDiffusion.h"
    #include "effects/matrix/PatternSMOneRing.h"
    #include "effects/matrix/PatternSMLumenjerPalette.h"
    #include "effects/matrix/PatternSMSquaresAndDots.h"
    #include "effects/matrix/PatternSMTraffic.h"
    #include "effects/matrix/PatternSM2DDPR.h"
    #include "effects/matrix/PatternSMSinusoidSin16.h"
    #include "effects/matrix/PatternSMFireworks.h"
    #include "effects/matrix/PatternSMStarDeep.h"
    #include "effects/matrix/PatternSMMagma.h"
    #include "effects/matrix/PatternSMAmberRain.h"
    #include "effects/matrix/PatternSMFire2012.h"
    #include "effects/matrix/PatternSMFire2021.h"
    #include "effects/matrix/PatternSMWisp.h"
    #include "effects/matrix/PatternSMNoise.h"
    #include "effects/matrix/PatternSMFPicasso3in1.h"
    #include "effects/matrix/PatternSMSnakes.h"
    #include "effects/matrix/PatternSMParticles.h"
#endif
    #include "effects/matrix/PatternSMRainbowFlow.h"
    #include "effects/matrix/PatternSMPastelFlutter.h"
    #include "effects/matrix/PatternSMSand.h"
#if BOOGER
    #include "effects/matrix/PatternSMRainbowSwirl.h"
    #include "effects/matrix/PatternSerendipity.h"
    #include "effects/matrix/PatternSwirl.h"
    #include "effects/matrix/PatternPulse.h"
    #include "effects/matrix/PatternWave.h"
    #include "effects/matrix/PatternMaze.h"
    #include "effects/matrix/PatternLife.h"
    #include "effects/matrix/PatternSpiro.h"
    #include "effects/matrix/PatternCube.h"
    #include "effects/matrix/PatternCircuit.h"
#if ENABLE_WIFI
    #include "effects/matrix/PatternSubscribers.h"
#endif
    #include "effects/matrix/PatternAlienText.h"
    #include "effects/matrix/PatternRadar.h"
    #include "effects/matrix/PatternPongClock.h"
    #include "effects/matrix/PatternBounce.h"
    #include "effects/matrix/PatternMandala.h"
    #include "effects/matrix/PatternSpin.h"
    #include "effects/matrix/PatternFlowField.h"
    #include "effects/matrix/PatternMisc.h"
    #include "effects/matrix/PatternNoiseSmearing.h"
    #include "effects/matrix/PatternQR.h"
#if ENABLE_WIFI
    #include "effects/matrix/PatternWeather.h"
#endif
#endif // BOOGER
#endif

#ifdef USESTRIP
    #include "ledstripgfx.h"
#endif
