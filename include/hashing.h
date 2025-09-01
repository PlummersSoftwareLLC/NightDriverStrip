//+--------------------------------------------------------------------------
//
// File:        hashing.h
//
// NightDriverStrip - (c) 2025 Plummer's Software LLC.  All Rights Reserved.
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
//    Hashing utilities for NightDriverStrip effects.
//
// History:     Sep-26-2023         Rbergen     Created for NightDriverStrip
//
//---------------------------------------------------------------------------

#pragma once

#include <type_traits>
#include <string>
#include <string_view>
#include <cstdint>
#include <cstddef>
#include <utility>
#include <vector>
#define FASTLED_ESP32_FLASH_LOCK 1
#define FASTLED_INTERNAL 1               // Suppresses build banners
#include <FastLED.h>

namespace fnv1a
{
    // Helper for dependent-false static_assert in templates
    template<class>
    struct dependent_false : std::false_type {};

    // Traits for FNV parameters per hash width
    template<typename H>
    struct traits;

    // FNV-1a 32-bit hash parameters
    template<>
    struct traits<uint32_t>
    {
        static constexpr uint32_t offset = 2166136261u;
        static constexpr uint32_t prime  = 16777619u;
    };

    // FNV-1a 64-bit hash parameters
    template<>
    struct traits<uint64_t>
    {
        static constexpr uint64_t offset = 1469598103934665603ull;
        static constexpr uint64_t prime  = 1099511628211ull;
    };

    // Generic FNV-1a over raw bytes for any supported hash type H (uint32_t/uint64_t)
    template<typename H>
    constexpr H hash_bytes(const void* data, size_t len, H seed = traits<H>::offset)
    {
        H h = seed;
        const auto* p = static_cast<const unsigned char*>(data);
        for (size_t i = 0; i < len; ++i)
        {
            h ^= static_cast<unsigned char>(p[i]);
            h *= traits<H>::prime;
        }
        return h;
    }

    // Compile-time friendly FNV-1a for C-strings, generic over H
    template<typename H>
    constexpr H hash_cstr(const char* str, H seed = traits<H>::offset)
    {
        H h = seed;
        while (*str)
        {
            h ^= static_cast<unsigned char>(*str++);
            h *= traits<H>::prime;
        }
        return h;
    }

    // -------------------------------------------------------------
    // Generic "hash" helpers that take the current hash value and
    // return the updated hash. Marked constexpr where feasible.
    // -------------------------------------------------------------

    // Unified hash for most types using if constexpr dispatch.
    // Supports arithmetic types, enums, string_view, C-strings, and trivially-copyable types.
    template<typename H, typename T>
    constexpr H hash(const T& v, H h = traits<H>::offset)
    {
        if constexpr (std::is_same_v<std::remove_cv_t<std::remove_reference_t<T>>, std::string_view>)
        {
            return hash_bytes<H>(v.data(), v.size(), h);
        }
        else if constexpr (std::is_enum_v<T>)
        {
            using U = std::underlying_type_t<T>;
            const U u = static_cast<U>(v);
            return hash_bytes<H>(&u, sizeof(u), h);
        }
        else if constexpr (std::is_arithmetic_v<T>)
        {
            return hash_bytes<H>(&v, sizeof(v), h);
        }
        else if constexpr (std::is_pointer_v<T> && std::is_same_v<std::remove_cv_t<std::remove_pointer_t<T>>, char>)
        {
            // const char* (C-string); treat null as no-op
            return v ? hash_cstr<H>(v, h) : h;
        }
        else if constexpr (std::is_array_v<T> && std::is_same_v<std::remove_cv_t<std::remove_extent_t<T>>, char>)
        {
            // char[N] -> treat as C-string
            return hash(static_cast<const char*>(v), h);
        }
        else if constexpr (std::is_trivially_copyable_v<T> && !std::is_pointer_v<T>)
        {
            // Generic trivially-copyable blob
            return hash_bytes<H>(&v, sizeof(v), h);
        }
        else
        {
            static_assert(dependent_false<T>::value, "Unsupported argument type for hashing");
        }
    }

    // Hashes an Arduino String by hashing its underlying C-string.
    template<typename H>
    inline H hash(const String& s, H h = traits<H>::offset)
    {
        return hash(s.c_str(), h);
    }

    // Hashes a CRGB struct by hashing its r, g, and b components.
    template<typename H>
    constexpr H hash(const CRGB& c, H h = traits<H>::offset)
    {
        h = hash(c.r, h);
        h = hash(c.g, h);
        h = hash(c.b, h);
        return h;
    }

    // Hashes a CRGBPalette16 by hashing each of its 16 CRGB entries.
    template<typename H>
    constexpr H hash(const CRGBPalette16& p, H h = traits<H>::offset)
    {
        for (int i = 0; i < 16; ++i)
            h = hash(p[i], h);
        return h;
    }

    // Hashes a std::vector by hashing its size and then each element in order.
    template<typename H, typename T>
    inline H hash(const std::vector<T>& v, H h = traits<H>::offset)
    {
        // Mix the size to reduce potential ambiguities between sequences
        h = hash(v.size(), h);
        for (const auto& e : v)
            h = hash(e, h);
        return h;
    }

    // Appends a value to an existing hash, updating the hash in place.
    template<typename H, typename T>
    inline void hash_append(H& h, T&& v)
    {
        h = hash<H>(std::forward<T>(v), h);
    }

    // Hashes a pack of arguments in order, starting with an initial hash value.
    template<typename H, typename... As>
    constexpr H hash_pack(H h, As&&... as)
    {
        ((h = hash<H>(std::forward<As>(as), h)), ...);
        return h;
    }

    // Runtime version of hash_pack. Hashes a pack of arguments in order.
    template<typename H, typename... As>
    inline H hash_pack_rt(H h, As&&... as)
    {
        ((h = hash<H>(std::forward<As>(as), h)), ...);
        return h;
    }

    // Appends a pack of arguments to an existing hash, updating the hash in place.
    template<typename H, typename... As>
    inline void hash_append_pack(H& h, As&&... as)
    {
        h = hash_pack<H>(h, std::forward<As>(as)...);
    }

    // Returns the length of the hash string for a given hash type H
    template<typename H>
    constexpr size_t hash_string_length()
    {
        return sizeof(H) * 2;
    }

    // Converts a hash value to a hexadecimal Arduino String of fixed length.
    template<typename H>
    inline String hash_to_string(H h)
    {
        constexpr size_t len = hash_string_length<H>();
        char buf[len + 1];
        buf[len] = '\0';

        int i = len - 1;

        // Process non-zero part of the hash
        while (h)
        {
            uint8_t nibble = h & 0x0F;
            buf[i--] = (nibble < 10) ? ('0' + nibble) : ('a' + nibble - 10);
            h >>= 4;
        }

        // Fill remaining leading characters with '0'
        while (i >= 0)
            buf[i--] = '0';

        return buf;
    }
}