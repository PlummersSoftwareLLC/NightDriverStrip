//+--------------------------------------------------------------------------
//
// File:        ledstripeffect.cpp
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
//    Contains things in LEDStripEffect that C++ won't allow in a header, like
//    initializers of static member variables.
//
// History:     Jun-10-2023         Rbergen      Created for NightDriverStrip
//
//---------------------------------------------------------------------------

#include "globals.h"
#include "types.h"
#include "ledstripeffect.h"

std::vector<SettingSpec, psram_allocator<SettingSpec>> LEDStripEffect::_baseSettingSpecs = {};