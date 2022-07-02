#include <Arduino.h>
#include "defines.h"

#include <Wire.h>
#include "SparkFun_AS3935.h"
#include <SparkFunMLX90614.h>
#include <SparkFunTSL2561.h>

bool raining;

IRTherm therm; // Create an IRTherm object to interact with throughout

// Create an SFE_TSL2561 object, here called "light":
SFE_TSL2561 light;

boolean gain;    // Gain setting, 0 = X1, 1 = X16;
unsigned int ms; // Integration ("shutter") time in milliseconds

// particle sensor
unsigned long duration;
unsigned long starttime;
unsigned long sampletime_ms = 3000; // sampe 3s ;
unsigned long lowpulseoccupancy = 0;
float ratio = 0;
float concentration = 0;

// 0x03 is default, but the address can also be 0x02, or 0x01.
// Adjust the address jumpers on the underside of the product.
#define AS3935_ADDR 0x00
#define INDOOR 0x12
#define OUTDOOR 0xE
#define LIGHTNING_INT 0x08
#define DISTURBER_INT 0x04
#define NOISE_INT 0x01

// If you're using I-squared-C then keep the following line. Address is set to
// default.
SparkFun_AS3935 lightning(AS3935_ADDR);
// Values for modifying the IC's settings. All of these values are set to their
// default values.
byte noiseFloor = 2;
byte watchDogVal = 2;
byte spike = 2;
byte lightningThresh = 1;
// This variable holds the number representing the lightning or non-lightning
// event issued by the lightning detector.
byte intVal = 0;

void getraining()
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

bool init_MLX90614()
{
  if (therm.begin() == false)
  { // Initialize thermal IR sensor
    Serial.println("Qwiic IR thermometer did not acknowledge! Freezing!");
    while (1)
      ;
  }
  Serial.println("Qwiic IR Thermometer did acknowledge.");

  therm.setUnit(TEMP_C); // Set the library's units to Farenheit
  // Alternatively, TEMP_F can be replaced with TEMP_C for Celsius or
  // TEMP_K for Kelvin.

  return true;
}

void read_MLX90614()
{
  // Call therm.read() to read object and ambient temperatures from the sensor.
  if (therm.read()) // On success, read() will return 1, on fail 0.
  {
    // Use the object() and ambient() functions to grab the object and ambient
    // temperatures.
    // They'll be floats, calculated out to the unit you set with setUnit().
    Serial.print("Object: " + String(therm.object(), 2));
    Serial.println("F");
    Serial.print("Ambient: " + String(therm.ambient(), 2));
    Serial.println("F");
  }
}

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

  // If gain = false (0), device is set to low gain (1X)
  // If gain = high (1), device is set to high gain (16X)

  gain = 0;

  // If time = 0, integration will be 13.7ms
  // If time = 1, integration will be 101ms
  // If time = 2, integration will be 402ms
  // If time = 3, use manual start / stop to perform your own integration

  unsigned char time = 2;

  // setTiming() will set the third parameter (ms) to the
  // requested integration time in ms (this will be useful later):

  if (!light.setTiming(gain, time, ms))
    return false;

  // To start taking measurements, power up the sensor:
  if (!light.setPowerUp())
    return false;

  // The sensor will now gather light during the integration time.
  // After the specified time, you can retrieve the result from the sensor.
  // Once a measurement occurs, another integration period will start.
  return true;
}

bool read_TSL2561()
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

    double lux;   // Resulting lux value
    boolean good; // True if neither sensor is saturated

    // Perform lux calculation:

    good = light.getLux(gain, ms, data0, data1, lux);

    // Print out the results:

    Serial.print(" lux: ");
    Serial.print(lux);
    if (good)
      Serial.println(" (good)");
    else
      Serial.println(" (BAD)");
    return true;
  }
  else
  {
    // getData() returned false because of an I2C error
    return false;
  }
}

