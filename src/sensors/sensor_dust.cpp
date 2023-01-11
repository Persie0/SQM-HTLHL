#include "settings.h"
#include <Arduino.h>

volatile unsigned long duration;
unsigned long starttime;
unsigned long lowpulseoccupancy = 0;
float ratio = 0;
float temp_concentration = 0;

void read_particle(int &concentration)
{
  starttime = millis(); // get the current time;
  while ((millis() - starttime) < sampletime_ms)
  {
    duration = pulseIn(particle_pin, LOW);
    lowpulseoccupancy = lowpulseoccupancy + duration;
  }
  ratio = lowpulseoccupancy / (sampletime_ms * 10.0);                                  // Integer percentage 0=>100
  temp_concentration = 1.1 * pow(ratio, 3) - 3.8 * pow(ratio, 2) + 520 * ratio + 0.62; // using spec sheet curve
  if (temp_concentration > 1.0)
  {
    concentration = temp_concentration;
  }
  else
  {
    concentration = -333;
  }
}
