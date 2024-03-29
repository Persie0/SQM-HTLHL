/**
 ******************************************************************************
 * @file    main.c
 * @author  Marvin Perzi
 * @version V03
 * @date    2023
 * @brief   ESP32 program for the SQM Diploma Thesis of Marvin Perzi and Lorenz Schneider
 ******************************************************************************
 */

#include <Arduino.h>
#include <vector>
#include "settings.h"
#include <Wire.h>
#include <WiFi.h>

#include "FreqCountESP.h"

#include "sensors/sensor_lightning.h"
#include "sensors/sensor_ir_temperature.h"
#include "sensors/sensor_light.h"
#include "sensors/sensor_dust.h"
#include "sensors/sensor_rain.h"
#include "sensors/sensor_SQM.h"

#include "hardware/wifi_fkt.h"
#include "hardware/seeing_fkt.h"
#include "hardware/display_and_pins.h"

using namespace std;

int sleepTime = 60;
bool sleepForever = false;

// RTC_DATA_ATTR values dont get reset after deepsleep
// state and error tracking variables
RTC_DATA_ATTR int noWifiCount = 0;
RTC_DATA_ATTR int sendCount = 0;
RTC_DATA_ATTR int serverErrorCount = 0;
RTC_DATA_ATTR bool settingsLoadCount = 0;
RTC_DATA_ATTR bool hasInitialized = false;
RTC_DATA_ATTR bool settingsLoaded = false;
RTC_DATA_ATTR bool hasWIFI = false;
RTC_DATA_ATTR bool hasServerError = false;
RTC_DATA_ATTR int NOWIFI_SLEEPTIME_s = 10;

// settings that get fetched from server
RTC_DATA_ATTR int SLEEPTIME_s = 10, NO_WIFI_MAX_RETRIES = 25, DISPLAY_TIMEOUT_s = 180, DISPLAY_ON = 1, seeing_thr = 5;
RTC_DATA_ATTR double SQM_LIMIT = 21, SP1 = 20, SP2 = 22, MAX_LUX = 50;
RTC_DATA_ATTR bool SEEING_ENABLED = false;

// sensor values
bool raining = false;
float ambient = -333; // TQ
float object = -333;  // HT
double lux = -333;    // Resulting lux value
int lightning_distanceToStorm = -333;
float luminosity = -333; // the SQM value, sky magnitude
String seeing = "-333";
double nelm = -333; // NE
int concentration = -333;
vector<String> sensorErrors;

// Sky state indicators
RTC_DATA_ATTR int CLOUD_STATE = -333;
RTC_DATA_ATTR int BAD_SKY_STATE_COUNT = 0;
RTC_DATA_ATTR int GOOD_SKY_STATE_COUNT = 0;

// across deepsleep stored connection settings
RTC_DATA_ATTR char WIFI_SSID[100] = "";
RTC_DATA_ATTR char WIFI_PASS[100] = "";
RTC_DATA_ATTR char SERVER_IP[100] = "";
RTC_DATA_ATTR char SEND_VALUES_SERVER[100] = "";
RTC_DATA_ATTR char FETCH_SETTINGS_SERVER[100] = "";

void setup()
{
  // Configure the display on or off based on DISPLAY_ON
  if (DISPLAY_ON)
  {
    // Keep the display on during deep sleep
    high_hold_Pin(EN_Display);
  }

  // Configure the seeing sensor on or off based on SEEING_ENABLED
  if (!SEEING_ENABLED)
  {
    // Keep the seeing sensor off
    high_hold_Pin(EN_SEEING);
  }
  else
  {
    // Keep the seeing sensor on
    low_hold_Pin(EN_SEEING);
  }

  // Start the serial communication with a baud rate of 115200
  Serial.begin(115200);
  Serial.println("Seeingsensor enabled: " + String(SEEING_ENABLED));
  bool pinstate = digitalRead(EN_SEEING);
  Serial.println("Seeingsensor pinstate: " + String(pinstate));

  // Start the I2C communication for the lightning sensor
  Wire1.begin(SDA_2, SCL_2, 100000U);
  // Start the I2C communication for the other sensors
  Wire.begin(SDA_1, SCL_1, 100000U);

  // Load the network settings from the server once
  if (!hasInitialized)
  {
    Serial.println("Loading wifi settings files...");
    getSavedWifiSettings(WIFI_SSID, WIFI_PASS, SERVER_IP, SEND_VALUES_SERVER, FETCH_SETTINGS_SERVER);
    hasInitialized = true;
  }

  // Enable and set the WiFi to station mode
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  Serial.println("WIFI:" + String(WIFI_SSID));
  WiFi.setTxPower(WIFI_POWER_19_5dBm);

  // Enable the supply voltage for the sensors
  /*pinMode(EN_3V3, OUTPUT);
  pinMode(EN_5V, OUTPUT);
  digitalWrite(EN_3V3, HIGH);
  digitalWrite(EN_5V, HIGH);
  delay(20);*/

  // Initialize the SQM sensor
  FreqCountESP.begin(SQMpin, 100);

  // Initialize the sensors, add errors to error array if sensor error
  if (!init_MLX90614())
  {
    sensorErrors.push_back("init_MLX90614");
  }
  delay(20);
  if (!init_TSL2561())
  {
    sensorErrors.push_back("init_TSL2561");
  }
  delay(20);
  if (!init_AS3935(Wire1))
  {
    sensorErrors.push_back("init_AS3935");
  }
  delay(20);

  // Set the pins for rain and particle sensors as input
  pinMode(rainS_DO, INPUT);
  pinMode(particle_pin, INPUT);
}

