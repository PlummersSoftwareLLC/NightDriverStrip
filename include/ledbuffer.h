#pragma once

//+--------------------------------------------------------------------------
//
// File:        LEDBuffer.h
//
// NightDriverStrip - (c) 2018 Plummer's Software LLC.  All Rights Reserved.
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
//   Provides a timestamped buffer of colordata.  The LEDBufferManager keeps
//   N of these buffers in a circular queue, and each has a timestamp on it
//   indicating when it becomes valid.
//
// History:     Oct-9-2018         Davepl      Created from other projects
//
//---------------------------------------------------------------------------

#include "globals.h"

#include <memory>
#include <pixeltypes.h>
#include <vector>

#include "gfxbase.h"

class LEDBuffer
{
  public:

     std::shared_ptr<GFXBase> _pStrand;

  private:

    std::unique_ptr<CRGB []> _leds;
    uint32_t                 _pixelCount;
    uint64_t                 _timeStampMicroseconds;
    uint64_t                 _timeStampSeconds;

  public:

    explicit LEDBuffer(std::shared_ptr<GFXBase> pStrand);

    ~LEDBuffer()
    = default;

    uint64_t Seconds()      const;
    uint64_t MicroSeconds() const;
    uint32_t Length()       const;

    double TimeTillDue() const;

    bool IsBufferOlderThan(const timeval & tv) const;

    // UpdateFromWire
    //
    // Parse and deposit a WiFi packet into a buffer

    bool UpdateFromWire(std::unique_ptr<uint8_t []> & payloadData, size_t payloadLength);

    void DrawBuffer();
};

// LEDBufferManager
//
// Manages a circular buffer of LEDBuffer objects.  The buffer itself is an array of shared_ptrs to
// LEDBuffer objects.  The buffer is managed through a unique_ptr.  The LEDBuffer objects are managed
// through shared_ptrs as they are also returned to callers.

class LEDBufferManager
{
    std::unique_ptr<std::vector<std::shared_ptr<LEDBuffer>>> _ppBuffers;          // The circular array of buffer ptrs
    std::shared_ptr<LEDBuffer> _pLastBufferAdded;   // Keeps track of the MRU buffer
    size_t                                               _iNextBuffer;        // Head pointer index
    size_t                                               _iLastBuffer;        // Tail pointer index
    uint32_t                                             _cBuffers;           // Number of buffers

  public:

    LEDBufferManager(uint32_t cBuffers, const std::shared_ptr<GFXBase>& pGFX);

    double AgeOfOldestBuffer() const;

    double AgeOfNewestBuffer() const;

    // BufferCount
    //
    // The fixed, maximum size of the whole thing if it were full

    size_t BufferCount() const;

    // Depth
    //
    // The variable, current count of buffers in use

    size_t Depth() const;

    bool IsEmpty() const;

    // PeekNewestBuffer
    //
    // Get a pointer to the most recently added (newest) buffer, or nullptr if empty

    std::shared_ptr<LEDBuffer> PeekNewestBuffer() const;

    // GetNewBuffer
    //
    // Grabs the next buffer in the circle, advancing the tail pointer as well if we've
    // 'caught up' to the head pointer, which effective throws away that buffer via reuse

    std::shared_ptr<LEDBuffer> GetNewBuffer();

    // GetOldestBuffer
    //
    // Return a pointer to the very oldest buffer, or nullptr if empty

    std::shared_ptr<LEDBuffer> GetOldestBuffer();

    // PeekOldestBuffer
    //
    // Take a "peek" at the newest buffer, or nullptr if empty

    std::shared_ptr<LEDBuffer> PeekOldestBuffer() const;

    // operator[]
    //
    // Returns a pointer to the buffer at the specified logical index, or nullptr if empty
    std::shared_ptr<LEDBuffer> operator[](size_t index) const;
};
