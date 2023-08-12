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
    bottomline.createCanvas(540, 40);
    bottomline.setTextSize(3);
    bottomline.clear();
}

void epd_print_topline(const String& s)
{
    DPRINT(s);
    topline.clear();
    topline.drawString(s, 10, 10);
    topline.pushCanvas(0, 0, UPDATE_MODE_DU4);
}

void epd_print_canvas(const StatusLines& sl)
{
    canvas.clear();
    int x = 10, y = 10;
    for (auto line : sl) {
        DPRINT(line);
        canvas.drawString(line, x, y);
        y += 40;
    }
    canvas.pushCanvas(0, 40, UPDATE_MODE_DU4);
}

void epd_print_bottomline(const String& s)
{
    DPRINT(s);
    bottomline.clear();
    bottomline.drawString(s, 10, 0);
    bottomline.pushCanvas(0, 910, UPDATE_MODE_DU4);
}
