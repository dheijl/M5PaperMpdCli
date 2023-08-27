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

#pragma once

#include <M5EPD.h>

#include "config.h"
#include "epdfunctions.h"

#define tft_write(a, b, c, d) ; //
#define tft_clear() ; //
#define tft_println(x) ; //

typedef struct menuline {
    uint16_t x;
    uint16_t y;
    const char* text;
} MENULINE;

class SubMenu {
private:
    uint16_t x;
    uint16_t y;
    uint16_t y_incr;
    vector<MENULINE*> lines;
    void draw_menu(const int selected)
    {
        int i = 0;
        for (auto l : this->lines) {
            if (i++ == selected) {
                tft_write(l->x, l->y, TFT_GREENYELLOW, String(l->text));
            } else {
                tft_write(l->x, l->y, TFT_WHITE, String(l->text));
            }
        }
    }

public:
    SubMenu(uint16_t y_incr)
        : x(2)
        , y(2)
    {
        this->y_incr = y_incr;
    }
    ~SubMenu()
    {
        for (auto ml = this->lines.begin(); ml != this->lines.end(); ++ml) {
            delete *ml;
        }
        this->lines.clear();
    }
    size_t size()
    {
        return this->lines.size();
    }
    void reserve(size_t n)
    {
        this->lines.reserve(n);
    }
    void add_line(const char* line)
    {
        this->lines.push_back(new MENULINE { this->x, this->y, line });
        this->y += this->y_incr;
    }
    int display_menu()
    {
        tft_clear();
        int selected = 0;
        bool repaint = true;
        while (true) {
            vTaskDelay(5);
            if (repaint) {
                repaint = false;
                this->draw_menu(selected);
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
            M5.update();
            auto count = M5.TP.getFingerNum();
            if (count > 0) {
                for (uint i = 0; i < count; ++i) {
                    auto det = M5.TP.readFinger(i);
                    int sel = 0;
                    for (auto ml : this->lines) {
                        if ((det.y >= ml->y) && (det.y <= ml->y + 12)) {
                            selected = sel;
                            repaint = true;
                            continue;
                        }
                        ++sel;
                    }
                }
            }
        }
    }
};

class Menu {
private:
    SubMenu MainMenu;
    SubMenu PlayerMenu;
    vector<SubMenu*> FavouriteMenus;
    void toggle_start_stop();
    void select_player();
    void select_favourite(int page);

public:
    static const int MAXLINES = 20;
    Menu()
        : MainMenu(30)
        , PlayerMenu(40)
    {
    }
    ~Menu()
    {
        for (auto fm = this->FavouriteMenus.begin(); fm != this->FavouriteMenus.end(); ++fm) {
            delete *fm;
        }
        this->FavouriteMenus.clear();
    }
    void CreateMenus();
    void Show();
};
