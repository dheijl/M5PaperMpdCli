#include "synctime.h"
#include "config.h"
#include "epdfunctions.h"

bool sync_time()
{
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
        epd_print_topline("Could not obtain time info");
        vTaskDelay(2000);
        return false;
    } else {
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
        epd_print_topline("RTC time synced with NTP");
        return true;
    }
}
