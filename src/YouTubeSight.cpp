//+--------------------------------------------------------------------------
//
// File:        YouTubeSight.cpp
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
//   Code that samples analog input and can run an FFT on it for stats,
//   and 
//
// History:     July-19-2022    Davepl      Header updated
//
//---------------------------------------------------------------------------

/*
    YouTube Sight - Arduino library for using YouTube Sight service at https://tools.tastethecode.com/youtube-sight

    Author: Blagojce Kolicoski - bkolicoski@gmail.com
    https://www.youtube.com/tastethecode

    This library is provided for free without any warranty.

    A lot of the library is inspired by the work of Brian Lough from the Arduino YouTube API library
    https://github.com/witnessmenow/arduino-youtube-api     
*/

#include "YouTubeSight.h"

YouTubeSight::YouTubeSight(String guid, Client &client)
{
    _guid = guid;
    if (_debug) { Serial.println("GUID: " + guid); }
    this->client = &client;
}

bool YouTubeSight::getData()
{
    String headers = "";
	String body = "";
    bool finishedHeaders = false;
    bool currentLineIsBlank = true;
    unsigned long now;
    bool avail = false;
    String command = "GET /api/youtube-sight/" + _guid + " HTTP/1.1";
    if (client->connect(YOUTUBE_SIGHT_URL, 80)) {
        if(_debug) {
            Serial.println("Connected to server");
            Serial.println(YOUTUBE_SIGHT_URL);
            Serial.println("Command");
            Serial.println(command);
        }
        client->println(command);
        client->print("HOST: ");
		client->println(YOUTUBE_SIGHT_URL);
        client->println();
        now = millis();
        while (millis() - now < TIMEOUT) {
            while (client->available()) {
                avail = finishedHeaders;
                String line = client->readStringUntil('\n');
				if(_debug) {
					Serial.println(line);
				}
                line.trim();
                if (!finishedHeaders) {
                    if (currentLineIsBlank && line.length() == 2) {
                        finishedHeaders = true;
                    }
                    else {
                        headers = headers + line;
                    }
                } else {
                    if (line != "0" && line != "") {
                        body = body + line;
                    }
                }

                if (line.length() == 0) {
                    currentLineIsBlank = true;
                }else if (line != "\r") {
                    currentLineIsBlank = false;
                }
            }

            if (avail) {
                if(_debug) {
                    Serial.println("Headers:");
                    Serial.println(headers);
                    Serial.println("END Headers");
                    Serial.println("Body:");
                    Serial.println(body);
                    Serial.println("END");
                }
                break;
            }
        }
    } else {
        if(_debug) {
            Serial.print("disconnected: ");
            Serial.println(YOUTUBE_SIGHT_URL);
        }
    }
    if(client->connected()) {
        client->stop();
    }

    if (body != "") {
        if(_debug) { Serial.println("parsing body"); }
        channelStats.views = getValue(body, ',', 0);
        channelStats.subscribers_gained = getValue(body, ',', 1);
        channelStats.subscribers_lost = getValue(body, ',', 2);
        channelStats.subscribers_count = getValue(body, ',', 3);
        channelStats.estimated_minutes_watched = getValue(body, ',', 4);
        channelStats.average_view_duration = getValue(body, ',', 5);

        return true;
    }
	
	if(_debug) {
		Serial.print("exit - body: ");
		Serial.println(body);
		Serial.print("exit - headers: ");
		Serial.println(headers);
	}

    return false;
}

//function to get a specific part of the combined string response
String YouTubeSight::getValue(String data, char separator, int index)
{
  int found = 0;
  int strIndex[] = {0, -1};
  int maxIndex = data.length()-1;

  for(int i=0; i<=maxIndex && found<=index; i++){
    if(data.charAt(i)==separator || i==maxIndex){
        found++;
        strIndex[0] = strIndex[1]+1;
        strIndex[1] = (i == maxIndex) ? i+1 : i;
    }
  }

  return found>index ? data.substring(strIndex[0], strIndex[1]) : "";
}
