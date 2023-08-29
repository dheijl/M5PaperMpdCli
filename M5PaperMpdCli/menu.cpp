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
#include <esp_task_wdt.h>

#include "flash_fs.h"
#include "menu.h"
#include "mpdcli.h"

void Menu::toggle_start_stop()
{
    mpd.toggle_mpd_status();
}

void Menu::select_player()
{
    auto players = Config.getPlayers();
    int selected = this->PlayerMenu.display_menu();
    if ((selected >= 0) && (selected < players.size())) {
        auto pl = players[selected]->player_name;
        epd_print_bottomline("New player @" + String(pl));
        Config.set_player_index((uint16_t)selected);
        NVS_Config::write_player_index(selected);
    }
}

void Menu::select_favourite(int page)
{
    auto favs = Config.getFavourites();
    auto fav_menu = this->FavouriteMenus[page];
    int selected = fav_menu->display_menu();
    if ((selected >= 0) && (selected < fav_menu->size() - 1)) {
        selected = (page * Menu::MAXLINES) + selected;
        FAVOURITE& fav = *favs[selected];
        mpd.play_favourite(fav);
    }
}

void Menu::Show()
{
    int nfavs = Config.getFavourites().size();
    int npages = (nfavs % Menu::MAXLINES) == 0 ? (nfavs / Menu::MAXLINES) : (nfavs / Menu::MAXLINES) + 1;
    int selected = this->MainMenu.display_menu();
    if (selected == 0) {
        this->toggle_start_stop();
    } else if (selected == 1) {
        this->select_player();
    } else if (selected <= (1 + npages)) {
        this->select_favourite(selected - 2);
    }
}

void Menu::CreateMenus()
{
    // main menu
    DPRINT("Creating MAIN menu");
    static const constexpr char* mlines[] {
        "Start/Stop Play",
        "Select Player",
        "Favourites 1",
        "Favourites 2",
        "Favourites 3",
        "Favourites 4",
        "Favourites 5",
        "Return"
    };
    auto favs = Config.getFavourites();
    int nfavs = favs.size();
    int npages = (nfavs % Menu::MAXLINES) == 0 ? (nfavs / Menu::MAXLINES) : (nfavs / Menu::MAXLINES) + 1;
    npages = min(npages, 5);
    this->MainMenu.reserve(npages + 3);
    for (int i = 0; i <= npages + 1; i++) {
        this->MainMenu.add_line(mlines[i]);
    }
    this->MainMenu.add_line(mlines[7]);
    DPRINT("Main menu lines: " + String(MainMenu.size()));
    // player menu
    DPRINT("Creating PLAYER menu");
    auto players = Config.getPlayers();
    this->PlayerMenu.reserve(players.size() + 1);
    for (auto p : players) {
        this->PlayerMenu.add_line(p->player_name);
    }
    this->PlayerMenu.add_line("Return");
    DPRINT("Player menu lines: " + String(MainMenu.size()));
    // favourites Menus
    this->FavouriteMenus.reserve(npages + 1);
    for (int page = 0; page < npages; ++page) {
        DPRINT("Creating FAVOURITES menu " + String(page));
        auto favmenu = new SubMenu(40);
        favmenu->reserve(11);
        int ifrom = page * Menu::MAXLINES;
        int ito = ifrom + Menu::MAXLINES;
        ito = min(nfavs, ito);
        for (int i = ifrom; i < ito; i++) {
            DPRINT("FAV#" + String(i) + ":" + String(favs[i]->fav_name));
            favmenu->add_line(favs[i]->fav_name);
        }
        favmenu->add_line("Return");
        this->FavouriteMenus.push_back(favmenu);
        DPRINT("Favourites menu " + String(page) + " lines: " + String(favmenu->size()));
    }
    DPRINT("Menus created");
}

int SubMenu::display_menu()
{
    int selected = 0;
    int oldselected = -1;
    bool repaint = true;
    while (true) {
        vTaskDelay(5);
        if (repaint) {
            repaint = false;
            epd_draw_menu(this->lines, selected);
            esp_task_wdt_reset();
        }
        M5.update();
        if (M5.BtnL.wasPressed()) { // up
            selected -= 1;
            if (selected < 0) {
                selected = this->size() - 1;
            }
            repaint = true;
            continue;
        }
        if (M5.BtnR.wasPressed()) { // down
            selected += 1;
            if (selected > (this->size() - 1)) {
                selected = 0;
            }
            repaint = true;
            continue;
        }
        if (M5.BtnP.wasPressed()) { // select
            return selected;
        }
        // select with touch
        M5.TP.update();
        if (M5.TP.available()) {
            auto count = M5.TP.getFingerNum();
            if (count > 0) {
                esp_task_wdt_reset();
                for (uint i = 0; i < count; ++i) {
                    DPRINT("TOUCH Finger: " + String(i));
                    auto det = M5.TP.readFinger(i);
                    M5.TP.flush();
                    DPRINT("TOUCH X=" + String(det.x) + ", Y=" + String(det.y));
                    int sel = 0;
                    for (auto ml : this->lines) {
                        if ((det.y >= ml->y + CANVAS_Y) && (det.y <= ml->y + CANVAS_Y + 30)) {
                            DPRINT("TOUCH: sel=" + String(sel));
                            selected = sel;
                            // hack: activate selection with right-hand side touch of selected menuline
                            if ((oldselected == selected) && (det.x > 270)) {
                                return selected;
                            }
                            oldselected = sel;
                            repaint = true;
                            continue;
                        }
                        ++sel;
                    }
                }
            }
        }
    }
}
