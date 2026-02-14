#pragma once

//+--------------------------------------------------------------------------
//
// File:        effectsupport.h
//
// NightDriverStrip - (c) 2023 Plummer's Software LLC.  All Rights Reserved.
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
//    Declarations of different types for the effect initializers included
//    in effects.cpp.
//
// History:     Sep-26-2023         Rbergen     Created for NightDriverStrip
//
//---------------------------------------------------------------------------

#include "globals.h"
#include "colordata.h"

#include <cstdint>

// Palettes used by a number of effects

extern const CRGBPalette16 BlueColors_p;
extern const CRGBPalette16 RedColors_p;
extern const CRGBPalette16 GreenColors_p;
extern const CRGBPalette16 RGBColors_p;
// spectrumBasicColors is in colordata.h
extern const CRGBPalette16 spectrumAltColors;
extern const CRGBPalette16 USAColors_p;
extern const CRGBPalette16 rainbowPalette;

// Defines used by some StarEffect instances

constexpr float kStarEffectProbability = 1.0f;
constexpr float kStarEffectMusicFactor = 1.0f;
