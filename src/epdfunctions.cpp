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
// IMPLIED, INCLUDING BUT NOT LIMITED TO THWARRANTIES OF MERCHANTABILITY, FITNESS
// FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
// COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
// IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
// CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

#include <M5EPD.h>

#include "config.h"
#include "epdfunctions.h"

static M5EPD_Canvas topline(&M5.EPD); // 0 - 40
static M5EPD_Canvas canvas(&M5.EPD); // 40 - 880
static M5EPD_Canvas bottomline(&M5.EPD); // 920 - 40

void epd_init()
{
    // init EPD
    M5.EPD.SetRotation(90);
    M5.TP.SetRotation(90);
    M5.EPD.Clear(true);
    // create canvases
    topline.createCanvas(540, 40);
    topline.setTextSize(3);
    topline.clear();
    canvas.createCanvas(540, 880);
    canvas.setTextSize(3);
    canvas.clear();
    canvas.setTextArea(10, 10, 530, 870);
    canvas.setTextWrap(true, false);
    bottomline.createCanvas(540, 40);
    bottomline.setTextSize(3);
    bottomline.clear();
}

void epd_print_topline(const String& s)
{
    DPRINT(s);
    topline.clear();
    topline.drawString(s, 10, 10);
    topline.pushCanvas(0, TOPLINE_Y, UPDATE_MODE_DU4);
}

void epd_print_canvas(const StatusLines& sl)
{
    canvas.clear();
    canvas.setCursor(0, 0);
    canvas.setTextColor(15, 0);
    for (auto line : sl) {
        DPRINT(line);
        canvas.setCursor(canvas.getCursorX(), canvas.getCursorY() + 14);
        canvas.println(line);
    }
    canvas.pushCanvas(0, CANVAS_Y, UPDATE_MODE_A2);
}

void epd_draw_menu(const MenuLines& lines, const int selected)
{
    canvas.clear();
    canvas.setCursor(0, 0);
    int i = 0;
    for (auto l : lines) {
        DPRINT(l->text);
        if (i++ == selected) {
            canvas.setTextColor(0, 15);
            canvas.drawString(String(l->text), l->x, l->y);
            canvas.setTextColor(15, 0);
        } else {
            canvas.drawString(String(l->text), l->x, l->y);
        }
    }
    canvas.pushCanvas(0, CANVAS_Y, UPDATE_MODE_A2);
}

void epd_print_bottomline(const String& s)
{
    DPRINT(s);
    bottomline.clear();
    bottomline.drawString(s, 10, 0);
    bottomline.pushCanvas(0, BOTTOMLINE_Y, UPDATE_MODE_DU4);
}
