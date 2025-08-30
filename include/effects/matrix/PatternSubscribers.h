
//+--------------------------------------------------------------------------
//
// File:        PatternSubscribers.h
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
//   Effect code ported from Aurora to Mesmerizer's draw routines
//
// History:     Jun-25-202         Davepl      Adapted from own code
//
//---------------------------------------------------------------------------

#ifndef PatternSub_H
#define PatternSub_H

#if USE_HUB75

#include <UrlEncode.h>
#include "systemcontainer.h"

// Update subscribers every 30 minutes, retry after 30 seconds on error, and check other things every 5 seconds
#define SUB_CHECK_INTERVAL          (30 * 60000)
#define SUB_CHECK_ERROR_INTERVAL    30000
#define SUB_READER_INTERVAL         5000

#define DEFAULT_CHANNEL_GUID "9558daa1-eae8-482f-8066-17fa787bc0e4"
#define DEFAULT_CHANNEL_NAME "Daves Garage"

class PatternSubscribers : public EffectWithId<PatternSubscribers>
{
  private:

    // This requires a matching INIT_EFFECT_SETTING_SPECS() in effects.cpp or linker errors will ensue
    DECLARE_EFFECT_SETTING_SPECS(mySettingSpecs);

    long subscribers                        = 0;
    String youtubeChannelGuid               = DEFAULT_CHANNEL_GUID;
    String youtubeChannelName               = DEFAULT_CHANNEL_NAME;
    CRGB backgroundColor                    = CRGB(0,16,64);
    CRGB borderColor                        = CRGB(160,160,255);
    bool guidUpdated                        = true;

    size_t readerIndex                      = SIZE_MAX;

    unsigned long msLastCheck;
    bool succeededBefore                    = false;

    time_t latestUpdate                     = 0;

    void SubscriberReader()
    {
        unsigned long msSinceLastCheck = millis() - msLastCheck;

        if (guidUpdated || !msLastCheck
            || (!succeededBefore && msSinceLastCheck > SUB_CHECK_ERROR_INTERVAL)
            || msSinceLastCheck > SUB_CHECK_INTERVAL)
        {
            UpdateSubscribers();
        }
    }

    void UpdateSubscribers()
    {
        if (!WiFi.isConnected())
        {
            debugW("Skipping Subscriber update, waiting for WiFi...");
            return;
        }

        if (youtubeChannelGuid.isEmpty())
        {
            debugW("No YouTube subscriber guid, so skipping check...");
            return;
        }

        msLastCheck = millis();

        if (guidUpdated)
            succeededBefore = false;

        guidUpdated = false;

        HTTPClient http;

        http.begin("http://tools.tastethecode.com/api/youtube-sight/" + youtubeChannelGuid);
        int httpResponseCode = http.GET();

        if (httpResponseCode <= 0)
        {
            debugW("Error fetching subscribers for channel %s (GUID %s)", youtubeChannelName.c_str(), youtubeChannelGuid.c_str());
            http.end();
        }

        String response = http.getString();
        int commaIndex = -1;
        int startIndex;

        for (int i = 0; i < 4; i++)
        {
            startIndex = commaIndex + 1;
            commaIndex = response.indexOf(',', startIndex);
            if (commaIndex < 0)
            {
                debugW("Malformed response while fetching subscribers for channel %s (GUID %s)", youtubeChannelName.c_str(), youtubeChannelGuid.c_str());
                return;
            }
        }

        subscribers = response.substring(startIndex, commaIndex).toInt();

        debugI("Got YouTube subscriber count for channel %s (GUID %s)", youtubeChannelName.c_str(), youtubeChannelGuid.c_str());

        succeededBefore = true;
    }

  protected:

    // Create our SettingSpec instances if needed, and return (a pointer to) them
    EffectSettingSpecs* FillSettingSpecs() override
    {
        // Lazily load this class' SettingSpec instances if they haven't been already
        if (mySettingSpecs.size() == 0)
        {
            mySettingSpecs.emplace_back(
                NAME_OF(youtubeChannelGuid),
                "YouTube channel GUID",
                "The <a href=\"http://tools.tastethecode.com/youtube-sight\">YouTube Sight</a> GUID of the channel for which "
                "the effect should show subscriber information.",
                SettingSpec::SettingType::String
            );
            mySettingSpecs.emplace_back(NAME_OF(backgroundColor), "Background Color",
                                        "Color for the background",
                                        SettingSpec::SettingType::Color);
            mySettingSpecs.emplace_back(NAME_OF(borderColor), "Border Color",
                                        "Color for the border around the edge", SettingSpec::SettingType::Color);
            mySettingSpecs
                .emplace_back(NAME_OF(youtubeChannelName), "YouTube channel name",
                              "The name of the channel for which the effect should show subscriber information.",
                              SettingSpec::SettingType::String)
                .EmptyAllowed = true;
        }

        return &mySettingSpecs;
    }

  public:

