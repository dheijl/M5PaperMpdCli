#pragma once

#include <Arduino.h>
#include <vector>

#include "menuline.h"

typedef vector<String> StatusLines;

const uint16_t TOPLINE_Y = 0;
const uint16_t CANVAS_Y = 40;
const uint16_t BOTTOMLINE_Y = 910;

void epd_init();
void epd_print_topline(const String& s);
void epd_print_canvas(const StatusLines& sl);
void epd_draw_menu(const MenuLines& lines, const int selected);
void epd_print_bottomline(const String& s);
