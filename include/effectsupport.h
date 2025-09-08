//+--------------------------------------------------------------------------
//
// File:        effectsupport.h
//
// NightDriverStrip - (c) 2023 Plummer's Software LLC.  All Rights
// Reserved.
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
// History:     Sep-26-2023         Rbergen     Created for
// NightDriverStrip
//
//---------------------------------------------------------------------------

#pragma once

#include "globals.h"
#include "hashing.h"

#include <algorithm>
#include <cstdint>
#include <string_view>
#include <type_traits>
#include <utility>
#include <vector>

// Palettes used by a number of effects

const CRGBPalette16 BlueColors_p = {
    CRGB::DarkBlue, CRGB::MediumBlue, CRGB::Blue, CRGB::MediumBlue,
    CRGB::DarkBlue, CRGB::MediumBlue, CRGB::Blue, CRGB::MediumBlue,
    CRGB::DarkBlue, CRGB::MediumBlue, CRGB::Blue, CRGB::MediumBlue,
    CRGB::DarkBlue, CRGB::MediumBlue, CRGB::Blue, CRGB::MediumBlue};

const CRGBPalette16 RedColors_p = {
    CRGB::Red, CRGB::DarkRed, CRGB::DarkRed, CRGB::DarkRed,

    CRGB::Red, CRGB::DarkRed, CRGB::DarkRed, CRGB::DarkRed,

    CRGB::Red, CRGB::DarkRed, CRGB::DarkRed, CRGB::DarkRed,

    CRGB::Red, CRGB::DarkRed, CRGB::DarkRed, CRGB::OrangeRed};

const CRGBPalette16 GreenColors_p = {
    CRGB::Green, CRGB::DarkGreen, CRGB::DarkGreen, CRGB::DarkGreen,

    CRGB::Green, CRGB::DarkGreen, CRGB::DarkGreen, CRGB::DarkGreen,

    CRGB::Green, CRGB::DarkGreen, CRGB::DarkGreen, CRGB::DarkGreen,

    CRGB::Green, CRGB::DarkGreen, CRGB::DarkGreen, CRGB::LimeGreen};

const CRGBPalette16 RGBColors_p = {
    CRGB::Red,   CRGB::Green, CRGB::Blue,  CRGB::Red,
    CRGB::Green, CRGB::Blue,  CRGB::Red,   CRGB::Green,
    CRGB::Blue,  CRGB::Red,   CRGB::Green, CRGB::Blue,
    CRGB::Red,   CRGB::Green, CRGB::Blue,  CRGB::Blue};

const CRGBPalette16 spectrumBasicColors = {
    CRGB(0xFD0E35), // Red
    CRGB(0xFF8833), // Orange
    CRGB(0xFFEB00), // Middle Yellow
    CRGB(0xAFE313), // Inchworm
    CRGB(0x3AA655), // Green
    CRGB(0x8DD9CC), // Middle Blue Green
    CRGB(0x0066FF), // Blue III
    CRGB(0xDB91EF), // Lilac
    CRGB(0xFD0E35), // Red
    CRGB(0xFF8833), // Orange
    CRGB(0xFFEB00), // Middle Yellow
    CRGB(0xAFE313), // Inchworm
    CRGB(0x3AA655), // Green
    CRGB(0x8DD9CC), // Middle Blue Green
    CRGB(0x0066FF), // Blue III
    CRGB(0xDB91EF)  // Lilac
};

const CRGBPalette16 spectrumAltColors = {
    CRGB::Red,         CRGB::OrangeRed, CRGB::Orange, CRGB::Green,
    CRGB::ForestGreen, CRGB::Cyan,      CRGB::Blue,   CRGB::Indigo,
    CRGB::Red,         CRGB::OrangeRed, CRGB::Orange, CRGB::Green,
    CRGB::ForestGreen, CRGB::Cyan,      CRGB::Blue,   CRGB::Indigo,
};

const CRGBPalette16 USAColors_p = {
    CRGB::Blue,  CRGB::Blue, CRGB::Blue,  CRGB::Blue,
    CRGB::Blue,  CRGB::Red,  CRGB::White, CRGB::Red,
    CRGB::White, CRGB::Red,  CRGB::White, CRGB::Red,
    CRGB::White, CRGB::Red,  CRGB::White, CRGB::Red,
};

const CRGBPalette16 rainbowPalette(RainbowColors_p);

// A pointer to the global effect factories.
extern DRAM_ATTR std::unique_ptr<EffectFactories> g_ptrEffectFactories;

// ------------------------------------------------------------
// Support for building stable factory IDs and combining them
// ------------------------------------------------------------

// A type alias for removing const, volatile, and reference from a type.
template <typename T>
using remove_cvref_t = std::remove_cv_t<std::remove_reference_t<T>>;

// Gets the effect ID of a given effect type.
template <typename T> constexpr EffectId effect_id_of_type()
{
    static_assert(std::is_base_of_v<LEDStripEffect, remove_cvref_t<T>>,
                  "Type must derive from EffectWithId<Id>");
    return remove_cvref_t<T>::ID; // compile-time constant
}

// Build a stable 64-bit ID for a factory based on effect type and ctor
// args Note that this is declared as a constexpr function, which means
// all ctor args need to be static types. The fact this code compiles
// confirms that is indeed the case.
template <typename TEffect, typename... Args>
constexpr FactoryId factory_id_of_instance(const Args &...args)
{
    FactoryId h = fnv1a::hash<FactoryId>("effect");
    h           = fnv1a::hash(effect_id_of_type<TEffect>(), h);
    h           = fnv1a::hash_pack(h, args...);
    return h;
}

// Adds a default and JSON effect factory for a specific effect number and
// type. All parameters beyond effectNumber and effect type are forwarded
// to the default constructor.
template <typename TEffect, typename... Args>
inline EffectFactories::NumberedFactory &AddEffect(
    EffectFactories &factories, Args &&...args)
{
    return factories.AddEffect(
        effect_id_of_type<TEffect>(),
        [=]() -> std::shared_ptr<LEDStripEffect> {
            return make_shared_psram<TEffect>(args...);
        },
        [](const JsonObjectConst &jsonObject)
            -> std::shared_ptr<LEDStripEffect> {
            return make_shared_psram<TEffect>(jsonObject);
        },
        factory_id_of_instance<TEffect>(args...));
}

// Fold-expression helper to register many at once with brief syntax:
//   RegisterAll(*g_ptrEffectFactories,
//       Effect<idStripPalette, MyEffect>(args...),
//       Disabled(Effect<idStripColorFill, OtherEffect>(args...)));
template <typename... Adders>
inline void RegisterAll(EffectFactories &factories, Adders &&...adders)
{
    (static_cast<void>(adders(factories)), ...);
}

// Builder for a single effect entry used with RegisterAll
template <typename TEffect, typename... Args>
inline auto Effect(Args &&...args)
{
    return [=](EffectFactories &factories)
               -> EffectFactories::NumberedFactory & {
        return AddEffect<TEffect>(factories, args...);
    };
}

// Decorator to mark an entry disabled-on-load when using RegisterAll
template <typename F> inline auto Disabled(F adder)
{
    return [=](EffectFactories &factories)
               -> EffectFactories::NumberedFactory & {
        auto &nf        = adder(factories);
        nf.LoadDisabled = true;
        return nf;
    };
}

// Defines used by some StarEffect instances

constexpr float kStarEffectProbability = 1.0f;
constexpr float kStarEffectMusicFactor = 1.0f;
