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
//   indicating wheb it becomes valid.
//
// History:     Oct-9-2018         Davepl      Created from other projects
//
//---------------------------------------------------------------------------

#pragma once

#include <pixeltypes.h>
#include <memory>
#include <iostream>

using namespace std;

class LEDBuffer
{
  public:
  
     shared_ptr<LEDMatrixGFX> _pStrand;

  private:
    
    unique_ptr<CRGB []> _leds;
    uint32_t            _pixelCount;
    uint64_t            _timeStampMicroseconds;
    uint64_t            _timeStampSeconds;
   
  public:

    explicit LEDBuffer(shared_ptr<LEDMatrixGFX> pStrand) : 
                 _pStrand(pStrand),
                 _pixelCount(0),
                 _timeStampMicroseconds(0),
                 _timeStampSeconds(0)
    {
        _leds = make_unique<CRGB []>(NUM_LEDS);

        for (int i = 0; i < ARRAYSIZE(_leds); i++)
            _leds[i] = CRGB::Yellow;
    }

    ~LEDBuffer()
    {
    }

    uint64_t Seconds()      const  { return _timeStampSeconds;      }
    uint64_t MicroSeconds() const  { return _timeStampMicroseconds; }
    uint32_t Length()       const  { return _pixelCount;            }

    bool IsBufferOlderThan(const timeval & tv) const
    {
        if (Seconds() < tv.tv_sec)
            return true;
        
        if (Seconds() == tv.tv_sec)
            if (MicroSeconds() < tv.tv_usec)
                return true;
        
        return false;
    }

    bool UpdateFromWire(uint8_t * payloadData, size_t payloadLength)
    {
        if (payloadLength < 24)                 // Our header size
        {
            debugW("Not enough data received to process");
            return false;
        }

        #if 0
            debugV("========");              
            for (int i = 0; i < 24; i++)
                debugV("%02x ", payloadData[i]);
            debugV("========");
        #endif

        uint16_t command16 = WORDFromMemory(&payloadData[0]);
        uint16_t channel16 = WORDFromMemory(&payloadData[2]);
        uint32_t length32  = DWORDFromMemory(&payloadData[4]);
        uint64_t seconds   = ULONGFromMemory(&payloadData[8]);
        uint64_t micros    = ULONGFromMemory(&payloadData[16]);

        //printf("UpdateFromWire -- Command: %u, Channel: %d, Length: %u, Seconds: %u, Micros: %u\n", command16, channel16, length32, seconds, micros);

        const size_t cbHeader = sizeof(command16) + sizeof(channel16) + sizeof(length32) + sizeof(seconds) + sizeof(micros);

        _timeStampSeconds      = seconds;
        _timeStampMicroseconds = micros;
        _pixelCount            = length32;

        if (payloadLength < length32 * sizeof(CRGB) + cbHeader)
        {
            debugW("command16: %d   length32: %d,  payloadLength: %d\n", command16, length32, payloadLength);
            debugW("Data size mismatch");
            return false;
        }
        if (length32 > STRAND_LEDS)
        {
            debugW("More data than we have LEDs\n");
            return false;
        }
        debugV("PayloadLength: %d, command16: %d, Length32: %d", payloadLength, command16, length32);
        
        CRGB * pRGB = reinterpret_cast<CRGB *>(&payloadData[cbHeader]);

        memcpy((void *)_leds.get(), pRGB, length32 * sizeof(CRGB));
        debugV("seconds, micros: %llu.%llu", seconds, micros);
        debugV("Color0: %08x", (uint32_t) _leds[0]);
        return true;
    }

    void DrawBuffer() 
    {
        _timeStampMicroseconds = 0;
        _timeStampSeconds      = 0;
        
        int iPixel = 0; 
        for (int i = 0; i < _pStrand->width() * _pStrand->height(); i++)
        {
            int x = iPixel % _pStrand->width();
            int y = iPixel / _pStrand->width();
            _pStrand->drawPixel(x, y, _leds[iPixel]);
            iPixel++;
            if (iPixel >= _pixelCount)
                break;
        }
    }
};

// LEDBufferManager
//
// Manages a circular buffer of LEDBuffer objects.  The buffer itself is an array of shared_ptrs to
// LEDBuffer objects.  The buffer is managed through a unique_ptr.  The LEDBuffer objects are managed
// through shared_ptrs as they are also returned to callers.

class LEDBufferManager
{
    const unique_ptr<shared_ptr<LEDBuffer> []> _ppBuffers;          // The circular array of buffer ptrs
    shared_ptr<LEDBuffer>                      _pLastBufferAdded;   // Keeps track of the MRU buffer
    size_t                                     _iNextBuffer;        // Head pointer index
    size_t                                     _iLastBuffer;        // Tail pointer index
    uint32_t                                   _cBuffers;           // Number of buffers
   
  public:

    LEDBufferManager(uint32_t cBuffers, shared_ptr<LEDMatrixGFX> pGFX)
     : _ppBuffers(make_unique<shared_ptr<LEDBuffer> []>(cBuffers)), // Create the circular array of ptrs
       _iNextBuffer(0),
       _iLastBuffer(0),
       _cBuffers(cBuffers)
    {
        // The initializer creates a uniquely owned table of shared pointers.
        // We exlcusively can see the table, but the buffer objects it contains
        // are returned back out to callers so they must be shared pointers.

        for (int i = 0; i < _cBuffers; i++)
             _ppBuffers[i] = make_shared<LEDBuffer>(pGFX);
    }

    // BufferCount
    //
    // The fixed, maximum size of the whole thing if it were full

    uint32_t BufferCount() const   
    { 
        return _cBuffers; 
    }
    
    // Depth
    // 
    // The variable, current count of buffers in use

    size_t Depth() const
    {
        if (_iNextBuffer < _iLastBuffer)
            return (_iNextBuffer + _cBuffers - _iLastBuffer);
        else
            return _iNextBuffer - _iLastBuffer;
    }

    bool IsEmpty() const
    {
        return _iNextBuffer == _iLastBuffer;
    }

    // PeekNewestBuffer
    //
    // Get a pointer to the most recently added (newest) buffer, or nullptr if empty

    shared_ptr<LEDBuffer> PeekNewestBuffer() const
    {
        if (IsEmpty())
            return nullptr; 
        return _pLastBufferAdded;
    }

    // GetNewBuffer
    //
    // Grabs the next buffer in the circle, advancing the tail pointer as well if we've
    // 'caught up' to the head pointer, which effective throws away that buffer via reuse

    shared_ptr<LEDBuffer> GetNewBuffer()
    {
        auto pResult = _ppBuffers[_iNextBuffer++];
        _iNextBuffer %= _cBuffers;
        if (_iNextBuffer == _iLastBuffer)
            _iLastBuffer++;
        _iLastBuffer %= _cBuffers;
        _pLastBufferAdded = pResult;
        return pResult;
    }

    // GetOldestBuffer
    // 
    // Return a pointer to the very oldest buffer, or nullptr if empty

    shared_ptr<LEDBuffer> GetOldestBuffer() 
    {
        if (IsEmpty())
            return nullptr; 
        auto pResult = _ppBuffers[_iLastBuffer++];
        _iLastBuffer %= _cBuffers;
        return pResult;
    }

    // PeekOldestBuffer
    //
    // Take a "peek" at the newest buffer, or nullptr if emptyu

    const shared_ptr<LEDBuffer> PeekOldestBuffer() const
    {
        if (_iNextBuffer == _iLastBuffer)
            return nullptr; 
        return _ppBuffers[_iLastBuffer];
    }
};


