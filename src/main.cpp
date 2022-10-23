/**
 ******************************************************************************
 * @file    main.c
 * @author  Marvin Perzi
 * @version V01
 * @date    2022
 * @brief   Sky Quality Meter that sends sensor values to an API endpoint
 ******************************************************************************
 */

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

HardwareSerial SerialPort(2); // use UART2

String incomingString;
int sleepTime = 0;
bool sleepForever = false;
vector<String> sensorErrors;

// if it has no internet - Access Point, WIFI settings
IPAddress localIP(192, 168, 1, 1);
IPAddress gateway(192, 168, 1, 0);
IPAddress subnet(255, 255, 255, 0);

// Create AsyncWebServer object on port 80
AsyncWebServer server(80);
// for 128x64 displays:
SSD1306Wire display(0x3c, SDA, SCL); // ADDRESS, SDA, SCL

// Replaces placeholder with Wifi info
String wifi_info(const String &var)
{
  return "SSID: " + WIFI_SSID + ", \nPW: " + WIFI_PASS + ", \nServer IP: " + SERVER_IP;
}

void activate_access_point()
{
  initSPIFFS();
  WiFi.mode(WIFI_MODE_NULL); // Switch WiFi off
  WiFi.mode(WIFI_AP);        // Switch WiFi off
  delay(10);
  WiFi.softAPConfig(localIP, gateway, subnet);
  // NULL sets an open Access Point
  WiFi.softAP("ESP-WIFI-MANAGER", NULL);

  // Web Server Root URL
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request)
            { request->send(SPIFFS, "/wifimanager.html", "text/html"); });

  server.serveStatic("/", SPIFFS, "/");

  server.on("/", HTTP_POST, [](AsyncWebServerRequest *request)
            {
      int params = request->params();
      for(int i=0;i<params;i++){
        AsyncWebParameter* p = request->getParam(i);
        if(p->isPost()){
          // HTTP POST ssid value
          if (p->name() == PARAM_INPUT_1) {
            WIFI_SSID = p->value().c_str();
            // Write file to save value
            writeFile(SPIFFS, ssidPath, WIFI_SSID.c_str());
          }
          // HTTP POST pass value
          if (p->name() == PARAM_INPUT_2) {
            WIFI_PASS = p->value().c_str();
            // Write file to save value
            writeFile(SPIFFS, passPath, WIFI_PASS.c_str());
          }
          // HTTP POST ip value
          if (p->name() == PARAM_INPUT_3) {
            SERVER_IP = p->value().c_str();
            // Write file to save value
            writeFile(SPIFFS, ipPath, SERVER_IP.c_str());
          }
        }
      }
      request->send(200, "text/plain", "Done. ESP will restart, connect to your router "+ String(WIFI_SSID.c_str())+" and go to IP address: " + SERVER_IP);
      delay(6000);
      ESP.restart(); });
  server.begin();
  unsigned long startTime = millis();
  while ((millis() - startTime) < 1200 * 1000) // run for 20min
  {
    delay(100);
  }
  // if no changes - sleep forever
  esp_deep_sleep(0);
}

void DisplayStatus()
{
  if (DISPLAY_ON)
  {
    display.init();
    display.setFont(ArialMT_Plain_10);
    display.setTextAlignment(TEXT_ALIGN_LEFT);
    if (ESP_MODE == 1)
    {
      if (sleepForever)
      {
        display.drawStringMaxWidth(0, 0, 128, "Sleeping forever");
        display.drawStringMaxWidth(0, 12, 128, "no WIFI, server error");
        display.drawStringMaxWidth(0, 24, 128, "or server not reachable");
      }
      else
      {
        if (hasWIFI)
        {
          display.drawStringMaxWidth(0, 0, 128, "Connected to Wifi");
        }
        else
        {
          display.drawStringMaxWidth(0, 0, 128, "NO Wifi");
        }
        if (hasWIFI)
        {
          display.drawStringMaxWidth(0, 12, 128, "send count: " + String(sendCount));
        }
        else
        {
          display.drawStringMaxWidth(0, 12, 128, "retry count: " + String(noWifiCount));
        }
        if (settingsLoaded)
        {
          display.drawStringMaxWidth(0, 24, 128, "settings loaded");
        }
        else
        {
          display.drawStringMaxWidth(0, 24, 128, "settings NOT loaded");
        }
        if (hasWIFI)
        {
          if (hasServerError)
          {
            display.drawStringMaxWidth(0, 36, 128, "server error!");
          }
          else
          {
            display.drawStringMaxWidth(0, 36, 128, "server is running");
          }
        }
      }
    }
    else if (ESP_MODE == 0)
    {
      display.drawStringMaxWidth(0, 0, 128, "SQM: " + String(luminosity));
      display.drawStringMaxWidth(0, 12, 128, "Airp.: " + String(concentration));
      display.drawStringMaxWidth(0, 24, 128, "Lux: " + String(lux));
      display.drawStringMaxWidth(0, 36, 128, "Raining: " + String(raining));
      display.drawStringMaxWidth(0, 48, 128, "A&O: " + String(ambient) + "; " + String(object) + " °C");
    }
    else
    {
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
    }
    display.display();
  }
}

