
#include "gsm.h"
#include <SoftwareSerial.h>

extern gsm gsm;
extern SoftwareSerial gsmSerial;

bool gsmconnect(char * apn, char * serverHost){
  while(!gsm.enablegps(true)){
    Serial.println(F("failed to initialised gps"));
    delay(1000);
  }

  while(!gsm.registerapn(apn)){
    Serial.println(F("failed to register apn"));
    delay(1000);
  }

  while(!gsm.startmqtt(true)){
    Serial.println(F("failed to start mqtt service"));
    delay(1000);
  }

  while(!gsm.connectmqtt(serverHost, 1883)){
    Serial.println(F("failed to connect to mqtt broker"));
    delay(1000);
  }
}

bool initgsm(long baudrate){
  long rate[] = {115200, 57600, 38400, 9600};
  uint8_t i = 0;
  while(!gsm.begin()){
    Serial.print(F("trying to connect to gsm via "));Serial.print(rate[i]);Serial.println(F(" baudrate"));
    gsmSerial.end();
    delay(200);
    gsmSerial.begin(rate[i++]);
    delay(1000);

    if(i > 3) i = 0;
  }

  while(!gsm.setbaudrate(baudrate)){
    delay(1000);
  }

  gsmSerial.end();
  delay(200);
  gsmSerial.begin(baudrate);
}