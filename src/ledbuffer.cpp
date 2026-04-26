#include "globals.h"
#include "byte_utils.h"
#include "ledbuffer.h"
#include "values.h"

// LEDBuffer
//
// Provides a timestamped buffer of colordata.  The LEDBufferManager keeps
// N of these buffers in a circular queue, and each has a timestamp on it
// indicating when it becomes valid.

LEDBuffer::LEDBuffer(std::shared_ptr<GFXBase> pStrand) :
             _pStrand(std::move(pStrand)),
             _pixelCount(0),
             _timeStampMicroseconds(0),
             _timeStampSeconds(0)
{
    _leds.reset(psram_allocator<CRGB>().allocate(_pStrand->GetLEDCount()));
}

uint64_t LEDBuffer::Seconds()      const  { return _timeStampSeconds;      }
uint64_t LEDBuffer::MicroSeconds() const  { return _timeStampMicroseconds; }
uint32_t LEDBuffer::Length()       const  { return _pixelCount;            }

double LEDBuffer::TimeTillDue() const
{
    return g_Values.AppTime.CurrentTime() - _timeStampSeconds - (_timeStampMicroseconds / (double) MICROS_PER_SECOND);
}

bool LEDBuffer::IsBufferOlderThan(const timeval & tv) const
{
    if (Seconds() < tv.tv_sec)
        return true;

    if (Seconds() == tv.tv_sec)
        if (MicroSeconds() < tv.tv_usec)
            return true;

    return false;
}

// UpdateFromWire
//
// Parse and deposit a WiFi packet into a buffer

bool LEDBuffer::UpdateFromWire(std::unique_ptr<uint8_t []> & payloadData, size_t payloadLength)
{
    if (payloadLength < 24)                 // Our header size
    {
        debugW("Not enough data received to process");
        return false;
    }

    uint16_t command16 = WORDFromMemory(&payloadData[0]);
    uint16_t channel16 = WORDFromMemory(&payloadData[2]);
    uint32_t length32  = DWORDFromMemory(&payloadData[4]);
    uint64_t seconds   = ULONGFromMemory(&payloadData[8]);
    uint64_t micros    = ULONGFromMemory(&payloadData[16]);

    const size_t cbHeader = sizeof(command16) + sizeof(channel16) + sizeof(length32) + sizeof(seconds) + sizeof(micros);

    _timeStampSeconds      = seconds;
    _timeStampMicroseconds = micros;
    _pixelCount            = length32;

    if (payloadLength < length32 * sizeof(CRGB) + cbHeader)
    {
        debugW("command16: %hu   length32: %lu,  payloadLength: %zu\n", command16, (unsigned long)length32, payloadLength);
        debugW("Data size mismatch");
        return false;
    }
    if (length32 > _pStrand->GetLEDCount())
    {
        debugW("More data than we have LEDs\n");
        return false;
    }
    debugV("PayloadLength: %zu, command16: %hu, Length32: %lu", payloadLength, command16, (unsigned long)length32);

    CRGB * pRGB = reinterpret_cast<CRGB *>(&payloadData[cbHeader]);

    memcpy(_leds.get(), pRGB, length32 * sizeof(CRGB));
    debugV("seconds, micros: %llu.%llu", seconds, micros);
    debugV("Color0: %08lx", (unsigned long)(uint32_t) _leds[0]);
    return true;
}

void LEDBuffer::DrawBuffer()
{
    _timeStampMicroseconds = 0;
    _timeStampSeconds      = 0;
    _pStrand->fillLeds(_leds);
}

void LEDBuffer::Reconfigure(std::shared_ptr<GFXBase> pStrand)
{
    _pStrand = std::move(pStrand);
    _leds.reset(psram_allocator<CRGB>().allocate(_pStrand->GetLEDCount()));
    _pixelCount = 0;
    _timeStampMicroseconds = 0;
    _timeStampSeconds = 0;
}

// LEDBufferManager
//
// Manages a circular buffer of LEDBuffer objects.  The buffer itself is an array of shared_ptrs to
// LEDBuffer objects.  The buffer is managed through a unique_ptr.  The LEDBuffer objects are managed
// through shared_ptrs as they are also returned to callers.

