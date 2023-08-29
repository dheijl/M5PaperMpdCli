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

#include <esp_task_wdt.h>

#include <time.h>

#include <M5EPD.h>

#include "config.h"
#include "epdfunctions.h"
#include "menu.h"
#include "mpdcli.h"
#include "synctime.h"
#include "utils.h"
#include "wifi.h"

static Menu menu;

// 60 seconds WDT
#define WDT_TIMEOUT 60

static bool restartByRTC = false;
static bool is_playing = false;
static int time_out = 0;

void setup()
{
    // m5paper-wakeup-cause
    // see forum: https://community.m5stack.com/topic/2851/m5paper-wakeup-cause/6
    //  Check power on reason before calling M5.begin()
    //  which calls RTC.begin() which clears the timer flag.
    Wire.begin(21, 22);
    uint8_t reason = M5.RTC.readReg(0x01);
    // now it's safe to start M5EPD & RTC
    M5.begin(true, true, true, true, false);
    // enable temp & humidity sensor
    M5.SHT30.Begin();
    // check reboot reason flag: TIE (timer int enable) && TF (timer flag active)
    if ((reason & 0b0000101) == 0b0000101) {
        restartByRTC = true;
        DPRINT("Reboot by RTC");
    } else {
        restartByRTC = false;
        DPRINT("Reboot by power button / USB");
    }
    // start watchdog timer in case somethings hangs
    esp_task_wdt_init(WDT_TIMEOUT, true); // enable panic so ESP32 restarts
    esp_task_wdt_add(NULL); // add current thread to WDT watch
    // setup EPD canvases
    epd_init();
    // try to load configuration from flash or SD
    while (!Config.load_config()) {
        epd_print_topline("No NVS-Config or SD-CONFIG");
        vTaskDelay(200);
        M5.shutdown(3600);
    }
    epd_print_topline("Config loaded");
    if (!start_wifi()) {
        epd_print_topline("No WIFI connection");
        stop_wifi();
        vTaskDelay(200);
        M5.shutdown(3600);
    }
    // sync time with NTP if not RTC wake-up
    if (!restartByRTC) {
        sync_time();
    }
    auto res = mpd.show_mpd_status();
    epd_print_canvas(res);
    stop_wifi();
    if (restartByRTC) {
        epd_print_topline("Power on by RTC timer");
        shutdown_and_wake();
    } else {
        epd_print_topline("Power on by PWR Btn/USB");
        epd_print_bottomline("Press any button for Menu");
        menu.CreateMenus();
    }
}

void loop()
{
    if (time_out > 60) {
        esp_task_wdt_reset();
        shutdown_and_wake();
    }
    M5.update();
    if (M5.BtnL.wasPressed() || M5.BtnP.wasPressed() || M5.BtnR.wasPressed()) {
        // reset watchdog timer, we're going interactive
        esp_task_wdt_reset();
        epd_print_bottomline("menu activated");
        menu.Show();
        vTaskDelay(1000);
        start_wifi();
        auto res = mpd.show_mpd_status();
        stop_wifi();
        epd_print_canvas(res);
        epd_print_bottomline("Press any button for Menu");
        time_out = 0;
    }
    vTaskDelay(100);
    time_out++;
}
