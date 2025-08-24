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

    template<>
    struct traits<uint32_t>
    {
        static constexpr uint32_t offset = 2166136261u;
        static constexpr uint32_t prime  = 16777619u;
    };

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

    // Unified hash for most types using if constexpr dispatch
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

    // Arduino String (cannot be constexpr due to API)
    template<typename H>
    inline H hash(const String& s, H h = traits<H>::offset)
    {
        return hash(s.c_str(), h);
    }

    // CRGB by hashing its r,g,b components (constexpr-friendly)
    template<typename H>
    constexpr H hash(const CRGB& c, H h = traits<H>::offset)
    {
        h = hash(c.r, h);
        h = hash(c.g, h);
        h = hash(c.b, h);
        return h;
    }

    // CRGBPalette16 by hashing each entry (explicit overload to iterate)
    template<typename H>
    constexpr H hash(const CRGBPalette16& p, H h = traits<H>::offset)
    {
        for (int i = 0; i < 16; ++i)
            h = hash(p[i], h);
        return h;
    }

    // std::vector<T> — iterate elements in order (not constexpr due to std::vector)
    template<typename H, typename T>
    inline H hash(const std::vector<T>& v, H h = traits<H>::offset)
    {
        // Mix the size to reduce potential ambiguities between sequences
        h = hash(v.size(), h);
        for (const auto& e : v)
            h = hash(e, h);
        return h;
    }

    // Append helpers (templated by hash type H) — single forwarding overload
    template<typename H, typename T>
    inline void hash_append(H& h, T&& v)
    {
        h = hash<H>(std::forward<T>(v), h);
    }

    // Value-returning pack hash (constexpr-friendly): applies in order and returns updated hash
    template<typename H, typename... As>
    constexpr H hash_pack(H h, As&&... as)
    {
        ((h = hash<H>(std::forward<As>(as), h)), ...);
        return h;
    }

    // Value-returning pack hash (constexpr-friendly): applies in order and returns updated hash
    template<typename H, typename... As>
    inline H hash_pack_rt(H h, As&&... as)
    {
        ((h = hash<H>(std::forward<As>(as), h)), ...);
        return h;
    }

    // By-ref convenience overload delegates to the constexpr value version
    template<typename H, typename... As>
    inline void hash_append_pack(H& h, As&&... as)
    {
        h = hash_pack<H>(h, std::forward<As>(as)...);
    }

    template<typename H>
    inline auto to_string(H h)
    {
        return String(h, 16);
    }
}