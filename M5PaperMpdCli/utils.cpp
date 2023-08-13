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
#include <M5EPD.h>

#include "config.h"
#include "epdfunctions.h"
#include "mpdcli.h"
#include "utils.h"

vector<string> split(const string& s, char delim)
{
    vector<string> result;
    size_t start;
    size_t end = 0;

    while ((start = s.find_first_not_of(delim, end)) != std::string::npos) {
        end = s.find(delim, start);
        result.push_back(s.substr(start, end - start));
    }
    return result;
}

void sleep_and_wake()
{
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

bool on_battery()
{
    // compute battery percentage, >= 99% = on usb power, less = on battery
    float bat_volt = (float)(M5.getBatteryVoltage() - 3200) / 1000.0f;
    int v = (int)(((float)bat_volt / 1.05f) * 100);
    return v >= 99 ? false : true;
}

String get_status()
{
    float bat_volt = (float)(M5.getBatteryVoltage() - 3200) / 1000.0f;
    int bat_level = (int)(((float)bat_volt / 1.05f) * 100);
    auto heap = ESP.getFreeHeap() / 1024;
    auto psram = ESP.getFreePsram() / (1024 * 1024);
    return String("Batt=" + String(bat_level) + "%,H=" + String(heap) + "K,PS=" + String(psram) + "M");
}

String get_date_time()
{
    rtc_date_t RTCDate;
    M5.RTC.getDate(&RTCDate);
    rtc_time_t RTCTime;
    M5.RTC.getTime(&RTCTime);
    char datebuf[64];
    snprintf(datebuf, 64, "%04d:%02d:%02d", RTCDate.year, RTCDate.mon, RTCDate.day);
    char timebuf[64];
    snprintf(timebuf, 64, "%02d:%02d:%02d", RTCTime.hour, RTCTime.min, RTCTime.sec);
    return String(String(datebuf) + " - " + String(timebuf));
}
