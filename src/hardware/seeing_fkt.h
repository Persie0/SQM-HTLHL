#ifndef SEEING_FKT_H
#define SEEING_FKT_H
#include <Arduino.h>

/// @brief Get the cloud state based on the ambient and object temperature
/// @param ambient the current ambient temperature
/// @param object the current object temperature
/// @param SP1 setpoint 1 is set for clear skies, a value that determines the threshold for the cloud state
/// @param SP2 setpoint 2 is set for cloudy skies, a value that determines the threshold for the cloud state
/// @return the cloud state, 1 for clear, 2 for partly cloudy, 3 for cloudy, 0 for unknown
int get_cloud_state(float ambient, float object, double SP1, double SP2);

/// @brief Send the "shut" command over UART to indicate planning to shut down the seeing
/// @return true if the command was sent successfully, false otherwise
bool UART_shutdown_Seeing();

/// @brief Send the "see" command over UART to get the current seeing value
/// @param seeing the current seeing value
/// @return true if the seeing value was received successfully, false otherwise
bool UART_get_Seeing(String &seeing);

/// @brief Check wheter to enable or disable the seeing based on the current sky state
void check_seeing_threshhold(int seeing_thr, int &GOOD_SKY_STATE_COUNT, int &BAD_SKY_STATE_COUNT, int CLOUD_STATE, float lux, double MAX_LUX, bool &SEEING_ENABLED, int SLEEPTIME_s);
#endif