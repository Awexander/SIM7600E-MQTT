#ifndef GSM_H
#define GSM_H

#include <Arduino.h>

#define OK_REPLY "OK"
#define DEFAULT_TIMEOUT 5000
#define DEBUG_SERIAL
#ifdef DEBUG_SERIAL
#define DEBUG_PRINT(...) Serial.print(__VA_ARGS__)
#define DEBUG_PRINTLN(...) Serial.println(__VA_ARGS__)
#else
#define DEBUG_PRINT(...)
#define DEBUG_PRINTLN(...)
#endif

typedef const __FlashStringHelper *flashString;
#define prog_char char PROGMEM

class gsm{
  public:
  //configuration pins
  uint8_t _powerPin;
  gsm(Stream *s, int pwrPin);

  bool begin();
  bool setbaudrate(long baudrate);

  bool registerapn(char * apn, char * username="", char * password="");
  bool enablegps(bool); 
  bool readgps(char *latitude, char *longitude, char *speed);

  //connect to mqtt broker
  bool connect();

  // disconnect and release client from broker
  bool disconnect();
  bool releaseClient();

  //start mqtt service on gsm
  bool startmqtt(bool onoff);

  //connect to mqtt broker
  bool connectmqtt(char * host, long port);
  
  //publish message to mqtt broker
  bool publish(char * topic, char * message, long timeout=DEFAULT_TIMEOUT);

  protected:
  //mySerial.print cmd to send AT command to gsm
  bool _send(char * cmd);
  bool _send(flashString cmd);
  
  //send cmd and strcmp replies from gsm, ie: OK
  bool _sendreply(char * cmd, flashString reply, long timeout=DEFAULT_TIMEOUT);
  bool _sendreply(flashString cmd, flashString reply, long timeout=DEFAULT_TIMEOUT);

  //send cmd and read replies or information from gsm, ie: +GPSINFO: 
  bool _checkreply(char * cmd, flashString reply, long timeout=DEFAULT_TIMEOUT);
  bool _checkreply(flashString cmd, flashString reply, long timeout=DEFAULT_TIMEOUT);
  
  //read replies from gsm
  bool _getreply(flashString reply, long timeout);
  bool _readreply(flashString reply, long timeout);

  //read replies from gsm by line
  bool _readline(char * buf);
  void _flushInput();

  protected:
  Stream *mySerial;
  char buf[255];
  char serverHost[50];
  long serverPort;
};

#endif
