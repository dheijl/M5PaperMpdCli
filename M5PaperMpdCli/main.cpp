#include <time.h>

#include <M5EPD.h>

#include "config.h"
#include "epdfunctions.h"
#include "mpdcli.h"
#include "synctime.h"
#include "wifi.h"

M5EPD_Canvas topline(&M5.EPD); // 0 - 40
M5EPD_Canvas canvas(&M5.EPD); // 40 - 880
M5EPD_Canvas bottomline(&M5.EPD); // 920 - 40

static bool restartByRTC = false;
static bool is_playing = false;

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
        vTaskDelay(3000);
    }
    epd_print_topline("Config loaded");
    if (!start_wifi()) {
        epd_print_topline("No WIFI connection");
        stop_wifi();
        vTaskDelay(2000);
        M5.shutdown(1);
    }
    epd_print_topline("Wifi connected");
    // enable external BM8563 RTC
    M5.RTC.begin();
    // sync time with NTP if not RTC wake-up
    if (!restartByRTC) {
        sync_time();
    }

    // compute battery percentage, >= 99% = on usb power, less = on battery
    float bat_volt = (float)(M5.getBatteryVoltage() - 3200) / 1000.0f;
    int v = (int)(((float)bat_volt / 1.05f) * 100);
    String b = v >= 99 ? " USB powered." : " on battery.";
    epd_print_topline("B: " + String(v) + "%" + b);
    auto res = mpd.show_mpd_status();
    stop_wifi();
    epd_print_canvas(res);
    if (restartByRTC) {
        epd_print_topline("Power on by RTC timer");
    } else {
        epd_print_topline("Power on by PWR Btn/USB");
    }
    int sleep_time = 60;
    String sleep_msg = "";
    if (mpd.is_playing()) {
        sleep_msg = "Sleeping for 1 minute";
    } else {
        rtc_time_t RTCTime;
        M5.RTC.getTime(&RTCTime);
        if (RTCTime.hour > 7) {
            // daytime
            sleep_msg = "Sleeping for 10 minutes";
            sleep_time = 600;
        } else {
            // nighttime
            sleep_msg = "Sleeping for 1 hour";
            sleep_time = 3600;
        }
    }
    epd_print_bottomline(sleep_msg);
    vTaskDelay(250);
    // shut down now and wake up after sleep_time seconds (if on battery)
    // this only disables MainPower, but is a NO-OP when on USB power
    M5.shutdown(sleep_time);
    // in case of USB power present: save power and wait for external RTC wakeup
    M5.disableEPDPower(); // digitalWrite(M5EPD_EPD_PWR_EN_PIN, 0);
    M5.disableEXTPower(); // digitalWrite(M5EPD_EXT_PWR_EN_PIN, 0);
    esp_deep_sleep((long)(sleep_time + 1) * 1000000L);
}

void loop()
{
    M5.update();
    vTaskDelay(100);
}
/*
if (M5.BtnL.wasPressed()) {
    // proper shutdown without wake-up (if not on USB power)
    canvas.drawString("I'm shutting down", 45, 550);
    canvas.pushCanvas(0, 0, UPDATE_MODE_DU4);
    vTaskDelay(250);
    M5.RTC.clearIRQ();
    M5.RTC.disableIRQ();
    M5.disableEPDPower(); // digitalWrite(M5EPD_EPD_PWR_EN_PIN, 0);
    M5.disableEXTPower(); // digitalWrite(M5EPD_EXT_PWR_EN_PIN, 0);
    M5.disableMainPower();
    esp_deep_sleep(60100000L);
}
*/
