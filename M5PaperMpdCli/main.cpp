#include <time.h>

#include <M5EPD.h>

#include "config.h"
#include "epdfunctions.h"
#include "mpdcli.h"
#include "synctime.h"
#include "utils.h"
#include "wifi.h"

M5EPD_Canvas topline(&M5.EPD); // 0 - 40
M5EPD_Canvas canvas(&M5.EPD); // 40 - 880
M5EPD_Canvas bottomline(&M5.EPD); // 920 - 40

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
    // now it's safe to start M5EPD
    M5.begin(true, true, true, true, false);
    M5.SHT30.Begin();
    // check reboot reason flag: TIE (timer int enable) && TF (timer flag active)
    if ((reason & 0b0000101) == 0b0000101) {
        restartByRTC = true;
        DPRINT("Reboot by RTC");
    } else {
        restartByRTC = false;
        DPRINT("Reboot by power button / USB");
    }
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
    // enable external BM8563 RTC
    M5.RTC.begin();
    // sync time with NTP if not RTC wake-up
    if (!restartByRTC) {
        sync_time();
    }
    on_battery ? " on battery." : " USB powered.";
    auto res = mpd.show_mpd_status();
    stop_wifi();
    epd_print_canvas(res);
    if (restartByRTC) {
        epd_print_topline("Power on by RTC timer");
        shutdown_and_wake();
    } else {
        epd_print_topline("Power on by PWR Btn/USB");
        epd_print_bottomline("Press any button for Menu");
    }
}

void loop()
{
    if (time_out > 50) {
        shutdown_and_wake();
    }
    M5.update();
    if (M5.BtnL.wasPressed() || M5.BtnP.wasPressed() || M5.BtnR.wasPressed()) {
        // show menu
        epd_print_bottomline("menu activated");
    }
    vTaskDelay(100);
    time_out++;
}
