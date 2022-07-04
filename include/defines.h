
#include <Arduino.h>

#ifndef mydefines_h
#define mydefines_h

// WiFi credentials.
const char* WIFI_SSID = "FRITZ!Box 7590 RN";
const char* WIFI_PASS = "63341235201028204472";

//Your Domain name with URL path or IP address with path
const char* serverName = "http://192.168.188.38:5000/SQM";


#define particle_pin           27                 //particle sensor pin; reciving the sensor values with it
#define particle_retries    5

#define lightning_pin           0                 //lightning Sensor Pin; reciving the sensor values with it
// 0x03 is default, but the address can also be 0x02, or 0x01.
// Adjust the address jumpers on the underside of the product.
#define AS3935_ADDR 0x00
#define INDOOR 0x12
#define OUTDOOR 0xE
#define LIGHTNING_INT 0x08
#define DISTURBER_INT 0x04
#define NOISE_INT 0x01


#define SQMpin     25                 // TSL237 OUT to digital pin 25 (cannot change)
#define rainS_AO           34                // the rain sensor analog port pin, voltage
#define rainS_DO           35                 // the rain sensor digital port pin, raining=yes/no

#define uS_FACTOR 1000000 /* Conversion factor for micro seconds to seconds */ 
#define SLEEPTIME_s 180 /* Time ESP32 will go to sleep (in seconds) */

#define sqm_limit 21.83                 // mag limit for earth is 21.95

#endif