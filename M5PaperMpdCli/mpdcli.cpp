// Copyright (c) 2023 @dheijl (danny.heijl@telenet.be)
//
// Permission is hereby granted, free of charge, to any person obtaining a copy of
// this software and associated documentation files (the "Software"), to deal in
// the Software without restriction, including without limitation the rights to
// use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
// the Software, and to permit persons to whom the Software is furnished to do so,
// subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
// FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
// COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
// IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
// CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

#include "config.h"

#include "mpdcli.h"
#include "tftfunctions.h"
#include "wifi.h"

#include <M5EPD.h>

static MPD_Client _mpd;
MPD_Client& mpd = _mpd;

StatusLines& MPD_Client::show_player(MPD_PLAYER& player)
{
    this->status.push_back("Player: " + String(player.player_name));
    return status;
}

StatusLines& MPD_Client::toggle_mpd_status()
{
    if (start_wifi()) {
        auto player = Config.get_active_mpd();
        this->status.clear();
        show_player(player);
        if (this->con.Connect(player.player_ip, player.player_port)) {
            this->appendStatus(this->con.GetResponse());
            if (this->con.IsPlaying()) {
                this->appendStatus(this->con.GetResponse());
                this->status.push_back("Stop playing");
                this->appendStatus(this->con.GetResponse());
                this->con.Stop();
                this->appendStatus(this->con.GetResponse());
            } else {
                this->status.push_back("Start playing");
                this->con.Play();
                this->appendStatus(this->con.GetResponse());
            }
            this->con.Disconnect();
            this->appendStatus(this->con.GetResponse());
        }
        return this->status;
    }
}

StatusLines& MPD_Client::show_mpd_status()
{
    float bat_volt = (float)(M5.getBatteryVoltage() - 3200) / 1000.0f;
    int bat_level = (int)(((float)bat_volt / 1.05f) * 100);
    auto heap = ESP.getFreeHeap() / 1024;
    auto psram = ESP.getFreePsram() / (1024 * 1024);
    this->status.clear();
    rtc_date_t RTCDate;
    M5.RTC.getDate(&RTCDate);
    rtc_time_t RTCTime;
    M5.RTC.getTime(&RTCTime);
    char datebuf[64];
    snprintf(datebuf, 64, "%04d:%02d:%02d", RTCDate.year, RTCDate.mon, RTCDate.day);
    char timebuf[64];
    snprintf(timebuf, 64, "%02d:%02d:%02d", RTCTime.hour, RTCTime.min, RTCTime.sec);
    this->status.push_back(String(datebuf) + " - " + String(timebuf));
    this->status.push_back("Batt=" + String(bat_level) + "%,H=" + String(heap) + "K,PS=" + String(psram) + "M");
    if (start_wifi()) {
        auto player = Config.get_active_mpd();
        show_player(player);
        if (this->con.Connect(player.player_ip, player.player_port)) {
            this->appendStatus(this->con.GetResponse());
            this->playing = this->con.GetStatus();
            this->appendStatus(this->con.GetResponse());
            this->con.GetCurrentSong();
            this->appendStatus(this->con.GetResponse());
            this->con.Disconnect();
            this->appendStatus(this->con.GetResponse());
        }
    }
    return this->status;
}

StatusLines& MPD_Client::play_favourite(const FAVOURITE& fav)
{
    if (start_wifi()) {
        this->status.clear();
        auto player = Config.get_active_mpd();
        show_player(player);
        this->status.push_back("Play " + String(fav.fav_name));
        if (this->con.Connect(player.player_ip, player.player_port)) {
            this->appendStatus(this->con.GetResponse());
            this->con.Clear();
            this->appendStatus(this->con.GetResponse());
            this->con.Add_Url(fav.fav_url);
            this->appendStatus(this->con.GetResponse());
            this->con.Play();
            this->appendStatus(this->con.GetResponse());
            this->con.Disconnect();
            this->appendStatus(this->con.GetResponse());
        }
    }
    return this->status;
}

bool MPD_Client::is_playing()
{
    return this->playing;
}
