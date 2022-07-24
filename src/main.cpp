#include <Arduino.h>
#include "settings.h"
#include <Wire.h>
#include "SparkFun_AS3935.h"
#include <SparkFunTSL2561.h>
#include "FreqCountESP.h"
#include "esp_wifi.h"
#include "driver/adc.h"
#include <WiFi.h>
#include <HTTPClient.h>
#include <sstream>
#include <SparkFunMLX90614.h>

// 0x03 is default, but the address can also be 0x02, or 0x01.
// Adjust the address jumpers on the underside of the product.
#define AS3935_ADDR 0x00
#define INDOOR 0x12
#define OUTDOOR 0xE
#define LIGHTNING_INT 0x08
#define DISTURBER_INT 0x04
#define NOISE_INT 0x01


#define uS_FACTOR 1000000 /* Conversion factor for micro seconds to seconds */ 

bool raining=-1;
float ambient, object=-1;
double lux=-1; // Resulting lux value
int lightning_distanceToStorm = -1;
float mySQMreading = 0.0; // the SQM value, sky magnitude
double irradiance = 1.0;
double nelm = 1.0;
uint32_t frequency = 1; // measured TSL237 frequency which is dependent on light


// Create an SFE_TSL2561 object, here called "light":
SFE_TSL2561 light;

boolean gain;    // Gain setting, 0 = X1, 1 = X16;
unsigned int ms; // Integration ("shutter") time in milliseconds

volatile unsigned long duration;
unsigned long starttime;
unsigned long sampletime_ms = 3000; // sample 100ms ;
unsigned long lowpulseoccupancy = 0;
float ratio = 0;
float temp_concentration = 0;
int concentration = -1;

// If you're using I-squared-C then keep the following line. Address is set to
// default.
SparkFun_AS3935 lightning(AS3935_ADDR);
// This variable holds the number representing the lightning or non-lightning
// event issued by the lightning detector.
byte intVal = 0;

WiFiClient client;
HTTPClient http;


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

void read_MLX90614()
{
    // Call therm.read() to read object and ambient temperatures from the sensor.
    if (therm.read()) // On success, read() will return 1, on fail 0.
    {
        // Use the object() and ambient() functions to grab the object and ambient
        // temperatures.
        // They'll be floats, calculated out to the unit you set with setUnit().
        object = therm.object();
        ambient = therm.ambient();
    }
}

void read_rain()
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

  gain = 1;

  // If time = 0, integration will be 13.7ms
  // If time = 1, integration will be 101ms
  // If time = 2, integration will be 402ms
  // If time = 3, use manual start / stop to perform your own integration

  unsigned char time = 0;

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

bool init_AS3935()
{
  // When lightning is detected the interrupt pin goes HIGH.
  pinMode(lightning_pin, INPUT);

  if (!lightning.begin())
  { // Initialize the sensor.
    
    return false;
  }
  else
    
  // "Disturbers" are events that are false lightning events. If you find
  // yourself seeing a lot of disturbers you can have the chip not report those
  // events on the interrupt lines.
  // lightning.maskDisturber(true);

    

  // The lightning detector defaults to an indoor setting (less
  // gain/sensitivity), if you plan on using this outdoors
  // uncomment the following line:
  lightning.setIndoorOutdoor(OUTDOOR);
  int enviVal = lightning.readIndoorOutdoor();
    

  // Noise floor setting from 1-7, one being the lowest. Default setting is
  // two. If you need to check the setting, the corresponding function for
  // reading the function follows.

  lightning.setNoiseLevel(noiseFloor);

  int noiseVal = lightning.readNoiseLevel();
  
  

  // Watchdog threshold setting can be from 1-10, one being the lowest. Default setting is
  // two. If you need to check the setting, the corresponding function for
  // reading the function follows.

  lightning.watchdogThreshold(watchDogVal);

  int watchVal = lightning.readWatchdogThreshold();
  
  

  // Spike Rejection setting from 1-11, one being the lowest. Default setting is
  // two. If you need to check the setting, the corresponding function for
  // reading the function follows.
  // The shape of the spike is analyzed during the chip's
  // validation routine. You can round this spike at the cost of sensitivity to
  // distant events.

  lightning.spikeRejection(spike);

  int spikeVal = lightning.readSpikeRejection();
  
  

  // This setting will change when the lightning detector issues an interrupt.
  // For example you will only get an interrupt after five lightning strikes
  // instead of one. Default is one, and it takes settings of 1, 5, 9 and 16.
  // Followed by its corresponding read function. Default is zero.

  lightning.lightningThreshold(lightningThresh);

  uint8_t lightVal = lightning.readLightningThreshold();
  
  

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
  //  
  // else
  // 

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
      
      // Too much noise? Uncomment the code below, a higher number means better
      // noise rejection.
      // lightning.setNoiseLevel(setNoiseLevel);
    }
    else if (intVal == DISTURBER_INT)
    {
      
      // Too many disturbers? Uncomment the code below, a higher number means better
      // disturber rejection.
      // lightning.watchdogThreshold(threshVal);
    }
    else if (intVal == LIGHTNING_INT)
    {
      
      // Lightning! Now how far away is it? Distance estimation takes into
      // account any previously seen events in the last 15 seconds.
      lightning_distanceToStorm = lightning.distanceToStorm();
      
      
      
    }
  }
}

