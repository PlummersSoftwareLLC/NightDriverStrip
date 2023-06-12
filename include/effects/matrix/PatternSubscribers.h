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

#include <UrlEncode.h>
#include "deviceconfig.h"
#include "YouTubeSight.h"

// Update subscribers every 30 minutes, retry after 30 seconds on error, and check other things every 5 seconds
#define SUB_CHECK_INTERVAL          (30 * 60000)
#define SUB_CHECK_ERROR_INTERVAL    30000
#define SIGHT_READER_INTERVAL       5000

#define DEFAULT_CHANNEL_GUID "9558daa1-eae8-482f-8066-17fa787bc0e4"
#define DEFAULT_CHANNEL_NAME "Daves Garage"

class PatternSubscribers : public LEDStripEffect
{
  private:
    long subscribers                        = 0;
    long views                              = 0;
    String youtubeChannelGuid               = DEFAULT_CHANNEL_GUID;
    String youtubeChannelName               = DEFAULT_CHANNEL_NAME;
    bool guidUpdated                        = true;
    std::vector<SettingSpec> mySettingSpecs;
    size_t readerIndex                      = std::numeric_limits<size_t>::max();

    unsigned long msLastCheck;
    bool succeededBefore                    = false;

    time_t latestUpdate                     = 0;

    WiFiClient http;
    std::unique_ptr<YouTubeSight> sight = nullptr;

    void SightReader()
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

        msLastCheck = millis();

        if (!sight || guidUpdated)
        {
            sight = std::make_unique<YouTubeSight>(urlEncode(youtubeChannelGuid), http);
            succeededBefore = false;
        }

        guidUpdated = false;

        // Use the YouTubeSight API call to get the current channel stats
        if (sight->getData())
        {
            debugI("Got YouTube subscriber data");
            subscribers = atol(sight->channelStats.subscribers_count.c_str());
            views       = atol(sight->channelStats.views.c_str());
            succeededBefore = true;
        }
        else
        {
            debugW("YouTubeSight Subscriber API failed\n");
        }
    }

  protected:

    virtual bool FillSettingSpecs() override
    {
        if (!LEDStripEffect::FillSettingSpecs())
            return false;

        mySettingSpecs.emplace_back(
            NAME_OF(youtubeChannelGuid),
            "YouTube channel GUID",
            "The <a href=\"http://tools.tastethecode.com/youtube-sight\">YouTube Sight</a> GUID of the channel for which "
            "the effect should show subscriber information.",
            SettingSpec::SettingType::String
        );
        mySettingSpecs.emplace_back(
            NAME_OF(youtubeChannelName),
            "YouTube channel name",
            "The name of the channel for which the effect should show subscriber information.",
            SettingSpec::SettingType::String
        );

        _settingSpecs.insert(_settingSpecs.end(), mySettingSpecs.begin(), mySettingSpecs.end());

        return true;
    }

  public:

    PatternSubscribers() : LEDStripEffect(EFFECT_MATRIX_SUBSCRIBERS, "Subs")
    {
    }

    PatternSubscribers(const JsonObjectConst& jsonObject) : LEDStripEffect(jsonObject)
    {
        if (jsonObject.containsKey("ycg"))
            youtubeChannelGuid = jsonObject["ycg"].as<String>();

        if (jsonObject.containsKey("ycn"))
            youtubeChannelName = jsonObject["ycn"].as<String>();
    }

    ~PatternSubscribers()
    {
        g_ptrNetworkReader->CancelReader(readerIndex);
    }

    virtual bool SerializeToJSON(JsonObject& jsonObject) override
    {
        StaticJsonDocument<256> jsonDoc;

        JsonObject root = jsonDoc.to<JsonObject>();
        LEDStripEffect::SerializeToJSON(root);

        jsonDoc["ycg"] = youtubeChannelGuid;
        jsonDoc["ycn"] = youtubeChannelName;

        return jsonObject.set(jsonDoc.as<JsonObjectConst>());
    }

    virtual bool RequiresDoubleBuffering() const override
    {
        return false;
    }

    virtual bool Init(std::shared_ptr<GFXBase> gfx[NUM_CHANNELS]) override
    {
        if (!LEDStripEffect::Init(gfx))
            return false;

        readerIndex = g_ptrNetworkReader->RegisterReader([this]() { SightReader(); }, SIGHT_READER_INTERVAL, true);

        return true;
    }

    virtual void Draw() override
    {
        LEDMatrixGFX::backgroundLayer.fillScreen(rgb24(0, 16, 64));
        LEDMatrixGFX::backgroundLayer.setFont(font5x7);

        // Draw a border around the edge of the panel
        LEDMatrixGFX::backgroundLayer.drawRectangle(0, 1, MATRIX_WIDTH-1, MATRIX_HEIGHT-2, rgb24(160,160,255));

        // Draw the channel name
        LEDMatrixGFX::backgroundLayer.drawString(2, 3, rgb24(255,255,255), youtubeChannelName.c_str());

        // Start in the middle of the panel and then back up a half a row to center vertically,
        // then back up left one half a char for every 10s digit in the subscriber count.  This
        // shoud center the number on the screen

        const int CHAR_WIDTH = 6;
        const int CHAR_HEIGHT = 7;
        int x = MATRIX_WIDTH / 2 - CHAR_WIDTH / 2;
        int y = MATRIX_HEIGHT / 2 - CHAR_HEIGHT / 2 - 3;        // -3 to put it above the caption
        long z = subscribers;                                  // Use a long in case of Mr Beast

        while (z/=10)
          x-= CHAR_WIDTH / 2;

        String result = str_sprintf("%ld", subscribers);
        const char * pszText = result.c_str();

        LEDMatrixGFX::backgroundLayer.setFont(gohufont11b);
        LEDMatrixGFX::backgroundLayer.drawString(x-1, y,   rgb24(0,0,0),          pszText);
        LEDMatrixGFX::backgroundLayer.drawString(x+1, y,   rgb24(0,0,0),          pszText);
        LEDMatrixGFX::backgroundLayer.drawString(x,   y-1, rgb24(0,0,0),          pszText);
        LEDMatrixGFX::backgroundLayer.drawString(x,   y+1, rgb24(0,0,0),          pszText);
        LEDMatrixGFX::backgroundLayer.drawString(x,   y,   rgb24(255,255,255),    pszText);
    }

    virtual bool SerializeSettingsToJSON(JsonObject& jsonObject) override
    {
        StaticJsonDocument<384> jsonDoc;
        auto rootObject = jsonDoc.to<JsonObject>();

        LEDStripEffect::SerializeSettingsToJSON(jsonObject);

        jsonDoc[NAME_OF(youtubeChannelGuid)] = youtubeChannelGuid;
        jsonDoc[NAME_OF(youtubeChannelName)] = youtubeChannelName;

        return jsonObject.set(jsonDoc.as<JsonObjectConst>());
    }

    virtual bool SetSetting(const String& name, const String& value) override
    {
        RETURN_IF_SET(name, NAME_OF(youtubeChannelGuid), youtubeChannelGuid, value);
        RETURN_IF_SET(name, NAME_OF(youtubeChannelName), youtubeChannelName, value);

        return LEDStripEffect::SetSetting(name, value);
    }
};

#endif
