//+--------------------------------------------------------------------------
//
// File:        colordata.cpp
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
//    Palettes and other color table defnitions that need to be in a CPP file
//
// History:     May-11-2021         Davepl      Commented
//
//---------------------------------------------------------------------------

#include "globals.h"

DEFINE_GRADIENT_PALETTE( vu_gpGreen ) 
{
      0,     0,   4,   0,   // near black green
     64,     0, 255,   0,   // green
    128,   255, 255,   0,   // yellow
    192,   255,   0,   0,   // red
    255,   255,   0,   0    // red
};
const CRGBPalette16 vuPaletteGreen = vu_gpGreen;

const CRGBPalette16 golden(CRGB::Gold);

DEFINE_GRADIENT_PALETTE( gpSeahawks ) 
{
    0,       0,     0,   4,      
    64,      3,    38,  58,      
   128,      0,    21,  50,      
   192,     78,   167,   1,      
   255,     54,    87, 140,      
};
const CRGBPalette16 vuPaletteSeahawks = gpSeahawks;

DEFINE_GRADIENT_PALETTE( vu_gpBlue ) 
{
      0,     0,   0,   4,   // near black green
     64,     0,   0, 255,   // blue
    128,     0, 255,   0,   // green
    192,   255,   0,   0,   // red
    255,   255,   0,   0    // red
};
const CRGBPalette16 vuPaletteBlue = vu_gpBlue;

DEFINE_GRADIENT_PALETTE(bluesky_gp)
{
    0, 0, 0, 64,          // black
    64, 0, 0, 128,       // blue
    96, 0, 64, 255,    // Cyan blue
    128, 255, 255, 255,      // white
    160, 0, 64, 255, 
    192, 0, 0, 128,    // dark blue
    255, 0, 0, 64
}; // dark blue
const CRGBPalette16 bluesky_pal = bluesky_gp;

DEFINE_GRADIENT_PALETTE(redorange_gp)
{
    0,    128,   0, 0,  
    64,   192,   0, 0,  
    96,   255,   64, 0,  
    128,  255,  255, 0,  
    160,  192,   64, 0,
    192,  128,    0, 0,  
    255,   64,    0, 0
}; 
const CRGBPalette16 redorange_pal = redorange_gp;


// For LEDMatrixGFX::from16Bit color conversions
//
// These tables can't go in the .H file so we have this .CPP file for them instead

const uint8_t GFXBase::gamma5[] =
{
    0x00, 0x01, 0x02, 0x03, 0x05, 0x07, 0x09, 0x0b,
    0x0e, 0x11, 0x14, 0x18, 0x1d, 0x22, 0x28, 0x2e,
    0x36, 0x3d, 0x46, 0x4f, 0x59, 0x64, 0x6f, 0x7c,
    0x89, 0x97, 0xa6, 0xb6, 0xc7, 0xd9, 0xeb, 0xff
};

const uint8_t GFXBase::gamma6[] =
{
    0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x08,
    0x09, 0x0a, 0x0b, 0x0d, 0x0e, 0x10, 0x12, 0x13,
    0x15, 0x17, 0x19, 0x1b, 0x1d, 0x20, 0x22, 0x25,
    0x27, 0x2a, 0x2d, 0x30, 0x33, 0x37, 0x3a, 0x3e,
    0x41, 0x45, 0x49, 0x4d, 0x52, 0x56, 0x5b, 0x5f,
    0x64, 0x69, 0x6e, 0x74, 0x79, 0x7f, 0x85, 0x8b,
    0x91, 0x97, 0x9d, 0xa4, 0xab, 0xb2, 0xb9, 0xc0,
    0xc7, 0xcf, 0xd6, 0xde, 0xe6, 0xee, 0xf7, 0xff
};

extern const TProgmemRGBPalette16 BlueHeatColors_p FL_PROGMEM =
{
    0x000000,
    0x000033, 0x000066, 0x000099, 0x0000CC, 0x0000FF,
    0x0033FF, 0x0066FF, 0x0099FF, 0x00CCFF, 0x00FFFF,
    0x33FFFF, 0x66FFFF, 0x99FFFF, 0xCCFFFF, 0xFFFFFF
};

extern const TProgmemRGBPalette16 GreenHeatColors_p FL_PROGMEM =
{
    0x000000,
    0x003300, 0x006600, 0x009900, 0x00CC00, 0x00FF00,
    0x33FF00, 0x66FF00, 0x99FF00, 0xCCFF00, 0xFFFF00,
    0xFFFF33, 0xFFFF66, 0xFFFF99, 0xFFFFCC, 0xFFFFFF
};

// HeatColors2_p
//
// A variant of HeatColors_p (built into FastLED) that has
// bright blue at the top of the color heat ramp.

extern const TProgmemRGBPalette16 HeatColors2_p FL_PROGMEM =
{
    0x000000,
    0x330000, 0x660000, 0x990000, 0xCC0000, 0xFF0000,
    0xFF3300, 0xFF6600, 0xFF9900, 0xFFCC00, 0xFFFF00,
    0xFFFF33, 0xFFFF66, 0xFFFF99, 0xFFFFCC, 0x0000FF
};