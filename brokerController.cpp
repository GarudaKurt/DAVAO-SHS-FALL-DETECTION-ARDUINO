#include "brokerController.h"

Controller::Controller(const char* wifiSsid, const char* wifiPassword)
    : ssid(wifiSsid), password(wifiPassword), client(espClient) {}

void Controller::begin() {
  WiFi.mode(WIFI_STA);
  WiFi.setAutoReconnect(true);
  WiFi.begin(ssid, password);  // Non-blocking.
  client.setServer(MQTT_SERVER, MQTT_PORT);
  client.setBufferSize(256);
}

bool Controller::ensureMqtt() {
  if (client.connected() || !wifiConnected()) return client.connected();

  const unsigned long now = millis();
  if (now - lastReconnectAttempt < 5000UL) return false;
  lastReconnectAttempt = now;

  String clientId = String(DEVICE_ID) + "-" +
                    String((uint32_t)esp_random(), HEX);
  Serial.printf("[MQTT] Connecting to %s:%u\n", MQTT_SERVER, MQTT_PORT);

  if (client.connect(clientId.c_str())) {
    Serial.println("[MQTT] connected");
    return true;
  }

  Serial.printf("[MQTT] connect failed, state=%d\n", client.state());
  return false;
}

void Controller::loop() {
  ensureMqtt();
  if (client.connected()) client.loop();

  // Retry queued events without blocking the sensor loop.
  if (client.connected() && eventPending) publishPendingEvent();
}

void Controller::publishFallEvent(const char* sourcePayload) {
  if (sourcePayload == nullptr) return;

  // Ignore the same fall event if the caller reports it more than once.
  if (strcmp(sourcePayload, lastPublishedEvent) == 0) return;
  if (eventPending && strcmp(sourcePayload, pendingSourcePayload) == 0) return;

  strlcpy(pendingSourcePayload, sourcePayload, sizeof(pendingSourcePayload));
  eventPending = true;
  Serial.printf("[MQTT] fall event queued: %s\n", pendingSourcePayload);

  if (client.connected()) publishPendingEvent();
}

bool Controller::publishPendingEvent() {
  if (!eventPending || !client.connected()) return false;

  const unsigned long now = millis();
  if (lastEventFailure != 0 && now - lastEventFailure < RETRY_MS) return false;

  StaticJsonDocument<256> doc;
  doc["device_id"] = DEVICE_ID;
  doc["event"] = "fall_detected";
  doc["message"] = "Fall Detected!";
  doc["source_payload"] = pendingSourcePayload;
  doc["timestamp_ms"] = now;

  char payload[256];
  const size_t n = serializeJson(doc, payload, sizeof(payload));
  if (n == 0 || n >= sizeof(payload)) return false;

  if (!client.publish(EVENT_TOPIC, payload)) {
    lastEventFailure = now;
    Serial.println("[MQTT] fall event publish failed; will retry");
    return false;
  }

  strlcpy(lastPublishedEvent, pendingSourcePayload,
          sizeof(lastPublishedEvent));
  eventPending = false;
  lastEventFailure = 0;
  Serial.printf("[MQTT] published: %s\n", payload);
  return true;
}

void Controller::updatePatientStatus(bool enabledAlert, bool enabledSensor) {
  if (!client.connected()) return;

  const bool alertChanged = !haveReportedStatus || enabledAlert != reportedAlert;
  const bool sensorChanged = !haveReportedStatus || enabledSensor != reportedSensor;
  if (!alertChanged && !sensorChanged) return;

  const unsigned long now = millis();
  if (lastStatusFailure != 0 && now - lastStatusFailure < RETRY_MS) return;

  StaticJsonDocument<128> doc;
  doc["device_id"] = DEVICE_ID;
  doc["enabled_alert"] = enabledAlert;
  doc["enabled_sensor"] = enabledSensor;

  char payload[128];
  const size_t n = serializeJson(doc, payload, sizeof(payload));
  if (n == 0 || n >= sizeof(payload)) return;

  if (!client.publish(STATUS_TOPIC, payload)) {
    lastStatusFailure = now;
    Serial.println("[MQTT] patient status publish failed");
    return;
  }

  reportedAlert = enabledAlert;
  reportedSensor = enabledSensor;
  haveReportedStatus = true;
  lastStatusFailure = 0;
}
