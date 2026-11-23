#ifndef __LORAMODULE__H
#define __LORAMODULE__H

#include "loraConfig.h"

class LORAMODULE {
  public:
    LORAMODULE(long _mhz = LORA_FREQUENCY_MHZ,
               int sck = LORA_SCK_PIN, int miso = LORA_MISO_PIN,
               int mosi = LORA_MOSI_PIN, int ss = LORA_SS_PIN,
               int reset = LORA_RESET_PIN, int dio0 = LORA_DIO0_PIN);
    ~LORAMODULE();
    void initLora();
    void sendNotify(const char * message = NULL);
  private:
    long mHz;
    int _sck, _miso, _mosi, _ss, _reset, _dio0;
    int cntr = 0;
};

#endif
