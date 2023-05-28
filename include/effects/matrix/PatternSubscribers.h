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

#define SUB_CHECK_WIFI_WAIT 5000
#define SUB_CHECK_INTERVAL 60000
#define SUB_CHECK_GUID_INTERVAL 5000
#define SUB_CHECK_ERROR_INTERVAL 20000

#define DEFAULT_CHANNEL_GUID "9558daa1-eae8-482f-8066-17fa787bc0e4"
#define DEFAULT_CHANNEL_NAME "Daves Garage"

class PatternSubscribers : public LEDStripEffect
{
  private:
    long subscribers          = 0;
    long views                = 0;
    String youtubeChannelGuid = DEFAULT_CHANNEL_GUID;
    String youtubeChannelName = DEFAULT_CHANNEL_NAME;
    bool guidUpdated          = true;

    unsigned long millisLastCheck;
    bool succeededBefore    = false;

    TaskHandle_t sightTask   = nullptr;
    time_t latestUpdate      = 0;

    WiFiClient http;
    std::unique_ptr<YouTubeSight> sight = nullptr;

    // Thread entry point so we can update the subscriber data asynchronously
    static void SightTaskEntryPoint(LEDStripEffect& effect)
    {
        PatternSubscribers& subsEffect = static_cast<PatternSubscribers&>(effect);

        for(;;)
        {
            unsigned long millisSinceLastCheck = millis() - subsEffect.millisLastCheck;

            if (subsEffect.guidUpdated || !subsEffect.millisLastCheck
                || (!subsEffect.succeededBefore && millisSinceLastCheck > SUB_CHECK_ERROR_INTERVAL)
                || millisSinceLastCheck > SUBCHECK_INTERVAL)
            {
                subsEffect.UpdateSubscribers();
            }

            // Sleep for a few seconds before we recheck if the GUID has changed
            vTaskDelay(SUB_CHECK_GUID_INTERVAL / portTICK_PERIOD_MS);
        }
    }

    void UpdateSubscribers()
    {
        while(!WiFi.isConnected())
        {
            debugI("Delaying Subscriber update, waiting for WiFi...");
            vTaskDelay(pdMS_TO_TICKS(SUB_CHECK_WIFI_WAIT));
        }

        millisLastCheck = millis();

        if (!sight || guidUpdated)
        {
            sight = std::make_unique<YouTubeSight>(urlEncode(youtubeChannelGuid), http);
            succeededBefore = false;
        }

        guidUpdated = false;

        // Use the YouTubeSight API call to get the current channel stats
        if (sight->getData())
        {
            subscribers = atol(sight->channelStats.subscribers_count.c_str());
            views       = atol(sight->channelStats.views.c_str());
            succeededBefore = true;
        }
        else
        {
            debugW("YouTubeSight Subscriber API failed\n");
        }
    }

    void construct()
    {
        _settingSpecs.emplace_back(
            NAME_OF(youtubeChannelGuid),
            "YouTube channel GUID",
            "The <a href=\"http://tools.tastethecode.com/youtube-sight\">YouTube Sight</a> GUID of the channel for which "
            "the effect should show subscriber information.",
            SettingSpec::SettingType::String
        );
        _settingSpecs.emplace_back(
            NAME_OF(youtubeChannelName),
            "YouTube channel name",
            "The name of the channel for which the effect should show subscriber information.",
            SettingSpec::SettingType::String
        );
    }

  public:

    PatternSubscribers() : LEDStripEffect(EFFECT_MATRIX_SUBSCRIBERS, "Subs")
    {
        construct();
    }

    PatternSubscribers(const JsonObjectConst& jsonObject) : LEDStripEffect(jsonObject)
    {
        if (jsonObject.containsKey("ycg"))
            youtubeChannelGuid = jsonObject["ycg"].as<String>();

        if (jsonObject.containsKey("ycn"))
            youtubeChannelName = jsonObject["ycn"].as<String>();

        construct();
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

        sightTask = g_TaskManager.StartEffectThread(SightTaskEntryPoint, this, "Subs");

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

    virtual bool HasSettings() const override
    {
        return true;
    }

    virtual bool SerializeSettingsToJSON(JsonObject& jsonObject) override
    {
        StaticJsonDocument<256> jsonDoc;

        jsonDoc[NAME_OF(youtubeChannelGuid)] = youtubeChannelGuid;
        jsonDoc[NAME_OF(youtubeChannelName)] = youtubeChannelName;

        return jsonObject.set(jsonDoc.as<JsonObjectConst>());
    }

    virtual bool SetSetting(const String& name, const String& value) override
    {
        RETURN_IF_SET(name, NAME_OF(youtubeChannelGuid), youtubeChannelGuid, value);
        return SetIfSelected(name, NAME_OF(youtubeChannelName), youtubeChannelName, value);
    }
};

#endif
