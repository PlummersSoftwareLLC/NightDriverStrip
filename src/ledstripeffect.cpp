//+--------------------------------------------------------------------------
//
// File:        ledstripeffect.cpp
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
// Description:
//
//    Definitions for LEDStripEffect helpers moved out of the header
//
// History:     Apr-13-2019         Davepl      Created for NightDriverStrip
//
//---------------------------------------------------------------------------

#include "globals.h"
#include "ledstripeffect.h"

#include <algorithm>
#include <cstdlib>
#include <iterator>
#include <stdexcept>

#include "gfxbase.h"
#include "jsonserializer.h"
#include "random_utils.h"

#if HEXAGON
#include "ws281xgfx.h"
#endif

// Static member definitions
EffectSettingSpecs LEDStripEffect::_baseSettingSpecs = {};

// This "lazy loads" the SettingSpec instances for LEDStripEffect. Note that it adds the actual
// instances to a static vector, meaning they are loaded once for all effects. The _settingSpecReferences
// instance variable vector only contains reference_wrappers to the actual SettingSpecs to save
// memory.
void LEDStripEffect::FillBaseSettingSpecs()
{
    // If the base SettingSpec instances already exist, bail out...
    if (!_baseSettingSpecs.empty())
        return;

    // ...otherwise, create and add them

    _baseSettingSpecs.push_back(SettingSpec::Validate(SettingSpec{
        .Name         = ACTUAL_NAME_OF(_friendlyName),
        .FriendlyName = "Friendly name",
        .Description  = "The friendly name of the effect, as shown in the web UI and/or on the matrix.",
        .Type         = SettingSpec::SettingType::String
    }));
    _baseSettingSpecs.push_back(SettingSpec::Validate(SettingSpec{
        .Name         = ACTUAL_NAME_OF(_maximumEffectTime),
        .FriendlyName = "Maximum effect time",
        .Description  = "The maximum time in ms that the effect is shown per effect rotation. This duration is only applied if it's "
                        "shorter than the default effect interval. A value of 0 means no maximum effect time is set.",
        .Type         = SettingSpec::SettingType::PositiveBigInteger
    }));
    _baseSettingSpecs.push_back(SettingSpec::Validate(SettingSpec{
        .Name         = "hasMaximumEffectTime",
        .FriendlyName = "Has maximum effect time set",
        .Description  = "Indicates if the effect has a maximum effect time set.",
        .Type         = SettingSpec::SettingType::Boolean,
        .Access       = SettingSpec::SettingAccess::ReadOnly
    }));
    _baseSettingSpecs.push_back(SettingSpec::Validate(SettingSpec{
        .Name         = "clearMaximumEffectTime",
        .FriendlyName = "Clear maximum effect time",
        .Description  = "Clear maximum effect time. Set to true to reset the maximum effect time to the default value.",
        .Type         = SettingSpec::SettingType::Boolean,
        .Access       = SettingSpec::SettingAccess::WriteOnly
    }));
}

// LEDStripEffect
//
// Base class for an LED strip effect.  At a minimum they must draw themselves and provide a unique name.

// Constructor doesn't take an effect number; effect identity is provided by effectId()

LEDStripEffect::LEDStripEffect(const String & strName)
{
    if (!strName.isEmpty())
        _friendlyName = strName;
}

LEDStripEffect::LEDStripEffect(const JsonObjectConst&  jsonObject)
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
    if (!(performedMigrations & static_cast<uint>(JSONMigrations::MaximumEffectTime)) && _maximumEffectTime == UINT_MAX)
        _maximumEffectTime = 0;
}

LEDStripEffect::~LEDStripEffect() = default;

bool LEDStripEffect::Init(std::vector<std::shared_ptr<GFXBase>>& gfx)
{
    debugV("Init %s", _friendlyName.c_str());

    _GFX = gfx;                                                 // There are up to 8 channel in play per effect and when we
                                                                //   start up, we are given copies to their graphics interfaces
                                                                //   so that we can call them directly later from other calls
    _cLEDs = _GFX[0]->GetLEDCount();

    debugV("Init Effect %s with %zu LEDs\n", _friendlyName.c_str(), _cLEDs);
    return true;
}

// Must provide at least one drawing instance, like the first matrix or strip we are drawing on
GFXBase& LEDStripEffect::g(size_t channel)
{
    return *_GFX[channel];
}

