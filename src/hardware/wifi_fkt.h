#ifndef WIFI_FKT_H
#define WIFI_FKT_H
#include <Arduino.h>
bool post_data(char *SEND_VALUES_SERVER, bool raining, int luminosity, String seeing, int nelm, int concentration, int object, int ambient, int lux, int lightning_distanceToStorm, std::vector<String> sensorErrors, bool SEEING_ENABLED);
bool fetch_settings(char *FETCH_SETTINGS_SERVER, int &seeing_thr, double &SP1, double &SP2, double &MAX_LUX, int &SLEEPTIME_s, int DISPLAY_TIMEOUT_s, int &DISPLAY_ON, double &SQM_LIMIT);
bool getSavedWifiSettings(char *WIFI_SSID, char *WIFI_PASS, char *SERVER_IP, char *SEND_VALUES_SERVER, char *FETCH_SETTINGS_SERVER);
void activate_access_point();
#endif