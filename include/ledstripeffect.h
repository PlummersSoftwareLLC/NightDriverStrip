#pragma once

//+--------------------------------------------------------------------------
//
// File:        LEDStripEffect.h
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
//    along with NightDriver.  It is normally found in copying.txt
//    If not, see <https://www.gnu.org/licenses/>.
//
// Description:
//
//    Defines the base class for effects that run locally on the chip
//
// History:     Apr-13-2019         Davepl      Created for NightDriverStrip
//
//---------------------------------------------------------------------------

#include "globals.h"
#include "effects.h"
#include "hashing.h"
#include "jsonserializer.h"

#include <functional>
#include <memory>
#include <vector>

class GFXBase;
struct BeatInfo;

#if HEXAGON
class HexagonGFX;
#endif

// Declarations related to effect settings, and their SettingSpecs. The definitions revolving around
// SettingSpecs are mainly there because getting it right is a bit finicky due to the container type
// used and the class static nature of the actual containers. Particularly adding an initializer for
// a SettingSpec container is easy to overlook.

// The type for effect SettingSpec containers
using EffectSettingSpecs = std::vector<SettingSpec, psram_allocator<SettingSpec>>;

// Declares a static class member variable that contains the SettingSpecs for an effect, if it has them.
// If an effect uses this macro, it also needs a matching INIT_EFFECT_SETTING_SPECS invocation in
// effects.cpp, or linker errors will ensue.
#define DECLARE_EFFECT_SETTING_SPECS(memberName) \
    static EffectSettingSpecs memberName

// Initializes the effect setting specs member that's been added to an effect class. There must be one
// use of this in effects.cpp for every use DECLARE_EFFECT_SETTING_SPECS, or the linker will balk.
#define INIT_EFFECT_SETTING_SPECS(effectName, specsMember) \
    EffectSettingSpecs effectName::specsMember = {}

// This macro returns from the invoking function (which would usually be SetSetting())
// if the settingName and propertyName passed to it match, and the "value" was thus
// assigned to the "property".
#define RETURN_IF_SET(settingName, propertyName, property, value) \
    if (SetIfSelected(settingName, propertyName, property, value)) \
        return true

// LEDStripEffect
//
// Base class for an LED strip effect.  At a minimum they must draw themselves and provide a unique name.

class LEDStripEffect : public IJSONSerializable
{
  private:

    // This enum is a set of bit flags of known JSON data migrations that either have or have not (yet) been
    // performed for a particular effect. "All" should always be a bitwise OR of all other flags in the enum.
    enum class JSONMigrations : uint
    {
        MaximumEffectTime = 1,                  // next one = 2, one after that = 4, etc.
        All               = MaximumEffectTime   // | next one | one after that | etc.
    };

    DECLARE_EFFECT_SETTING_SPECS(_baseSettingSpecs);
    std::vector<std::reference_wrapper<SettingSpec>> _settingSpecReferences;

    bool   _coreEffect = false;

    // This "lazy loads" the SettingSpec instances for LEDStripEffect. Note that it adds the actual
    // instances to a static vector, meaning they are loaded once for all effects. The _settingSpecReferences
    // instance variable vector only contains reference_wrappers to the actual SettingSpecs to save
    // memory.
    static void FillBaseSettingSpecs();

  protected:

    size_t _cLEDs = 0;
    String _friendlyName;
    bool   _enabled = true;
    size_t _maximumEffectTime = 0;

    std::vector<std::shared_ptr<GFXBase>> _GFX;

    // Helper functions for known setting types, as defined in SettingSpec::SettingType

    static bool SetIfSelected(const String& settingName, const String& propertyName, int& property, const String& value);
    static bool SetIfSelected(const String& settingName, const String& propertyName, size_t& property, const String& value);
    static bool SetIfSelected(const String& settingName, const String& propertyName, float& property, const String& value);
    static bool SetIfSelected(const String& settingName, const String& propertyName, bool& property, const String& value);
    static bool SetIfSelected(const String& settingName, const String& propertyName, String& property, const String& value);
    static bool SetIfSelected(const String& settingName, const String& propertyName, CRGBPalette16& property, const String& value);
    static bool SetIfSelected(const String& settingName, const String& propertyName, CRGB& property, const String& value);

    // Overrides of this method should fill the respective effect's SettingSpec vector and return a pointer to it.
    // Returning nullptr indicates the effect has no SettingSpec instances to add to the base set.
    virtual EffectSettingSpecs* FillSettingSpecs() { return nullptr; }

    static float fmap(const float x, const float in_min, const float in_max, const float out_min, const float out_max);

  public:

    // Constructor doesn't take an effect number; effect identity is provided by effectId()

