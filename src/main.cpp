/**
 ******************************************************************************
 * @file    main.c
 * @author  Marvin Perzi
 * @version V01
 * @date    2022
 * @brief   Sky Quality Meter that sends sensor values to an API endpoint
 ******************************************************************************
 */

// Debugger inbestriebnahme, breakpoints, variablen, watchpoints,
#include <Arduino.h>
#include <vector>
#include "settings.h"
#include <Wire.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <sstream>
#include "soc/rtc_cntl_reg.h"
#include "soc/rtc.h"
#include "driver/rtc_io.h"
#include <ArduinoJson.h>
#include <Preferences.h>
#include <ESPAsyncWebServer.h>
#include <AsyncTCP.h>
#include <SPIFFS.h>

#include "FreqCountESP.h"
#include "SSD1306Wire.h"

#include "sensors/sensor_lightning.h"
#include "sensors/sensor_ir_temperature.h"
#include "sensors/sensor_light.h"
#include "sensors/sensor_dust.h"
#include "sensors/sensor_rain.h"
#include "sensors/sensor_SQM.h"
#include "hardware/spiffs_fkt.h"
#include "variables.h"

using namespace std;
vector<String> sensorErrors;
RTC_DATA_ATTR vector<bool> lastSeeingChecks;

#define SDA_2 17
#define SCL_2 5

// Replaces placeholder on website with Wifi info
String wifi_info(const String &var)
{
  return "SSID: " + String(WIFI_SSID) + ", \nPW: " + String(WIFI_PASS) + ", \nServer IP: " + String(SERVER_IP);
}

// Function to activate access point and provide website for changing WIFI settings
void activate_access_point()
{
  // Initialize storage to later read/write from/to it
  initSPIFFS();

  // Switch WiFi off and switch to AP mode
  WiFi.mode(WIFI_MODE_NULL);
  WiFi.mode(WIFI_AP);
  delay(10);

  // Set AP IP settings
  WiFi.softAPConfig(localIP, gateway, subnet);

  // Create an open access point with the name "ESP-WIFI-MANAGER"
  WiFi.softAP("ESP-WIFI-MANAGER", NULL);

  // Set up the server to serve the website at "/" URL
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request)
            { request->send(SPIFFS, "/wifimanager.html", "text/html"); });

  // Serve static files stored in SPIFFS
  server.serveStatic("/", SPIFFS, "/");

  // Handle a POST request to change WIFI settings on the website
  server.on("/", HTTP_POST, [](AsyncWebServerRequest *request)
            {
    // Get the parameters from the HTML form
    int params = request->params();
    
    // Loop through the parameters
    for (int i = 0; i < params; i++) {
      AsyncWebParameter *p = request->getParam(i);
      if (p->isPost()) {
        // Handle the HTTP POST for the SSID parameter
        if (p->name() == PARAM_INPUT_1) {
          (p->value()).toCharArray(WIFI_SSID, 100);
          // Write the SSID to a file for persistence
          if (!writeLineOfFile(SPIFFS, ssidPath, WIFI_SSID)) {
            Serial.println("Couldn't write");
          }
        }
        // Handle the HTTP POST for the password parameter
        if (p->name() == PARAM_INPUT_2) {
          (p->value()).toCharArray(WIFI_PASS, 100);
          // Write the password to a file for persistence
          writeLineOfFile(SPIFFS, passPath, WIFI_PASS);
        }
        // Handle the HTTP POST for the IP address parameter
        if (p->name() == PARAM_INPUT_3) {
          (p->value()).toCharArray(SERVER_IP, 100);
          // Write the IP address to a file for persistence
          writeLineOfFile(SPIFFS, ipPath, SERVER_IP);
        }
      }
    }
    // Send a confirmation message and restart the ESP
    request->send(200, "text/plain", "Done. ESP will restart, connect to your router " + String(WIFI_SSID) + " and go to IP address: " + SERVER_IP);
    delay(100);
    ESP.restart(); });

  // Start the server (website)
  server.begin();

  // Record the start time
  unsigned long startTime = millis();

  // turn sensors off
  digitalWrite(EN_3V3, LOW);
  digitalWrite(EN_5V, LOW);
  // Wait for 20 minutes or until the website changes the WiFi settings
  while ((millis() - startTime) < 1200 * 1000)
  {
    delay(100);
  }

  // If no changes were made in the website, put the device into deep sleep mode
  esp_deep_sleep(77777777);
}

