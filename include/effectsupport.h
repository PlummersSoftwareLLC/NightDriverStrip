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

#pragma once

#include "globals.h"
#include <type_traits>
#include <string_view>
#include <cstdint>
#include <algorithm>
#include <vector>
#include <utility>
// Needed for StarryNightEffect used by helpers below
#include "effects/strip/stareffect.h"
#include <type_traits>

// Palettes used by a number of effects

const CRGBPalette16 BlueColors_p =
{
    CRGB::DarkBlue,
    CRGB::MediumBlue,
    CRGB::Blue,
    CRGB::MediumBlue,
    CRGB::DarkBlue,
    CRGB::MediumBlue,
    CRGB::Blue,
    CRGB::MediumBlue,
    CRGB::DarkBlue,
    CRGB::MediumBlue,
    CRGB::Blue,
    CRGB::MediumBlue,
    CRGB::DarkBlue,
    CRGB::MediumBlue,
    CRGB::Blue,
    CRGB::MediumBlue
};

const CRGBPalette16 RedColors_p =
{
    CRGB::Red,
    CRGB::DarkRed,
    CRGB::DarkRed,
    CRGB::DarkRed,

    CRGB::Red,
    CRGB::DarkRed,
    CRGB::DarkRed,
    CRGB::DarkRed,

    CRGB::Red,
    CRGB::DarkRed,
    CRGB::DarkRed,
    CRGB::DarkRed,

    CRGB::Red,
    CRGB::DarkRed,
    CRGB::DarkRed,
    CRGB::OrangeRed
};

const CRGBPalette16 GreenColors_p =
{
    CRGB::Green,
    CRGB::DarkGreen,
    CRGB::DarkGreen,
    CRGB::DarkGreen,

    CRGB::Green,
    CRGB::DarkGreen,
    CRGB::DarkGreen,
    CRGB::DarkGreen,

    CRGB::Green,
    CRGB::DarkGreen,
    CRGB::DarkGreen,
    CRGB::DarkGreen,

    CRGB::Green,
    CRGB::DarkGreen,
    CRGB::DarkGreen,
    CRGB::LimeGreen
};

const CRGBPalette16 RGBColors_p =
{
    CRGB::Red,
    CRGB::Green,
    CRGB::Blue,
    CRGB::Red,
    CRGB::Green,
    CRGB::Blue,
    CRGB::Red,
    CRGB::Green,
    CRGB::Blue,
    CRGB::Red,
    CRGB::Green,
    CRGB::Blue,
    CRGB::Red,
    CRGB::Green,
    CRGB::Blue,
    CRGB::Blue
};

