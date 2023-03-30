#ifndef DISPLAY_AND_PINS_H
#define DISPLAY_AND_PINS_H
#include <Arduino.h>

/// @brief Display the status message on the OLED display based on the current state
/// @param hasWIFI whether the device is connected to WIFI
/// @param hasServerError whether could not connect to the server
/// @param settingsLoaded whether the settings were loaded successfully from the server
/// @param sendCount how many times the device has sent data to the server successfully
/// @param noWifiCount how many times the device has tried to connect to WIFI but failed
/// @param sleepForever whether the device is in sleep forever mode
/// @param DISPLAY_ON whether the display should be turned on
void DisplayStatusMessage(bool hasWIFI, bool hasServerError, bool settingsLoaded, int sendCount, int noWifiCount, bool sleepForever, bool DISPLAY_ON);

/// @brief switch the specified pin to high state and hold it during deep sleep
/// @param pin the pin to switch to high state
void high_hold_Pin(gpio_num_t pin);

/// @brief switch the specified pin to low state and hold it during deep sleep
/// @param pin the pin to switch to low state
void low_hold_Pin(gpio_num_t pin);
#endif