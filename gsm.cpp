/*
  TODO::
    1. RESET CONNECTION 
    2. RESET GSM WHEN LOST CONNECTION 
    3. TEST LONGER STRING (>60 BYTES)
    4. CHECK CONNECTION BEFORE SEND 
    5. DISCONNET AFTER SEND MSG

    //FlashStringHelper parser and char * parser


*/
#include "gsm.h"
size_t str2cmp(char * buf, char * reply);
size_t str2cmp(char * buf, flashString reply);

size_t str2len(char * buf);
size_t str2len(flashString buf);

char * str2str(char * buf, char * reply);
char * str2str(char * buf, flashString reply);

char * str2cpy(char * buf, char * reply, uint8_t len);
char * str2cpy(char * buf, flashString reply, uint8_t len);

double min2deg(double num, char * direction);

gsm::gsm(Stream *s, int pwrPin){
  mySerial = s;
  _powerPin = pwrPin;
}

bool gsm::begin(){
  uint8_t i = 0;
  while(i++ < 5){
    if(_sendreply(F("AT"), F(OK_REPLY))){
      return true;
    }
    delay(500);
  }
  return false;
}

bool gsm::setbaudrate(long baudrate){
  char cmd[50];
  memset(cmd, '\0', sizeof(cmd));
  sprintf(cmd, "AT+IPR=%ld", baudrate);

  if(_sendreply(cmd, F(OK_REPLY))) return true;

  return false;
}

bool gsm::registerapn(char *apn, char * username="", char * password=""){
  char cmd[50];
  memset(cmd, '\0', sizeof(cmd));
  sprintf(cmd, "AT+CGDCONT=1,\"IP\",\"%s\"",apn);
  
  if(!_sendreply(cmd, F(OK_REPLY))) return false;
  if(!_checkreply("AT+CGPADDR", F("+CGPADDR: "), 10000)) return false;

  char * ptr = buf; 
  if(ptr[0] == '1'){
    ptr += 2;
    if(ptr != 0){
      // flush replies from gsm after confirm connected to internet
      _flushInput(); 
      
      return true;
    } 
  }

  return false;
}

bool gsm::startmqtt(bool onoff){
  if(onoff){
    if(!_sendreply(F("AT+CMQTTSTART"), F(OK_REPLY))) {
      startmqtt(false);
      return false;
    }
  }
  else{
    disconnect();
    releaseClient();
    if(!_sendreply(F("AT+CMQTTSTOP"), F("+CMQTTNONET"))) {
      startmqtt(true);
      return false;
    }
  }

  return true;
}
bool gsm::releaseClient(){
    return _sendreply(F("AT+CMQTTREL=0"), F(OK_REPLY));
}

bool gsm::connectmqtt(char *host, long port){
  strcpy(serverHost, host);
  this->serverPort = port;
  if(_sendreply(F("AT+CMQTTACCQ=0,\"DAIDEN\""), F("ERROR"))){
    disconnect();
    releaseClient();
    return false;
  }

  return connect();
}

bool gsm::connect(){
  char cmd[150];
  memset(cmd, '\0', sizeof(cmd));
  sprintf(cmd, "AT+CMQTTCONNECT=0,\"tcp://%s:%ld\",60,1", serverHost, serverPort);

  //error checking for connectin had been established or not
  if(!_checkreply(F("AT+CMQTTCONNECT?"), F("+CMQTTCONNECT: "))) return false;
  if(strlen(buf) > 1) return true;

  //connecting to the host server  
  if(!_checkreply(cmd, F("+CMQTTCONNECT: "), 10000)) return false;
  char * p = buf;
  if(p == 0) return false;

  p += 2;
  // if not error which is not 0x30, return false
  if(p[0] != 0x30) return false; 

  return true;
}

bool gsm::disconnect(){
  //check connection before disconnect(), if no connection, return true
  if(!_checkreply(F("AT+CMQTTCONNECT?"), F("+CMQTTCONNECT: "))) return false;
  uint8_t len = strlen(buf);
  if(len <= 1) return true;

  return _sendreply(F("AT+CMQTTDISC=0,120"), F(OK_REPLY));
}
bool gsm::publish(char * topic, char * message, long timeout){
  if(!connect()) return false;
  char cmd[50];
  memset(cmd, '\0', sizeof(cmd));
  sprintf(cmd, "AT+CMQTTTOPIC=0,%d", (int)strlen(topic));
  _send(cmd);
  if(!_sendreply(topic, F(OK_REPLY), timeout)) return false;

  memset(cmd, '\0', sizeof(cmd));
  sprintf(cmd,"AT+CMQTTPAYLOAD=0,%d",(int)strlen(message));
  _send(cmd);
  if(!_sendreply(message, F(OK_REPLY), timeout)) return false;

  if(!_sendreply(F("AT+CMQTTPUB=0,1,120"), F(OK_REPLY), timeout)) return false;

  if(!disconnect()) return false;

  return true;
}
bool gsm::enablegps(bool onoff){
  bool status = _sendreply(F("AT+CGPS=0"), F(OK_REPLY));
  if(onoff){
    return _sendreply(F("AT+CGPS=1"), F(OK_REPLY));
  }

  return status;
} 
bool gsm::readgps(char *latitude, char *longitude, char *speed){
  if(!_checkreply(F("AT+CGPS?"), F("+CGPS: "))) return false;
  char * p = buf; 
  if((p == 0) || (p[0] != 0x31)) {
    enablegps(true);
    return false;
  }

  if(!_checkreply(F("AT+CGPSINFO"), F("+CGPSINFO: "))) return false;

  //latitude
  char *latp = strtok(buf, ",");
  if(!latp) return false;
  //DEBUG_PRINT("latp: ");DEBUG_PRINTLN(latp);

  //direction of latitude
  char *tok = strtok(NULL, ",");
  if(!tok) return false;

  memset(latitude, '\0', sizeof(latitude));
  double lat = min2deg(atof(latp), tok);
  dtostrf(lat, 8, 6, latitude); 

  //longitude
  char * lonp = strtok(NULL, ",");
  if(!lonp) return false;
    
  //longitude direction
  tok = strtok(NULL, ",");
  if(!tok) return false;

  memset(longitude, '\0', sizeof(longitude));
  double lon = min2deg(atof(lonp), tok);
  dtostrf(lon, 10, 6, longitude); 
  
  //time
  tok = strtok(NULL, ",");
  if(!tok) return false;

  //speed
  char * spdp = strtok(NULL, ",");
  if(!spdp) return false;

  memset(speed, '\0', sizeof(speed));
  dtostrf(atof(spdp), 2, 2, speed); 

  return true;
}
/*////////////////////
low level function 
//////////////////*/

