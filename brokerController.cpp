#include "brokerController.h"

Controller::Controller(const char* wifiSsid, const char* wifiPassword)
    : ssid(wifiSsid), password(wifiPassword), client(espClient) {}

void Controller::begin() {
  WiFi.mode(WIFI_STA);
  WiFi.setAutoReconnect(true);
  WiFi.begin(ssid, password); 
  client.setServer(MQTT_SERVER, MQTT_PORT);
  client.setBufferSize(256);
  // Keep failed broker attempts short so the sensor/LoRa loop can continue.
  client.setSocketTimeout(1);
  client.setKeepAlive(15);
}

bool Controller::ensureMqtt() {
  if (client.connected() || !wifiConnected()) return client.connected();

  const unsigned long now = millis();
  if (now - lastReconnectAttempt < MQTT_RECONNECT_INTERVAL_MS) return false;
  lastReconnectAttempt = now;

  String clientId = String(DEVICE_ID) + "-" +
                    String((uint32_t)esp_random(), HEX);
  Serial.printf("[MQTT] Connecting to %s:%u\n", MQTT_SERVER, MQTT_PORT);

  if (client.connect(clientId.c_str())) {
    Serial.println("[MQTT] connected");
    haveReportedSensor = false;
    return true;
  }

  Serial.printf("[MQTT] connect failed, state=%d\n", client.state());
  return false;
}

void Controller::loop() {
  ensureMqtt();
  if (client.connected()) client.loop();
}

void Controller::updatePatientStatus(bool enabledAlert, bool enabledSensor) {
  if (enabledAlert && !lastAlert) {
    pendingFall = true;
    pendingClear = false;
  } else if (!enabledAlert && lastAlert && AUTO_CLEAR_FALL) {
    pendingClear = true;
    pendingFall = false;
  }
  lastAlert = enabledAlert;

  if (!client.connected()) return;

  const unsigned long now = millis();
  if (lastStatusFailure != 0 && now - lastStatusFailure < RETRY_MS) return;

  const bool sensorChanged = !haveReportedSensor || enabledSensor != reportedSensor;
  if (!pendingFall && !pendingClear && !sensorChanged) return;

  StaticJsonDocument<128> doc;
  doc["device_id"] = DEVICE_ID;
  if (pendingFall) doc["hasFall"] = true;       
  else if (pendingClear) doc["hasFall"] = false;
  if (sensorChanged) doc["enabled_sensor"] = enabledSensor;

  char payload[128];
  const size_t n = serializeJson(doc, payload, sizeof(payload));
  if (n == 0 || n >= sizeof(payload)) return;

  if (!client.publish(STATUS_TOPIC, payload)) { 
    lastStatusFailure = now;
    Serial.println("[MQTT] patient status publish failed, will retry");
    return;
  }

  pendingFall = false;
  pendingClear = false;
  if (sensorChanged) {
    reportedSensor = enabledSensor;
    haveReportedSensor = true;
  }
  lastStatusFailure = 0;
  Serial.printf("[PATIENT TX] %s\n", payload);
}
