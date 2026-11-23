#include "loraSender.h"
#include "loraConfig.h"
#include <Arduino.h>
#include <SPI.h>
#include <LoRa.h>

LORAMODULE::LORAMODULE(long _mhz, int sck, int miso, int mosi, int ss, int reset, int dio0) {
  mHz = _mhz;
  _sck = sck;
  _miso = miso;
  _mosi = mosi;
  _ss = ss;
  _reset = reset;
  _dio0 = dio0;
}

LORAMODULE::~LORAMODULE() {}

void LORAMODULE::initLora() {
  SPI.begin(_sck, _miso, _mosi, _ss);
  LoRa.setPins(_ss, _reset, _dio0);

  if (!LoRa.begin(mHz)) {
    Serial.println("Starting LoRa failed!");
    while (1);
  }

  LoRa.setSpreadingFactor(LORA_SPREADING_FACTOR);
  LoRa.setSignalBandwidth(LORA_SIGNAL_BANDWIDTH);
  LoRa.setCodingRate4(LORA_CODING_RATE);
  LoRa.setSyncWord(LORA_SYNC_WORD);
}

void LORAMODULE::sendNotify(const char * message) {
  if (message != NULL) {
    LoRa.beginPacket();
    LoRa.print(message);
    LoRa.print('|');   
    LoRa.print(cntr);
    int result = LoRa.endPacket();
    Serial.print("LoRa packet sent, result: ");
    Serial.println(result);
    cntr++;
  }
}
