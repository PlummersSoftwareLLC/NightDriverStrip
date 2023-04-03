#pragma once

#include "ledstripeffect.h"

#define EFFECT_STRIP_BOUNCING_BALL                       1
#define EFFECT_STRIP_DOUBLE_PALETTE                      2
#define EFFECT_STRIP_PALETTE_SPIN                        3
#define EFFECT_STRIP_COLOR_CYCLE                         4
#define EFFECT_STRIP_FIRE_FAN                            5
#define EFFECT_STRIP_RING_TEST                           6
#define EFFECT_STRIP_LANTERN                             7
#define EFFECT_STRIP_FIRE                                8
#define EFFECT_STRIP_CLASSIC_FIRE                        9
#define EFFECT_STRIP_SMOOTH_FIRE                        10
#define EFFECT_STRIP_BASE_FIRE                          11
#define EFFECT_STRIP_LASER_LINE                         12
#define EFFECT_STRIP_METEOR                             13
#define EFFECT_STRIP_SIMPLE_RAINBOW                     14
#define EFFECT_STRIP_RAINBOW_TWINKLE                    15
#define EFFECT_STRIP_RAINBOW_FILL                       16
#define EFFECT_STRIP_COLOR_FILL                         17
#define EFFECT_STRIP_STATUS                             18
#define EFFECT_STRIP_TWINKLE                            19
#define EFFECT_STRIP_SIMPLE_COLOR_BEAT                  20
#define EFFECT_STRIP_PALETTE                            21
#define EFFECT_STRIP_COLOR_BEAT_WITH_FLASH              22
#define EFFECT_STRIP_COLOR_BEAT_OVER_RED                23
#define EFFECT_STRIP_MOLTEN_GLASS_ON_VIOLET_BKGND       24
#define EFFECT_STRIP_NEW_MOLTEN_GLASS_ON_VIOLET_BKGND   25
#define EFFECT_STRIP_SPARKLY_SPINNING_MUSIC             26
#define EFFECT_STRIP_MUSICAL_HOT_WHITE_INSULATOR        27
#define EFFECT_STRIP_SNAKE                              28

#define EFFECT_MATRIX_WEATHER                           99

typedef LEDStripEffect* (*JsonEffectFactory)(const JsonObject&);

