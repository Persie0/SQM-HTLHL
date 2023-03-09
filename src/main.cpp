/**
 ******************************************************************************
 * @file    main.c
 * @author  Marvin Perzi
 * @version V03
 * @date    2023
 * @brief   Sky Quality Meter that sends sensor values to an API endpoint
 ******************************************************************************
 */

// Debugger inbestriebnahme, breakpoints, variablen, watchpoints,
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
#include <hardware\display_and_pins.h>

using namespace std;


static int sleepTime = 5;
static bool sleepForever = false;

// RTC_DATA_ATTR values dont get reset after deepsleep
//state and error tracking variables
RTC_DATA_ATTR int noWifiCount = 0;
RTC_DATA_ATTR int sendCount = 0;
RTC_DATA_ATTR int serverErrorCount = 0;
RTC_DATA_ATTR bool settingsLoadCount = 0;
RTC_DATA_ATTR bool hasInitialized = false;
RTC_DATA_ATTR bool settingsLoaded = false;
RTC_DATA_ATTR bool hasWIFI = false;
RTC_DATA_ATTR bool hasServerError = false;

// settings that get fetched from server
RTC_DATA_ATTR int SLEEPTIME_s = FALLBACK_SLEEPTIME_s;
RTC_DATA_ATTR int NO_WIFI_MAX_RETRIES = FALLBACK_NO_WIFI_MAX_RETRIES;
RTC_DATA_ATTR int DISPLAY_TIMEOUT_s = FALLBACK_DISPLAY_TIMEOUT_s;
RTC_DATA_ATTR int DISPLAY_ON = FALLBACK_DISPLAY_ON;
RTC_DATA_ATTR double SQM_LIMIT = FALLBACK_SQM_LIMIT;
RTC_DATA_ATTR bool SEEING_ENABLED = false;
RTC_DATA_ATTR int seeing_thr = FALLBACK_seeing_thr;
RTC_DATA_ATTR double SP1 = FALLBACK_SP1;
RTC_DATA_ATTR double SP2 = FALLBACK_SP2;
RTC_DATA_ATTR double MAX_LUX = FALLBACK_MAXLUX;

// sensor values
bool raining = false;
float ambient = -333;//TQ
float object = -333; //HT
double lux = -333; // Resulting lux value
int lightning_distanceToStorm = -333;
float luminosity = -333; // the SQM value, sky magnitude
String seeing = "-333"; 
double nelm = -333; //NE
int concentration = -333;
vector<String> sensorErrors;

// Sky state indicators
RTC_DATA_ATTR int CLOUD_STATE = -333;
RTC_DATA_ATTR int BAD_SKY_STATE_COUNT = 0;
RTC_DATA_ATTR int GOOD_SKY_STATE_COUNT = 0;
RTC_DATA_ATTR vector<bool> lastSeeingChecks;