    explicit LEDStripEffect(const String & strName);
    explicit LEDStripEffect(const JsonObjectConst&  jsonObject);
    virtual ~LEDStripEffect();

    // Init may be called more than once during the lifetime of an effect. In particular, live WS281x
    // topology/output changes re-run Init() on every effect so it can refresh cached geometry, LED counts,
    // and any size-dependent allocations. Implementations must therefore be re-entrant/idempotent and
    // replace or rebuild any topology-dependent state instead of assuming one-time construction.
    virtual bool Init(std::vector<std::shared_ptr<GFXBase>>& gfx);

    virtual void Start() {}                                         // Optional method called when time to clean/init the effect
    virtual void Draw() = 0;                                        // Your effect must implement these
    virtual void OnBeat(const BeatInfo&) {}                         // Optional beat callback for audio-reactive effects

    GFXBase& g(size_t channel = 0);
    const GFXBase& g(size_t channel = 0) const;

    #if HEXAGON
      std::shared_ptr<HexagonGFX> hg(size_t channel = 0);
    #endif

    virtual bool CanDisplayVUMeter() const;

    virtual const String & FriendlyName() const             // User-visible effect name
    {
        return _friendlyName;
    }

    // Runtime effect id. Subclasses must override to return their EffectId


    virtual EffectId effectId() const = 0;

    virtual size_t DesiredFramesPerSecond() const           // Desired framerate of the LED drawing
    {
        return 30;
    }

    virtual size_t MaximumEffectTime() const                // For splash screens and similar, a max display time for the effect
    {
        return _maximumEffectTime;
    }

    virtual bool HasMaximumEffectTime() const
    {
        return MaximumEffectTime() != 0;
    }

    virtual bool ShouldShowTitle() const                    // True if the effect should show the title overlay
    {
        return true;
    }

    // RequiresDoubleBuffering
    //
    // If a matrix effect requires the state of the last buffer be preserved, then it requires double buffering.
    // If, on the other hand, it renders from scratch every time, starting with a black fill, etc., then it does not,
    // and it can override this method and return false;

    virtual bool RequiresDoubleBuffering() const;

    // RandomRainbowColor
    //
    // Returns a random color of the rainbow
    // BUGBUG Should likely be in GFXBase, not in LEDStripEffect

    static CRGB RandomRainbowColor();

    // RandomSaturatedColor
    //
    // A random, but fully saturated, color

    static CRGB RandomSaturatedColor();

    // GetBlackBodyHeatColor
    //
    // Given a temp in the 0-1 range, returns a fire-appropriate black body radiator color for it

    virtual CRGB GetBlackBodyHeatColor(float temp) const;

    // The variant allows you to specify a base flame color other than red, and the result
    // is interpolated from black to your color and on through yellow and white

    virtual CRGB GetBlackBodyHeatColor(float temp, CRGB baseColor) const;

    // fillSolidOnAllChannels
    //
    // Fill all of the LEDs specified with the color indicated.  Can have arbitrary start, length, and step

    void fillSolidOnAllChannels(CRGB color, int iStart = 0, int numToFill = 0, uint everyN = 1);

    // SetPixelsFOnAllChannels
    //
    // Smooth drawing on fractional pixels on all channels in the given color; if merge is specified,

    void setPixelsFOnAllChannels(float fPos, float count, CRGB c, bool bMerge = false);

    // ClearFrameOnAllChannels
    //
    // Clears ALL the channels

    void ClearFrameOnAllChannels();

    // ColorFraction
    //
    // Returns a fraction of a color; abstracts the fadeToBlack away so that we can later
    // do better color correction as needed

    static CRGB ColorFraction(const CRGB colorIn, float fraction);

    // fillRainbowAllChannels
    //
    // Fill all channels with a progressive rainbow, using arbitrary start, length, step, and initial color and hue change rate

    void fillRainbowAllChannels(int iStart, int numToFill, uint8_t initialhue, uint8_t deltahue, uint8_t everyNth = 1, bool bMirrored = false);

    // fadePixelToBlackOnAllChannelsBy
    //
    // Given a 0-255 fade value, fades all channels by that amount

    void fadePixelToBlackOnAllChannelsBy(int pixel, uint8_t fadeValue) const;

    void fadeAllChannelsToBlackBy(uint8_t fadeValue) const;

    void setAllOnAllChannels(uint8_t r, uint8_t g, uint8_t b) const;

    // setPixelOnAllChannels
    //
    // Sets the indexed pixel to a given color on all channels

    void setPixelOnAllChannels(int i, CRGB c);
    void setPixelOnAllChannels(int x, int y, CRGB c);
    // setPixelsOnAllChannels
    //
    // Smooth drawing on fractional pixels on all channels in the given color; if merge is specified,
    // color drawing is additive, otherwise replacement

