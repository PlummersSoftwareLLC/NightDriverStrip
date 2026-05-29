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
#include <esp_heap_caps.h>
#include <limits>
#include <memory>
#include <new>
#include <optional>
#include <type_traits>

// Memory placement policy
//
// The platform default allocator (plain malloc / new / std::make_unique /
// std::make_shared / std::vector) may route allocations above
// PSRAM_DEFAULT_THRESHOLD to PSRAM on PSRAM-enabled builds. That is a useful
// heap-pressure release valve, but it is not a correctness contract.
//
// Any allocation the project itself wants in PSRAM (large LED frame buffers,
// effect state, GIF/QR scratch, JSON config docs, etc.) must say so
// explicitly via psram_allocator<T> / make_unique_psram / make_shared_psram.
//
// Allocations that must be DMA-reachable (peripheral buffers we own) use
// dma_allocator<T> / make_unique_dma / make_shared_dma. Most peripheral
// drivers already call heap_caps_malloc(MALLOC_CAP_DMA) themselves, so this
// is rarely needed at the call site.
//
// Allocations that should explicitly stay in internal SRAM for latency
// reasons (hot per-frame state, ISR-touched data, code touched while flash
// cache may be disabled) use internal_allocator<T> / make_unique_internal /
// make_shared_internal.

template <typename T>
inline size_t CheckedAllocationSize(size_t n)
{
    if (n > std::numeric_limits<size_t>::max() / sizeof(T))
        throw std::bad_array_new_length();

    const auto bytes = n * sizeof(T);
    return bytes == 0 ? 1 : bytes;
}

// psram_allocator<T>
//
// Allocator that places objects in PSRAM. PSRAM is large but slower than
// internal SRAM, has no DMA capability, and cannot be accessed from ISRs
// while flash cache is disabled. Use for big, cold, project-owned buffers.
template <typename T>
class psram_allocator
{
public:
    using value_type      = T;
    using size_type       = size_t;
    using difference_type = ptrdiff_t;
    using pointer         = T*;
    using const_pointer   = const T*;
    using reference       = T&;
    using const_reference = const T&;

    psram_allocator() = default;
    template <class U> psram_allocator(const psram_allocator<U>&) noexcept {}

