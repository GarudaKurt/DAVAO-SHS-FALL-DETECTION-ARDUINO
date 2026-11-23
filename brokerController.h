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
  void updatePatientStatus(bool enabledAlert, bool enabledSensor);
  void publishFallEvent(const char* sourcePayload);

 private:
  bool ensureMqtt();
  bool publishPendingEvent();

  const char* ssid;
  const char* password;
  const char* MQTT_SERVER = "broker.hivemq.com";
  const uint16_t MQTT_PORT = 1883;
  const char* STATUS_TOPIC = "patient/status";
  const char* EVENT_TOPIC = "patient/events";
  const char* DEVICE_ID = "esp32-01";

  static constexpr unsigned long RETRY_MS = 1000;
  unsigned long lastReconnectAttempt = 0;
  unsigned long lastEventFailure = 0;
  unsigned long lastStatusFailure = 0;
  bool reportedAlert = false;
  bool reportedSensor = false;
  bool haveReportedStatus = false;

  bool eventPending = false;
  char pendingSourcePayload[128] = {};
  char lastPublishedEvent[128] = {};

  WiFiClient espClient;
  PubSubClient client;
};

#endif
