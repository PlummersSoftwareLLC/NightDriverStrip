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

using EffectId = uint32_t;
using FactoryId = uint64_t;

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