    pointer allocate(size_type n)
    {
        const auto bytes = CheckedAllocationSize<T>(n);
        void* p = heap_caps_malloc(bytes, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
        // Fall back to internal SRAM on boards without PSRAM (or if PSRAM is
        // exhausted) so the project still runs; PSRAM was an optimization,
        // never a hard requirement.
        if (!p) p = heap_caps_malloc(bytes, MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
        if (!p) throw std::bad_alloc();
        return static_cast<pointer>(p);
    }
    void deallocate(pointer p, size_type) noexcept { heap_caps_free(p); }
};
template <typename T, typename U>
inline bool operator==(const psram_allocator<T>&, const psram_allocator<U>&) { return true; }
template <typename T, typename U>
inline bool operator!=(const psram_allocator<T>&, const psram_allocator<U>&) { return false; }

// internal_allocator<T>
//
// Allocator that explicitly requests internal SRAM, bypassing the PSRAM-default
// routing. Use for state that must not live in PSRAM: hot per-frame buffers,
// data accessed from ISRs, small frequently-mutated structures.
template <typename T>
class internal_allocator
{
public:
    using value_type      = T;
    using size_type       = size_t;
    using difference_type = ptrdiff_t;
    using pointer         = T*;
    using const_pointer   = const T*;
    using reference       = T&;
    using const_reference = const T&;

    internal_allocator() = default;
    template <class U> internal_allocator(const internal_allocator<U>&) noexcept {}

    pointer allocate(size_type n)
    {
        void* p = heap_caps_malloc(CheckedAllocationSize<T>(n), MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
        if (!p) throw std::bad_alloc();
        return static_cast<pointer>(p);
    }
    void deallocate(pointer p, size_type) noexcept { heap_caps_free(p); }
};
template <typename T, typename U>
inline bool operator==(const internal_allocator<T>&, const internal_allocator<U>&) { return true; }
template <typename T, typename U>
inline bool operator!=(const internal_allocator<T>&, const internal_allocator<U>&) { return false; }

// dma_allocator<T>
//
// Allocator for buffers that must be DMA-reachable. Internal SRAM is the only
// memory region the ESP32 DMA engines can touch; PSRAM is not DMA-capable.
template <typename T>
class dma_allocator
{
public:
    using value_type = T;
    dma_allocator() = default;
    template <class U> dma_allocator(const dma_allocator<U>&) noexcept {}

    T* allocate(size_t n)
    {
        void* p = heap_caps_malloc(CheckedAllocationSize<T>(n), MALLOC_CAP_DMA | MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
        if (!p) throw std::bad_alloc();
        return static_cast<T*>(p);
    }
    void deallocate(T* p, size_t) noexcept { heap_caps_free(p); }
};
template <typename T, typename U>
inline bool operator==(const dma_allocator<T>&, const dma_allocator<U>&) { return true; }
template <typename T, typename U>
inline bool operator!=(const dma_allocator<T>&, const dma_allocator<U>&) { return false; }

template <typename T, typename Allocator>
inline std::enable_if_t<std::is_array<T>::value && std::extent<T>::value == 0, std::unique_ptr<T>>
make_unique_array_with_allocator(size_t n, Allocator alloc)
{
    using U = std::remove_extent_t<T>;
    using Element = std::remove_all_extents_t<T>;
    static_assert(std::is_default_constructible<Element>::value && std::is_trivially_destructible<Element>::value,
        "make_unique_*<T[]>(n) requires default-constructible, trivially-destructible element types.");

    U* p = alloc.allocate(n);
    try { std::uninitialized_value_construct_n(p, n); }
    catch (...) { alloc.deallocate(p, n); throw; }
    return std::unique_ptr<T>(p);
}

// make_unique_internal / make_shared_internal
//
// Construct an object explicitly in internal SRAM. Use for hot-path or
// DMA-adjacent state that should not tolerate PSRAM access latency.
template <typename T, typename... Args>
inline std::enable_if_t<!std::is_array<T>::value, std::unique_ptr<T>>
make_unique_internal(Args&&... args)
{
    internal_allocator<T> alloc;
    T* p = alloc.allocate(1);
    try { new (p) T(std::forward<Args>(args)...); }
    catch (...) { alloc.deallocate(p, 1); throw; }
    return std::unique_ptr<T>(p);
}

template <typename T, typename... Args>
inline std::shared_ptr<T> make_shared_internal(Args&&... args)
{
    return std::allocate_shared<T>(internal_allocator<T>{}, std::forward<Args>(args)...);
}

template <typename T>
inline std::enable_if_t<std::is_array<T>::value && std::extent<T>::value == 0, std::unique_ptr<T>>
make_unique_internal(size_t n)
{
    using U = std::remove_extent_t<T>;
    return make_unique_array_with_allocator<T>(n, internal_allocator<U>{});
}

// make_unique_dma / make_shared_dma
//
// Construct an object in DMA-capable internal RAM. The peripherals that
// require this (FastLED RMT, SmartMatrix HUB75, I2S audio, ADC continuous)
// generally call heap_caps_malloc(MALLOC_CAP_DMA) directly, but these helpers
// are available for any project-side buffer that needs to be DMA-reachable.
template <typename T, typename... Args>
inline std::enable_if_t<!std::is_array<T>::value, std::unique_ptr<T>>
make_unique_dma(Args&&... args)
{
    dma_allocator<T> alloc;
    T* p = alloc.allocate(1);
    try { new (p) T(std::forward<Args>(args)...); }
    catch (...) { alloc.deallocate(p, 1); throw; }
    return std::unique_ptr<T>(p);
}

template <typename T, typename... Args>
inline std::shared_ptr<T> make_shared_dma(Args&&... args)
{
    return std::allocate_shared<T>(dma_allocator<T>{}, std::forward<Args>(args)...);
}

template <typename T>
inline std::enable_if_t<std::is_array<T>::value && std::extent<T>::value == 0, std::unique_ptr<T>>
make_unique_dma(size_t n)
{
    using U = std::remove_extent_t<T>;
    return make_unique_array_with_allocator<T>(n, dma_allocator<U>{});
}

// make_unique_psram / make_shared_psram
//
// Construct an object in PSRAM. Use for large, project-owned buffers that
// don't need DMA, don't need ISR access while flash cache is disabled, and
// don't sit in a tight per-frame hot loop. Falls back to internal SRAM if
// PSRAM is absent or full so non-PSRAM boards still work.
template <typename T, typename... Args>
inline std::enable_if_t<!std::is_array<T>::value, std::unique_ptr<T>>
make_unique_psram(Args&&... args)
{
    psram_allocator<T> alloc;
    T* p = alloc.allocate(1);
    try { new (p) T(std::forward<Args>(args)...); }
    catch (...) { alloc.deallocate(p, 1); throw; }
    return std::unique_ptr<T>(p);
}

template <typename T, typename... Args>
inline std::shared_ptr<T> make_shared_psram(Args&&... args)
{
    return std::allocate_shared<T>(psram_allocator<T>{}, std::forward<Args>(args)...);
}

// make_unique_psram<U[]>(n) - unknown-bound array overload, used for raw
// scratch buffers (decoders, pixel pools). This preserves
// std::make_unique<U[]>(n) value-initialization semantics while placing the
// backing storage in PSRAM.
template <typename T>
inline std::enable_if_t<std::is_array<T>::value && std::extent<T>::value == 0, std::unique_ptr<T>>
make_unique_psram(size_t n)
{
    using U = std::remove_extent_t<T>;
    return make_unique_array_with_allocator<T>(n, psram_allocator<U>{});
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
