#include <M5EPD.h>

#include "config.h"
#include "mpdcli.h"
#include "tftfunctions.h"
#include "wifi.h"

M5EPD_Canvas canvas(&M5.EPD);

bool restartByRTC = false;

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
        Serial.println("Reboot by RTC");
    } else {
        restartByRTC = false;
        Serial.println("Reboot by power button / USB");
    }
    // init EPD
    M5.EPD.SetRotation(90);
    M5.TP.SetRotation(90);
    M5.EPD.Clear(true);
    // enable external BM8563 RTC
    M5.RTC.begin();

    // compute battery percentage, >= 99% = on usb power, less = on battery
    float bat_volt = (float)(M5.getBatteryVoltage() - 3200) / 1000.0f;
    int v = (int)(((float)bat_volt / 1.05f) * 100);

    // show some data
    canvas.createCanvas(540, 960);
    canvas.setTextSize(3);
    canvas.clear();
    int x = 20;
    int y = 10;
    String b = v >= 99 ? " USB powered." : " on battery.";
    canvas.drawString("Batt: " + String(v) + "%" + b, x, y);
    y += 40;
    if (restartByRTC)
        canvas.drawString("Power on by RTC timer", x, y);
    else
        canvas.drawString("Power on by PWR Btn/USB", x, y);
    y += 40;
    canvas.drawString("Press BtnR for sleep!", x, y);
    y += 40;
    canvas.drawString("Press BtnL for shutdown!", x, y);
    y += 40;
    canvas.drawString("Wakeup after 5 seconds!", x, y);
    y += 40;
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
    while (!start_wifi()) {
        tft_println("Can't start WIFI");
        canvas.drawString("No WIFI connection", x, y);
        canvas.pushCanvas(0, 0, UPDATE_MODE_DU4);
        vTaskDelay(3000);
    }
    y += 40;
    canvas.drawString("Wifi connected", x, y);
    canvas.pushCanvas(0, 0, UPDATE_MODE_DU4);
    auto res = mpd.show_mpd_status();
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
    if (M5.BtnR.wasPressed()) {
        canvas.drawString("I'm going to sleep.zzzZZZ~", 20, 550);
        canvas.pushCanvas(0, 0, UPDATE_MODE_DU4);
        delay(500);
        // this only disables MainPower, a NO-OP when on USB power
        M5.shutdown(5); // shut down now and wake up after 5 seconds if on battery
        // in case of USB power present: save power and wait for external RTC wakeup
        M5.disableEPDPower(); // digitalWrite(M5EPD_EPD_PWR_EN_PIN, 0);
        M5.disableEXTPower(); // digitalWrite(M5EPD_EXT_PWR_EN_PIN, 0);
        esp_deep_sleep(5100000L);
    } else if (M5.BtnL.wasPressed()) {
        // proper shutdown without wake-up (if not on USB power)
        canvas.drawString("I'm shutting down", 45, 550);
        canvas.pushCanvas(0, 0, UPDATE_MODE_DU4);
        delay(500);
        M5.RTC.clearIRQ();
        M5.RTC.disableIRQ();
        M5.disableEPDPower(); // digitalWrite(M5EPD_EPD_PWR_EN_PIN, 0);
        M5.disableEXTPower(); // digitalWrite(M5EPD_EXT_PWR_EN_PIN, 0);
        M5.disableMainPower();
        esp_deep_sleep(5100000L);
    }
    M5.update();
    delay(100);
}
