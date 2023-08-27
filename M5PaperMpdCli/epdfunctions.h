#pragma once

#include <Arduino.h>
#include <vector>

#include "menu.h"

using std::vector;

typedef vector<String> StatusLines;

void epd_init();
void epd_print_topline(const String& s);
void epd_print_canvas(const StatusLines& sl);
void epd_draw_menu(const vector<MENULINE*>& lines, const int selected);
void epd_print_bottomline(const String& s);
