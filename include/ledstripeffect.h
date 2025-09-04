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

#pragma once

#include "effects.h"
#include "gfxbase.h"
#include "jsonserializer.h"
#include "hub75gfx.h"
#include "types.h"
#include "hashing.h"

#include <memory>
#include <list>
#include <cstdlib>

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
    static void FillBaseSettingSpecs()
    {
        // If the base SettingSpec instances already exist, bail out...
        if (!_baseSettingSpecs.empty())
            return;

        // ...otherwise, create and add them

        _baseSettingSpecs.emplace_back(
            ACTUAL_NAME_OF(_friendlyName),
            "Friendly name",
            "The friendly name of the effect, as shown in the web UI and/or on the matrix.",
            SettingSpec::SettingType::String
        );
        _baseSettingSpecs.emplace_back(
            ACTUAL_NAME_OF(_maximumEffectTime),
            "Maximum effect time",
            "The maximum time in ms that the effect is shown per effect rotation. This duration is only applied if it's "
            "shorter than the default effect interval. A value of 0 means no maximum effect time is set.",
            SettingSpec::SettingType::PositiveBigInteger
        );
        _baseSettingSpecs.emplace_back(
            "hasMaximumEffectTime",
            "Has maximum effect time set",
            "Indicates if the effect has a maximum effect time set.",
            SettingSpec::SettingType::Boolean
        ).Access = SettingSpec::SettingAccess::ReadOnly;
        _baseSettingSpecs.emplace_back(
            "clearMaximumEffectTime",
            "Clear maximum effect time",
            "Clear maximum effect time. Set to true to reset the maximum effect time to the default value.",
            SettingSpec::SettingType::Boolean
        ).Access = SettingSpec::SettingAccess::WriteOnly;
    }

  protected:

    size_t _cLEDs = 0;
    String _friendlyName;
    bool   _enabled = true;
    size_t _maximumEffectTime = 0;

    std::vector<std::shared_ptr<GFXBase>> _GFX;

    // Function that assigns a value to a property if two names match
    template <typename TProperty, typename TValue>
    static bool SetIfNameMatches(const String& firstName, const String& secondName, TProperty& property, const TValue& value)
    {
        if (firstName == secondName)
        {
            property = value;
            return true;
        }
        return false;
    }

    // Helper functions for known setting types, as defined in SettingSpec::SettingType

    static bool SetIfSelected(const String& settingName, const String& propertyName, int& property, const String& value)
    {
        return SetIfNameMatches(settingName, propertyName, property, value.toInt());
    }

    static bool SetIfSelected(const String& settingName, const String& propertyName, size_t& property, const String& value)
    {
        return SetIfNameMatches(settingName, propertyName, property, strtoul(value.c_str(), nullptr, 10));
    }

    static bool SetIfSelected(const String& settingName, const String& propertyName, float& property, const String& value)
    {
        return SetIfNameMatches(settingName, propertyName, property, value.toFloat());
    }

    static bool SetIfSelected(const String& settingName, const String& propertyName, bool& property, const String& value)
    {
        return SetIfNameMatches(settingName, propertyName, property, BoolFromText(value));
    }

    static bool SetIfSelected(const String& settingName, const String& propertyName, String& property, const String& value)
    {
        return SetIfNameMatches(settingName, propertyName, property, value);
    }

    static bool SetIfSelected(const String& settingName, const String& propertyName, CRGBPalette16& property, const String& value)
    {
        if (settingName != propertyName)
            return false;

        auto src = CreateJsonDocument();
        deserializeJson(src, value);
        CRGB colors[16];
        int colorIndex = 0;

        const auto & componentsArray = src.as<JsonArrayConst>();
        for (const auto &v: componentsArray)
        {
            colors[colorIndex++] = v.as<CRGB>();
        }

        property = CRGBPalette16(colors);

        return true;
    }

    static bool SetIfSelected(const String& settingName, const String& propertyName, CRGB& property, const String& value)
    {
        return SetIfNameMatches(settingName, propertyName, property, CRGB(strtoul(value.c_str(), NULL, 10)));
    }

    // Overrides of this method should fill the respective effect's SettingSpec vector and return a pointer to it.
    // Returning nullptr indicates the effect has no SettingSpec instances to add to the base set.
    virtual EffectSettingSpecs* FillSettingSpecs()
    {
        return nullptr;
    }

    static float fmap(const float x, const float in_min, const float in_max, const float out_min, const float out_max)
    {
        return (out_max - out_min) * (x - in_min) / (in_max - in_min) + out_min;
    }

  public:

    // Constructor doesn't take an effect number; effect identity is provided by effectId()

    explicit LEDStripEffect(const String & strName)
    {
        if (!strName.isEmpty())
            _friendlyName = strName;
    }

    explicit LEDStripEffect(const JsonObjectConst&  jsonObject)
        : _friendlyName(jsonObject["fn"].as<String>())
    {
        if (jsonObject["es"].is<int>())
            _enabled = jsonObject["es"].as<int>() == 1;
        if (jsonObject["mt"].is<size_t>())
            _maximumEffectTime = jsonObject["mt"];

        // Pull the migrations bitmap from the JSON object if it has one, otherwise default to "nothing set"
        uint performedMigrations = 0;
        if (jsonObject["mi"].is<uint>())
            performedMigrations = jsonObject["mi"];

        // If we haven't migrated the "has no maximum effect time" yet, do so now
        if (!(performedMigrations & to_value(JSONMigrations::MaximumEffectTime)) && _maximumEffectTime == UINT_MAX)
            _maximumEffectTime = 0;
    }

    virtual ~LEDStripEffect() = default;

    virtual bool Init(std::vector<std::shared_ptr<GFXBase>>& gfx)
    {
        debugV("Init %s", _friendlyName.c_str());

        _GFX = gfx;                                                 // There are up to 8 channel in play per effect and when we
                                                                    //   start up, we are given copies to their graphics interfaces
                                                                    //   so that we can call them directly later from other calls
        _cLEDs = _GFX[0]->GetLEDCount();

        debugV("Init Effect %s with %zu LEDs\n", _friendlyName.c_str(), _cLEDs);
        return true;
    }

    virtual void Start() {}                                         // Optional method called when time to clean/init the effect
    virtual void Draw() = 0;                                        // Your effect must implement these

    std::shared_ptr<GFXBase> g(size_t channel = 0) const
    {
        return _GFX[channel];
    }

    // mg is a shortcut for MATRIX projects to retrieve a pointer to the specialized HUB75GFX type

    #if USE_HUB75
      std::shared_ptr<HUB75GFX> mg(size_t channel = 0)
      {
        return std::static_pointer_cast<HUB75GFX>(_GFX[channel]);
      }
    #endif

    #if HEXAGON
      std::shared_ptr<HexagonGFX> hg(size_t channel = 0)
      {
        return std::static_pointer_cast<HexagonGFX>(_GFX[channel]);
      }
    #endif

    virtual bool CanDisplayVUMeter() const
    {
        return true;
    }

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

    virtual bool RequiresDoubleBuffering() const
    {
        return true;
    }

    // RandomRainbowColor
    //
    // Returns a random color of the rainbow
    // BUGBUG Should likely be in GFXBase, not in LEDStripEffect

    static CRGB RandomRainbowColor()
    {
        static const CRGB colors[] =
            {
                CRGB::Green,
                CRGB::Red,
                CRGB::Blue,
                CRGB::Orange,
                CRGB::Indigo,
                CRGB::Violet
            };
        int randomColorIndex = random_range(0U, std::size(colors));
        return colors[randomColorIndex];
    }

    // RandomSaturatedColor
    //
    // A random, but fully saturated, color

    static CRGB RandomSaturatedColor()
    {
        CRGB c;
        c.setHSV(random_range(0,255), 255, 255);
        return c;
    }

    // GetBlackBodyHeatColor
    //
    // Given a temp in the 0-1 range, returns a fire-appropriate black body radiator color for it

    virtual CRGB GetBlackBodyHeatColor(float temp) const
    {
        return ColorFromPalette(HeatColors_p, 255 * temp);
    }

    // The variant allows you to specify a base flame color other than red, and the result
    // is interpolated from black to your color and on through yellow and white

    virtual CRGB GetBlackBodyHeatColor(float temp, CRGB baseColor) const
    {
        if (baseColor == CRGB::Red)
            return GetBlackBodyHeatColor(temp);

        temp = std::clamp(temp, 0.0f, 1.0f);

        if (temp < 0.33f)
            return ColorFraction(baseColor, temp * 3.0f);                                                   // Interpolate from black to baseColor

        if (temp < 0.66f)
            return baseColor + ColorFraction(CRGB::Yellow - baseColor, (temp - 0.33f) * 3.0f);              // Interpolate from baseColor to Yellow

        return CRGB::Yellow + ColorFraction(CRGB::Blue,  (temp - 0.66f) * 3.0f);                            // Interpolate from Yellow to White
    }

    // fillSolidOnAllChannels
    //
    // Fill all of the LEDs specified with the color indicated.  Can have arbitrary start, length, and step

    void fillSolidOnAllChannels(CRGB color, int iStart = 0, int numToFill = 0, uint everyN = 1)
    {
        if (_GFX.size() == 0)
        {
            debugE("fillSolidOnAllChannels called with no GFX devices");
            throw std::runtime_error("Graphics not set up properly, no GFX devices");
        }

        if (numToFill == 0)
            numToFill = _cLEDs-iStart;

        if (iStart + numToFill > _cLEDs)
        {
            debugE("Boundary Exceeded in FillRainbow");
            return;
        }

        for (auto& device : _GFX)
        {
            for (int i = iStart; i < iStart + numToFill; i+= everyN)
                device->setPixel(i, color);
        }
    }

    // SetPixelsFOnAllChannels
    //
    // Smooth drawing on fractional pixels on all channels in the given color; if merge is specified,

    void setPixelsFOnAllChannels(float fPos, float count, CRGB c, bool bMerge = false)
    {
        for (auto& device : _GFX)
            device->setPixelsF(fPos, count, c, bMerge);
    }

    // ClearFrameOnAllChannels
    //
    // Clears ALL the channels

    void ClearFrameOnAllChannels()
    {
        for (auto& device : _GFX)
            device->Clear();
    }

    // ColorFraction
    //
    // Returns a fraction of a color; abstracts the fadeToBlack away so that we can later
    // do better color correction as needed

    static CRGB ColorFraction(const CRGB colorIn, float fraction)
    {
        fraction = std::clamp(fraction, 0.0f, 1.0f);
        return CRGB(colorIn).fadeToBlackBy(255 * (1.0f - fraction));
    }

    // fillRainbowAllChannels
    //
    // Fill all channels with a progressive rainbow, using arbitrary start, length, step, and initial color and hue change rate

    void fillRainbowAllChannels(int iStart, int numToFill, uint8_t initialhue, uint8_t deltahue, uint8_t everyNth = 1, bool bMirrored = false)
    {

        for (int i = 0; i < numToFill; i+=everyNth)
        {
            CHSV hsv;
            hsv.hue = initialhue + i * deltahue;
            hsv.val = 255;
            hsv.sat = 255;
            CRGB rgb;
            hsv2rgb_rainbow(hsv, rgb);
            if (bMirrored)
            {
                setPixelOnAllChannels(iStart + i, rgb);
                setPixelOnAllChannels(iStart + numToFill - i - 1, rgb);
            }
            else
            {
                setPixelOnAllChannels(iStart + i, rgb);
            }
            for (int q = 1; q < everyNth; q++)
                setPixelOnAllChannels(iStart + i + q, CRGB::Black);
        }
    }

    // fadePixelToBlackOnAllChannelsBy
    //
    // Given a 0-255 fade value, fades all channels by that amount

    void fadePixelToBlackOnAllChannelsBy(int pixel, uint8_t fadeValue) const
    {
        if (pixel >= 0 && pixel < _cLEDs)
            for (auto& device : _GFX)
                device->fadePixelToBlackBy(pixel, fadeValue);

    }

    void fadeAllChannelsToBlackBy(uint8_t fadeValue) const
    {
        for (auto& device : _GFX)
            for (int i = 0; i < _cLEDs; i++)
                device->fadePixelToBlackBy(i, fadeValue);
    }

    void setAllOnAllChannels(uint8_t r, uint8_t g, uint8_t b) const
    {
        for (auto& device : _GFX)
            for (int i = 0; i < _cLEDs; i++)
                device->setPixel(i, r, g, b);
    }

    // setPixelOnAllChannels
    //
    // Sets the indexed pixel to a given color on all channels

    void setPixelOnAllChannels(int i, CRGB c)
    {
        for (auto& device : _GFX)
            device->setPixel(i, c);
    }

    void setPixelOnAllChannels(int x, int y, CRGB c)
    {
        for (auto& device : _GFX)
            device->setPixel(x, y, c);
    }
    // setPixelsOnAllChannels
    //
    // Smooth drawing on fractional pixels on all channels in the given color; if merge is specified,
    // color drawing is additive, otherwise replacement

    void setPixelsOnAllChannels(float fPos, float count, CRGB c, bool bMerge = false) const
    {
        for (auto& device : _GFX)
            device->setPixelsF(fPos, count, c, bMerge);
    }

    // SerializeToJSON
    //
    // Serialize this effects parameters to a JSON document

    bool SerializeToJSON(JsonObject& jsonObject) override
    {
        auto jsonDoc = CreateJsonDocument();

        jsonDoc[PTY_EFFECTNR]       = static_cast<int>(effectId());
        jsonDoc["fn"]               = _friendlyName;
        jsonDoc["es"]               = _enabled ? 1 : 0;

        // Migrations are done when the effect is constructed from JSON, so by definition all known
        // migrations have been performed by the time we get here.
        jsonDoc["mi"]               = to_value(JSONMigrations::All);

        // Only add the max effect time and core effect flag if they're not the default, to save space
        if (HasMaximumEffectTime())
            jsonDoc["mt"]           = _maximumEffectTime;
        if (_coreEffect)
            jsonDoc[PTY_COREEFFECT] = 1;

        return SetIfNotOverflowed(jsonDoc, jsonObject, __PRETTY_FUNCTION__);
    }

    virtual bool IsEnabled() const
    {
        return _enabled;
    }

    virtual void SetEnabled(bool enabled)
    {
        _enabled = enabled;
    }

    void MarkAsCoreEffect()
    {
        _coreEffect = true;
    }

    bool IsCoreEffect() const
    {
        return _coreEffect;
    }

    // Lazily loads the SettingsSpecs for this effect if they haven't been loaded yet, and
    // returns a vector with reference_wrappers to them.
    virtual const std::vector<std::reference_wrapper<SettingSpec>>& GetSettingSpecs()
    {
        // If the SettingSpecs reference_wrapper vector is already filled, return that
        if (!_settingSpecReferences.empty())
            return _settingSpecReferences;

        // Create the SettingSpec instances that are available for all effects
        FillBaseSettingSpecs();

        // Add reference_wrappers for the base SettingSpecs instances to the vector
        _settingSpecReferences.insert(_settingSpecReferences.end(), _baseSettingSpecs.begin(), _baseSettingSpecs.end());

        // Get any SettingSpec instances that our effect subclass has defined
        auto pEffectSettingSpecs = FillSettingSpecs();

        if (pEffectSettingSpecs)
        {
            // Add reference_wrappers for the effect SettingSpecs instances to the vector
            _settingSpecReferences.insert(_settingSpecReferences.end(), pEffectSettingSpecs->begin(), pEffectSettingSpecs->end());
        }

        return _settingSpecReferences;
    }

    // Serialize the "known effect settings" for this effect to JSON. In principle, there
    // should be a SettingSpec instance returned by GetSettingSpecs() for every setting value
    // that's serialized by this function.
    virtual bool SerializeSettingsToJSON(JsonObject& jsonObject)
    {
        auto jsonDoc = CreateJsonDocument();

        jsonDoc[ACTUAL_NAME_OF(_friendlyName)] = _friendlyName;
        jsonDoc[ACTUAL_NAME_OF(_maximumEffectTime)] = _maximumEffectTime;
        jsonDoc["hasMaximumEffectTime"] = HasMaximumEffectTime();

        return SetIfNotOverflowed(jsonDoc, jsonObject, __PRETTY_FUNCTION__);
    }

    // Changes the value for one "known" effect setting. All setting values are passed to this
    // function are Strings; the conversion to the target type of a member variable that
    // corresponds with a setting can be taken care of by using one of the SetIfSelected()
    // overloads (either via RETURN_IF_SET or directly).
    virtual bool SetSetting(const String& name, const String& value)
    {
        RETURN_IF_SET(name, ACTUAL_NAME_OF(_friendlyName), _friendlyName, value);
        RETURN_IF_SET(name, ACTUAL_NAME_OF(_maximumEffectTime), _maximumEffectTime, value);

        bool clearMaximumEffectTime = false;
        if (SetIfSelected(name, "clearMaximumEffectTime", clearMaximumEffectTime, value))
        {
            if (clearMaximumEffectTime)
                _maximumEffectTime = 0;

            return true;
        }

        return false;
    }
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
// - Uses a function-local static boolean to ensure "log once" behavior for each instantiation.
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

    template <typename T>                                       // Optional one-time debug print of the token string and hash
    inline void debug_log_type_token_once() {
#if EFFECT_ID_DEBUG
        static bool logged = false;
        if (!logged) {
            logged = true;
            const char* token = type_token<T>();
            const EffectId id = token_id_for_type<T>();
            // Print both the raw token string and the resulting hash used as the EffectId
            debugI("Effect ID token string: %s", token);
            debugI("Effect ID hash: 0x%08lx", static_cast<unsigned long>(id));
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