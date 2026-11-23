#ifndef DAVAO_SHS_LORA_RECEIVER_H
#define DAVAO_SHS_LORA_RECEIVER_H

#include <Arduino.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SH110X.h>

class LoraReceiver {
  public:
    LoraReceiver();
    void begin();
    void poll();
    void updateDisplay();

  private:
    enum DisplayMode {
      STANDBY_MODE,
      FALL_ALERT_MODE,
      BUTTON_ALERT_MODE
    };

    Adafruit_SH1106G _display;
    bool _displayReady = false;
    DisplayMode _mode = STANDBY_MODE;
    unsigned long _alertStartedAt = 0;
    unsigned long _lastDisplayUpdate = 0;
    unsigned long _lastBuzzerUpdate = 0;
    bool _buzzerState = false;

    void handlePayload(const String &payload);
    void setMode(DisplayMode mode);
    void drawStandby(unsigned long now);
    void drawAlert(unsigned long now);
};

#endif
