#pragma once

//+--------------------------------------------------------------------------
//
// File:        byte_utils.h
//
// NightDriverStrip - (c) 2026 Plummer's Software LLC.  All Rights Reserved.
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
//    Byte-level helpers for endian-safe reads.
//

#include "globals.h"

#include <cstdint>
#include <cstring>

inline uint64_t ByteswapU64(uint64_t value)
{
    return  (value >> 56) |
            ((value >> 40) & 0x000000000000FF00ULL) |
            ((value >> 24) & 0x0000000000FF0000ULL) |
            ((value >> 8)  & 0x00000000FF000000ULL) |
            ((value << 8)  & 0x000000FF00000000ULL) |
            ((value << 24) & 0x0000FF0000000000ULL) |
            ((value << 40) & 0x00FF000000000000ULL) |
            (value << 56);
}

inline uint32_t ByteswapU32(uint32_t value)
{
    return  (value >> 24) |
            ((value >> 8)  & 0x0000FF00U) |
            ((value << 8)  & 0x00FF0000U) |
            (value << 24);
}

inline uint16_t ByteswapU16(uint16_t value)
{
    return (uint16_t)((value >> 8) | (value << 8));
}

inline uint64_t ULONGFromMemory(const uint8_t * payloadData)
{
    uint64_t value = 0;
    std::memcpy(&value, payloadData, sizeof(value));
#if __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
    return ByteswapU64(value);
#else
    return value;
#endif
}

inline uint32_t DWORDFromMemory(const uint8_t * payloadData)
{
    uint32_t value = 0;
    std::memcpy(&value, payloadData, sizeof(value));
#if __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
    return ByteswapU32(value);
#else
    return value;
#endif
}

inline uint16_t WORDFromMemory(const uint8_t * payloadData)
{
    uint16_t value = 0;
    std::memcpy(&value, payloadData, sizeof(value));
#if __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
    return ByteswapU16(value);
#else
    return value;
#endif
}