    PatternSubscribers() : EffectWithId<PatternSubscribers>("Subs") {}
    PatternSubscribers(const JsonObjectConst& jsonObject) : EffectWithId<PatternSubscribers>(jsonObject)
    {
        if (jsonObject["ycg"].is<String>())
            youtubeChannelGuid = jsonObject["ycg"].as<String>();

        if (jsonObject["ycn"].is<String>())
            youtubeChannelName = jsonObject["ycn"].as<String>();
        if (jsonObject["bgc"].is<CRGB>())
            backgroundColor = jsonObject["bgc"].as<CRGB>();
        if (jsonObject["boc"].is<CRGB>())
            borderColor = jsonObject["boc"].as<CRGB>();
    }

    ~PatternSubscribers()
    {
        g_ptrSystem->NetworkReader().CancelReader(readerIndex);
    }

    bool SerializeToJSON(JsonObject& jsonObject) override
    {
        auto jsonDoc = CreateJsonDocument();

        JsonObject root = jsonDoc.to<JsonObject>();
        LEDStripEffect::SerializeToJSON(root);

        jsonDoc["ycg"] = youtubeChannelGuid;
        jsonDoc["ycn"] = youtubeChannelName;
        jsonDoc["bgc"] = backgroundColor;
        jsonDoc["boc"] = borderColor;

        return SetIfNotOverflowed(jsonDoc, jsonObject, __PRETTY_FUNCTION__);
    }

    bool RequiresDoubleBuffering() const override
    {
        return true;            // BUGBUG Flickers without this, but should NOT need it?
    }

    bool Init(std::vector<std::shared_ptr<GFXBase>>& gfx) override
    {
        if (!LEDStripEffect::Init(gfx))
            return false;

        readerIndex = g_ptrSystem->NetworkReader().RegisterReader([this] { SubscriberReader(); }, SUB_READER_INTERVAL, true);

        return true;
    }

    void Draw() override
    {
        SmartMatrixGFX::backgroundLayer.fillScreen(rgb24(backgroundColor.r, backgroundColor.g, backgroundColor.b));
        SmartMatrixGFX::backgroundLayer.setFont(font5x7);

        // Draw a border around the edge of the panel
        SmartMatrixGFX::backgroundLayer.drawRectangle(0, 1, MATRIX_WIDTH - 1, MATRIX_HEIGHT - 2,
                                                    rgb24(borderColor.r, borderColor.g, borderColor.b));

        // Draw the channel name
        SmartMatrixGFX::backgroundLayer.drawString(2, 3, rgb24(255,255,255), youtubeChannelName.c_str());

        // Start in the middle of the panel and then back up a half a row to center vertically,
        // then back up left one half a char for every 10s digit in the subscriber count.  This
        // should center the number on the screen

        const int CHAR_WIDTH = 6;
        const int CHAR_HEIGHT = 7;
        int x = MATRIX_WIDTH / 2 - CHAR_WIDTH / 2;
        int y = MATRIX_HEIGHT / 2 - CHAR_HEIGHT / 2 - 3;        // -3 to put it above the caption
        long z = subscribers;                                  // Use a long in case of Mr Beast

        while (z/=10)
          x-= CHAR_WIDTH / 2;

        String result = str_sprintf("%ld", subscribers);
        const char * pszText = result.c_str();

        SmartMatrixGFX::backgroundLayer.setFont(gohufont11b);
        SmartMatrixGFX::backgroundLayer.drawString(x-1, y,   rgb24(0,0,0),          pszText);
        SmartMatrixGFX::backgroundLayer.drawString(x+1, y,   rgb24(0,0,0),          pszText);
        SmartMatrixGFX::backgroundLayer.drawString(x,   y-1, rgb24(0,0,0),          pszText);
        SmartMatrixGFX::backgroundLayer.drawString(x,   y+1, rgb24(0,0,0),          pszText);
        SmartMatrixGFX::backgroundLayer.drawString(x,   y,   rgb24(255,255,255),    pszText);
    }

    // Extension override to serialize our settings on top of those from LEDStripEffect
    bool SerializeSettingsToJSON(JsonObject& jsonObject) override
    {
        auto jsonDoc = CreateJsonDocument();

        JsonObject root = jsonDoc.to<JsonObject>();
        LEDStripEffect::SerializeSettingsToJSON(root);

        jsonDoc[NAME_OF(youtubeChannelGuid)] = youtubeChannelGuid;
        jsonDoc[NAME_OF(youtubeChannelName)] = youtubeChannelName;
        jsonDoc[NAME_OF(backgroundColor)] = backgroundColor;
        jsonDoc[NAME_OF(borderColor)] = borderColor;

        return SetIfNotOverflowed(jsonDoc, jsonObject, __PRETTY_FUNCTION__);
    }

    // Extension override to accept our settings on top of those known by LEDStripEffect
    bool SetSetting(const String& name, const String& value) override
    {
        RETURN_IF_SET(name, NAME_OF(youtubeChannelGuid), youtubeChannelGuid, value);
        RETURN_IF_SET(name, NAME_OF(youtubeChannelName), youtubeChannelName, value);
        RETURN_IF_SET(name, NAME_OF(backgroundColor), backgroundColor, value);
        RETURN_IF_SET(name, NAME_OF(borderColor), borderColor, value);

        return LEDStripEffect::SetSetting(name, value);
    }
};

#endif
#endif
