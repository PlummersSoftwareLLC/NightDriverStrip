#pragma once

//+--------------------------------------------------------------------------
//
// File:        interfaces.h
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
//    Common interfaces and specification structures to decouple core
//    logic from heavy system headers.
//
//---------------------------------------------------------------------------

#include <ArduinoJson.h>
#include <memory>
#include <optional>
#include <type_traits>
#include <vector>

// PreferPSRAMAlloc
//
// Will return PSRAM if it's available, regular ram otherwise
void * PreferPSRAMAlloc(size_t s);

// psram_allocator
//
// A C++ allocator that allocates from PSRAM instead of the regular heap. Initially
// I had just overloaded new for the classes I wanted in PSRAM, but that doesn't work
// with make_shared<> so I had to provide this allocator instead.

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

    psram_allocator()= default;
    ~psram_allocator()= default;

    template <class U> struct rebind { typedef psram_allocator<U> other; };
    template <class U> explicit psram_allocator(const psram_allocator<U>&){}

    size_type max_size() const noexcept { return size_t(-1) / sizeof(value_type); }

    pointer allocate(size_type n, const void * = nullptr)
    {
        return static_cast<pointer>(PreferPSRAMAlloc(n * sizeof(T)));
    }

    void deallocate(pointer p, size_type n)
    {
        free(p);
    }

    template <class U, class... Args>
    void construct(U* p, Args&&... args)
    {
        new(p) U(std::forward<Args>(args)...);
    }

    void destroy(pointer p)
    {
        p->~T();
    }
};

template <typename T, typename U>
inline bool operator==(const psram_allocator<T>&, const psram_allocator<U>&) { return true; }

template <typename T, typename U>
inline bool operator!=(const psram_allocator<T>&, const psram_allocator<U>&) { return false; }

template<typename T>
struct psram_array_deleter
{
    size_t count = 0;

    void operator()(T* ptr) const
    {
        if (!ptr)
            return;

        for (size_t i = 0; i < count; ++i)
            ptr[i].~T();

        free(ptr);
    }
};

// make_unique_psram
//
// Overload for single objects (non-array types)
template<typename T, typename... Args>
std::enable_if_t<!std::is_array<T>::value, std::unique_ptr<T>>
make_unique_psram(Args&&... args)
{
    psram_allocator<T> allocator;
    T* ptr = allocator.allocate(1);
    allocator.construct(ptr, std::forward<Args>(args)...);
    return std::unique_ptr<T>(ptr);
}

// Overload for unknown-bound arrays: make_unique_psram<U[]>(n)
template<typename T>
std::enable_if_t<std::is_array<T>::value && std::extent<T>::value == 0, std::unique_ptr<T>>
make_unique_psram(size_t size)
{
    using U = typename std::remove_extent<T>::type; // element type
    psram_allocator<U> allocator;
    U* ptr = allocator.allocate(size);
    return std::unique_ptr<T>(ptr);
}

// Overload for constructed arrays with a custom deleter so non-trivial element destructors run correctly.
template<typename T>
std::unique_ptr<T[], psram_array_deleter<T>> make_unique_psram_constructed(size_t size)
{
    psram_allocator<T> allocator;
    T* ptr = allocator.allocate(size);
    size_t constructed = 0;

    try
    {
        for (; constructed < size; ++constructed)
            new(ptr + constructed) T();
    }
    catch (...)
    {
        while (constructed > 0)
            ptr[--constructed].~T();

        free(ptr);
        throw;
    }

    return std::unique_ptr<T[], psram_array_deleter<T>>(ptr, psram_array_deleter<T>{size});
}

