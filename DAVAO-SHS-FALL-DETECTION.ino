#include "loraSender.h"
#include "brokerController.h"
#include <Wire.h>
#include <math.h>

LORAMODULE lora;

// Set these to the Wi-Fi network used by the loraSender ESP32.
const char* WIFI_SSID = "YOUR_WIFI_NAME";
const char* WIFI_PASSWORD = "YOUR_WIFI_PASSWORD";
Controller mqttBroker(WIFI_SSID, WIFI_PASSWORD);

const uint8_t MPU_ADDRESS = 0x68;
const float FALL_IMPACT_G = 2.5f;
const uint8_t SOS_BUTTON_PIN = 25;
const uint8_t FALL_BUTTON_PIN = 26;
const unsigned long MPU_SAMPLE_INTERVAL_MS = 100;
const unsigned long SENSOR_REPORT_INTERVAL_MS = 2000;
const unsigned long ALERT_COOLDOWN_MS = 5000;
const unsigned long ALERT_DISPLAY_MS = 10000;

bool mpuReady = false;
bool alertActive = false;
bool lastSosPressed = false;
bool lastFallButtonPressed = false;
unsigned long lastMpuSample = 0;
unsigned long lastSensorReport = 0;
unsigned long lastFallAlert = 0;
unsigned long alertStartedAt = 0;
float latestX = 0.0f;
float latestY = 0.0f;
float latestZ = 0.0f;

bool writeMpuRegister(uint8_t reg, uint8_t value) {
  Wire.beginTransmission(MPU_ADDRESS);
  Wire.write(reg);
  Wire.write(value);
  return Wire.endTransmission() == 0;
}

bool setupMpu6050() {
  Wire.begin(21, 22);

  Wire.beginTransmission(MPU_ADDRESS);
  if (Wire.endTransmission() != 0) {
    return false;
  }

  // Wake the MPU6050 and select the +/-2 g accelerometer range.
  return writeMpuRegister(0x6B, 0x00) && writeMpuRegister(0x1C, 0x00);
}

bool readAcceleration(float &x, float &y, float &z) {
  Wire.beginTransmission(MPU_ADDRESS);
  Wire.write(0x3B);  // ACCEL_XOUT_H
  if (Wire.endTransmission(false) != 0 || Wire.requestFrom(MPU_ADDRESS, (uint8_t)6) != 6) {
    return false;
  }

  int16_t rawX = (Wire.read() << 8) | Wire.read();
  int16_t rawY = (Wire.read() << 8) | Wire.read();
  int16_t rawZ = (Wire.read() << 8) | Wire.read();

  x = rawX / 16384.0f;
  y = rawY / 16384.0f;
  z = rawZ / 16384.0f;
  return true;
}

void setup() {
  Serial.begin(115200);
  pinMode(SOS_BUTTON_PIN, INPUT_PULLUP);
  pinMode(FALL_BUTTON_PIN, INPUT_PULLUP);

  lora.initLora();
  mqttBroker.begin();

  mpuReady = setupMpu6050();
  if (mpuReady) {
    Serial.println("MPU6050 ready.");
  } else {
    Serial.println("MPU6050 not detected. Check SDA, SCL, VCC, and GND.");
  }
}

void loop() {
  unsigned long now = millis();

  mqttBroker.loop();

  if (alertActive && now - alertStartedAt >= ALERT_DISPLAY_MS) {
    alertActive = false;
  }

  bool sosPressed = digitalRead(SOS_BUTTON_PIN) == LOW;
  bool fallButtonPressed = digitalRead(FALL_BUTTON_PIN) == LOW;

  if (sosPressed && !lastSosPressed) {
    Serial.println("SOS button pressed!");
    lora.sendNotify("SOS_BUTTON");
    alertActive = true;
    alertStartedAt = now;
  }
  if (fallButtonPressed && !lastFallButtonPressed) {
    Serial.println("Fall Alert button pressed!");
    lora.sendNotify("FALL_BUTTON");
    alertActive = true;
    alertStartedAt = now;
  }

  lastSosPressed = sosPressed;
  lastFallButtonPressed = fallButtonPressed;

  if (mpuReady && now - lastMpuSample >= MPU_SAMPLE_INTERVAL_MS) {
    lastMpuSample = now;

    if (readAcceleration(latestX, latestY, latestZ)) {
      float totalAcceleration = sqrt((latestX * latestX) +
                                     (latestY * latestY) +
                                     (latestZ * latestZ));

      if (totalAcceleration >= FALL_IMPACT_G &&
          (lastFallAlert == 0 || now - lastFallAlert >= ALERT_COOLDOWN_MS)) {
        Serial.println("FALL DETECTED! Sending warning.");
        lora.sendNotify("FALL_MPU6050");
        alertActive = true;
        alertStartedAt = now;
        lastFallAlert = now;
      }
    } else {
      Serial.println("MPU6050 read failed.");
    }
  }

  if (mpuReady && now - lastSensorReport >= SENSOR_REPORT_INTERVAL_MS) {
    lastSensorReport = now;

    char payload[96];
    snprintf(payload, sizeof(payload), "SENSOR_X:%.2f,SENSOR_Y:%.2f,SENSOR_Z:%.2f",
             latestX, latestY, latestZ);

    Serial.println(payload);
    lora.sendNotify(payload);
  }
  
  // enabled_sensor describes MPU6050 availability; enabledAlert describes
  // the current patient alert state. Both values are serialized as booleans.
  mqttBroker.updatePatientStatus(alertActive, mpuReady);

  delay(5);
}
