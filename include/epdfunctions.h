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

#include <Arduino.h>
#include <vector>

#include "menuline.h"

typedef vector<String> StatusLines;

const uint16_t TOPLINE_Y = 0;
const uint16_t CANVAS_Y = 40;
const uint16_t BOTTOMLINE_Y = 920;

void epd_init();
void epd_print_topline(const String& s);
void epd_print_canvas(const StatusLines& sl);
void epd_draw_menu(const MenuLines& lines, const int selected);
void epd_print_bottomline(const String& s);
