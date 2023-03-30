#ifndef WIFI_FKT_H
#define WIFI_FKT_H
#include <Arduino.h>

/// @brief Send the sensor data to the server
/// @param SEND_VALUES_SERVER the server ip route to send the data to
/// @param raining 
/// @param luminosity 
/// @param seeing 
/// @param nelm 
/// @param concentration 
/// @param object 
/// @param ambient 
/// @param lux 
/// @param lightning_distanceToStorm 
/// @param sensorErrors 
/// @param SEEING_ENABLED 
/// @return true if the data was sent successfully, false otherwise
bool post_data(char *SEND_VALUES_SERVER, bool raining, float luminosity, String seeing, double nelm, int concentration, float object, float ambient, double lux, int lightning_distanceToStorm, std::vector<String> sensorErrors, bool SEEING_ENABLED);

/// @brief Fetch the settings from the server
/// @param FETCH_SETTINGS_SERVER the server ip route to fetch the settings from
/// @param seeing_thr
/// @param SP1
/// @param SP2
/// @param MAX_LUX
/// @param SLEEPTIME_s
/// @param DISPLAY_TIMEOUT_s
/// @param DISPLAY_ON
/// @param SQM_LIMIT
/// @return true if the settings were fetched successfully, false otherwise
bool fetch_settings(char *FETCH_SETTINGS_SERVER, int &seeing_thr, double &SP1, double &SP2, double &MAX_LUX, int &SLEEPTIME_s, int &DISPLAY_TIMEOUT_s, int &DISPLAY_ON, double &SQM_LIMIT);

/// @brief Read the saved wifi settings from the SPIFFS file system
/// @param WIFI_SSID the wifi ssid
/// @param WIFI_PASS the wifi password
/// @param SERVER_IP the server ip
/// @param SEND_VALUES_SERVER the server ip route to send the data to
/// @param FETCH_SETTINGS_SERVER the server ip route to fetch the settings from
bool getSavedWifiSettings(char *WIFI_SSID, char *WIFI_PASS, char *SERVER_IP, char *SEND_VALUES_SERVER, char *FETCH_SETTINGS_SERVER);

/// @brief activate the access point and save the obtained wifi settings to the SPIFFS file system
void activate_access_point();
#endif