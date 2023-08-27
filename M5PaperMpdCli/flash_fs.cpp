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
#include <Preferences.h>

#include "flash_fs.h"

#include "epdfunctions.h"
#include "utils.h"

#include <string.h>
#include <string>

static const constexpr char* NVS_WIFI = "wifi";
static const constexpr char* NVS_PLAYERS = "players";
static const constexpr char* NVS_FAVS = "favs";
static const constexpr char* NVS_CUR_MPD = "curmpd";

bool NVS_Config::write_wifi(const NETWORK_CFG& nw_cfg)
{
    Preferences prefs;
    bool result = false;
    if (!prefs.begin(NVS_WIFI, false)) {
        epd_print_topline("wifi prefs begin error");
        prefs.end();
        vTaskDelay(2000);
        return result;
    }
    prefs.clear();
    result = true;
    DPRINT("wprefs: " + String(nw_cfg.ssid) + "|" + String(nw_cfg.psw));
    result = prefs.putString("ssid", nw_cfg.ssid) > 0;
    result = prefs.putString("psw", nw_cfg.psw) > 0;
    result = prefs.putString("ntp_server", nw_cfg.ntp_server) > 0;
    result = prefs.putString("tz", nw_cfg.tz) > 0;
    if (!result) {
        epd_print_topline("wifi prefs put error");
        vTaskDelay(2000);
    }
    prefs.end();
    return result;
}

bool NVS_Config::read_wifi(NETWORK_CFG& nw_cfg)
{
    Preferences prefs;
    if (!prefs.begin(NVS_WIFI, true)) {
        epd_print_topline("wifi prefs begin error");
        prefs.end();
        vTaskDelay(2000);
        return false;
    }
    String ssid = prefs.getString("ssid");
    String psw = prefs.getString("psw");
    String ntp_server = prefs.getString("ntp_server");
    String tz = prefs.getString("tz");
    prefs.end();
    DPRINT(ssid + "|" + psw);
    if (ssid.isEmpty() || psw.isEmpty()) {
        epd_print_topline("empty wifi prefs!");
        return false;
    }
    if (ntp_server.isEmpty() || tz.isEmpty()) {
        epd_print_topline("empty NTP prefs!");
        return false;
    }
    nw_cfg.ssid = strdup(ssid.c_str());
    nw_cfg.psw = strdup(psw.c_str());
    nw_cfg.ntp_server = strdup(ntp_server.c_str());
    nw_cfg.tz = strdup(tz.c_str());
    DPRINT("Wifi config: " + String(nw_cfg.ssid) + "|" + String(nw_cfg.psw));
    DPRINT("NTP config: " + String(nw_cfg.ntp_server) + "|" + String(nw_cfg.tz));
    return true;
}

bool NVS_Config::write_players(const PLAYERS& players)
{
    Preferences prefs;
    bool result = false;
    if (!prefs.begin(NVS_PLAYERS, false)) {
        epd_print_topline("players prefs begin error");
        prefs.end();
        vTaskDelay(2000);
        return result;
    }
    prefs.clear();
    result = true;
    int i = 0;
    for (auto pl : players) {
        string key = std::to_string(i);
        String data = String(pl->player_name) + "|" + String(pl->player_ip) + "|" + String(std::to_string(pl->player_port).c_str());
        if (prefs.putString(key.c_str(), data) == 0) {
            result = false;
            epd_print_topline("players prefs put error");
            vTaskDelay(2000);
            break;
        }
        DPRINT(data.c_str());
        ++i;
    }
    prefs.end();
    epd_print_topline("Saved " + String(i) + "players");
    return result;
}

