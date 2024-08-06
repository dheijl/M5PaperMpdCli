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
#include "flash_fs.h"
#include "sdcard_fs.h"
#include "utils.h"
#include <ESPmDNS.h>
#include <wifi_utils.h>

static Configuration config;

Configuration& Config = config;

///
/// try to load a config from SD or from flash
///
bool Configuration::load_config()
{
    if (this->load_SD_config()) {
        if (this->save_FLASH_config()) {
            epd_print_topline("Configuration saved to FLASH");
        } else {
            epd_print_topline("Error saving to FLASH");
        }
    } else {
        if (!this->load_FLASH_config()) {
            return false;
        }
    }
    return true;
}

void Configuration::set_player_index(uint16_t new_pl)
{
    this->player_index = new_pl;
}

const MPD_PLAYER& Configuration::get_active_mpd()
{
    auto player = &*(this->mpd_players[config.player_index]);
    // convert .local hostname to ip if needed
    MDNS.begin("M5Paper");
    if (player->player_ip == NULL) {
        string localname = string(player->player_hostname);
        auto pos = localname.find(".local");
        if (pos != std::string::npos) {
            epd_print_topline("MDNS lookup: " + String(player->player_hostname));
            // ESP32 MDNS bug: .local suffix has to be stripped !
            localname.erase(pos, localname.length());
            IPAddress ip = MDNS.queryHost(localname.c_str());
            epd_print_topline("MDNS IP: " + ip.toString());
            player->player_ip = strdup(ip.toString().c_str());
        } else {
            player->player_ip = strdup(player->player_hostname);
        }
    }
    return *player;
}

const NETWORK_CFG& Configuration::getNW_CFG()
{
    return this->nw_cfg;
}

const PLAYERS& Configuration::getPlayers()
{
    return this->mpd_players;
}

const FAVOURITES& Configuration::getFavourites()
{
    return this->favourites;
}

bool Configuration::load_SD_config()
{
    epd_print_topline("Check SD config");
    if (SD_Config::read_wifi(this->nw_cfg)
        && SD_Config::read_players(this->mpd_players)
        && SD_Config::read_favourites(this->favourites)) {
        return true;
    } else {
        return false;
    }
}

bool Configuration::load_FLASH_config()
{
    epd_print_topline("Load FLASH config");
    if (NVS_Config::read_wifi(this->nw_cfg)
        && NVS_Config::read_players(this->mpd_players)
        && NVS_Config::read_favourites(this->favourites)
        && NVS_Config::read_player_index()) {
        return true;
    } else {
        return false;
    }
}

bool Configuration::save_FLASH_config()
{
    epd_print_topline("Save FLASH config");
    if (NVS_Config::write_wifi(this->nw_cfg)
        && NVS_Config::write_players(this->mpd_players)
        && NVS_Config::write_favourites(this->favourites)) {
        return true;
    } else {
        return false;
    }
}
