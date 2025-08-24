#pragma once

#include <type_traits>
#include <string>
#define FASTLED_ESP32_FLASH_LOCK 1
#define FASTLED_INTERNAL 1               // Suppresses build banners
#include <FastLED.h>


namespace hashing
{
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
}