#include "wifi.h"
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
            break;
        }
        vTaskDelay(50);
    }
    epd_print_topline("Wifi connected");
    return have_wifi;
}

void stop_wifi()
{
    WiFi.disconnect();
    epd_print_topline("Wifi disconnected");
    have_wifi = false;
}
