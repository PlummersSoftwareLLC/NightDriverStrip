//+--------------------------------------------------------------------------
//
// File:        hashing.cpp
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
//    Hashing utilities implementation.
//
// History:     Sep-26-2023         Rbergen     Created for NightDriverStrip
//
//---------------------------------------------------------------------------

#include "globals.h"
#include "hashing.h"

namespace fnv1a
{
    // Hashes a CRGB struct by hashing its r, g, and b components.
    template<typename H>
    H hash(const CRGB& c, H h)
    {
        h = hash(c.r, h);
        h = hash(c.g, h);
        h = hash(c.b, h);
        return h;
    }

    // Explicitly instantiate for common hash sizes
    template uint32_t hash<uint32_t>(const CRGB& c, uint32_t h);
    template uint64_t hash<uint64_t>(const CRGB& c, uint64_t h);

    // Hashes a CRGBPalette16 by hashing each of its 16 CRGB entries.
    template<typename H>
    H hash(const CRGBPalette16& p, H h)
    {
        for (int i = 0; i < 16; ++i)
            h = hash(p[i], h);
        return h;
    }

    // Explicitly instantiate for common hash sizes
    template uint32_t hash<uint32_t>(const CRGBPalette16& p, uint32_t h);
    template uint64_t hash<uint64_t>(const CRGBPalette16& p, uint64_t h);

    // Converts a hash value to a hexadecimal Arduino String of fixed length.
    String hash_to_string(uint32_t h)
    {
        return String(hash_to_string_impl(h).data);
    }

    String hash_to_string(uint64_t h)
    {
        return String(hash_to_string_impl(h).data);
    }
}