bool NVS_Config::read_players(PLAYERS& players)
{
    Preferences prefs;
    bool result = false;
    if (!prefs.begin(NVS_PLAYERS, true)) {
        epd_print_topline("players prefs begin error");
        prefs.end();
        vTaskDelay(2000);
        return result;
    }
    int i = 0;
    while (true) {
        string key = std::to_string(i);
        String pl = prefs.getString(key.c_str());
        if (pl.isEmpty()) {
            break;
        } else {
            vector<string> parts = split(string(pl.c_str()), '|');
            if (parts.size() == 3) {
                auto mpd = new MPD_PLAYER();
                mpd->player_name = strdup(parts[0].c_str());
                mpd->player_ip = strdup(parts[1].c_str());
                mpd->player_port = stoi(parts[2]);
                players.push_back(mpd);
                epd_print_topline(String(mpd->player_name) + " " + String(mpd->player_ip) + ":" + String(mpd->player_port));
            }
        }
        DPRINT(pl);
        ++i;
    }
    if (i > 0) {
        result = true;
    }
    prefs.end();
    epd_print_topline("Loaded " + String(i) + " players");
    return result;
}

bool NVS_Config::write_favourites(const FAVOURITES& favourites)
{
    Preferences prefs;
    bool result = false;
    if (!prefs.begin("favs", false)) {
        epd_print_topline("favs prefs begin error");
        prefs.end();
        vTaskDelay(2000);
        return result;
    }
    prefs.clear();
    result = true;
    int i = 0;
    for (auto f : favourites) {
        string key = std::to_string(i);
        String data = String(f->fav_name) + "|" + String(f->fav_url);
        if (prefs.putString(key.c_str(), data) == 0) {
            result = false;
            epd_print_topline("favs prefs put error");
            vTaskDelay(2000);
            break;
        }
        DPRINT(data.c_str());
        ++i;
    }
    prefs.end();
    epd_print_topline("Saved " + String(i) + "favourites");
    return result;
}

bool NVS_Config::read_favourites(FAVOURITES& favourites)
{
    Preferences prefs;
    bool result = false;
    if (!prefs.begin("favs", true)) {
        epd_print_topline("favs prefs begin error");
        prefs.end();
        vTaskDelay(2000);
        return result;
    }
    int i = 0;
    while (true) {
        string key = std::to_string(i);
        String fav = prefs.getString(key.c_str());
        if (fav.isEmpty()) {
            break;
        }
        DPRINT(fav);
        vector<string> parts = split(string(fav.c_str()), '|');
        if (parts.size() == 2) {
            FAVOURITE* f = new FAVOURITE();
            f->fav_name = strdup(parts[0].c_str());
            f->fav_url = strdup(parts[1].c_str());
            favourites.push_back(f);
        }
        ++i;
    }
    if (i > 0) {
        result = true;
    }
    prefs.end();
    epd_print_topline("Loaded " + String(favourites.size()) + " favourites");
    return result;
}

void NVS_Config::write_player_index(uint16_t new_pl)
{
    Preferences prefs;
    if (!prefs.begin(NVS_CUR_MPD, false)) {
        epd_print_topline("cur_mpd prefs begin error");
        prefs.end();
        vTaskDelay(2000);
        return;
    }
    DPRINT("wprefs player: " + String(new_pl));
    Config.set_player_index(new_pl);
    bool result = prefs.putUShort("cur_mpd", new_pl) > 0;
    if (!result) {
        epd_print_topline("cur_mpd prefs put error");
        vTaskDelay(2000);
    }
    prefs.end();
}

bool NVS_Config::read_player_index()
{
    Preferences prefs;
    if (!prefs.begin(NVS_CUR_MPD, true)) {
        prefs.end();
        epd_print_topline("No cur_mpd prefs!");
        write_player_index(0);
        Config.set_player_index(0);
        return true;
    }
    bool result = false;
    int cur_mpd = prefs.getUShort("cur_mpd", 999);
    DPRINT("cur_mpd = " + String(cur_mpd));
    if (cur_mpd != 999) {
        Config.set_player_index(cur_mpd);
        result = true;
    }
    prefs.end();
    return result;
}
