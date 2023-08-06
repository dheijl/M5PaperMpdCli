#include <M5EPD.h>

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
    M5.EPD.SetRotation(90);
    M5.TP.SetRotation(90);
    M5.EPD.Clear(true);
    M5.RTC.begin();

    canvas.createCanvas(540, 960);
    canvas.setTextSize(3);
    if (restartByRTC)
        canvas.drawString("Power on by RTC timer", 20, 250); // <= this one we should when waking up from sleep
    else
        canvas.drawString("Power on by PWR Btn / USB", 20, 250);

    canvas.drawString("Press BtnR for sleep!", 40, 350);
    canvas.drawString("Wakeup after 5 seconds!", 50, 450);
    canvas.pushCanvas(0, 0, UPDATE_MODE_DU4);
}

void loop()
{
    if (M5.BtnR.wasPressed()) {
        canvas.drawString("I'm going to sleep.zzzZZZ~", 45, 550);
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
