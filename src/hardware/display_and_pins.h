#ifndef DISPLAY_AND_PINS_H
#define DISPLAY_AND_PINS_H
#include <Arduino.h>
void DisplayStatusMessage(bool hasWIFI, bool hasServerError, bool settingsLoaded, int sendCount, int noWifiCount, bool sleepForever, bool DISPLAY_ON);
void high_hold_Pin(gpio_num_t pin);
void low_hold_Pin(gpio_num_t pin);
#endif