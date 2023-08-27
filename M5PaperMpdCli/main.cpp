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
    stop_wifi();
    epd_print_canvas(res);
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
    if (time_out > 50) {
        esp_task_wdt_reset();
        shutdown_and_wake();
    }
    M5.update();
    if (M5.BtnL.wasPressed() || M5.BtnP.wasPressed() || M5.BtnR.wasPressed()) {
        // reset watchdog timer, we're going interactive
        esp_task_wdt_reset();
        epd_print_bottomline("menu activated");
        menu.Show();
    }
    vTaskDelay(100);
    time_out++;
}