// across deepsleep stored values
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

  // Start the I2C communication for the lightning sensor
  Wire1.begin(SDA_2, SCL_2, 20000U);
  // Start the I2C communication for the other sensors
  Wire.begin(21, 22, 100000U);

  // Load the saved network settings
  if (!hasInitialized)
  {
    getSavedWifiSettings(WIFI_SSID, WIFI_PASS, SERVER_IP, SEND_VALUES_SERVER, FETCH_SETTINGS_SERVER);
    hasInitialized = true;
  }

  // Enable and set the WiFi to station mode
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  Serial.println("WIFI:" + String(WIFI_SSID));
  WiFi.setTxPower(WIFI_POWER_19_5dBm);

  // Enable the supply voltage for the sensors
  pinMode(EN_3V3, OUTPUT);
  pinMode(EN_5V, OUTPUT);
  digitalWrite(EN_3V3, HIGH);
  digitalWrite(EN_5V, HIGH);
  delay(3);

  // Initialize the SQM sensor
  FreqCountESP.begin(SQMpin, 100);

  // Initialize the sensors
  if (!init_MLX90614())
  {
    sensorErrors.push_back("init_MLX90614");
  }
  delay(50);
  if (!init_TSL2561())
  {
    sensorErrors.push_back("init_TSL2561");
  }
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
  // read sensors, if sensor error add to array
  if (!read_MLX90614(ambient, object))
  {
    sensorErrors.push_back("read_MLX90614");
  }
  delay(50);
  if (!read_TSL2561(lux))
  {
    sensorErrors.push_back("read_TSL2561");
  }
  if (!read_AS3935(lightning_distanceToStorm))
  {
    sensorErrors.push_back("read_AS3935");
  }
  if (!read_TSL237(luminosity, nelm, SQM_LIMIT))
  {
    sensorErrors.push_back("read_TSL237");
  }
  delay(20);
  // read the sensor values
  read_particle(concentration);
  read_rain(raining);

  if (SEEING_ENABLED)
  {
    UART_get_Seeing(seeing);
  }
  if (ESP_MODE == 1)
  {
    // turn display off after set time
    if (DISPLAY_ON && hasWIFI && DISPLAY_TIMEOUT_s != 0 && (DISPLAY_TIMEOUT_s < ((sendCount * SLEEPTIME_s) + (noWifiCount * (NOWIFI_SLEEPTIME_s + 6)))))
    {
      // disable display supply voltage
      DISPLAY_ON = false;
      digitalWrite(EN_Display, LOW);
    }

    // send data if connected
    if (WiFi.status() == WL_CONNECTED)
    {
      // fetch settings if not loaded yet or desired
      if (!settingsLoaded)
      {
        settingsLoaded = fetch_settings(FETCH_SETTINGS_SERVER, seeing_thr, SP1, SP2, MAX_LUX, SLEEPTIME_s, DISPLAY_TIMEOUT_s, DISPLAY_ON, SQM_LIMIT);
      }
      //bool post_data(char *SEND_VALUES_SERVER, bool raining, int luminosity, int seeing, int nelm, int concentration, String object, int ambient, int lux, int lightning_distanceToStorm, std::vector<String> sensorErrors, bool SEEING_ENABLED)
      hasServerError = !post_data(SEND_VALUES_SERVER, raining, luminosity, seeing, nelm, concentration, object, ambient, lux, lightning_distanceToStorm, sensorErrors, SEEING_ENABLED);
      if (hasServerError)
      {
        serverErrorCount++;
        sleepTime = NOWIFI_SLEEPTIME_s;
      }
      else
      {
        serverErrorCount = 0;
      }
      noWifiCount = 0;
      hasWIFI = true;
      sendCount++;
      // set custom sleep time
      sleepTime = SLEEPTIME_s;
    }

    // else wait for connection
    else
    {
      sleepTime = NOWIFI_SLEEPTIME_s;
      noWifiCount++;
      hasWIFI = false;
    }

    // sleep forever if max retries reached
    if (noWifiCount >= NO_WIFI_MAX_RETRIES || serverErrorCount >= NO_WIFI_MAX_RETRIES)
    {
      // open AP for changing WIFI settings
      sleepForever = true;
      DisplayStatusMessage(hasWIFI, hasServerError, settingsLoaded, sendCount, noWifiCount, sleepForever, DISPLAY_ON);
      activate_access_point();
    }

    // if couldnt send data - turn display on again and show error message
    if ((!hasWIFI || hasServerError) && !DISPLAY_ON)
    {
      DISPLAY_ON = true;
      // keep display on in deepsleep
      high_hold_Pin(EN_Display);
      delay(5);
    }
    check_seeing_threshhold(seeing_thr, GOOD_SKY_STATE_COUNT, BAD_SKY_STATE_COUNT, lastSeeingChecks, CLOUD_STATE, lux, MAX_LUX, SEEING_ENABLED, SLEEPTIME_s);
  }
  DisplayStatusMessage(hasWIFI, hasServerError, settingsLoaded, sendCount, noWifiCount, sleepForever, DISPLAY_ON);
  WiFi.mode(WIFI_MODE_NULL);           // Switch WiFi off
  esp_deep_sleep(sleepTime * 1000000); // send ESP32 to deepsleep
}
