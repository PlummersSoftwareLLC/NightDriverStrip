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

#if USE_MATRIX
    #include "ledmatrixgfx.h"
    #include "effects/strip/misceffects.h"
    #include "effects/matrix/PatternBalls.h"

    #include "effects/matrix/PatternSMStrobeDiffusion.h"
    #include "effects/matrix/PatternSMOneRing.h"
    #include "effects/matrix/PatternSMLumenjerPalette.h"
    #include "effects/matrix/PatternSMSquaresAndDots.h"
    #include "effects/matrix/PatternSMTraffic.h"
    #include "effects/matrix/PatternSM2DDPR.h"

    #include "effects/matrix/PatternSMFireworks.h"
    #include "effects/matrix/PatternSMStarDeep.h"
    #include "effects/matrix/PatternSMMagma.h"
    #include "effects/matrix/PatternSMAmberRain.h"
    #include "effects/matrix/PatternSMFire2012.h"
    #include "effects/matrix/PatternSMFire2021.h"
//    #include "effects/matrix/PatternSMWisp.h"
    #include "effects/matrix/PatternSMNoise.h"
    #include "effects/matrix/PatternSMFPicasso3in1.h"
    #include "effects/matrix/PatternSMSnakes.h"
    #include "effects/matrix/PatternSMParticles.h"
    #include "effects/matrix/PatternSMRainbowFlow.h"
    #include "effects/matrix/PatternSMPastelFlutter.h"
    #include "effects/matrix/PatternSMSand.h"
    #include "effects/matrix/PatternSMSpiro.h"
    #include "effects/matrix/PatternSMLightning.h"
    #include "effects/matrix/PatternSMEyeTunnel.h"
    #include "effects/matrix/PatternSMPSPCloud.h"
    #include "effects/matrix/PatternSMColorPopcorn.h"
    #include "effects/matrix/PatternSMSpiroPulse.h"
    #include "effects/matrix/PatternSMTwist.h"
    #include "effects/matrix/PatternSMTwister.h"
    #include "effects/matrix/PatternSMMetaBalls.h"
    #include "effects/matrix/PatternSMFlying.h"
    #include "effects/matrix/PatternSMSinDots.h"
    #include "effects/matrix/PatternSMSunRadiation.h"
    #include "effects/matrix/PatternSMPatternTrick.h"
    #include "effects/matrix/PatternSMGravityBalls.h"
    #include "effects/matrix/PatternSMGoogleNexus.h"
    #include "effects/matrix/PatternSMHolidayLights.h"
    #include "effects/matrix/PatternSMStarshipTroopers.h"
    #include "effects/matrix/PatternSMGamma.h"
    // A Small block of 3D-ish effects using Particle systems or Boids.
    #include "effects/matrix/PatternSMBubbles.h" // Boids
    #include "effects/matrix/PatternSMFlocking.h" // Boids
    #include "effects/matrix/PatternSMFlocking.h"
    #include "effects/matrix/PatternSMFlowFields.h"
    #include "effects/matrix/PatternSMBlurringColors.h"
    #include "effects/matrix/PatternSMSupernova.h"
    #include "effects/matrix/PatternSMBoidExplosion.h"
    #include "effects/matrix/PatternSMTixyLand.h" // Dozens of simple effects
    #include "effects/matrix/PatternSMXorCircles.h"
    #include "effects/matrix/PatternSMWalkingMachine.h"
    #include "effects/matrix/PatternSMHypnosis.h"
    #include "effects/matrix/PatternSMRainbowTunnel.h"
    #include "effects/matrix/PatternSMRadialWave.h"
    #include "effects/matrix/PatternSMMirage.h"
    #include "effects/matrix/PatternSMMaze2.h"
    #include "effects/matrix/PatternSMRadialFire.h"
    #include "effects/matrix/PatternSMPrismata.h"
    #include "effects/matrix/PatternSMAurora.h"
    #include "effects/matrix/PatternSMSmoke.h"
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
#endif  // USE_MATRIX


#ifdef USESTRIP
    #include "ledstripgfx.h"
#endif