// Display the current status message on the display
void DisplayStatusMessage()
{
  if (!DISPLAY_ON)
  {
    return;
  }

  // Initialize the display
  display.init();
  delay(3);
  display.clear();
  // Set the font and text alignment
  display.setFont(ArialMT_Plain_10);
  display.setTextAlignment(TEXT_ALIGN_LEFT);

  // Display status based on ESP_MODE
  switch (ESP_MODE)
  {
  case 1:
    if (sleepForever)
    {
      display.drawStringMaxWidth(0, 0, 128, "WIFI AP / Dead");
      display.drawStringMaxWidth(0, 12, 128, "no WIFI, server error");
      display.drawStringMaxWidth(0, 24, 128, "or server not reachable");
    }
    else
    {
      // WIFI status
      if (hasWIFI)
      {
        display.drawStringMaxWidth(0, 0, 128, "Connected to Wifi");
        display.drawStringMaxWidth(0, 12, 128, "send count: " + String(sendCount));
      }
      else
      {
        display.drawStringMaxWidth(0, 0, 128, "NO Wifi");
        display.drawStringMaxWidth(0, 12, 128, "retry count: " + String(noWifiCount));
      }
      // Settings status
      display.drawStringMaxWidth(0, 24, 128, settingsLoaded ? "settings loaded" : "settings NOT loaded");

      // Server status
      display.drawStringMaxWidth(0, 36, 128, hasServerError ? "server error!" : "server is running");
    }
    break;
  case 0:
    // Display environmental data
    display.drawStringMaxWidth(0, 0, 128, "SQM: " + String(luminosity));
    display.drawStringMaxWidth(0, 12, 128, "Airp.: " + String(concentration));
    display.drawStringMaxWidth(0, 24, 128, "Lux: " + String(lux));
    display.drawStringMaxWidth(0, 36, 128, "Raining: " + String(raining));
    display.drawStringMaxWidth(0, 48, 128, "A&O: " + String(ambient) + "; " + String(object) + " °C");
    break;
  default:
    // Display sensor errors, if any
    if (sensorErrors.size() != 0)
    {
      for (int i = 0; i < sensorErrors.size(); i++)
      {
        display.drawStringMaxWidth(0, 12 * i, 128, sensorErrors[i]);
      }
    }
    else
    {
      display.drawStringMaxWidth(0, 0, 128, "No errors :)");
    }
    break;
  }

  display.display();
}

// Function to configure and hold a specified pin in high state during deep sleep
void high_hold_Pin(gpio_num_t pin)
{
  // Set the specified pin as output
  pinMode(pin, OUTPUT);
  // Write high state to the pin
  digitalWrite(pin, HIGH);
  // Enable holding the pin high state during deep sleep
  gpio_hold_en(pin);
  // Enable holding GPIO states during deep sleep
  gpio_deep_sleep_hold_en();
}

// Set a pin as output and keep it low during deep sleep
void low_hold_Pin(gpio_num_t pin)
{
  // Set pin as output
  pinMode(pin, OUTPUT);

  // Set the pin to LOW
  digitalWrite(pin, LOW);

  // Enable pin retention during deep sleep
  gpio_hold_en(pin);
  gpio_deep_sleep_hold_en();
}

// Function to fetch settings from a server
bool fetch_settings()
{
  // Create client object for communication
  WiFiClient client;

  // Create HTTP client object
  HTTPClient http;

  // Send request to the server
  http.useHTTP10(true);
  http.begin(client, FETCH_SERVER);
  int httpResponseCode = http.GET();

  // Check if the request was successful
  if (httpResponseCode == 200)
  {
    // Parse the JSON response from the server
    DynamicJsonDocument doc(2048);
    deserializeJson(doc, http.getStream());
    // Read values from the JSON document
    // The code checks if the key exists before trying to read its value
    // This ensures that the code does not crash if the key is not present in the JSON document

    // Read values
    if (doc.containsKey("seeing_thr"))
    {
      seeing_thr = doc["seeing_thr"].as<int>();
    }
    if (doc.containsKey("setpoint1"))
    {
      SP1 = doc["setpoint1"].as<double>();
    }
    if (doc.containsKey("setpoint2"))
    {
      SP2 = doc["setpoint2"].as<double>();
    }
    if (doc.containsKey("max_lux"))
    {
      MAX_LUX = doc["max_lux"].as<double>();
    }

    if (doc.containsKey("SLEEPTIME_s"))
    {
      SLEEPTIME_s = doc["SLEEPTIME_s"].as<int>();
    }
    if (doc.containsKey("DISPLAY_TIMEOUT_s"))
    {
      DISPLAY_TIMEOUT_s = doc["DISPLAY_TIMEOUT_s"].as<int>();
    }
    if (doc.containsKey("DISPLAY_ON"))
    {
      DISPLAY_ON = doc["DISPLAY_ON"].as<bool>();
    }
    if (doc.containsKey("set_sqm_limit"))
    {
      SQM_LIMIT = doc["set_sqm_limit"].as<double>();
    }
    // Disconnect
    http.end();
    return true;
  }
  // Disconnect
  http.end();
  return false;
}

