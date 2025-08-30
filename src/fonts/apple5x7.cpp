//+--------------------------------------------------------------------------
//
// File:        apple5x7.cpp
//
// NightDriverStrip - (c) 2025 Plummer's Software LLC.  All Rights Reserved.
//
// Adafruit_GFX-compatible Apple 5x7 font definition.
// This TU force-includes Adafruit gfxfont/GFX to lock the font types and
// avoid LovyanGFX/M5 alias collisions.
//---------------------------------------------------------------------------

#include <gfxfont.h>
#include <Adafruit_GFX.h>
#include "fonts/apple5x7.h"

// Adafruit GFX font Apple5x7 size: 5x7

const uint8_t Apple5x7Bitmaps[] PROGMEM = {
  0xF4,	// exclam
  0xB6,0x80,	// quotedbl
  0x57,0xD5,0xF5,0x00,	// numbersign
  0x75,0x1C,0x57,0x00,	// dollar
  0x89,0x24,0x91,	// percent
  0x4A,0x4A,0x50,	// ampersand
  0xE0,	// quotesingle
  0x6A,0x90,	// parenleft
  0x95,0x60,	// parenright
  0xAB,0xAA,	// asterisk
  0x21,0x3E,0x42,0x00,	// plus
  0x6A,0x00,	// comma
  0xF0,	// hyphen
  0xF0,	// period
  0x12,0x48,	// slash
  0x56,0xDA,0x80,	// zero
  0x59,0x25,0xC0,	// one
  0x69,0x12,0x4F,	// two
  0xF1,0x61,0x96,	// three
  0x26,0xAF,0x22,	// four
  0xF8,0xE1,0x96,	// five
  0x68,0xE9,0x96,	// six
  0xF1,0x22,0x44,	// seven
  0x69,0x69,0x96,	// eight
  0x69,0x97,0x16,	// nine
  0xF3,0xC0,	// colon
  0x6C,0x35,0x00,	// semicolon
  0x2A,0x22,	// less
  0xF0,0xF0,	// equal
  0x88,0xA8,	// greater
  0x54,0xA0,0x80,	// question
  0x69,0xBB,0x86,	// at
  0x69,0x9F,0x99,	// A
  0xE9,0xE9,0x9E,	// B
  0x69,0x88,0x96,	// C
  0xE9,0x99,0x9E,	// D
  0xF8,0xE8,0x8F,	// E
  0xF8,0xE8,0x88,	// F
  0x69,0x8B,0x97,	// G
  0x99,0xF9,0x99,	// H
  0xE9,0x25,0xC0,	// I
  0x11,0x11,0x96,	// J
  0x9A,0xCC,0xA9,	// K
  0x88,0x88,0x8F,	// L
  0x9F,0xF9,0x99,	// M
  0x9D,0xDB,0xB9,	// N
  0x69,0x99,0x96,	// O
  0xE9,0x9E,0x88,	// P
  0x69,0x99,0xD6,0x10,	// Q
  0xE9,0x9E,0xA9,	// R
  0x69,0x42,0x96,	// S
  0xE9,0x24,0x80,	// T
  0x99,0x99,0x96,	// U
  0x99,0x99,0x66,	// V
  0x99,0x9F,0xF9,	// W
  0x99,0x66,0x99,	// X
  0xB6,0xA4,0x80,	// Y
  0xF1,0x24,0x8F,	// Z
  0xF2,0x49,0xC0,	// bracketleft
  0x84,0x21,	// backslash
  0xE4,0x93,0xC0,	// bracketright
  0x54,	// asciicircum
  0xF0,	// underscore
  0x90,	// grave
  0x79,0xB5,	// a
  0x88,0xE9,0x9E,	// b
  0x72,0x30,	// c
  0x11,0x79,0x97,	// d
  0x6B,0xC6,	// e
  0x25,0x4E,0x44,	// f
  0x79,0x68,0x70,	// g
  0x88,0xE9,0x99,	// h
  0x43,0x25,0xC0,	// i
  0x20,0x93,0x50,	// j
  0x88,0xAC,0xA9,	// k
  0xC9,0x25,0xC0,	// l
  0xAF,0x99,	// m
  0xE9,0x99,	// n
  0x69,0x96,	// o
  0xE9,0x9E,0x80,	// p
  0x79,0x97,0x10,	// q
  0xE9,0x88,	// r
  0x7C,0x3E,	// s
  0x44,0xE4,0x43,	// t
  0x99,0x97,	// u
  0xB6,0xA0,	// v
  0x99,0xFF,	// w
  0x96,0x69,	// x
  0x99,0x52,0x40,	// y
  0xF2,0x4F,	// z
  0x2B,0x24,0x40,	// braceleft
  0xFC,	// bar
  0x89,0xA5,0x00,	// braceright
  0x5A	// asciitilde
};

