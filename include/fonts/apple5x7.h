//+--------------------------------------------------------------------------
//
// File:        apple5x7.h
//
// NightDriverStrip - (c) 2025 Plummer's Software LLC.  All Rights Reserved.
//
// Adafruit_GFX-compatible Apple 5x7 font declaration.
// This header explicitly includes Adafruit's gfxfont types to avoid conflicts
// with M5/LGFX font types in other translation units.
//---------------------------------------------------------------------------

#pragma once

#include <Arduino.h>     // For PROGMEM
#include <gfxfont.h>     // Ensures ::GFXfont and ::GFXglyph are Adafruit types

#ifdef __cplusplus
extern "C" {
#endif

extern const ::GFXfont Apple5x7 PROGMEM;

#ifdef __cplusplus
}
#endif