// send the sensor values via http post request to the server
bool post_data()
{

  WiFiClient client;
  HTTPClient http;

  std::stringstream data;

  String errors = "";
  for (int i = 0; i < sensorErrors.size(); i++)
  {
    errors = errors + sensorErrors[i] + ", ";
  }
  // create a json string
  String search = "{\"raining\":\"" + String(raining) + "\",\"luminosity\":\"" + luminosity + "\",\"seeing\":\"" + seeing + "\",\"nelm\":\"" + nelm +
                  "\",\"concentration\":\"" + concentration + "\",\"object\":\"" + object + "\",\"ambient\":\"" + ambient + "\",\"lux\":\"" + lux +
                  "\",\"lightning_distanceToStorm\":\"" + lightning_distanceToStorm + "\",\"errors\":\"" + errors + "\",\"isSeeing\":\"" + SEEING_ENABLED + "\"}";
  // Start the HTTP connection
  http.begin(client, SEND_SERVER);
  http.addHeader("Content-Type", "application/json");

  // Send the JSON string as the HTTP body
  int httpResponseCode = http.POST(search);
  http.end(); // End the connection

  // Return true if the post request is successful
  return httpResponseCode == 200;
}

// calculate if cloudy/clear sky
void getcloudstate()
{
  float TempDiff = ambient - object;
  // object temp is IR temp of sky which at night time will be a lot less than ambient temp
  // so TempDiff is basically ambient + abs(object)
  // setpoint 1 is set for clear skies
  // setpoint 2 is set for cloudy skies
  // setpoint2 should be lower than setpoint1
  // For clear, Object will be very low, so TempDiff is largest
  // For cloudy, Object is closer to ambient, so TempDiff will be lowest

  // Readings are only valid at night when dark and sensor is pointed to sky
  // During the day readings are meaningless
  if (TempDiff > SP1)
  {
    CLOUD_STATE = SKYCLEAR; // clear
  }
  else if ((TempDiff > SP2) && (TempDiff < SP1))
  {
    CLOUD_STATE = SKYPCLOUDY; // partly cloudy
  }
  else if (TempDiff < SP2)
  {
    CLOUD_STATE = SKYCLOUDY; // cloudy
  }
  else
  {
    CLOUD_STATE = SKYUNKNOWN; // unknown
  }
}

// Send the "shut" command over UART to indicate planning to shut down the seeing
bool UART_shutdown_Seeing()
{
  SerialPort.begin(9600, 134217756U, MYPORT_RX, MYPORT_TX, false); // initialize the serial port with a baud rate of 9600

  // Send the "shut" command
  SerialPort.println("shut");

  // Read the response from the device and store it in a string variable
  String response = SerialPort.readString();
  response.trim();

  // Check if the response is "ok"
  return response == "ok";
}

// get Seeing value over UART
bool UART_get_Seeing()
{
  // initialize the serial port with a baud rate of 9600
  SerialPort.begin(9600, 134217756U, MYPORT_RX, MYPORT_TX, false);
  // Send the "get" command
  SerialPort.println("get");
  String teststr = SerialPort.readString(); // read until timeout
  // trim the string
  teststr.trim();
  seeing = teststr;
  // check if string is empty
  return !teststr.isEmpty();
}

// check skystate if ok for Seeing
bool check_seeing()
{
  getcloudstate();
  return (CLOUD_STATE == SKYCLEAR && lux < MAX_LUX);
}