bool init_AS3935()
{
  // When lightning is detected the interrupt pin goes HIGH.
  pinMode(lightning_pin, INPUT);

  if (!lightning.begin())
  { // Initialize the sensor.
    Serial.println("Lightning Detector did not start up");
    return false;
  }
  else
    Serial.println("Schmow-ZoW, Lightning Detector Ready!\n");
  // "Disturbers" are events that are false lightning events. If you find
  // yourself seeing a lot of disturbers you can have the chip not report those
  // events on the interrupt lines.
  // lightning.maskDisturber(true);
  int maskVal = lightning.readMaskDisturber();
  Serial.print("Are disturbers being masked: ");
  if (maskVal == 1)
    Serial.println("YES");
  else if (maskVal == 0)
    Serial.println("NO");

  // The lightning detector defaults to an indoor setting (less
  // gain/sensitivity), if you plan on using this outdoors
  // uncomment the following line:
  lightning.setIndoorOutdoor(OUTDOOR);
  int enviVal = lightning.readIndoorOutdoor();
  Serial.print("Are we set for indoor or outdoor: ");
  if (enviVal == INDOOR)
    Serial.println("Indoor.");
  else if (enviVal == OUTDOOR)
    Serial.println("Outdoor.");
  else
    Serial.println(enviVal, BIN);

  // Noise floor setting from 1-7, one being the lowest. Default setting is
  // two. If you need to check the setting, the corresponding function for
  // reading the function follows.

  lightning.setNoiseLevel(noiseFloor);

  int noiseVal = lightning.readNoiseLevel();
  Serial.print("Noise Level is set at: ");
  Serial.println(noiseVal);

  // Watchdog threshold setting can be from 1-10, one being the lowest. Default setting is
  // two. If you need to check the setting, the corresponding function for
  // reading the function follows.

  lightning.watchdogThreshold(watchDogVal);

  int watchVal = lightning.readWatchdogThreshold();
  Serial.print("Watchdog Threshold is set to: ");
  Serial.println(watchVal);

  // Spike Rejection setting from 1-11, one being the lowest. Default setting is
  // two. If you need to check the setting, the corresponding function for
  // reading the function follows.
  // The shape of the spike is analyzed during the chip's
  // validation routine. You can round this spike at the cost of sensitivity to
  // distant events.

  lightning.spikeRejection(spike);

  int spikeVal = lightning.readSpikeRejection();
  Serial.print("Spike Rejection is set to: ");
  Serial.println(spikeVal);

  // This setting will change when the lightning detector issues an interrupt.
  // For example you will only get an interrupt after five lightning strikes
  // instead of one. Default is one, and it takes settings of 1, 5, 9 and 16.
  // Followed by its corresponding read function. Default is zero.

  lightning.lightningThreshold(lightningThresh);

  uint8_t lightVal = lightning.readLightningThreshold();
  Serial.print("The number of strikes before interrupt is triggerd: ");
  Serial.println(lightVal);

  // When the distance to the storm is estimated, it takes into account other
  // lightning that was sensed in the past 15 minutes. If you want to reset
  // time, then you can call this function.

  // lightning.clearStatistics();

  // The power down function has a BIG "gotcha". When you wake up the board
  // after power down, the internal oscillators will be recalibrated. They are
  // recalibrated according to the resonance frequency of the antenna - which
  // should be around 500kHz. It's highly recommended that you calibrate your
  // antenna before using these two functions, or you run the risk of schewing
  // the timing of the chip.

  // lightning.powerDown();
  // delay(1000);
  // if( lightning.wakeUp() )
  //  Serial.println("Successfully woken up!");
  // else
  // Serial.println("Error recalibrating internal osciallator on wake up.");

  // Set too many features? Reset them all with the following function.
  // lightning.resetSettings();
  return true;
}

void read_AS3935()
{
  if (digitalRead(lightning_pin) == HIGH)
  {
    // Hardware has alerted us to an event, now we read the interrupt register
    // to see exactly what it is.
    intVal = lightning.readInterruptReg();
    if (intVal == NOISE_INT)
    {
      Serial.println("Noise.");
      // Too much noise? Uncomment the code below, a higher number means better
      // noise rejection.
      // lightning.setNoiseLevel(setNoiseLevel);
    }
    else if (intVal == DISTURBER_INT)
    {
      Serial.println("Disturber.");
      // Too many disturbers? Uncomment the code below, a higher number means better
      // disturber rejection.
      // lightning.watchdogThreshold(threshVal);
    }
    else if (intVal == LIGHTNING_INT)
    {
      Serial.println("Lightning Strike Detected!");
      // Lightning! Now how far away is it? Distance estimation takes into
      // account any previously seen events in the last 15 seconds.
      byte distance = lightning.distanceToStorm();
      Serial.print("Approximately: ");
      Serial.print(distance);
      Serial.println("km away!");
    }
  }
}

void setup()
{
  Serial.begin(115200); // Initialize Serial to log output
  Wire.begin();         // I2C bus

  init_MLX90614();
  init_TSL2561();
  init_AS3935();
}

void loop()
{
  read_MLX90614();
  read_TSL2561();
  read_AS3935();
  Serial.println();
  vTaskDelay(4000 / portTICK_RATE_MS);
}