LEDBufferManager::LEDBufferManager(uint32_t cBuffers, const std::shared_ptr<GFXBase>& pGFX)
 : _ppBuffers(std::make_unique<std::vector<std::shared_ptr<LEDBuffer>>>()), // Create the circular array of ptrs
   _iNextBuffer(0),
   _iLastBuffer(0),
   _cBuffers(cBuffers)
{
    // The initializer creates a uniquely owned table of shared pointers.
    // We exclusively can see the table, but the buffer objects it contains
    // are returned back out to callers so they must be shared pointers.

    for (uint32_t i = 0; i < _cBuffers; i++)
        _ppBuffers->push_back(make_shared_psram<LEDBuffer>(pGFX));
}

double LEDBufferManager::AgeOfOldestBuffer() const
{
    auto pOldest = PeekOldestBuffer();
    if (pOldest) {
        return (pOldest->Seconds() + pOldest->MicroSeconds() / (float)MICROS_PER_SECOND) - g_Values.AppTime.CurrentTime();
    }
    else
    {
        return 0.0;
    }
}

double LEDBufferManager::AgeOfNewestBuffer() const
{
    auto pNewest = PeekNewestBuffer();
    if (pNewest) {
        return (pNewest->Seconds() + pNewest->MicroSeconds() / (float)MICROS_PER_SECOND) - g_Values.AppTime.CurrentTime();
    }
    else
    {
        return 0.0;
    }
}

// BufferCount
//
// The fixed, maximum size of the whole thing if it were full

size_t LEDBufferManager::BufferCount() const
{
    return _cBuffers;
}

// Depth
//
// The variable, current count of buffers in use

size_t LEDBufferManager::Depth() const
{
    if (_iNextBuffer < _iLastBuffer)
        return (_iNextBuffer + _cBuffers - _iLastBuffer);
    else
        return _iNextBuffer - _iLastBuffer;
}

bool LEDBufferManager::IsEmpty() const
{
    return _iNextBuffer == _iLastBuffer;
}

// PeekNewestBuffer
//
// Get a pointer to the most recently added (newest) buffer, or nullptr if empty

std::shared_ptr<LEDBuffer> LEDBufferManager::PeekNewestBuffer() const
{
    if (IsEmpty())
        return nullptr;
    return _pLastBufferAdded;
}

// GetNewBuffer
//
// Grabs the next buffer in the circle, advancing the tail pointer as well if we've
// 'caught up' to the head pointer, which effective throws away that buffer via reuse

std::shared_ptr<LEDBuffer> LEDBufferManager::GetNewBuffer()
{
    auto pResult = (*_ppBuffers)[_iNextBuffer++];

    if (IsEmpty())
        _iLastBuffer++;

    _iLastBuffer %= _cBuffers;
    _iNextBuffer %= _cBuffers;

    _pLastBufferAdded = pResult;

    return pResult;
}

// GetOldestBuffer
//
// Return a pointer to the very oldest buffer, or nullptr if empty

std::shared_ptr<LEDBuffer> LEDBufferManager::GetOldestBuffer()
{
    if (IsEmpty())
        return nullptr;

    auto pResult = (*_ppBuffers)[_iLastBuffer];
    _iLastBuffer++;
    _iLastBuffer %= _cBuffers;

    return pResult;
}

// PeekOldestBuffer
//
// Take a "peek" at the newest buffer, or nullptr if empty

std::shared_ptr<LEDBuffer> LEDBufferManager::PeekOldestBuffer() const
{
    if (IsEmpty())
        return nullptr;

    return (*_ppBuffers)[_iLastBuffer];
}

void LEDBufferManager::Reconfigure(const std::shared_ptr<GFXBase>& pGFX)
{
    // Runtime topology changes should not leave stale-sized WiFi buffers behind. Resetting the circular
    // queue here makes the active transport size match the active graphics context immediately.
    for (auto& buffer : *_ppBuffers)
        buffer->Reconfigure(pGFX);

    _iNextBuffer = 0;
    _iLastBuffer = 0;
    _pLastBufferAdded.reset();
}

// operator[]
//
// Returns a pointer to the buffer at the specified logical index, or nullptr if empty
std::shared_ptr<LEDBuffer> LEDBufferManager::operator[](size_t index) const
{
    if (IsEmpty())
        return nullptr;
    size_t i = (_iLastBuffer + index) % _cBuffers;
    return (*_ppBuffers)[i];
}
