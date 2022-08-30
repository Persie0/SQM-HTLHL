#ifndef SENSOR_IR_TEMPERATURE_H
#define SENSOR_IR_TEMPERATURE_H
bool init_MLX90614();
void read_MLX90614(float &ambient,float &object);
#endif