// set a Pin as output and keep it high also in deepsleep
void high_hold_Pin(gpio_num_t pin)
{
  // keep display on in deepsleep
  pinMode(pin, OUTPUT);
  digitalWrite(pin, HIGH);
  gpio_hold_en(pin);
  gpio_deep_sleep_hold_en();
}

// fetch settings from server
bool fetch_settings()
{
  WiFiClient client; // or WiFiClientSecure for HTTPS
  HTTPClient http;

  // Send request
  http.useHTTP10(true);
  http.begin(client, FETCH_SERVER);
  int httpResponseCode = http.GET();
  if (httpResponseCode == 200)
  {
    // Parse response
    DynamicJsonDocument doc(2048);
    deserializeJson(doc, http.getStream());
    // Read values
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
    if (doc.containsKey("check_everytime"))
    {
      ALWAYS_FETCH_SETTINGS = doc["check_everytime"].as<bool>();
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
  // create a json string
  data << "{\"raining\":\"" << raining << "\",\"luminosity\":\"" << luminosity << "\",\"seeing\":\"" << seeing << "\",\"nelm\":\"" << nelm << "\",\"concentration\":\"" << concentration << "\",\"object\":\"" << object << "\",\"ambient\":\"" << ambient << "\",\"lux\":\"" << lux << "\",\"lightning_distanceToStorm\":\"" << lightning_distanceToStorm << "\"}";
  std::string s = data.str();
  // Your Domain name with URL path or IP address with path
  http.begin(client, SEND_SERVER);
  // If you need an HTTP request with a content type: application/json, use the following:
  http.addHeader("Content-Type", "application/json");
  int httpResponseCode = http.POST(s.c_str());
  // Free resources
  http.end();
  if (httpResponseCode == 200)
  {
    return true;
  }
  else
  {
    return false;
  }
}

void getcloudstate()
{
  static int setpoint1 = 22; // setpoint values used to determine sky state
  static int setpoint2 = 2;
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
  if (TempDiff > setpoint1)
  {
    CLOUD_STATE = SKYCLEAR; // clear
  }
  else if ((TempDiff > setpoint2) && (TempDiff < setpoint1))
  {
    CLOUD_STATE = SKYPCLOUDY; // partly cloudy
  }
  else if (TempDiff < setpoint2)
  {
    CLOUD_STATE = SKYCLOUDY; // cloudy
  }
  else
  {
    CLOUD_STATE = SKYUNKNOWN; // unknown
  }
}

bool check_seeing()
{
  getcloudstate();
  if (CLOUD_STATE == SKYCLEAR && !raining && lux < 50 && luminosity > 18 && concentration < 10000)
  {
    return true;
  }
  else
  {
    return false;
  }
}

bool shutdown_Seeing()
{
  SerialPort.begin(9600, 134217756U, MYPORT_RX, MYPORT_TX, false);
  SerialPort.println("shut");
  unsigned long startTime = millis();
  while (Serial.available() == 0 && ((millis() - startTime) < 15 * 1000))// wait for 15s for data available
  {}                                     
  String str = Serial.readString(); // read until timeout
  str.trim();
  if(str=="ok"){
    SerialPort.println("shutdown");
    return true;
  }
  return false;
}

bool get_Seeing()
{
  SerialPort.begin(9600, 134217756U, MYPORT_RX, MYPORT_TX, false);
  SerialPort.println("get");
  unsigned long startTime = millis();
  while (Serial.available() == 0)// wait for 15s for data available
  {
    if((millis() - startTime) > 15 * 1000){
      return false;
    }
  }                                     
  String teststr = Serial.readString(); // read until timeout
  teststr.trim();
  seeing = teststr;
  return true;
}

void setup()
{
  // Serial.begin(115200);

  // enable display if set
  if (!hasInitialized)
  {
    initSPIFFS();
    // Load values saved in SPIFFS
    WIFI_SSID = readFile(SPIFFS, ssidPath);
    WIFI_PASS = readFile(SPIFFS, passPath);
    SERVER_IP = readFile(SPIFFS, ipPath);
    hasInitialized = true;

    // Post sensor values - Domain name with URL path or IP address with path
    SEND_SERVER = "http://" + String(SERVER_IP) + ":" + String(serverPort) + "/SQM";
    // Get settings - Domain name with URL path or IP address with path
    FETCH_SERVER = "http://" + String(SERVER_IP) + ":" + String(serverPort) + "/getsettings";
    SPIFFS.end();
  }

  // Enable & Set WiFi to station mode
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID.c_str(), WIFI_PASS.c_str());
  WiFi.setTxPower(WIFI_POWER_19_5dBm);
  // init SQM sensor
  FreqCountESP.begin(SQMpin, 55);
  Wire.begin(); // I2C bus begin, so can talk to display

  if (DISPLAY_ON)
  {
    // keep display on in deepsleep
    high_hold_Pin(EN_Display);
  }
  // enable the 5V/3,3V supply voltage for the sensors
  pinMode(EN_3V3, OUTPUT);
  pinMode(EN_5V, OUTPUT);
  digitalWrite(EN_3V3, HIGH);
  digitalWrite(EN_5V, HIGH);

  delay(50);

  // initialise sensors, if sensor error add to array
  if (!init_MLX90614())
  {
    sensorErrors.push_back("init_MLX90614");
  }
  init_TSL2561();
  if (!init_TSL2561())
  {
    sensorErrors.push_back("init_TSL2561");
  }
  init_AS3935();
  if (!init_AS3935())
  {
    sensorErrors.push_back("init_AS3935");
  }

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
  // read the sensor values
  read_particle(concentration);
  read_rain(raining);

  // turn display off after set time
  if (DISPLAY_ON && hasWIFI && DISPLAY_TIMEOUT_s != 0 && (DISPLAY_TIMEOUT_s < ((sendCount * SLEEPTIME_s) + (noWifiCount * (NOWIFI_SLEEPTIME_s + 6)))))
  {
    // disable sensor supply voltage
    DISPLAY_ON = false;
    digitalWrite(EN_Display, LOW);
  }

  // send data if connected
  if (WiFi.status() == WL_CONNECTED)
  {
    // fetch settings if not loaded yet and desired
    if (!settingsLoaded || ALWAYS_FETCH_SETTINGS)
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
    // sleep forever if max retries reached
    if (noWifiCount >= NO_WIFI_MAX_RETRIES)
    {
      // open AP for changing WIFI settings
      sleepForever = true;
      DisplayStatus();
      activate_access_point();
    }
  }

  // if too couldnt send data too many times - sleep forever
  if (serverErrorCount > sendCount && serverErrorCount > NO_WIFI_MAX_RETRIES)
  {
    // sleep forever
    sleepTime = 0;
    sleepForever = true;
  }

  // if couldnt send data - turn display on again and show error message
  if ((!hasWIFI || hasServerError) && !DISPLAY_ON)
  {
    // keep display on in deepsleep
    high_hold_Pin(EN_Display);
  }

  //check if sensor values are good and if seeing should be enabled
  if(check_seeing()){
    ++GOOD_SKY_STATE_COUNT;
    //check if skystate is constant
    if(BAD_SKY_STATE_COUNT==1)
    {
      BAD_SKY_STATE_COUNT=0;
    }
    if(GOOD_SKY_STATE_COUNT>2){
    BAD_SKY_STATE_COUNT=0;
    // keep Seeing on in deepsleep
    high_hold_Pin(EN_SEEING);
    }
  }
  else{
    ++BAD_SKY_STATE_COUNT;
    //check if skystate is constant
    if(GOOD_SKY_STATE_COUNT==1)
    {
      GOOD_SKY_STATE_COUNT=0;
    }
    //shutdown SEEING if bad skystate
    if(BAD_SKY_STATE_COUNT==2){
      shutdown_Seeing();
      GOOD_SKY_STATE_COUNT=0;
    }
    //cut power for SEEING if bad skystate after 3rd time + buffertime
    //to make shure RPi is shut down
    if(BAD_SKY_STATE_COUNT==3+(60/SLEEPTIME_s)){
    digitalWrite(EN_SEEING, LOW);
    }
  }

  DisplayStatus();

  WiFi.mode(WIFI_MODE_NULL); // Switch WiFi off

  esp_deep_sleep(sleepTime * 1000000);//send ESP32 to deepsleep
}