const GFXglyph Apple5x7Glyphs[] PROGMEM = {
  { 0, 0, 0, 5, 0, 0 },	// space
  { 0, 1, 6, 5, 2, -6 },	// exclam
  { 1, 3, 3, 5, 1, -6 },	// quotedbl
  { 3, 5, 5, 5, 0, -5 },	// numbersign
  { 7, 5, 5, 5, 0, -5 },	// dollar
  { 11, 4, 6, 5, 0, -6 },	// percent
  { 14, 4, 5, 5, 0, -5 },	// ampersand
  { 17, 1, 3, 5, 2, -6 },	// quotesingle
  { 18, 2, 6, 5, 1, -6 },	// parenleft
  { 20, 2, 6, 5, 1, -6 },	// parenright
  { 22, 3, 5, 5, 1, -5 },	// asterisk
  { 24, 5, 5, 5, 0, -5 },	// plus
  { 28, 3, 3, 5, 1, -2 },	// comma
  { 30, 4, 1, 5, 0, -3 },	// hyphen
  { 31, 2, 2, 5, 1, -2 },	// period
  { 32, 4, 4, 5, 0, -5 },	// slash
  { 34, 3, 6, 5, 1, -6 },	// zero
  { 37, 3, 6, 5, 1, -6 },	// one
  { 40, 4, 6, 5, 0, -6 },	// two
  { 43, 4, 6, 5, 0, -6 },	// three
  { 46, 4, 6, 5, 0, -6 },	// four
  { 49, 4, 6, 5, 0, -6 },	// five
  { 52, 4, 6, 5, 0, -6 },	// six
  { 55, 4, 6, 5, 0, -6 },	// seven
  { 58, 4, 6, 5, 0, -6 },	// eight
  { 61, 4, 6, 5, 0, -6 },	// nine
  { 64, 2, 5, 5, 1, -5 },	// colon
  { 66, 3, 6, 5, 0, -5 },	// semicolon
  { 69, 3, 5, 5, 1, -5 },	// less
  { 71, 4, 3, 5, 0, -4 },	// equal
  { 73, 3, 5, 5, 1, -5 },	// greater
  { 75, 3, 6, 5, 1, -6 },	// question
  { 78, 4, 6, 5, 0, -6 },	// at
  { 81, 4, 6, 5, 0, -6 },	// A
  { 84, 4, 6, 5, 0, -6 },	// B
  { 87, 4, 6, 5, 0, -6 },	// C
  { 90, 4, 6, 5, 0, -6 },	// D
  { 93, 4, 6, 5, 0, -6 },	// E
  { 96, 4, 6, 5, 0, -6 },	// F
  { 99, 4, 6, 5, 0, -6 },	// G
  { 102, 4, 6, 5, 0, -6 },	// H
  { 105, 3, 6, 5, 1, -6 },	// I
  { 108, 4, 6, 5, 0, -6 },	// J
  { 111, 4, 6, 5, 0, -6 },	// K
  { 114, 4, 6, 5, 0, -6 },	// L
  { 117, 4, 6, 5, 0, -6 },	// M
  { 120, 4, 6, 5, 0, -6 },	// N
  { 123, 4, 6, 5, 0, -6 },	// O
  { 126, 4, 6, 5, 0, -6 },	// P
  { 129, 4, 7, 5, 0, -6 },	// Q
  { 133, 4, 6, 5, 0, -6 },	// R
  { 136, 4, 6, 5, 0, -6 },	// S
  { 139, 3, 6, 5, 1, -6 },	// T
  { 142, 4, 6, 5, 0, -6 },	// U
  { 145, 4, 6, 5, 0, -6 },	// V
  { 148, 4, 6, 5, 0, -6 },	// W
  { 151, 4, 6, 5, 0, -6 },	// X
  { 154, 3, 6, 5, 1, -6 },	// Y
  { 157, 4, 6, 5, 0, -6 },	// Z
  { 160, 3, 6, 5, 1, -6 },	// bracketleft
  { 163, 4, 4, 5, 0, -5 },	// backslash
  { 165, 3, 6, 5, 1, -6 },	// bracketright
  { 168, 3, 2, 5, 1, -6 },	// asciicircum
  { 169, 4, 1, 5, 0, -1 },	// underscore
  { 170, 2, 2, 5, 1, -6 },	// grave
  { 171, 4, 4, 5, 0, -4 },	// a
  { 173, 4, 6, 5, 0, -6 },	// b
  { 176, 3, 4, 5, 0, -4 },	// c
  { 178, 4, 6, 5, 0, -6 },	// d
  { 181, 4, 4, 5, 0, -4 },	// e
  { 183, 4, 6, 5, 0, -6 },	// f
  { 186, 4, 5, 5, 0, -4 },	// g
  { 189, 4, 6, 5, 0, -6 },	// h
  { 192, 3, 6, 5, 1, -6 },	// i
  { 195, 3, 7, 5, 1, -6 },	// j
  { 198, 4, 6, 5, 0, -6 },	// k
  { 201, 3, 6, 5, 1, -6 },	// l
  { 204, 4, 4, 5, 0, -4 },	// m
  { 206, 4, 4, 5, 0, -4 },	// n
  { 208, 4, 4, 5, 0, -4 },	// o
  { 210, 4, 5, 5, 0, -4 },	// p
  { 213, 4, 5, 5, 0, -4 },	// q
  { 216, 4, 4, 5, 0, -4 },	// r
  { 218, 4, 4, 5, 0, -4 },	// s
  { 220, 4, 6, 5, 0, -6 },	// t
  { 223, 4, 4, 5, 0, -4 },	// u
  { 225, 3, 4, 5, 1, -4 },	// v
  { 227, 4, 4, 5, 0, -4 },	// w
  { 229, 4, 4, 5, 0, -4 },	// x
  { 231, 4, 5, 5, 0, -4 },	// y
  { 234, 4, 4, 5, 0, -4 },	// z
  { 236, 3, 6, 5, 1, -6 },	// braceleft
  { 239, 1, 6, 5, 2, -6 },	// bar
  { 240, 3, 6, 5, 1, -6 },	// braceright
  { 243, 4, 2, 5, 0, -6 }	// asciitilde
};

const GFXfont Apple5x7 PROGMEM = {
 (uint8_t *)Apple5x7Bitmaps,
 (GFXglyph *)Apple5x7Glyphs,
 32, 126, 7};


