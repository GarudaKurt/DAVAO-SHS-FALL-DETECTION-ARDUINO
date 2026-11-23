#include "loraReceiver.h"
#include "../loraConfig.h"

#include <Arduino.h>
#include <SPI.h>
#include <LoRa.h>

namespace {
constexpr uint8_t OLED_ADDRESS = 0x3C;
constexpr uint8_t OLED_SDA_PIN = 21;
constexpr uint8_t OLED_SCL_PIN = 22;
constexpr uint8_t BUZZER_PIN = 25;
constexpr unsigned long ALERT_DISPLAY_MS = 10000;
constexpr unsigned long DISPLAY_FRAME_MS = 250;
}

LoraReceiver::LoraReceiver()
    : _display(128, 64, &Wire, -1) {}

void LoraReceiver::setMode(DisplayMode mode) {
  _mode = mode;
  _alertStartedAt = millis();
  _lastBuzzerUpdate = 0;
  _buzzerState = false;
  digitalWrite(BUZZER_PIN, LOW);
}

void LoraReceiver::handlePayload(const String &payload) {
  if (payload.indexOf("FALL_MPU6050") >= 0 ||
      payload.indexOf("FALL_BUTTON") >= 0) {
    Serial.println("*** FALL ALERT RECEIVED ***");
    setMode(FALL_ALERT_MODE);
  } else if (payload.indexOf("SOS_BUTTON") >= 0) {
    Serial.println("*** SOS ALERT RECEIVED ***");
    setMode(BUTTON_ALERT_MODE);
  }
}

void LoraReceiver::drawStandby(unsigned long now) {
  _display.clearDisplay();
  _display.setTextColor(SH110X_WHITE);
  _display.setTextSize(2);
  _display.setCursor(16, 8);
  _display.println("STANDBY");

  _display.setTextSize(1);
  _display.setCursor(28, 36);
  _display.println("Monitoring...");

  int dot = (now / DISPLAY_FRAME_MS) % 8;
  _display.fillCircle(52 + (dot * 4), 53, 2, SH110X_WHITE);
  _display.display();
}

void LoraReceiver::drawAlert(unsigned long now) {
  bool frame = ((now / DISPLAY_FRAME_MS) % 2) == 0;

  _display.clearDisplay();
  _display.setTextColor(SH110X_WHITE);
  if (frame) {
    _display.drawRect(0, 0, 128, 64, SH110X_WHITE);
    _display.drawRect(3, 3, 122, 58, SH110X_WHITE);
  }

  _display.setTextSize(2);
  _display.setCursor(22, 8);
  _display.println(_mode == FALL_ALERT_MODE ? "FALL" : "ALERT");
  _display.setCursor(18, 30);
  _display.println(_mode == FALL_ALERT_MODE ? "DETECTED" : "SOS!");

  _display.setTextSize(1);
  _display.setCursor(25, 53);
  _display.println("Check person");
  _display.display();

  // Active buzzer: short repeating warning beeps.
  if (now - _lastBuzzerUpdate >= 150) {
    _lastBuzzerUpdate = now;
    _buzzerState = !_buzzerState;
    digitalWrite(BUZZER_PIN, _buzzerState ? HIGH : LOW);
  }
}

void LoraReceiver::updateDisplay() {
  if (!_displayReady) return;

  unsigned long now = millis();
  if (now - _lastDisplayUpdate < DISPLAY_FRAME_MS) return;
  _lastDisplayUpdate = now;

  if (_mode != STANDBY_MODE && now - _alertStartedAt >= ALERT_DISPLAY_MS) {
    setMode(STANDBY_MODE);
  }

  if (_mode == STANDBY_MODE) {
    digitalWrite(BUZZER_PIN, LOW);
    drawStandby(now);
  } else {
    drawAlert(now);
  }
}

void LoraReceiver::begin() {
  pinMode(BUZZER_PIN, OUTPUT);
  digitalWrite(BUZZER_PIN, LOW);

  Wire.begin(OLED_SDA_PIN, OLED_SCL_PIN);
  _displayReady = _display.begin(OLED_ADDRESS, true);
  if (!_displayReady) {
    Serial.println("OLED not detected; LoRa receiver will continue.");
  }

  SPI.begin(LORA_SCK_PIN, LORA_MISO_PIN, LORA_MOSI_PIN, LORA_SS_PIN);
  LoRa.setPins(LORA_SS_PIN, LORA_RESET_PIN, LORA_DIO0_PIN);

  if (!LoRa.begin(LORA_FREQUENCY_MHZ)) {
    Serial.println("Starting LoRa receiver failed!");
    while (true) {
      delay(1000);
    }
  }

  LoRa.setSpreadingFactor(LORA_SPREADING_FACTOR);
  LoRa.setSignalBandwidth(LORA_SIGNAL_BANDWIDTH);
  LoRa.setCodingRate4(LORA_CODING_RATE);
  LoRa.setSyncWord(LORA_SYNC_WORD);

  Serial.println("LoRa receiver ready.");
  Serial.print("Frequency: ");
  Serial.print(LORA_FREQUENCY_MHZ / 1E6);
  Serial.println(" MHz");
  Serial.println("Listening for LoRa packets...");
  setMode(STANDBY_MODE);
  LoRa.receive();
}

void LoraReceiver::poll() {
  int packetSize = LoRa.parsePacket();
  if (!packetSize) {
    return;
  }

  String payload;
  while (LoRa.available()) {
    payload += static_cast<char>(LoRa.read());
  }

  Serial.print("Received packet (bytes: ");
  Serial.print(packetSize);
  Serial.print("): ");
  Serial.println(payload);
  handlePayload(payload);

  Serial.print("RSSI: ");
  Serial.print(LoRa.packetRssi());
  Serial.println(" dBm");
  Serial.print("SNR: ");
  Serial.print(LoRa.packetSnr());
  Serial.println(" dB");
  Serial.println();

  // Re-enter continuous receive mode after handling the packet.
  LoRa.receive();
}