bool gsm::_send(char * cmd){
  _flushInput();
  DEBUG_PRINT(F("\t\t--->"));DEBUG_PRINTLN(cmd);

  mySerial->flush();
  mySerial->print(cmd);
  mySerial->print(F("\r\n"));
  delay(100);
}

bool gsm::_send(flashString cmd){
  _flushInput();
  DEBUG_PRINT(F("\t\t--->"));DEBUG_PRINTLN(cmd);

  mySerial->flush();
  mySerial->print(cmd);
  mySerial->print(F("\r\n"));
  delay(100);
}

bool gsm::_sendreply(char * cmd, flashString reply, long timeout){
  _send(cmd);
  return _getreply(reply, timeout);
}
bool gsm::_sendreply(flashString cmd, flashString reply, long timeout){
  _send(cmd);
  return _getreply(reply, timeout);
}

bool gsm::_getreply(flashString reply, long timeout){
  long i = 0;
  while(i++ < timeout){
    memset(buf, '\0', sizeof(buf));
    if(_readline(buf)){
      i = 0;
      if(str2str(buf, F("AT+")) != 0) continue;

      DEBUG_PRINT(F("\t\t<---"));DEBUG_PRINTLN(buf);
      if(str2cmp(buf, reply) == 0){
        return true;
      }
    }
  }
  return false;
}
bool gsm::_checkreply(char * cmd, flashString reply, long timeout){
  _send(cmd);
  return _readreply(reply, timeout);
}

bool gsm::_checkreply(flashString cmd, flashString reply, long timeout){
  _send(cmd);
  return _readreply(reply, timeout);
}

bool gsm::_readreply(flashString reply, long timeout){
  long i = 0;
  while(i++ < timeout){
    memset(buf, '\0', sizeof(buf));
    uint8_t rep = _readline(buf);
    if(rep){
      i = 0;
      char * p = str2str(buf, reply);
      if(p == 0) continue;

      p+=str2len(reply);
      if(p == 0) continue;

      DEBUG_PRINT(F("\t\t<---"));DEBUG_PRINTLN(buf);
      uint8_t len = max(sizeof(buf) - 1, (int)str2len(p));
      str2cpy(buf, p, len);
      *(buf + len) = '\0';

      return true;
    }
  }
  return false;
}

bool gsm::_readline(char * buf){
  uint8_t i = 0;
  while(mySerial->available()){
    char c = mySerial->read();
    if(c < 0x7F){
      if(c == 0xD) continue;
      if(c == 0xA){
        if(i == 0) continue;
        else break;
      }

      *(buf + i) = c;
      if(++i >= 254){
        break;
      }
    }
    delay(1);
  }

  return i;
}

void gsm::_flushInput(void){
  uint16_t timeoutloop = 0;
  while(timeoutloop++ < 255){
    memset(buf, '\0', sizeof(buf));
    uint8_t rep = _readline(buf);
    if(rep) {
      timeoutloop = 0;
    }
    delay(1);
  }
}

size_t str2cmp(char * buf, char * reply){
  return strcmp(buf, reply);
}

size_t str2cmp(char * buf, flashString reply){
  return strcmp_P(buf, (prog_char *)reply);
}

size_t str2len(char * buf){
  return strlen(buf);
}

size_t str2len(flashString buf){
  return strlen_P((prog_char *)buf);
} 

char * str2str(char * buf, char * reply){
  return strstr(buf, reply);
}

char * str2str(char * buf, flashString reply){
  return strstr_P(buf, (prog_char *)reply);
}

char * str2cpy(char * buf, char * reply, uint8_t len){
  return strncpy(buf, reply, len);
}

char * str2cpy(char * buf, flashString reply, uint8_t len){
  return strncpy_P(buf, (prog_char *)reply, len);
}

double min2deg(double num, char * direction){
  float deg = floor(num / 100);
  double min = num - (100 * deg);
  min /= 60;
  deg += min;

  if(direction == 'W' || direction == 'S') {
    deg *= -1;
  }
  return deg;
}