// Overload for fixed-bound 2D arrays (like [W][H])
template<typename T>
std::enable_if_t<std::rank<T>::value == 2, std::unique_ptr<T>>
make_unique_psram()
{
    using U = typename std::remove_all_extents<T>::type;
    size_t size = std::extent<T, 0>::value * std::extent<T, 1>::value;
    psram_allocator<U> allocator;
    U* ptr = allocator.allocate(size);
    return std::unique_ptr<T>(reinterpret_cast<T*>(ptr));
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

struct IJSONSerializable
{
    virtual bool SerializeToJSON(JsonObject& jsonObject) = 0;
    virtual bool DeserializeFromJSON(const JsonObjectConst& jsonObject) { return false; }
    virtual ~IJSONSerializable() = default;
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

    // Hint to the UI layer about what kind of input control this setting wants.
    // The renderer consults this and the widget-specific fields below to build
    // the right widget; a spec with WidgetKind::Default falls back to a plain
    // input picked from SettingType.
    enum class WidgetKind : int
    {
        Default,           // type-driven default (Integer -> number input, Boolean -> checkbox, etc.)
        Slider,            // numeric slider; honors DisplayScale and DisplaySuffix
        Select,            // dropdown sourced from inline Options or via OptionsSource
        IntervalToggle     // boolean-on + numeric value composite (effectInterval-style)
    };

    // Where the widget gets its option list from when WidgetKind::Select is used.
    // The UI layer handles each source:
    //   Inline: use Options/OptionLabels carried on the spec
    //   SchemaPath: e.g. "outputs.ws281x.allowedColorOrders" relative to the
    //               unified schema document; the schema is authoritative
    //   IntlCountryCodes: ISO 3166-1 alpha-2 list materialized client-side
    //   ExternalTimeZones: load from /timezones.json
    enum class OptionsSource : int
    {
        Inline,
        SchemaPath,
        IntlCountryCodes,
        ExternalTimeZones
    };

    // "Technical" name of the setting, as in the (JSON) property it is stored in.
    const char* Name{};

    // "Friendly" name of the setting, as in the one to be presented to the user in a user interface.
    const char* FriendlyName{};

    // Description of the purpose and/or value of the setting
    const char* Description{};

    // Value type of the setting
    SettingType Type;

    // Indication if validation for the setting's value is available
    bool HasValidation = false;

    // Indication if a setting is read-only, write-only or read/write
    SettingAccess Access = SettingAccess::ReadWrite;

    // Indication if an empty value is allowed for the setting. This only applies to String settings.
    std::optional<bool> EmptyAllowed = {};

    // Minimum valid value for the setting. This only applies to numeric settings.
    std::optional<double> MinimumValue = {};

    // Maximum valid value for the setting. This only applies to numeric settings.
    std::optional<double> MaximumValue = {};

    // ---- UI metadata (consumed by the front-end so it doesn't need hardcoded knowledge) ----

    // Section identifier this setting belongs to ("topology", "output", "appearance", etc.).
    // The section catalog (id -> friendly title, description) is published separately
    // as part of /api/v1/settings/schema, so the UI can group and label without
    // knowing the section names in advance.
    const char* Section = nullptr;

    // Display priority within the section. Lower values sort to the top.
    // Specs without an explicit priority sort lexicographically by FriendlyName.
    std::optional<int> Priority = {};

    // True if a chip reboot is required for this setting's change to take effect.
    bool RequiresReboot = false;

    // Canonical dotted path in the unified settings document (/api/v1/settings),
    // e.g. "topology.width" or "outputs.ws281x.colorOrder". Settings whose ApiPath
    // is null are written through the legacy /settings endpoint by name.
    const char* ApiPath = nullptr;

    // Hint to the UI about which widget to render. See WidgetKind.
    WidgetKind Widget = WidgetKind::Default;

    // For WidgetKind::Slider with a display scale that differs from the raw
    // value (e.g. raw 10..255 displayed as 5..100 percent). All four of these
    // must be set together; the UI maps raw <-> display linearly.
    std::optional<double> DisplayRawMin = {};
    std::optional<double> DisplayRawMax = {};
    std::optional<double> DisplayMin = {};
    std::optional<double> DisplayMax = {};

    // Suffix appended to the displayed value (e.g. "%", " ms", " sec").
    const char* DisplaySuffix = nullptr;

    // For WidgetKind::IntervalToggle: the unit divisor between the raw stored
    // value and the displayed value (e.g. 1000 if stored in ms, displayed in s).
    int IntervalUnitDivisor = 1;

    // Verb-form label for the toggle that turns the interval on (rendered next
    // to the switch in the form, e.g. "Rotate effects").
    const char* IntervalOnLabel = nullptr;

    // Noun-form label for the displayed value when raw is 0 (rendered in
    // status panels and summaries, e.g. "Pinned").
    const char* IntervalOffLabel = nullptr;

    // Suffix shown next to the interval value when the toggle is on.
    const char* IntervalUnitLabel = nullptr;

    // For WidgetKind::Select: how to populate the options list.
    OptionsSource Options = OptionsSource::Inline;

    // Parallel arrays of raw values and friendly labels for Select options.
    // For OptionsSource::Inline: the full option list. OptionLabels may be
    //   empty, in which case values double as labels.
    // For OptionsSource::SchemaPath: optional label overrides for schema-
    //   derived values (e.g. "ws281x" -> "WS281x"). Either both arrays are
    //   empty (no overrides) or both are non-empty and the same length.
    // For OptionsSource::ExternalTimeZones: optional label overrides for
    //   specific time zone identifiers. Same empty-or-matched-length rule.
    std::vector<const char*> OptionValues = {};
    std::vector<const char*> OptionLabels = {};

    // For Options == OptionsSource::SchemaPath: the dotted path within
    // /api/v1/settings/schema where the option array lives.
    const char* OptionsSchemaPath = nullptr;

    // For Options == OptionsSource::ExternalTimeZones (or any future external
    // source): the URL to fetch the option document from. Carrying it on the
    // spec keeps the UI from having to bake a path like "/timezones.json".
    const char* OptionsExternalUrl = nullptr;

    // Validates the consistency of the spec's contents. Called by Construct(); can also be
    // called directly when constructing specs without using Construct().
    void FinishAndValidateInitialization();

    // Factory method: construct a SettingSpec via C++20 designated initializers, then call
    // FinishAndValidateInitialization() and return the validated spec. Use this for specs
    // that need no post-construction field assignments:
    //
    //   settingSpecs.push_back(SettingSpec::Validate(SettingSpec{
    //       .Name         = "myName",
    //       .FriendlyName = "My Name",
    //       .Type         = SettingType::String,
    //       .Section      = "section",
    //       .ApiPath      = "path.to.setting"
    //   }));
    //
    // Fields not listed receive their in-class defaults. Designated initializers must appear
    // in the same order as the members are declared in this struct (C++20 requirement).
    static SettingSpec Validate(SettingSpec spec);

    String TypeName() const;

    // String form of WidgetKind, suitable for emission in the spec response.
    const char* WidgetName() const;

    // String form of OptionsSource.
    const char* OptionsSourceName() const;
};