const CRGBPalette16 spectrumBasicColors =
{
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

const CRGBPalette16 spectrumAltColors =
{
    CRGB::Red,
    CRGB::OrangeRed,
    CRGB::Orange,
    CRGB::Green,
    CRGB::ForestGreen,
    CRGB::Cyan,
    CRGB::Blue,
    CRGB::Indigo,
    CRGB::Red,
    CRGB::OrangeRed,
    CRGB::Orange,
    CRGB::Green,
    CRGB::ForestGreen,
    CRGB::Cyan,
    CRGB::Blue,
    CRGB::Indigo,
};

const CRGBPalette16 USAColors_p =
{
    CRGB::Blue,
    CRGB::Blue,
    CRGB::Blue,
    CRGB::Blue,
    CRGB::Blue,
    CRGB::Red,
    CRGB::White,
    CRGB::Red,
    CRGB::White,
    CRGB::Red,
    CRGB::White,
    CRGB::Red,
    CRGB::White,
    CRGB::Red,
    CRGB::White,
    CRGB::Red,
};

const CRGBPalette16 rainbowPalette(RainbowColors_p);

extern DRAM_ATTR std::unique_ptr<EffectFactories> g_ptrEffectFactories;

// ------------------------------------------------------------
// Support for building stable factory IDs and combining them
// ------------------------------------------------------------

namespace
{
    // Map star template parameter to a stable integer id used for hashing
    template<typename TStar>
    struct StarTypeId;

    template<>
    struct StarTypeId<Star> { static constexpr int value = idStar; };

    template<>
    struct StarTypeId<BubblyStar> { static constexpr int value = idStarBubbly; };

    template<>
    struct StarTypeId<HotWhiteStar> { static constexpr int value = idStarHotWhite; };

    template<>
    struct StarTypeId<LongLifeSparkleStar> { static constexpr int value = idStarLongLifeSparkle; };

    #if ENABLE_AUDIO
    template<>
    struct StarTypeId<MusicStar> { static constexpr int value = idStarMusic; };
    #endif

    template<>
    struct StarTypeId<QuietStar> { static constexpr int value = idStarQuiet; };
}

// Actual hash support class
class EffectFactoryIdSupport
{
  public:
    // Build a stable 64-bit ID for a factory based on effect type and ctor args
    template<typename TEffect, typename... Args>
    static inline uint64_t MakeFactoryId(const Args&... args)
    {
        static_assert(std::is_enum_v<decltype(TEffect::kId)>, "TEffect must have static constexpr kId enum");
        uint64_t h = FNV_OFFSET;
        hash_append(h, std::string_view{"effect"});
        hash_append(h, TEffect::kId);
        hash_pack(h, args...);
        return h;
    }

    // Build a stable 64-bit ID for StarryNight factories (includes star type)
    template<typename TStar, typename... Args>
    static inline uint64_t MakeStarryFactoryId(int starryEffectNumber, const Args&... args)
    {
        uint64_t h = FNV_OFFSET;
        hash_append(h, std::string_view{"starry"});
        hash_append(h, starryEffectNumber); // e.g., idStripStarryNight
        hash_append(h, static_cast<int>(StarTypeId<TStar>::value));
        hash_pack(h, args...);
        return h;
    }

    // Order-sensitive hash of a list of 64-bit ids
    static inline uint64_t Hash(std::vector<uint64_t>&& ids)
    {
        auto bytes = reinterpret_cast<const unsigned char*>(ids.data());
        const size_t len = ids.size() * sizeof(uint64_t);
        return fnv1a64_bytes(bytes, len, FNV_OFFSET);
    }

    static inline auto HashToString(uint64_t hash)
    {
        return String(hash, 16);
    }

    static inline auto HashString(std::vector<uint64_t>&& ids)
    {
        return HashToString(Hash(std::move(ids)));
    }

  private:
    // FNV-1a 64-bit constants
    static constexpr uint64_t FNV_OFFSET = 1469598103934665603ull;
    static constexpr uint64_t FNV_PRIME  = 1099511628211ull;

    // Computes a 64-bit FNV-1a hash over a raw byte sequence.
    //
    // FNV-1a iterates over the input one byte at a time. For each byte, it:
    // 1) XORs the current hash with the byte, then
    // 2) Multiplies the result by the FNV prime, with 64-bit wraparound (mod 2^64).
    // Starting from an offset basis (seed), this yields a fast, well-distributed,
    // non-cryptographic hash suitable for hash tables and identifiers.
    //
    // Note: Requires FNV_OFFSET (offset basis) and FNV_PRIME (prime multiplier)
    // to be defined as the standard 64-bit FNV-1a constants. The algorithm
    // is byte-oriented and endianness-agnostic. This is not suitable for
    // cryptographic purposes.
    static inline uint64_t fnv1a64_bytes(const void* data, size_t len, uint64_t seed = FNV_OFFSET)
    {
        uint64_t h = seed;
        const auto* p = static_cast<const unsigned char*>(data);

        for (size_t i = 0; i < len; ++i)
        {
            h ^= p[i];
            h *= FNV_PRIME;
        }

        return h;
    }

    // Append a string_view's bytes to the running FNV-1a hash 'h'.
    static inline void hash_append(uint64_t& h, std::string_view sv)
    {
        h = fnv1a64_bytes(sv.data(), sv.size(), h);
    }

    template<typename T>
    // Append any arithmetic value (integers/floats/bool) by value bytes.
    static inline std::enable_if_t<std::is_arithmetic_v<T>, void>
    hash_append(uint64_t& h, T v)
    {
        h = fnv1a64_bytes(&v, sizeof(v), h);
    }

    template<typename T>
    // Append an enum by hashing its underlying integral value.
    static inline std::enable_if_t<std::is_enum_v<T>, void>
    hash_append(uint64_t& h, T v)
    {
        using U = std::underlying_type_t<T>;
        U u = static_cast<U>(v);

        h = fnv1a64_bytes(&u, sizeof(u), h);
    }

    // Append a C-string; null is treated as an empty string.
    static inline void hash_append(uint64_t& h, const char* s)
    {
        if (s)
            hash_append(h, std::string_view{s});
    }

    template<size_t N>
    // Append a fixed-size char array, excluding the trailing NUL.
    static inline void hash_append(uint64_t& h, const char (&s)[N])
    {
        // Exclude trailing NUL
        hash_append(h, std::string_view{s, N ? (N - 1) : 0});
    }

    // Arduino String
    static inline void hash_append(uint64_t& h, const String& s)
    {
        hash_append(h, std::string_view{s.c_str()});
    }

    // CRGB and CRGBPalette16 content
    // Append a CRGB color by hashing its raw bytes.
    static inline void hash_append(uint64_t& h, const CRGB& c)
    {
        h = fnv1a64_bytes(&c, sizeof(c), h);
    }

    // Append a CRGBPalette16 by hashing each CRGB entry in order.
    static inline void hash_append(uint64_t& h, const CRGBPalette16& p)
    {
        for (int i = 0; i < 16; ++i)
            hash_append(h, p[i]);
    }

    template<typename T>
    // Append a trivially-copyable non-pointer, non-arithmetic, non-enum type by raw bytes.
    static inline std::enable_if_t<!std::is_pointer_v<T> && !std::is_arithmetic_v<T> && !std::is_enum_v<T>, void>
    hash_append(uint64_t& h, const T& v)
    {
        if constexpr (std::is_trivially_copyable_v<T>)
            h = fnv1a64_bytes(&v, sizeof(v), h);
        else
            static_assert(sizeof(T) == 0, "Unsupported argument type for factory-id hashing");
    }

    template<typename... As>
    // Append a pack of values to the running hash 'h' in order.
    static inline void hash_pack(uint64_t& h, As&&... as)
    {
        (hash_append(h, std::forward<As>(as)), ...);
    }
};

template<class T>
using remove_cvref_t = std::remove_cv_t<std::remove_reference_t<T>>;

template <class T>
constexpr EffectId effect_id_of_type() {
    static_assert(std::is_base_of_v<LEDStripEffect, remove_cvref_t<T>>,
                  "Type must derive from EffectWithId<Id>");
    return remove_cvref_t<T>::ID;   // compile-time constant
}

// --- Macro-free helpers for concise, type-safe effect registration ---

// Adds a default and JSON effect factory for a specific effect number and type.
// All parameters beyond effectNumber and effect type are forwarded to the default constructor.
template<typename TEffect, typename... Args>
inline EffectFactories::NumberedFactory& AddEffect(EffectFactories& factories, Args&&... args)
{
    const uint64_t factoryId = EffectFactoryIdSupport::MakeFactoryId<TEffect>(args...);
    return factories.AddEffect(
        effect_id_of_type<TEffect>(),
        [=]() -> std::shared_ptr<LEDStripEffect> { return make_shared_psram<TEffect>(args...); },
        [](const JsonObjectConst& jsonObject) -> std::shared_ptr<LEDStripEffect> { return make_shared_psram<TEffect>(jsonObject); },
        factoryId
    );
}

// Used by Starry Night helper
std::shared_ptr<LEDStripEffect> CreateStarryNightEffectFromJSON(const JsonObjectConst& jsonObject);

// Adds a default and JSON effect factory for a StarryNightEffect with a specific star type.
template<typename TStar, typename... Args>
inline EffectFactories::NumberedFactory& AddStarryNightEffect(EffectFactories& factories, Args&&... args)
{
    const uint64_t factoryId = EffectFactoryIdSupport::MakeStarryFactoryId<TStar>(idStripStarryNight, args...);
    return factories.AddEffect(
        idStripStarryNight,
        [=]() -> std::shared_ptr<LEDStripEffect> { return make_shared_psram<StarryNightEffect<TStar>>(args...); },
        [](const JsonObjectConst& jsonObject) -> std::shared_ptr<LEDStripEffect> { return CreateStarryNightEffectFromJSON(jsonObject); },
        factoryId
    );
}

// Fold-expression helper to register many at once with brief syntax:
//   RegisterAll(*g_ptrEffectFactories,
//       Effect<idStripPalette, MyEffect>(args...),
//       Starry<MyStar>(args...),
//       Disabled(Effect<idStripColorFill, OtherEffect>(args...)));
template<typename... Adders>
inline void RegisterAll(EffectFactories& factories, Adders&&... adders)
{
    (static_cast<void>(adders(factories)), ...);
}

// Builder for a single effect entry used with RegisterAll
template<typename TEffect, typename... Args>
inline auto Effect(Args&&... args)
{
    return [=](EffectFactories& factories) -> EffectFactories::NumberedFactory&
    {
        return AddEffect<TEffect>(factories, args...);
    };
}

// Builder for a Starry Night entry used with RegisterAll
template<typename TStar, typename... Args>
inline auto Starry(Args&&... args)
{
    return [=](EffectFactories& factories) -> EffectFactories::NumberedFactory&
    {
        return AddStarryNightEffect<TStar>(factories, args...);
    };
}

// Decorator to mark an entry disabled-on-load when using RegisterAll
template<typename F>
inline auto Disabled(F adder)
{
    return [=](EffectFactories& factories) -> EffectFactories::NumberedFactory&
    {
        auto& nf = adder(factories);
        nf.LoadDisabled = true;
        return nf;
    };
}

// Defines used by some StarryNightEffect instances

constexpr float kStarryNightProbability = 1.0f;
constexpr float kStarryNightMusicFactor = 1.0f;
