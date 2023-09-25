//+--------------------------------------------------------------------------
//
// File:        types.h
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
// Description:
//
//    Types of a somewhat general use
//
// History:     May-23-2023         Rbergen      Created
//
//---------------------------------------------------------------------------

#pragma once

#include <cstddef>
#include <cstdint>
#include <sys/time.h>
#include <optional>
#include <WString.h>

#ifndef MICROS_PER_SECOND
    #define MICROS_PER_SECOND 1000000
#endif

// AppTime
//
// A class that keeps track of the clock, how long the last frame took, calculating FPS, etc.

class CAppTime
{
  protected:

    double _lastFrame = CurrentTime();
    double _deltaTime = 1.0;

  public:

    // NewFrame
    //
    // Call this at the start of every frame or udpate, and it'll figure out and keep track of how
    // long between frames

    void NewFrame()
    {
        timeval tv;
        gettimeofday(&tv, nullptr);
        double current = CurrentTime();
        _deltaTime = current - _lastFrame;

        // Cap the delta time at one full second

        if (_deltaTime > 1.0)
            _deltaTime = 1.0;

        _lastFrame = current;
    }

    CAppTime() : _lastFrame(CurrentTime())
    {
        NewFrame();
    }

    double FrameStartTime() const
    {
        return _lastFrame;
    }

    static double CurrentTime()
    {
        timeval tv;
        gettimeofday(&tv, nullptr);
        return (double)tv.tv_sec + (tv.tv_usec/(double)MICROS_PER_SECOND);
    }

    double FrameElapsedTime() const
    {
        return FrameStartTime() - CurrentTime();
    }

    static double TimeFromTimeval(const timeval & tv)
    {
        return tv.tv_sec + (tv.tv_usec/(double)MICROS_PER_SECOND);
    }

    static timeval TimevalFromTime(double t)
    {
        timeval tv;
        tv.tv_sec = (long)t;
        tv.tv_usec = t - tv.tv_sec;
        return tv;
    }

    double LastFrameTime() const
    {
        return _deltaTime;
    }
};

struct EmbeddedFile
{
    // Embedded file size in bytes
    const size_t length;
    // Contents as bytes
    const uint8_t *const contents;

    EmbeddedFile(const uint8_t start[], const uint8_t end[]) :
        length(end - start),
        contents(start)
    {}
};

struct SettingSpec
{
    // Note that if this enum is expanded, TypeName() must be also!
    enum class SettingType : int
    {
        Integer,
        PositiveBigInteger,
        Float,
        Boolean,
        String,
        Palette,
        Color
    };

    enum class SettingAccess : char
    {
        ReadOnly,
        WriteOnly,
        ReadWrite
    };

    const char* Name;
    const char* FriendlyName;
    const char* Description;
    SettingType Type;
    bool HasValidation = false;
    SettingAccess Access = SettingAccess::ReadWrite;

    std::optional<double> MinimumValue = {};
    std::optional<double> MaximumValue = {};


    SettingSpec(const char* name, const char* friendlyName, const char* description, SettingType type)
      : Name(name),
        FriendlyName(friendlyName),
        Description(description),
        Type(type)
    {}

    SettingSpec(const char* name, const char* friendlyName, SettingType type) : SettingSpec(name, friendlyName, nullptr, type)
    {}

    // Constructor that sets both mininum and maximum values
    SettingSpec(const char* name, const char* friendlyName, const char* description, SettingType type, double min, double max)
      : SettingSpec(name, friendlyName, description, type)
    {
        MinimumValue = min;
        MaximumValue = max;
    }

    // Constructor that sets both mininum and maximum values
    SettingSpec(const char* name, const char* friendlyName, SettingType type, double min, double max)
      : SettingSpec(name, friendlyName, nullptr, type, min, max)
    {}

    SettingSpec()
    {}

