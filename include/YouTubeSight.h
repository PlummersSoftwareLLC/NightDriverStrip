//+--------------------------------------------------------------------------
//
// File:        YouTubeSight.h 
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

#ifndef YouTubeSight_h
#define YouTubeSight_h

#include "Arduino.h"
#include "Client.h"

#define YOUTUBE_SIGHT_URL "tools.tastethecode.com"
#define TIMEOUT 1500

struct channelStatistics {
    String views;
    String subscribers_gained;
    String subscribers_lost;
    String subscribers_count;
    String estimated_minutes_watched;
    String average_view_duration;
};

class YouTubeSight
{
    public:
        YouTubeSight(String guid, Client &client);
        channelStatistics channelStats;
        bool getData();
        bool _debug = false;

    private:
        String _guid;
        Client *client;
        String getValue(String data, char separator, int index);
};

#endif