// check if sky quality was good for long enough
void check_seeing_threshhold()
{
  // check if sensor values are good and if seeing should be enabled
  // good sky state
  if (check_seeing())
  {
    ++GOOD_SKY_STATE_COUNT;

    // insert true at beginning of vector
    lastSeeingChecks.insert(lastSeeingChecks.begin(), true);
    // if lastSeeingChecks.size() > 5, pop last element
    if (lastSeeingChecks.size() > 5)
    {
      lastSeeingChecks.pop_back();
    }

    // check if more than 2 good in last 5 checks
    int goodcount = 0;
    for (int i = 0; i < lastSeeingChecks.size(); i++)
    {
      if (lastSeeingChecks[i])
      {
        goodcount++;
      }
    }
    // if >= 2 good in last 5 checks, reset BAD_SKY_STATE_COUNT
    if (goodcount >= 2)
    {
      BAD_SKY_STATE_COUNT = 0;
    }

    // if good skystate for long enough, enable seeing
    if (GOOD_SKY_STATE_COUNT >= seeing_thr)
    {
      // keep Seeing on in deepsleep
      SEEING_ENABLED = true;
      low_hold_Pin(EN_SEEING);
    }
  }
  else
  {
    ++BAD_SKY_STATE_COUNT;

    // insert false at beginning of vector
    lastSeeingChecks.insert(lastSeeingChecks.begin(), false);
    // if lastSeeingChecks.size() > 5, pop last element
    if (lastSeeingChecks.size() > 5)
    {
      lastSeeingChecks.pop_back();
    }

    // check if more than 2 false in last 5 checks
    int falsecount = 0;
    for (int i = 0; i < lastSeeingChecks.size(); i++)
    {
      if (lastSeeingChecks[i])
      {
        falsecount++;
      }
    }
    // if >= 2 false in last 5 checks, reset GoodSkyStateCount
    if (falsecount >= 2)
    {
      GOOD_SKY_STATE_COUNT = 0;
    }

    // shutdown SEEING if bad skystate
    if (BAD_SKY_STATE_COUNT == seeing_thr)
    {
      UART_shutdown_Seeing();
      SEEING_ENABLED = false;
    }
    // cut power for SEEING if bad skystate after seeing_thr time + buffertime
    // to make sure RPi is shut down
    if (BAD_SKY_STATE_COUNT == seeing_thr + (int)(60 / SLEEPTIME_s))
    {
      high_hold_Pin(EN_SEEING);
    }
  }
}

bool getSavedWifiSettings()
{
  if (!initSPIFFS())
  {
    return false;
  }

  // Load values saved in SPIFFS (if exists, else fallback to settings.h)
  String temp;
  // Load SSID
  if (SPIFFS.exists(ssidPath))
  {
    temp = readLineOfFile(SPIFFS, ssidPath);
    temp.toCharArray(WIFI_SSID, 100);
  }
  // Load password
  if (SPIFFS.exists(passPath))
  {
    temp = readLineOfFile(SPIFFS, passPath);
    temp.toCharArray(WIFI_PASS, 100);
  }
  // Load IP address
  if (SPIFFS.exists(ipPath))
  {
    temp = readLineOfFile(SPIFFS, ipPath);
    temp.toCharArray(SERVER_IP, 100);
  }

  // Construct URLs for posting sensor values and fetching settings
  ("http://" + String(SERVER_IP) + ":" + String(serverPort) + "/SQM").toCharArray(SEND_SERVER, 100);
  ("http://" + String(SERVER_IP) + ":" + String(serverPort) + "/getsettings").toCharArray(FETCH_SERVER, 100);

  SPIFFS.end();
  return true;
}

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
  Wire1.begin(SDA_2, SCL_2, 100000U);
  // Start the I2C communication for the other sensors
  Wire.begin(21, 22, 10000U);

  // Load the saved network settings
  if (!hasInitialized)
  {
    getSavedWifiSettings();
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
  delay(50);
  if (!init_AS3935(Wire1))
  {
    sensorErrors.push_back("init_AS3935");
  }
  delay(10);

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
  delay(50);
  if (!read_AS3935(lightning_distanceToStorm))
  {
    sensorErrors.push_back("read_AS3935");
  }
  delay(50);
  if (!read_TSL237(luminosity, nelm, SQM_LIMIT))
  {
    sensorErrors.push_back("read_TSL237");
  }
  // read the sensor values
  read_particle(concentration);
  read_rain(raining);

  if (SEEING_ENABLED)
  {
    UART_get_Seeing();
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
        settingsLoaded = fetch_settings();
      }
      hasServerError = !post_data();
      if (hasServerError)
      {
        serverErrorCount++;
      }
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
      DisplayStatusMessage();
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

    check_seeing_threshhold();
  }

  DisplayStatusMessage();

  WiFi.mode(WIFI_MODE_NULL);           // Switch WiFi off
  esp_deep_sleep(sleepTime * 1000000); // send ESP32 to deepsleep
}
