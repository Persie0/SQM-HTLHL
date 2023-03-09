#ifndef SEEING_FKT_H
#define SEEING_FKT_H
#include <Arduino.h>
#include <vector>

int get_cloud_state(float ambient, float object, double SP1, double SP2);
bool UART_shutdown_Seeing();
bool UART_get_Seeing(String &seeing);
void check_seeing_threshhold(int seeing_thr, int &GOOD_SKY_STATE_COUNT, int &BAD_SKY_STATE_COUNT, std::vector<bool> &lastSeeingChecks, int CLOUD_STATE, float lux, double MAX_LUX, bool &SEEING_ENABLED, int SLEEPTIME_s);
#endif