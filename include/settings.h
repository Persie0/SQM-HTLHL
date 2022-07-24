
#include <Arduino.h>

#ifndef mydefines_h
#define mydefines_h


// WiFi credentials.
const char* WIFI_SSID = "FRITZ!Box 7590 RN";
const char* WIFI_PASS = "63341235201028204472";
//Your Domain name with URL path or IP address with path
const char* serverName = "http://192.168.188.38:5000/SQM";


#define particle_pin           27                 //particle sensor pin; reciving the sensor values with it

#define lightning_pin           0                 //lightning Sensor Pin; reciving the sensor values with it

// Values for modifying the IC's settings. All of these values are set to their
// default values.
byte noiseFloor = 2;
byte watchDogVal = 2;
byte spike = 2;
byte lightningThresh = 1;


#define SQMpin     25                 // TSL237 OUT to digital pin 25 (cannot change)
#define rainS_AO           34                // the rain sensor analog port pin, voltage
#define rainS_DO           35                 // the rain sensor digital port pin, raining=yes/no

#define SLEEPTIME_s 180 /* Time ESP32 will go to sleep (in seconds) */
#define EN_3V3          2                // power-control pin to enable the 3,3V for the sensors
#define EN_5V          15                // power-control pin to enable the 5V for the sensors

#define sqm_limit 21.83                 // mag limit for earth is 21.95

#endif