void loop()
{
  // read sensors, if sensor error add to  error array to be able send them to the server
  if (!read_MLX90614(ambient, object))
  {
    sensorErrors.push_back("read_MLX90614");
  }
  delay(20);
  if (!read_TSL2561(lux))
  {
    sensorErrors.push_back("read_TSL2561");
  }
  if (!read_AS3935(lightning_distanceToStorm))
  {
    sensorErrors.push_back("read_AS3935");
  }
  delay(20);
  if (!read_TSL237(luminosity, nelm, SQM_LIMIT))
  {
    sensorErrors.push_back("read_TSL237");
  }
  delay(20);
  // read the sensor values
  read_particles(concentration); // read the particle sensor
  read_rain(raining);            // read the rain sensor

  // send data if connected to wifi
  if (WiFi.status() == WL_CONNECTED)
  {
    // fetch settings if not loaded yet
    if (!settingsLoaded)
    {
      // fetch settings from server, save if successfully fetched
      settingsLoaded = fetch_settings(FETCH_SETTINGS_SERVER, seeing_thr, SP1, SP2, MAX_LUX, SLEEPTIME_s, DISPLAY_TIMEOUT_s, DISPLAY_ON, SQM_LIMIT);
      // start setup() again if settings loaded to apply changes
      Serial.println("Settings loaded: " + String(settingsLoaded));
      esp_deep_sleep(100000);
    }

    // if seeing enabled, get seeing value
    if (SEEING_ENABLED)
    {
      Serial.println("Getting seeing...");
      if (!UART_get_Seeing(seeing))
      {
        sensorErrors.push_back("UART_get_Seeing");
        seeing = "-333";
      }
      // check if seeing only contains numbers and a dot
    }

    // else send sensor values to server
    hasServerError = !post_data(SEND_VALUES_SERVER, raining, luminosity, seeing, nelm, concentration, object, ambient, lux, lightning_distanceToStorm, sensorErrors, SEEING_ENABLED);

    if (hasServerError) // if server error, increase error count
    {
      serverErrorCount++;
      sleepTime = NOWIFI_SLEEPTIME_s;
    }
    else // else reset error count
    {
      serverErrorCount = 0;
    }
    // if connected to server, reset no wifi count
    noWifiCount = 0;
    hasWIFI = true;
    sendCount++;
    // set custom sleep time
    sleepTime = SLEEPTIME_s;
  }
  // if not connected to wifi, increase no wifi count and sleep for set time
  else
  {
    Serial.println("No WIFI");
    sleepTime = NOWIFI_SLEEPTIME_s;
    noWifiCount++;
    hasWIFI = false;
  }

  // check cloud state and if seeing should be enabled if has wifi
  if (hasWIFI)
  {
    // calculate the cloud state based on IR sensor values
    CLOUD_STATE = get_cloud_state(object, ambient, SP1, SP2);
    // check if seeing should be enabled with settings and sensor values
    check_seeing_threshhold(seeing_thr, GOOD_SKY_STATE_COUNT, BAD_SKY_STATE_COUNT, CLOUD_STATE, lux, MAX_LUX, SEEING_ENABLED, SLEEPTIME_s);
  }

  // turn display off after set time
  if (DISPLAY_ON && hasWIFI && DISPLAY_TIMEOUT_s != 0 && (DISPLAY_TIMEOUT_s < ((sendCount * SLEEPTIME_s) + (noWifiCount * (NOWIFI_SLEEPTIME_s + 6)))))
  {
    // disable display supply voltage
    DISPLAY_ON = false;
    digitalWrite(EN_Display, LOW);
  }

  // sleep forever if max retries reached
  if (noWifiCount >= NO_WIFI_MAX_RETRIES || serverErrorCount >= NO_WIFI_MAX_RETRIES)
  {
    if (!settingsLoaded)
    {
      sleepForever = true;
      // show status message on display
      DisplayStatusMessage(hasWIFI, hasServerError, settingsLoaded, sendCount, noWifiCount, sleepForever, DISPLAY_ON);
      // open AP for changing WIFI settings
      activate_access_point();
    }
    else
    {
      // reconnect to wifi by turning off and on again
      WiFi.disconnect();
      delay(100);
      WiFi.mode(WIFI_STA);
      WiFi.begin(WIFI_SSID, WIFI_PASS);
      delay(100);
      Serial.println("WIFI:" + String(WIFI_SSID));
    }
  }

  // if couldnt send data - turn display on again and show error message
  if ((!hasWIFI || hasServerError) && !DISPLAY_ON)
  {
    DISPLAY_ON = true;
    // keep display on in deepsleep
    high_hold_Pin(EN_Display);
    delay(5);
  }
  // show status message on display
  DisplayStatusMessage(hasWIFI, hasServerError, settingsLoaded, sendCount, noWifiCount, sleepForever, DISPLAY_ON);

  Serial.println("Going to sleep now for " + String(sleepTime) + " seconds");

  // WiFi.mode(WIFI_MODE_NULL); // Switch WiFi off
  // esp_deep_sleep(sleepTime * 1000000); // send ESP32 to deepsleep
  delay(sleepTime * 1000); // wait between measurements
}
