#include "loraReceiver.h"

LoraReceiver receiver;

void setup() {
  Serial.begin(115200);
  delay(500);
  receiver.begin();
}

void loop() {
  receiver.poll();
  receiver.updateDisplay();
}
