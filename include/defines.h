
#include <Arduino.h>

#ifndef mydefines_h
#define mydefines_h



#define particle_pin           27                 //particle sensor pin; reciving the sensor values with it
#define lightning_pin           15                 //lightning Sensor Pin; reciving the sensor values with it
#define SQMpin     25                 // TSL237 OUT to digital pin 5 (cannot change)
#define rainS_AO           34                // the rain sensor analog port pin, voltage
#define rainS_DO           35                 // the rain sensor digital port pin, raining=yes/no
esp_sleep_enable_timer_wakeup(TIME_TO_SLEEP * uS_TO_S_FACTOR);
#define uS_TO_S_FACTOR 1000000 /* Conversion factor for micro seconds to seconds */ 
#define TIME_TO_SLEEP 5 /* Time ESP32 will go to sleep (in seconds) */

#define sqm_limit 21.83                 // mag limit for earth is 21.95

#endif