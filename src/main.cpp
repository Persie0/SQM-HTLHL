/**
 ******************************************************************************
 * @file    main.c
 * @author  Marvin Perzi
 * @version V01
 * @date    2022
 * @brief   Sky Quality Meter that sends sensor values to a API endpoint
 ******************************************************************************
 */

#include <Arduino.h>
#include "settings.h"
#include <Wire.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <sstream>
#include <driver/rtc_io.h>
#include <ArduinoJson.h>

#include "FreqCountESP.h"
#include "SSD1306Wire.h"

#include "sensors/sensor_lightning.h"
#include "sensors/sensor_ir_temperature.h"
#include "sensors/sensor_light.h"
#include "sensors/sensor_dust.h"
#include "sensors/sensor_rain.h"
#include "sensors/sensor_SQM.h"

// value doesnt get reset after deepsleep
RTC_DATA_ATTR int noWifiCount = 0;
RTC_DATA_ATTR int sendCount = 0;
RTC_DATA_ATTR int serverErrorCount = 0;
RTC_DATA_ATTR bool settingsLoadCount = 0;

RTC_DATA_ATTR bool hasInitialized = false;
RTC_DATA_ATTR bool firstBoot = true;
RTC_DATA_ATTR bool settingsLoaded = false;
RTC_DATA_ATTR bool hasWIFI = false;
RTC_DATA_ATTR bool serverError = false;

RTC_DATA_ATTR int SLEEPTIME_s = FALLBACK_SLEEPTIME_s;
RTC_DATA_ATTR int NO_WIFI_MAX_RETRIES = FALLBACK_NO_WIFI_MAX_RETRIES;
RTC_DATA_ATTR int DISPLAY_TIMEOUT_s = FALLBACK_DISPLAY_TIMEOUT_s;
RTC_DATA_ATTR bool DISPLAY_ON = FALLBACK_DISPLAY_ON;

int sleepTime = 0;
bool sleepForever=false;

// sensor values
bool raining = false;
float ambient, object = -1;
double lux = -1; // Resulting lux value
int lightning_distanceToStorm = -1;
float mySQMreading = -1; // the SQM value, sky magnitude
double irradiance = -1;
double nelm = -1;
int concentration = -1;

// for 128x64 displays:
SSD1306Wire display(0x3c, SDA, SCL); // ADDRESS, SDA, SCL

void DisplayStatus()
{
  if (DISPLAY_ON)
  {
    display.init();
    display.setFont(ArialMT_Plain_10);
    display.setTextAlignment(TEXT_ALIGN_LEFT);
    if(sleepForever){
      display.drawStringMaxWidth(0, 0, 128, "Sleeping forever");
      display.drawStringMaxWidth(0, 12, 128, "no WIFI, server error");
      display.drawStringMaxWidth(0, 24, 128, "or server not reachable");
    }
    else{
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
      display.drawStringMaxWidth(0, 12, 128, "retry count: " + String(sendCount));
    }
    if (settingsLoaded)
    {
      display.drawStringMaxWidth(0, 24, 128, "settings loaded");
    }
    else
    {
      display.drawStringMaxWidth(0, 24, 128, "settings NOT loaded");
    }
    if (serverError)
    {
      display.drawStringMaxWidth(0, 36, 128, "server error!");
    }
    else{
      display.drawStringMaxWidth(0, 36, 128, "server is running");
    }
    }
    display.display();
  }
}

