#include "settings.h"
#include <Arduino.h>

void read_rain(bool &raining)
{
  if (!digitalRead(rainS_DO))
  {
    raining = true;
  }
  else
  {
    raining = false;
  }
}
