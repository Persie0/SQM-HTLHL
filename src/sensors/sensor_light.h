#ifndef SENSOR_LIGHT_H
#define SENSOR_LIGHT_H
bool init_TSL2561();
void read_TSL2561(double &lux);
#endif