const GFXBase& LEDStripEffect::g(size_t channel) const
{
    return *_GFX[channel];
}

#if HEXAGON
std::shared_ptr<HexagonGFX> LEDStripEffect::hg(size_t channel)
{
    return std::static_pointer_cast<HexagonGFX>(_GFX[channel]);
}
#endif

bool LEDStripEffect::CanDisplayVUMeter() const { return true; }

// RandomRainbowColor
//
// Returns a random color of the rainbow
// BUGBUG Should likely be in GFXBase, not in LEDStripEffect

CRGB LEDStripEffect::RandomRainbowColor()
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
    int randomColorIndex = random_range((uint32_t)0, (uint32_t)std::size(colors));
    return colors[randomColorIndex];
}

// RandomSaturatedColor
//
// A random, but fully saturated, color

CRGB LEDStripEffect::RandomSaturatedColor()
{
    CRGB c;
    c.setHSV(random_range(0,255), 255, 255);
    return c;
}

// GetBlackBodyHeatColor
//
// Given a temp in the 0-1 range, returns a fire-appropriate black body radiator color for it

CRGB LEDStripEffect::GetBlackBodyHeatColor(float temp) const
{
    return ColorFromPalette(HeatColors_p, 255 * temp);
}

// The variant allows you to specify a base flame color other than red, and the result
// is interpolated from black to your color and on through yellow and white

CRGB LEDStripEffect::GetBlackBodyHeatColor(float temp, CRGB baseColor) const
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

void LEDStripEffect::fillSolidOnAllChannels(CRGB color, int iStart, int numToFill, uint everyN)
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

void LEDStripEffect::setPixelsFOnAllChannels(float fPos, float count, CRGB c, bool bMerge)
{
    for (auto& device : _GFX)
        device->setPixelsF(fPos, count, c, bMerge);
}

// ClearFrameOnAllChannels
//
// Clears ALL the channels

void LEDStripEffect::ClearFrameOnAllChannels()
{
    for (auto& device : _GFX)
        device->Clear();
}

// ColorFraction
//
// Returns a fraction of a color; abstracts the fadeToBlack away so that we can later
// do better color correction as needed

CRGB LEDStripEffect::ColorFraction(const CRGB colorIn, float fraction)
{
    fraction = std::clamp(fraction, 0.0f, 1.0f);
    return CRGB(colorIn).fadeToBlackBy(255 * (1.0f - fraction));
}

// fillRainbowAllChannels
//
// Fill all channels with a progressive rainbow, using arbitrary start, length, step, and initial color and hue change rate

void LEDStripEffect::fillRainbowAllChannels(int iStart, int numToFill, uint8_t initialhue, uint8_t deltahue, uint8_t everyNth, bool bMirrored)
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

void LEDStripEffect::fadePixelToBlackOnAllChannelsBy(int pixel, uint8_t fadeValue) const
{
    if (pixel >= 0 && pixel < (int)_cLEDs)
        for (auto& device : _GFX)
            device->fadePixelToBlackBy(pixel, fadeValue);
}

void LEDStripEffect::fadeAllChannelsToBlackBy(uint8_t fadeValue) const
{
    for (auto& device : _GFX)
        for (int i = 0; i < (int)_cLEDs; i++)
            device->fadePixelToBlackBy(i, fadeValue);
}

void LEDStripEffect::setAllOnAllChannels(uint8_t r, uint8_t g, uint8_t b) const
{
    for (auto& device : _GFX)
        for (int i = 0; i < (int)_cLEDs; i++)
            device->setPixel(i, r, g, b);
}

// setPixelOnAllChannels
//
// Sets the indexed pixel to a given color on all channels

void LEDStripEffect::setPixelOnAllChannels(int i, CRGB c)
{
    for (auto& device : _GFX)
        device->setPixel(i, c);
}

void LEDStripEffect::setPixelOnAllChannels(int x, int y, CRGB c)
{
    for (auto& device : _GFX)
        device->setPixel(x, y, c);
}

// setPixelsOnAllChannels
//
// Smooth drawing on fractional pixels on all channels in the given color; if merge is specified,
// color drawing is additive, otherwise replacement

void LEDStripEffect::setPixelsOnAllChannels(float fPos, float count, CRGB c, bool bMerge) const
{
    for (auto& device : _GFX)
        device->setPixelsF(fPos, count, c, bMerge);
}

