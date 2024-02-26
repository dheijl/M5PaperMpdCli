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

void shutdown_and_wake()
{
    rtc_time_t RTCTime;
    M5.RTC.getTime(&RTCTime);
    // sleep time is adjusted to the next minute or hour
    int sleep_time = 60 - RTCTime.sec;
    String sleep_msg = "";
    if (mpd.is_playing()) {
        sleep_msg = "Sleeping for 1 minute";
    } else {
        if (RTCTime.hour > 7) {
            // daytime
            sleep_msg = "Sleeping for 10 minutes";
            sleep_time = 600 - ((RTCTime.min % 10) * 60) - RTCTime.sec;
        } else {
            // nighttime
            sleep_msg = "Sleeping for 1 hour";
            sleep_time = 3600 - (RTCTime.min * 60) - RTCTime.sec;
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

// battery percentage, see https://github.com/m5stack/M5EPD/issues/48
static uint bat_percent()
{
    const uint32_t BAT_LOW = 3300;
    const uint32_t BAT_HIGH = 4350;
    auto clamped = std::min(std::max(M5.getBatteryVoltage(), BAT_LOW), BAT_HIGH);
    auto perc = (float)(clamped - BAT_LOW) / (float)(BAT_HIGH - BAT_LOW) * 100.0f;
    auto bat_perc = (uint)perc;
    return bat_perc;
}

bool on_battery()
{
    // compute battery percentage, >= 99% = on usb power, less = on battery
    return bat_percent() >= 99 ? false : true;
}

String get_status()
{
    // heap and psram
    auto heap = ESP.getFreeHeap() / 1024;
    auto psram = ESP.getFreePsram() / (1024 * 1024);
    // temperaturte and humidity
    M5.SHT30.UpdateData();
    auto temp = M5.SHT30.GetTemperature();
    auto hum = M5.SHT30.GetRelHumidity();
    // format
    char stemp[10];
    char shum[10];
    dtostrf(temp, 2, 1, stemp);
    dtostrf(hum, 2, 1, shum);
    auto bat_perc = bat_percent();
    return String("B" + String(bat_perc) + "%,H" + String(heap) + "K,R" + String(psram) + "M," + "T" + stemp + "C,H" + shum + "%");
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
