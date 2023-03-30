#ifndef SENSOR_LIGHTNING_H
#define SENSOR_LIGHTNING_H
bool init_AS3935( TwoWire &wirePort );
bool read_AS3935(int &lightning_distanceToStorm);
#endif