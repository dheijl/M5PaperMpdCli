#include "wifi.h"
#include "tftfunctions.h"

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
    tft_println("Connecting wifi...");
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
    return have_wifi;
}

void stop_wifi()
{
    tft_println("Disconnect Wifi");
    WiFi.disconnect();
    have_wifi = false;
    tft_clear();
}
