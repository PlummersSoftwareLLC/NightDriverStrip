#include "globals.h"
#include "byte_utils.h"
#include "ledbuffer.h"
#include "values.h"

#include <limits>

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
    _leds = make_unique_psram<CRGB[]>(_pStrand->GetLEDCount());
}

uint64_t LEDBuffer::Seconds()      const  { return _timeStampSeconds;      }
uint64_t LEDBuffer::MicroSeconds() const  { return _timeStampMicroseconds; }
uint32_t LEDBuffer::Length()       const  { return _pixelCount;            }

double LEDBuffer::TimeTillDue() const
{
    return _timeStampSeconds + (_timeStampMicroseconds / (double) MICROS_PER_SECOND) - g_Values.AppTime.CurrentTime();
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

bool LEDBuffer::ValidateWirePayload(const uint8_t* payloadData,
                                    size_t payloadLength,
                                    size_t ledCount,
                                    size_t* payloadByteCount)
{
    if (payloadByteCount)
        *payloadByteCount = 0;

    if (!payloadData)
    {
        debugW("No payload data received to process");
        return false;
    }

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

    if (length32 > (std::numeric_limits<size_t>::max() - cbHeader) / sizeof(CRGB))
    {
        debugW("LED payload size overflow");
        return false;
    }

    const size_t requiredPayloadBytes = length32 * sizeof(CRGB);
    if (payloadLength < requiredPayloadBytes + cbHeader)
    {
        debugW("command16: %hu   length32: %lu,  payloadLength: %zu\n", command16, (unsigned long)length32, payloadLength);
        debugW("Data size mismatch");
        return false;
    }
    if (length32 > ledCount)
    {
        debugW("More data than we have LEDs\n");
        return false;
    }

    if (payloadByteCount)
        *payloadByteCount = requiredPayloadBytes;

    return true;
}

bool LEDBuffer::UpdateFromWire(const uint8_t* payloadData, size_t payloadLength)
{
    if (!_pStrand)
    {
        debugW("No strand attached to LED buffer");
        return false;
    }

    size_t payloadBytes = 0;
    if (!ValidateWirePayload(payloadData, payloadLength, _pStrand->GetLEDCount(), &payloadBytes))
        return false;

    uint16_t command16 = WORDFromMemory(&payloadData[0]);
    uint16_t channel16 = WORDFromMemory(&payloadData[2]);
    uint32_t length32  = DWORDFromMemory(&payloadData[4]);
    uint64_t seconds   = ULONGFromMemory(&payloadData[8]);
    uint64_t micros    = ULONGFromMemory(&payloadData[16]);

    const size_t cbHeader = sizeof(command16) + sizeof(channel16) + sizeof(length32) + sizeof(seconds) + sizeof(micros);

    debugV("PayloadLength: %zu, command16: %hu, Length32: %lu", payloadLength, command16, (unsigned long)length32);

    const CRGB * pRGB = reinterpret_cast<const CRGB *>(&payloadData[cbHeader]);

    // Keep this commit after the shared validation used by ProcessIncomingData.
    // New queue slots are now reserved only after that validation passes, and
    // existing-buffer updates still avoid committing invalid metadata.
    _timeStampSeconds      = seconds;
    _timeStampMicroseconds = micros;
    _pixelCount            = length32;

    memcpy(_leds.get(), pRGB, payloadBytes);
    debugV("seconds, micros: %llu.%llu", seconds, micros);
    if (length32 > 0)
        debugV("Color0: %08lx", (unsigned long)(uint32_t) _leds[0]);
    return true;
}

void LEDBuffer::DrawBuffer()
{
    _timeStampMicroseconds = 0;
    _timeStampSeconds      = 0;
    _pStrand->fillLeds(_leds.get());
}

void LEDBuffer::Reconfigure(std::shared_ptr<GFXBase> pStrand)
{
    const auto nextLedCount = pStrand ? pStrand->GetLEDCount() : 0;
    const auto currentLedCount = _pStrand ? _pStrand->GetLEDCount() : 0;

    _pStrand = std::move(pStrand);

    // Pin/color-order changes should not churn every buffered frame. Only reallocate when the
    // actual LED count changes; otherwise just retarget the buffer to the new strand config.
    if (nextLedCount != currentLedCount)
    {
        _leds = make_unique_psram<CRGB[]>(nextLedCount);
    }

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
   _cQueuedBuffers(0),
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

// LEDCount
// The number of LEDs in each buffer, which is the same as the number of LEDs in the strand
// that the buffers are configured for. If there are no buffers or the buffers aren't configured, returns 0.

size_t LEDBufferManager::LEDCount() const
{
    return (_cBuffers > 0 && !_ppBuffers->empty() && (*_ppBuffers)[0] && (*_ppBuffers)[0]->_pStrand)
        ? (*_ppBuffers)[0]->_pStrand->GetLEDCount()
        : 0;
}

// Depth
//
// The variable, current count of buffers in use

size_t LEDBufferManager::Depth() const
{
    return _cQueuedBuffers;
}

bool LEDBufferManager::IsEmpty() const
{
    return _cQueuedBuffers == 0;
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
    if (_cBuffers == 0)
        return nullptr;

    auto pResult = (*_ppBuffers)[_iNextBuffer];

    // Full and empty used to be represented by the same head/tail indices, so
    // an exact fill at the wrap point made the whole queue look empty. Track
    // depth explicitly and advance the tail only when a real overwrite occurs.
    _iNextBuffer = (_iNextBuffer + 1) % _cBuffers;
    if (_cQueuedBuffers == _cBuffers)
        _iLastBuffer = (_iLastBuffer + 1) % _cBuffers;
    else
        ++_cQueuedBuffers;

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
    _iLastBuffer = (_iLastBuffer + 1) % _cBuffers;
    --_cQueuedBuffers;

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
    _cQueuedBuffers = 0;
    _pLastBufferAdded.reset();
}

// operator[]
//
// Returns a pointer to the buffer at the specified logical index, or nullptr if empty
std::shared_ptr<LEDBuffer> LEDBufferManager::operator[](size_t index) const
{
    if (IsEmpty() || index >= _cQueuedBuffers)
        return nullptr;
    size_t i = (_iLastBuffer + index) % _cBuffers;
    return (*_ppBuffers)[i];
}
