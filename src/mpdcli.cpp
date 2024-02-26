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

#include <M5EPD.h>

#include "config.h"
#include "mpdcli.h"
#include "utils.h"
#include "wifi_utils.h"

static MPD_Client _mpd;
MPD_Client& mpd = _mpd;

String MPD_Client::show_player(MPD_PLAYER& player)
{
    return ("Player: " + String(player.player_name));
}

StatusLines& MPD_Client::toggle_mpd_status()
{
    if (start_wifi()) {
        auto player = Config.get_active_mpd();
        this->status.clear();
        this->status.push_back(show_player(player));
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
    } else {
        this->status.clear();
        return this->status;
    }
}

StatusLines& MPD_Client::show_mpd_status()
{
    this->status.clear();
    this->status.push_back(get_date_time());
    this->status.push_back(get_status());
    this->status.push_back(" ");
    if (start_wifi()) {
        auto player = Config.get_active_mpd();
        this->status.push_back(show_player(player));
        if (this->con.Connect(player.player_ip, player.player_port)) {
            this->appendStatus(this->con.GetResponse());
            this->playing = this->con.GetStatus();
            this->appendStatus(this->con.GetResponse());
            this->con.GetCurrentSong();
            this->appendStatus(this->con.GetResponse());
            // attempt to capture the "Alsa underrun sending silence" error message
            // but unfortunately it looks like mpd does only log the message, it never gets here
            if (this->playing && (this->con.GetLastError().find("silence") != string::npos)) {
                this->status.push_back("*ALSA XRUN -> restarting play");
                this->con.Stop();
                this->con.Play();
            }
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
        this->status.push_back(show_player(player));
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

string MPD_Client::GetLastError()
{
    return this->con.GetLastError();
}
