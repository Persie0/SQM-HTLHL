#include <SparkFunMLX90614.h>
#include "settings.h"

IRTherm therm; // Create an IRTherm object to interact with throughout

bool init_MLX90614()
{
    if (therm.begin() == false)
    { // Initialize thermal IR sensor

    }
    therm.setUnit(TEMP_C); // Set the library's units to Fahrenheit
    // Alternatively, TEMP_F can be replaced with TEMP_C for Celsius or
    // TEMP_K for Kelvin.

    return true;
}

bool read_MLX90614(float &ambient,float &object)
{
    // Call therm.read() to read object and ambient temperatures from the sensor.
    if (therm.read()) // On success, read() will return 1, on fail 0.
    {
        // Use the object() and ambient() functions to grab the object and ambient
        // temperatures.
        // They'll be floats, calculated out to the unit you set with setUnit().
        object = therm.object();
        ambient = therm.ambient();
        return true;
    }
    return false;
}
