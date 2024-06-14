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

#include "wifi_utils.h"
#include "epdfunctions.h"

#include <M5EPD.h>
#include <WiFi.h>
#include <WiFiMulti.h>

static bool have_wifi = false;

bool is_wifi_connected()
{
    return have_wifi;
}

bool start_wifi()
{

    if ((have_wifi) && (WiFi.status() == WL_CONNECTED)) {
        return true;
    }
    int retries = 5;

    while (retries-- > 0 && !have_wifi) {
        // Turn on WiFi
        WiFi.disconnect();
        WiFi.softAPdisconnect(true);
        epd_print_topline("Connecting wifi...");
        WiFi.mode(WIFI_STA);
        auto ap = Config.getNW_CFG();
        WiFi.begin(ap.ssid, ap.psw);
        have_wifi = false;
        long now = millis();
        while ((millis() - now) < 10000) {
            if (WiFi.status() == WL_CONNECTED) {
                have_wifi = true;
                epd_print_topline("Wifi connected");
                return have_wifi;
            }
        }
        stop_wifi();
        vTaskDelay(500);
    }
    epd_print_topline("NO Wifi connection");
    return have_wifi;
}

void stop_wifi(bool wifi_off)
{
    WiFi.disconnect(wifi_off);
    epd_print_topline("Wifi disconnected");
    have_wifi = false;
}