// SerializeToJSON
//
// Serialize this effects parameters to a JSON document

bool LEDStripEffect::SerializeToJSON(JsonObject& jsonObject)
{
    auto jsonDoc = CreateJsonDocument();

    jsonDoc[PTY_EFFECTNR]       = static_cast<int>(effectId());
    jsonDoc["fn"]               = _friendlyName;
    jsonDoc["es"]               = _enabled ? 1 : 0;

    // Migrations are done when the effect is constructed from JSON, so by definition all known
    // migrations have been performed by the time we get here.
    jsonDoc["mi"]               = static_cast<uint>(JSONMigrations::All);

    // Only add the max effect time and core effect flag if they're not the default, to save space
    if (HasMaximumEffectTime())
        jsonDoc["mt"]           = _maximumEffectTime;
    if (_coreEffect)
        jsonDoc[PTY_COREEFFECT] = 1;

    return SetIfNotOverflowed(jsonDoc, jsonObject, __PRETTY_FUNCTION__);
}

bool LEDStripEffect::IsEnabled() const { return _enabled; }
void LEDStripEffect::SetEnabled(bool enabled) { _enabled = enabled; }
void LEDStripEffect::MarkAsCoreEffect() { _coreEffect = true; }
bool LEDStripEffect::IsCoreEffect() const { return _coreEffect; }

// RequiresDoubleBuffering
//
// If a matrix effect requires the state of the last buffer be preserved, then it requires double buffering.
// If, on the other hand, it renders from scratch every time, starting with a black fill, etc., then it does not,
// and it can override this method and return false;

bool LEDStripEffect::RequiresDoubleBuffering() const { return true; }

// Lazily loads the SettingsSpecs for this effect if they haven't been loaded yet, and
// returns a vector with reference_wrappers to them.
const std::vector<std::reference_wrapper<SettingSpec>>& LEDStripEffect::GetSettingSpecs()
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
bool LEDStripEffect::SerializeSettingsToJSON(JsonObject& jsonObject)
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
bool LEDStripEffect::SetSetting(const String& name, const String& value)
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

float LEDStripEffect::fmap(const float x, const float in_min, const float in_max, const float out_min, const float out_max)
{
    return (out_max - out_min) * (x - in_min) / (in_max - in_min) + out_min;
}

namespace {
    template <typename TProperty, typename TValue>
    bool SetIfNameMatches(const String& firstName, const String& secondName, TProperty& property, const TValue& value)
    {
        if (firstName == secondName)
        {
            property = value;
            return true;
        }
        return false;
    }
}

// Helper functions for known setting types, as defined in SettingSpec::SettingType

bool LEDStripEffect::SetIfSelected(const String& settingName, const String& propertyName, int& property, const String& value)
{
    return SetIfNameMatches(settingName, propertyName, property, value.toInt());
}

bool LEDStripEffect::SetIfSelected(const String& settingName, const String& propertyName, size_t& property, const String& value)
{
    return SetIfNameMatches(settingName, propertyName, property, (size_t)strtoul(value.c_str(), nullptr, 10));
}

bool LEDStripEffect::SetIfSelected(const String& settingName, const String& propertyName, float& property, const String& value)
{
    return SetIfNameMatches(settingName, propertyName, property, value.toFloat());
}

bool LEDStripEffect::SetIfSelected(const String& settingName, const String& propertyName, bool& property, const String& value)
{
    return SetIfNameMatches(settingName, propertyName, property, BoolFromText(value));
}

bool LEDStripEffect::SetIfSelected(const String& settingName, const String& propertyName, String& property, const String& value)
{
    return SetIfNameMatches(settingName, propertyName, property, value);
}

bool LEDStripEffect::SetIfSelected(const String& settingName, const String& propertyName, CRGBPalette16& property, const String& value)
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

bool LEDStripEffect::SetIfSelected(const String& settingName, const String& propertyName, CRGB& property, const String& value)
{
    return SetIfNameMatches(settingName, propertyName, property, CRGB(strtoul(value.c_str(), NULL, 10)));
}

namespace _effect_id_detail {
    void debug_log_effect_id(const char* token, EffectId id)
    {
        debugI("Effect ID token string: %s", token);
        debugI("Effect ID hash: 0x%08lx", static_cast<unsigned long>(id));
    }
}
