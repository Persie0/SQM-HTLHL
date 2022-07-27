#include <SparkFunTSL2561.h>
#include "settings.h"

// Create an SFE_TSL2561 object, here called "light":
SFE_TSL2561 light;

unsigned int ms; // Integration ("shutter") time in milliseconds


bool init_TSL2561()
{
  // Initialize the SFE_TSL2561 library

  // You can pass nothing to light.begin() for the default I2C address (0x39),
  // or use one of the following presets if you have changed
  // the ADDR jumper on the board:

  // TSL2561_ADDR_0 address with '0' shorted on board (0x29)
  // TSL2561_ADDR   default address (0x39)
  // TSL2561_ADDR_1 address with '1' shorted on board (0x49)

  if (!light.begin())
    return false;

  // The light sensor has a default integration time of 402ms,
  // and a default gain of low (1X).

  // If you would like to change either of these, you can
  // do so using the setTiming() command.


  if (!light.setTiming(gain, shuttertime, ms))
    return false;

  // To start taking measurements, power up the sensor:
  if (!light.setPowerUp())
    return false;

  // The sensor will now gather light during the integration time.
  // After the specified time, you can retrieve the result from the sensor.
  // Once a measurement occurs, another integration period will start.
  return true;
}

bool read_TSL2561(double &lux)
{

  // There are two light sensors on the device, one for visible light
  // and one for infrared. Both sensors are needed for lux calculations.

  // Retrieve the data from the device:

  unsigned int data0, data1;

  if (light.getData(data0, data1))
  {
    // getData() returned true, communication was successful

    // To calculate lux, pass all your settings and readings
    // to the getLux() function.

    // The getLux() function will return 1 if the calculation
    // was successful, or 0 if one or both of the sensors was
    // saturated (too much light). If this happens, you can
    // reduce the integration time and/or gain.
    // For more information see the hookup guide at: https://learn.sparkfun.com/tutorials/getting-started-with-the-tsl2561-luminosity-sensor

    boolean good; // True if neither sensor is saturated

    // Perform lux calculation:

    good = light.getLux(gain, ms, data0, data1, lux);

    // Print out the results:
      
    return true;
  }
  else
  {
    // getData() returned false because of an I2C error
    return false;
  }
}