    virtual String TypeName() const
    {
        const String names[] = { "Integer", "PositiveBigInteger", "Float", "Boolean", "String", "Palette", "Color" };
        return names[static_cast<int>(Type)];
    }
};

// PreferPSRAMAlloc
//
// Will return PSRAM if it's available, regular ram otherwise

inline void * PreferPSRAMAlloc(size_t s)
{
    if (psramInit())
    {
        debugV("PSRAM Array Request for %u bytes\n", s);
        return ps_malloc(s);
    }
    else
    {
        return malloc(s);
    }
}

// psram_allocator
//
// A C++ allocator that allocates from PSRAM instead of the regular heap. Initially
// I had just overloaded new for the classes I wanted in PSRAM, but that doesn't work
// with make_shared<> so I had to provide this allocator instead.
//
// When enabled, this puts all of the LEDBuffers in PSRAM.  The table that keeps track
// of them is still in base ram.
//
// (Davepl - I opted to make this *prefer* psram but return regular ram otherwise. It
//           avoids a lot of ifdef USE_PSRAM in the code.  But I've only proved it
//           correct, not tried it on a chip without yet.

template <typename T>
class psram_allocator
{
public:
    typedef size_t size_type;
    typedef ptrdiff_t difference_type;
    typedef T* pointer;
    typedef const T* const_pointer;
    typedef T& reference;
    typedef const T& const_reference;
    typedef T value_type;

    psram_allocator(){}
    ~psram_allocator(){}

    template <class U> struct rebind { typedef psram_allocator<U> other; };
    template <class U> psram_allocator(const psram_allocator<U>&){}

    pointer address(reference x) const {return &x;}
    const_pointer address(const_reference x) const {return &x;}
    size_type max_size() const throw() {return size_t(-1) / sizeof(value_type);}

    pointer allocate(size_type n, const void * hint = 0)
    {
        void * pmem = PreferPSRAMAlloc(n*sizeof(T));
        return static_cast<pointer>(pmem) ;
    }

    void deallocate(pointer p, size_type n)
    {
        free(p);
    }

    template< class U, class... Args >
    void construct( U* p, Args&&... args )
    {
        ::new((void *) p ) U(std::forward<Args>(args)...);
    }

    void destroy(pointer p)
    {
        p->~T();
    }
};

// Typically we do not need a deleter because the regular one can handle PSRAM deallocations just fine,
// but for completeness, here it is.

template<typename T>
struct psram_deleter
{
    void operator()(T* ptr)
    {
        psram_allocator<T> allocator;
        allocator.destroy(ptr);
        allocator.deallocate(ptr, 1);
    }
};

// make_unique_psram
//
// Like std::make_unique, but returns PSRAM instead of base RAM.  We cheat a little here by not providing
// a deleter, because we know that PSRAM can be freed with the regular free() call and does not require
// special handling.

template<typename T, typename... Args>
std::unique_ptr<T> make_unique_psram(Args&&... args)
{
    psram_allocator<T> allocator;
    T* ptr = allocator.allocate(1);
    allocator.construct(ptr, std::forward<Args>(args)...);
    return std::unique_ptr<T>(ptr);
}

template<typename T>
std::unique_ptr<T[]> make_unique_psram_array(size_t size)
{
    psram_allocator<T> allocator;
    T* ptr = allocator.allocate(size);
    // No need to call construct since arrays don't have constructors
    return std::unique_ptr<T[]>(ptr);
}

// make_shared_psram
//
// Same as std::make_shared except allocates preferentially from the PSRAM pool

template<typename T, typename... Args>
std::shared_ptr<T> make_shared_psram(Args&&... args)
{
    psram_allocator<T> allocator;
    return std::allocate_shared<T>(allocator, std::forward<Args>(args)...);
}

template<typename T>
std::shared_ptr<T> make_shared_psram_array(size_t size)
{
    psram_allocator<T> allocator;
    return std::allocate_shared<T>(allocator, size);
}

