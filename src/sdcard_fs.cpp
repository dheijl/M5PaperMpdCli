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

#include "epdfunctions.h"
#include "sdcard_fs.h"
#include "utils.h"

#include <SD.h>

#include <M5EPD.h>

#include <string.h>

#define TFCARD_CS_PIN GPIO_NUM_4

bool SD_Config::read_wifi(NETWORK_CFG& nw_cfg)
{
    bool result = false;
    if (!SD.begin(TFCARD_CS_PIN, SPI, 25000000)) {
        SD.end();
        return result;
    }
    epd_print_topline("SD card present!");
    vTaskDelay(50);
    epd_print_topline("Loading wifi");
    File wifif = SD.open("/wifi.txt", FILE_READ);
    if (wifif) {
        result = parse_wifi_file(wifif, nw_cfg);
    } else {
        epd_print_topline("error reading wifi.txt");
        vTaskDelay(1000);
    }
    SD.end();
    return result;
}

bool SD_Config::read_players(PLAYERS& players)
{
    bool result = false;
    if (!SD.begin(TFCARD_CS_PIN, SPI, 25000000)) {
        SD.end();
        return result;
    }
    epd_print_topline("Loading players");
    File plf = SD.open("/players.txt", FILE_READ);
    if (plf) {
        result = parse_players_file(plf, players);
    } else {
        epd_print_topline("error reading players.txt");
        vTaskDelay(1000);
    }

    SD.end();
    return result;
}

bool SD_Config::read_favourites(FAVOURITES& favourites)
{
    bool result = false;
    if (!SD.begin(TFCARD_CS_PIN, SPI, 25000000)) {
        SD.end();
        return result;
    }
    epd_print_topline("Loading favourites");
    File favf = SD.open("/favs.txt", FILE_READ);
    if (favf) {
        result = parse_favs_file(favf, favourites);
    } else {
        epd_print_topline("error reading favs.txt");
        vTaskDelay(1000);
    }

    SD.end();
    return result;
}

bool SD_Config::parse_wifi_file(File wifif, NETWORK_CFG& nw_cfg)
{
    bool have_ntp = false;
    bool have_wifi = false;
    epd_print_topline("Parsing WiFi ssid/psw");
    while (wifif.available()) {
        String line = wifif.readStringUntil('\n');
        line.trim();
        DPRINT(line);
        string wifi = line.c_str();
        if (wifi.length() > 1) {
            vector<string> parts = split(wifi, '|');
            if (parts.size() == 2) {
                nw_cfg.ssid = strdup(parts[0].c_str());
                nw_cfg.psw = strdup(parts[1].c_str());
                have_wifi = true;
            }
        }
        line = wifif.readStringUntil('\n');
        line.trim();
        DPRINT(line);
        string ntp = line.c_str();
        if (ntp.length() > 1) {
            vector<string> parts = split(ntp, '|');
            if (parts.size() == 2) {
                nw_cfg.ntp_server = strdup(parts[0].c_str());
                nw_cfg.tz = strdup(parts[1].c_str());
                have_ntp = true;
            }
        }
    }
    wifif.close();
    return have_wifi && have_ntp;
}

bool SD_Config::parse_players_file(File plf, PLAYERS& players)
{
    bool result = false;
    epd_print_topline("Parsing players:");
    while (plf.available() && (players.size() <= 5)) { // max 5 players
        String line = plf.readStringUntil('\n');
        line.trim();
        DPRINT(line);
        string pl = line.c_str();
        if (pl.length() > 1) {
            vector<string> parts = split(pl, '|');
            if (parts.size() == 3) {
                MPD_PLAYER* mpd = new MPD_PLAYER();
                mpd->player_name = strdup(parts[0].c_str());
                mpd->player_ip = strdup(parts[1].c_str());
                mpd->player_port = stoi(parts[2]);
                players.push_back(mpd);
                epd_print_topline(String(mpd->player_name) + " " + String(mpd->player_ip) + ":" + String(mpd->player_port));
            }
        }
    }
    plf.close();
    if (players.size() > 0) {
        result = true;
    } else {
        epd_print_topline("No players!");
    }
    return result;
}

bool SD_Config::parse_favs_file(File favf, FAVOURITES& favourites)
{
    bool result = false;
    epd_print_topline("Parsing favourites");
    while (favf.available() && favourites.size() <= 50) { // max 50
        String line = favf.readStringUntil('\n');
        line.trim();
        DPRINT(line);
        string fav = line.c_str();
        if (fav.length() > 1) {
            vector<string> parts = split(fav, '|');
            if (parts.size() == 2) {
                FAVOURITE* f = new FAVOURITE();
                f->fav_name = strdup(parts[0].c_str());
                f->fav_url = strdup(parts[1].c_str());
                favourites.push_back(f);
            }
        }
    }
    favf.close();
    if (favourites.size() > 0) {
        epd_print_topline("Loaded " + String(favourites.size()) + " favourites");
        result = true;
    } else {
        epd_print_topline("No favourites!");
    }
    return result;
}
