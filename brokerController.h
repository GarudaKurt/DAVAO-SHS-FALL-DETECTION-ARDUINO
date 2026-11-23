#ifndef BROKER_CONTROLLER_H
#define BROKER_CONTROLLER_H

#include <Arduino.h>
#include <ArduinoJson.h>
#include <PubSubClient.h>
#include <WiFi.h>

class Controller {
 public:
  Controller(const char* ssid, const char* password);
  void begin();
  void loop();
  bool connected() { return client.connected(); }
  bool wifiConnected() const { return WiFi.status() == WL_CONNECTED; }
  // Same signature as before. enabledAlert is mapped to the hasFall column.
  void updatePatientStatus(bool enabledAlert, bool enabledSensor);

 private:
  bool ensureMqtt();

  const char* ssid;
  const char* password;
  const char* MQTT_SERVER = "broker.hivemq.com";
  const uint16_t MQTT_PORT = 1883;
  const char* STATUS_TOPIC = "patient/status";
  const char* DEVICE_ID = "esp32-01";

  static constexpr bool AUTO_CLEAR_FALL = false;
  static constexpr unsigned long RETRY_MS = 1000;
  static constexpr unsigned long MQTT_RECONNECT_INTERVAL_MS = 5000;

  unsigned long lastReconnectAttempt = 0;
  unsigned long lastStatusFailure = 0;
  bool lastAlert = false;
  bool pendingFall = false;
  bool pendingClear = false;
  bool reportedSensor = false;
  bool haveReportedSensor = false;

  WiFiClient espClient;
  PubSubClient client;
};

#endif
