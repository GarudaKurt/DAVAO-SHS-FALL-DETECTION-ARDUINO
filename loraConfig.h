#ifndef DAVAO_SHS_LORA_CONFIG_H
#define DAVAO_SHS_LORA_CONFIG_H

// Both radios must use the same frequency.
#define LORA_FREQUENCY_MHZ 433E6

// SX127x wiring used by the existing transmitter.
// PIN CONNECTION LORA -> ESP32
// NOTE LORA VCC pins should connect to 3.3v of ESP32
#define LORA_SCK_PIN 14
#define LORA_MISO_PIN 19
#define LORA_MOSI_PIN 23
#define LORA_SS_PIN 27
#define LORA_RESET_PIN 33
#define LORA_DIO0_PIN 32

#define LORA_SPREADING_FACTOR 7
#define LORA_SIGNAL_BANDWIDTH 125E3
#define LORA_CODING_RATE 5
#define LORA_SYNC_WORD 0x12

#endif
