//+--------------------------------------------------------------------------
//
// File:        gfxbase_palette.cpp
//
// This file is part of gfxbase.cpp; see that file header for additional context.
//
// Split scope: GFXBase palette selection, cycling, listing, and palette setup helpers.
//---------------------------------------------------------------------------


#include "globals.h"

#include <algorithm>
#include <cmath>
#include <gfxfont.h>
#include <memory>
#include <stdexcept>
#include <unordered_map>

#include "effectmanager.h"
#include "effects/matrix/Boid.h"
#include "gfxbase.h"
#include "systemcontainer.h"

void GFXBase::CyclePalette(int offset)
{
    loadPalette(_paletteIndex + offset);
}

void GFXBase::ChangePalettePeriodically()
{
    if (_palettePaused)
        return;

    const int minutesPerPaletteCycle = 2;
    uint8_t secondHand = ((millis() / minutesPerPaletteCycle) / 1000) % 60;

    if (_lastSecond != secondHand)
    {
        _lastSecond = secondHand;
        if (secondHand == 0)
        {
            _targetPalette = RainbowColors_p;
        }
        if (secondHand == 10)
        {
            _targetPalette = HeatColors_p;
        }
        if (secondHand == 20)
        {
            _targetPalette = ForestColors_p;
        }
        if (secondHand == 30)
        {
            _targetPalette = LavaColors_p;
        }
        if (secondHand == 40)
        {
            _targetPalette = CloudColors_p;
        }
        if (secondHand == 50)
        {
            _targetPalette = PartyColors_p;
        }
    }
}

// Cross-fade current palette slowly toward the target palette
//
// Each time that nblendPaletteTowardPalette is called, small changes
// are made to currentPalette to bring it closer to matching targetPalette.
// You can control how many changes are made in each call:
//   - the default of 24 is a good balance
//   - meaningful values are 1-48.  1=very very slow, 48=quickest
//   - "0" means do not change the currentPalette at all; freeze

void GFXBase::PausePalette(bool bPaused)
{
    _palettePaused = bPaused;
}

void GFXBase::UpdatePaletteCycle()
{
    ChangePalettePeriodically();
    uint8_t maxChanges = 24;
    nblendPaletteTowardPalette(_currentPalette, _targetPalette, maxChanges);
}

void GFXBase::RandomPalette()
{
    loadPalette(_randomPaletteIndex);
}

void GFXBase::fillRectangle(int x0, int y0, int x1, int y1, CRGB color)
{
    for (int x = x0; x < x1; x++)
        for (int y = y0; y < y1; y++)
            drawPixel(x, y, color);
}

void GFXBase::setPalette(const CRGBPalette16& palette)
{
    _currentPalette = palette;
    _targetPalette = palette;
    _currentPaletteName = "Custom";
}

// loadPalette
//
// Note that this function may recurse without
// bound if your random() is very very dumb.

void GFXBase::loadPalette(int index)
{
    _paletteIndex = index;

    if (_paletteIndex >= _paletteCount)
        _paletteIndex = 0;
    else if (_paletteIndex < 0)
        _paletteIndex = _paletteCount - 1;

    switch (_paletteIndex)
    {
    case 0:
        _targetPalette = RainbowColors_p;
        _currentPaletteName = "Rainbow";
        break;
    case 1:
        _targetPalette = OceanColors_p;
        _currentPaletteName = "Ocean";
        break;
    case 2:
        _targetPalette = CloudColors_p;
        _currentPaletteName = "Cloud";
        break;
    case 3:
        _targetPalette = ForestColors_p;
        _currentPaletteName = "Forest";
        break;
    case 4:
        _targetPalette = PartyColors_p;
        _currentPaletteName = "Party";
        break;
    case 5:
        setupGrayscalePalette();
        _currentPaletteName = "Grey";
        break;
    case _heatColorsPaletteIndex:
        _targetPalette = HeatColors_p;
        _currentPaletteName = "Heat";
        break;
    case 7:
        _targetPalette = LavaColors_p;
        _currentPaletteName = "Lava";
        break;
    case 8:
        setupIcePalette();
        _currentPaletteName = "Ice";
        break;
    case _randomPaletteIndex:
        loadPalette(random(0, _paletteCount - 1));
        _paletteIndex = _randomPaletteIndex;
        _currentPaletteName = "Random";
        break;
    }
    _currentPalette = _targetPalette;
}

void GFXBase::setPalette(const String& paletteName)
{
    static const std::unordered_map<const char*, int> paletteMap = {
        {"Rainbow", 0},
        {"Ocean", 1},
        {"Cloud", 2},
        {"Forest", 3},
        {"Party", 4},
        {"Grayscale", 5},
        {"Heat", 6},
        {"Lava", 7},
        {"Ice", 8}
    };

    auto it = paletteMap.find(paletteName.c_str());
    if (it != paletteMap.end()) {
        loadPalette(it->second);
    } else if (paletteName == "Random") {
        RandomPalette();
    }
}

void GFXBase::listPalettes()
{
    Serial.println("{");
    Serial.print("  \"count\": ");
    Serial.print(_paletteCount);
    Serial.println(",");
    Serial.println("  \"results\": [");

    static constexpr const char* paletteNames[] =
    {
        "Rainbow", "Ocean", "Cloud", "Forest", "Party",
        "Grayscale", "Heat", "Lava", "Ice", "Random"
    };

    for (int i = 0; i < _paletteCount; i++)
    {
        Serial.print("    \"");
        Serial.print(paletteNames[i]);
        if (i == _paletteCount - 1)
            Serial.println("\"");
        else
            Serial.println("\",");
    }

    Serial.println("  ]");
    Serial.println("}");
}

void GFXBase::setupGrayscalePalette()
{
    _targetPalette = CRGBPalette16(CRGB::Black, CRGB::White);
}

void GFXBase::setupIcePalette()
{
    _targetPalette = CRGBPalette16(CRGB::Black, CRGB::Blue, CRGB::Aqua, CRGB::White);
}