bool read_TSL237()
{
  if (FreqCountESP.available())
  {
    frequency = FreqCountESP.read();
    if (frequency < 1)
    {
      frequency = 1;
    }
    // 0.973 was derived by comparing TLS237 sensor readings against Unihedron and plotting on graph then deriving coefficient
    mySQMreading = (sqm_limit - (2.5 * log10(frequency)) * 0.973); // frequency to magnitudes/arcSecond2
    if (isnan(mySQMreading))
    {
      mySQMreading = 0.0; // avoid nan and negative values
    }
    if (mySQMreading == -0.0)
    {
      mySQMreading = 0.0;
    }
    nelm = 7.93 - 5.0 * log10((pow(10, (4.316 - (mySQMreading / 5.0))) + 1));
    irradiance = frequency / 2.3e3; // calculate irradiance as uW/(cm^2)
    return true;
  }
  else
  {
    return false;
  }
}

void read_particle()
{
  starttime = millis(); // get the current time;
  while ((millis() - starttime) < sampletime_ms)
  {
    duration = pulseIn(particle_pin, LOW);
    lowpulseoccupancy = lowpulseoccupancy + duration;

    if ((millis() - starttime) > sampletime_ms) // if the sampel time == 100ms
    {
      ratio = lowpulseoccupancy / (sampletime_ms * 10.0);                                  // Integer percentage 0=>100
      temp_concentration = 1.1 * pow(ratio, 3) - 3.8 * pow(ratio, 2) + 520 * ratio + 0.62; // using spec sheet curve
      if (temp_concentration != 0.62)
      {
        concentration = temp_concentration;
      }
    }
  }
}

bool post_data()
{

  // Check WiFi connection status
  if (WiFi.status() == WL_CONNECTED)
  {
    WiFiClient client;
    HTTPClient http;

    std::stringstream data;
    data << "{\"raining\":\"" << raining << "\",\"SQMreading\":\"" << mySQMreading << "\",\"irradiance\":\"" << irradiance << "\",\"nelm\":\"" << nelm << "\",\"concentration\":\"" << concentration << "\",\"object\":\"" << object << "\",\"ambient\":\"" << ambient << "\",\"lux\":\"" << lux << "\",\"lightning_distanceToStorm\":\"" << lightning_distanceToStorm << "\"}";
    std::string s = data.str();
    // Your Domain name with URL path or IP address with path
    http.begin(client, serverName);
    // If you need an HTTP request with a content type: application/json, use the following:
    http.addHeader("Content-Type", "application/json");
    int httpResponseCode = http.POST(s.c_str());
    // Free resources
    http.end();
    return true;
  }
  else
  {
    return false;
  }
}



void setup()
{
  // Set WiFi to station mode and disconnect from an AP if it was previously connected
  esp_wifi_start();
  WiFi.mode(WIFI_STA);
  //WiFi.disconnect();

  WiFi.begin(WIFI_SSID, WIFI_PASS);
  pinMode(EN_3V3, OUTPUT); 
  pinMode(EN_5V, OUTPUT); 
  digitalWrite(EN_3V3, HIGH);
  digitalWrite(EN_5V, HIGH);

  Wire.begin(); // I2C bus

  init_MLX90614();
  init_TSL2561();
  init_AS3935();

  FreqCountESP.begin(SQMpin, 1000);
  pinMode(rainS_DO, INPUT);
  pinMode(particle_pin, INPUT);
}

void loop()
{
  // 
  read_MLX90614();
  read_TSL2561();
  read_particle();
  read_AS3935();
  read_rain();
  
  
  read_TSL237();
  

  if (WiFi.status() == WL_CONNECTED)
  {
    post_data();
    esp_sleep_enable_timer_wakeup(uS_FACTOR * SLEEPTIME_s);
  }
  else
  {
    esp_sleep_enable_timer_wakeup(uS_FACTOR * 5);
  }

  

  

  //WiFi.disconnect(true);     // Disconnect from the network
  WiFi.mode(WIFI_MODE_NULL); // Switch WiFi off
  esp_wifi_stop();
  
  digitalWrite(EN_3V3, LOW);
  digitalWrite(EN_5V, LOW);
  esp_deep_sleep_start();
}
