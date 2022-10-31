
#include <SoftwareSerial.h>
#include "gsm.h"

//pin configuration 
const int rxPin = 2;
const int txPin = 3;
const int rstPin = 4;

// data time constant
unsigned long prevTimer = 0;
unsigned long dataTime = 5000;

//gsm and mqtt configuration
const long gsm_baudrate = 57600;
const char * serverHost = "test.mosquitto.org";
const char * apn = "";

//class and helper function 
SoftwareSerial gsmSerial(rxPin, txPin);
gsm gsm(&gsmSerial, 4);
bool gsmconnect(char * apn, char * serverHost);
bool initgsm(long baudrate);

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  Serial.println("start");
  delay(100);

  //initialised gsm and set the baudrate to read data 
  initgsm(gsm_baudrate);

  //connect to apn and mqtt broker 
  gsmconnect(apn, serverHost);
}

void loop() {
  //only send data every dataTime passed, ie: 5seconds
  if(millis() - prevTimer > dataTime){
    int percentFuel = readFuel();
    char msg[255];
    char latitude[15];
    char longitude[15];
    char speed[5];
    
    //read latitude, longitude and speed from gps 
    /* sprintf() cannot parse float (%f) datatype, so read latitude,
       longitude and speed in char string and passed to sprintf() function.
    */
    gsm.readgps(latitude, longitude, speed);

    //sprintf data that need to be send to mqtt broker
    memset(msg, '\0', sizeof(msg));
    sprintf(msg, "%d,%s,%s", percentFuel, latitude, longitude);

    //publish the data to broker
    gsm.publish("", msg);
    Serial.println(); //beautify Serial monitor
    
    //reset timer
    prevTimer = millis();
  }

  //do nothing
}

int readFuel(){
  return map(analogRead(A0), 0, 1023, 0, 100);
}

