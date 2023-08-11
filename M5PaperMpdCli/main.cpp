#include <time.h>

#include <M5EPD.h>

#include "config.h"
#include "mpdcli.h"
#include "tftfunctions.h"
#include "wifi.h"

M5EPD_Canvas canvas(&M5.EPD);

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
    // now it's safe
    M5.begin();
    // check reboot reason flag: TIE (timer int enable) && TF (timer flag active)
    if ((reason & 0b0000101) == 0b0000101) {
        restartByRTC = true;
        DPRINT("Reboot by RTC");
    } else {
        restartByRTC = false;
        DPRINT("Reboot by power button / USB");
    }
    // init EPD
    M5.EPD.SetRotation(90);
    M5.TP.SetRotation(90);
    M5.EPD.Clear(true);
    int x = 20;
    int y = 10;
    // enable external BM8563 RTC
    M5.RTC.begin();
    // create canvas
    canvas.createCanvas(540, 960);
    canvas.setTextSize(3);
    canvas.clear();
    // try to load configuration from flash or SD
    while (!Config.load_config()) {
        tft_println_error("Missing  config!!");
        canvas.drawString("No NVS-Config or SD-CONFIG", x, y);
        canvas.pushCanvas(0, 0, UPDATE_MODE_DU4);
        vTaskDelay(3000);
    }
    y += 40;
    canvas.drawString("Config loaded", x, y);
    y += 40;
    canvas.pushCanvas(0, 0, UPDATE_MODE_DU4);
    if (!start_wifi()) {
        tft_println("Can't start WIFI");
        canvas.drawString("No WIFI connection", x, y);
        y += 40;
        canvas.pushCanvas(0, 0, UPDATE_MODE_DU4);
        stop_wifi();
        vTaskDelay(3000);
        M5.shutdown(1);
    }
    canvas.drawString("Wifi connected", x, y);
    y += 40;
    canvas.pushCanvas(0, 0, UPDATE_MODE_DU4);
    if (!restartByRTC) {
        // get network config
        auto cfg = Config.getNW_CFG();
        DPRINT("NTP: " + String(cfg.ntp_server));
        DPRINT("TZ: " + String(cfg.tz));
        // get UTC time from SNTP
        configTime(0, 0, cfg.ntp_server);
        // configure local time
        setenv("TZ", cfg.tz, 1);
        tzset();
        struct tm timeInfo;
        if (!getLocalTime(&timeInfo)) {
            canvas.drawString("Could not obtain time info", x, y);
            y += 40;
            canvas.pushCanvas(0, 0, UPDATE_MODE_DU4);
            vTaskDelay(3000);
        }
        rtc_time_t time_struct;
        time_struct.hour = timeInfo.tm_hour;
        time_struct.min = timeInfo.tm_min;
        time_struct.sec = timeInfo.tm_sec;
        M5.RTC.setTime(&time_struct);
        rtc_date_t date_struct;
        date_struct.week = timeInfo.tm_wday;
        date_struct.mon = timeInfo.tm_mon + 1;
        date_struct.day = timeInfo.tm_mday;
        date_struct.year = timeInfo.tm_year + 1900;
        M5.RTC.setDate(&date_struct);
        canvas.drawString("RTC time synced with NTP", x, y);
        y += 40;
        canvas.pushCanvas(0, 0, UPDATE_MODE_DU4);
    }

    // compute battery percentage, >= 99% = on usb power, less = on battery
    float bat_volt = (float)(M5.getBatteryVoltage() - 3200) / 1000.0f;
    int v = (int)(((float)bat_volt / 1.05f) * 100);

    String b = v >= 99 ? " USB powered." : " on battery.";
    canvas.drawString("B: " + String(v) + "%" + b, x, y);
    y += 40;
    if (restartByRTC)
        canvas.drawString("Power on by RTC timer", x, y);
    else
        canvas.drawString("Power on by PWR Btn/USB", x, y);
    y += 40;
    canvas.pushCanvas(0, 0, UPDATE_MODE_DU4);
    auto res = mpd.show_mpd_status();
    stop_wifi();
    canvas.clear();
    x = 20;
    y = 10;
    for (auto line : res) {
        canvas.drawString(line, x, y);
        y += 40;
    }
    canvas.pushCanvas(0, 0, UPDATE_MODE_DU4);
}

void loop()
{
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
    int sleep_time = 60;
    if (mpd.is_playing()) {
        canvas.drawString("Sleeping 1 minute", 20, 550);
    } else {
        rtc_time_t RTCTime;
        M5.RTC.getTime(&RTCTime);
        if (RTCTime.hour > 7) {
            // daytime
            canvas.drawString("Sleeping 10 minutes", 20, 550);
            sleep_time = 600;
        } else {
            // nighttime
            canvas.drawString("Sleeping 1 hour", 20, 550);
            sleep_time = 3600;
        }
    }
    canvas.pushCanvas(0, 0, UPDATE_MODE_DU4);
    vTaskDelay(250);
    // shut down now and wake up after sleep_time seconds (if on battery)
    // this only disables MainPower, but is a NO-OP when on USB power
    M5.shutdown(sleep_time); 
    // in case of USB power present: save power and wait for external RTC wakeup
    M5.disableEPDPower(); // digitalWrite(M5EPD_EPD_PWR_EN_PIN, 0);
    M5.disableEXTPower(); // digitalWrite(M5EPD_EXT_PWR_EN_PIN, 0);
    esp_deep_sleep((long)(sleep_time + 1) * 1000000L);
    M5.update();
    vTaskDelay(100);
}