// fetch setting from API
bool fetch_settings()
{
  WiFiClient client; // or WiFiClientSecure for HTTPS
  HTTPClient http;

  // Send request
  http.useHTTP10(true);
  http.begin(client, fetchserverName);
  int httpResponseCode = http.GET();
  if (httpResponseCode == 200)
  {
    // Parse response
    DynamicJsonDocument doc(2048);
    deserializeJson(doc, http.getStream());
    // Read values
    if(doc.containsKey("SLEEPTIME_s"))
    {
      SLEEPTIME_s = doc["SLEEPTIME_s"].as<int>();
    }
       if(doc.containsKey("sqm_limit"))
    {
      SLEEPTIME_s = doc["sqm_limit"].as<double>();
    }
       if(doc.containsKey("DISPLAY_TIMEOUT_s"))
    {
      SLEEPTIME_s = doc["DISPLAY_TIMEOUT_s"].as<int>();
    }
       if(doc.containsKey("DISPLAY_ON"))
    {
      SLEEPTIME_s = doc["DISPLAY_ON"].as<bool>();
    }
  
  }
  // Disconnect
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

// send the sensor values via http post request
bool post_data()
{

  WiFiClient client;
  HTTPClient http;

  std::stringstream data;
  // create a json string
  data << "{\"raining\":\"" << raining << "\",\"SQMreading\":\"" << mySQMreading << "\",\"irradiance\":\"" << irradiance << "\",\"nelm\":\"" << nelm << "\",\"concentration\":\"" << concentration << "\",\"object\":\"" << object << "\",\"ambient\":\"" << ambient << "\",\"lux\":\"" << lux << "\",\"lightning_distanceToStorm\":\"" << lightning_distanceToStorm << "\"}";
  std::string s = data.str();
  // Your Domain name with URL path or IP address with path
  http.begin(client, sendserverName);
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

void setup()
{
  Serial.begin(115200);
  // Enable & Set WiFi to station mode
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  WiFi.setTxPower(WIFI_POWER_19_5dBm);

  // fix bug not connecting to wifi
  if (firstBoot)
  {
     if (WiFi.status() != WL_CONNECTED){
    firstBoot = false;
    esp_deep_sleep(2000000);}
  }

  Wire.begin(); // I2C bus begin, so can talk to display

  // fetch settings once from server, enable display if set
  if (!hasInitialized)
  {
    hasInitialized = true;
          // enable the 3,3V supply voltage for the display
    if (DISPLAY_ON)
    {
      pinMode(EN_Display, OUTPUT);
      digitalWrite(EN_Display, HIGH);
     //gpio_hold_en(EN_DisplayGPIO);
      rtc_gpio_init(EN_DisplayGPIO); //initialize the RTC GPIO port
    rtc_gpio_set_direction(EN_DisplayGPIO, RTC_GPIO_MODE_OUTPUT_ONLY); //set the port to output only mode
    rtc_gpio_hold_dis(EN_DisplayGPIO); //disable hold before setting the level

    rtc_gpio_set_level(EN_DisplayGPIO, 1); //set high/low

    //gpio_deep_sleep_hold_en();
    rtc_gpio_hold_en(EN_DisplayGPIO); // enable hold for the RTC GPIO port
    }
  }

  // enable the 5V/3,3V supply voltage for the sensors
  pinMode(EN_3V3, OUTPUT);
  pinMode(EN_5V, OUTPUT);
  pinMode(EN_Lightning, OUTPUT);
  digitalWrite(EN_Lightning, HIGH);
  digitalWrite(EN_3V3, HIGH);
  digitalWrite(EN_5V, HIGH);

  init_MLX90614();
  init_TSL2561();
  init_AS3935();
  FreqCountESP.begin(SQMpin, 1000);
  pinMode(rainS_DO, INPUT);
  pinMode(particle_pin, INPUT);
      //gpio_hold_en(EN_LightningGPIO);
}

void loop()
{

  // read the sensor values
  read_MLX90614(ambient, object);
  read_TSL2561(lux);
  read_particle(concentration);
  read_AS3935(lightning_distanceToStorm);
  read_rain(raining);
  read_TSL237(mySQMreading, irradiance, nelm);

  // turn display off after set time
  if (DISPLAY_ON && hasWIFI && (DISPLAY_TIMEOUT_s < ((sendCount*SLEEPTIME_s) + (noWifiCount * (NOWIFI_SLEEPTIME_s + 6)))))
  {
    // disable sensor supply voltage
    DISPLAY_ON = false;
    digitalWrite(EN_Display, LOW);
  }

  // send data if connected
  if (WiFi.status() == WL_CONNECTED)
  {
    if(!settingsLoaded){
      settingsLoaded=fetch_settings();
    }
    serverError = !post_data();
    if(serverError){
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
    // enable display if it was turned off
    hasWIFI = false;
    // sleep forever if max retries reached
    if (noWifiCount >= NO_WIFI_MAX_RETRIES)
    {
      // sleep forever
      sleepTime = 9999999;
      sleepForever=true;
    }
  }

  if(serverErrorCount>sendCount && serverErrorCount > NO_WIFI_MAX_RETRIES){
      // sleep forever
      sleepTime = 9999999;
      sleepForever=true;
  }

  if ((!hasWIFI || serverError)&& !DISPLAY_ON)
  {
    pinMode(EN_Display, OUTPUT);
    digitalWrite(EN_Display, HIGH);
    // hold GPIO voltage in deepsleep
    gpio_hold_en(EN_DisplayGPIO);
  }

  DisplayStatus();
  gpio_deep_sleep_hold_en();

  WiFi.mode(WIFI_MODE_NULL); // Switch WiFi off
  // esp_wifi_stop();

  esp_deep_sleep(sleepTime * 1000000);
}