    void setPixelsOnAllChannels(float fPos, float count, CRGB c, bool bMerge = false) const;

    // SerializeToJSON
    //
    // Serialize this effects parameters to a JSON document

    bool SerializeToJSON(JsonObject& jsonObject) override;

    virtual bool IsEnabled() const;
    virtual void SetEnabled(bool enabled);
    void MarkAsCoreEffect();
    bool IsCoreEffect() const;

    // Lazily loads the SettingsSpecs for this effect if they haven't been loaded yet, and
    // returns a vector with reference_wrappers to them.
    virtual const std::vector<std::reference_wrapper<SettingSpec>>& GetSettingSpecs();

    // Serialize the "known effect settings" for this effect to JSON. In principle, there
    // should be a SettingSpec instance returned by GetSettingSpecs() for every setting value
    // that's serialized by this function.
    virtual bool SerializeSettingsToJSON(JsonObject& jsonObject);

    // Changes the value for one "known" effect setting. All setting values are passed to this
    // function are Strings; the conversion to the target type of a member variable that
    // corresponds with a setting can be taken care of by using one of the SetIfSelected()
    // overloads (either via RETURN_IF_SET or directly).
    virtual bool SetSetting(const String& name, const String& value);
};

#ifndef EFFECT_ID_DEBUG
#define EFFECT_ID_DEBUG 0
#endif

// Internal helpers for deriving a per-type EffectId and (optionally) logging what was used to compute it.
// The approach is: obtain a compiler-provided, human-readable token string that uniquely identifies T,
// then hash that string with FNV-1a to produce a compact 32-bit EffectId.
//
// type_token<T>:
// - Returns the compiler's decorated function signature string (__PRETTY_FUNCTION__) for this template instantiation.
// - This string contains the fully qualified type T (including template parameters), making it suitable as a stable token
//   within the same compiler family/version.
// - Only supported on GCC/Clang. Other compilers will hit a hard #error.
//
// token_id_for_type<T>:
// - Computes EffectId by hashing the token string with a constexpr FNV-1a 32-bit hash.
// - When the hash implementation is constexpr, the resulting EffectId can be a compile-time constant.
// - EffectId is derived from a 32-bit hash; collisions are possible (though unlikely). Do not rely on cryptographic strength.
//
// debug_log_type_token_once<T> (enabled when EFFECT_ID_DEBUG is true):
// - Prints the raw token string and the computed EffectId once per T per translation unit.
// - Uses a function-local static bool to ensure "log once" behavior for each instantiation.
// - Note: because this is a header-only template with a function-local static, if the same T is instantiated
//   in multiple translation units, each TU may log once.
//
// Stability and portability notes:
// - The exact __PRETTY_FUNCTION__ format is not standardized and can vary by compiler and version.
//   Consequently, EffectId values may change when switching compilers, versions, or certain build flags.
//
// Intended usage:
// - Use token_id_for_type<MyType>() wherever a deterministic, type-based EffectId is needed, as EffectWithId does.
// - Enable EFFECT_ID_DEBUG to inspect the underlying token string and the derived hash during development.
namespace _effect_id_detail {

    template <typename T>                                       // Return the compiler-provided token string used for hashing
    constexpr const char* type_token() {
#if defined(__GNUC__) || defined(__clang__)
        return __PRETTY_FUNCTION__;
#else
        #error "EffectWithId requires a compiler that supports __PRETTY_FUNCTION__"
#endif
    }

    template <typename T>
    constexpr EffectId token_id_for_type()
    {
        return fnv1a::hash_cstr<EffectId>(type_token<T>());
    }

    void debug_log_effect_id(const char* token, EffectId id);

    template <typename T>                                       // Optional one-time debug print of the token string and hash
    inline void debug_log_type_token_once() {
#if EFFECT_ID_DEBUG
        static bool logged = false;
        if (!logged) {
            logged = true;
            debug_log_effect_id(type_token<T>(), token_id_for_type<T>());
        }
#endif
    }
}

// CRTP helper: derive as EffectWithId<Derived> to auto-provide a unique, stable ID per type

template<typename TDerived>
class EffectWithId : public LEDStripEffect
{
public:
    static constexpr EffectId ID = _effect_id_detail::token_id_for_type<TDerived>();

    explicit EffectWithId(const String & strName) : LEDStripEffect(strName) {}
    explicit EffectWithId(const JsonObjectConst&  jsonObject) : LEDStripEffect(jsonObject) {}

    EffectId effectId() const override
    {
#if EFFECT_ID_DEBUG
    // Log the token string and hash once per type at first use
    _effect_id_detail::debug_log_type_token_once<TDerived>();
#endif
        return ID;
    }
};
