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
#include "FreqCountESP.h"
#include "SSD1306Wire.h"

#include "sensor_lightning.h"
#include "sensor_ir_temperature.h"
#include "sensor_light.h"
#include "sensor_dust.h"
#include "sensor_rain.h"
#include "sensor_SQM.h"

//value doesnt get reset after deepsleep
RTC_DATA_ATTR int noWifiCount = 0;
RTC_DATA_ATTR int sendCount = 0;
RTC_DATA_ATTR bool hasWIFI =  false;

int sleepTime=0;

//sensor values 
bool raining=-1;
float ambient, object=-1;
double lux=-1; // Resulting lux value
int lightning_distanceToStorm = -1;
float mySQMreading = -1; // the SQM value, sky magnitude
double irradiance = -1;
double nelm = -1;
int concentration = -1;

// for 128x64 displays:
SSD1306Wire display(0x3c, SDA, SCL);  // ADDRESS, SDA, SCL

void DisplayText(String text) {
  display.setFont(ArialMT_Plain_10);
  display.setTextAlignment(TEXT_ALIGN_LEFT);
  display.drawStringMaxWidth(0, 0, 128, text);
  display.display();
}

void DisplayStatus(String status) {
  display.setTextAlignment(TEXT_ALIGN_LEFT);
  display.setFont(ArialMT_Plain_10);
  display.drawString(0, 26, status);
  display.display();
}

//send the sensor values via http post request
bool post_data()
{

  WiFiClient client;
  HTTPClient http;

  std::stringstream data;
  //create a json string
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


void setup()
{
  //enable the 5V/3,3V supply voltage for the sensors
  pinMode(EN_3V3, OUTPUT); 
  pinMode(EN_5V, OUTPUT); 
  digitalWrite(EN_3V3, HIGH);
  digitalWrite(EN_5V, HIGH);

  // Set WiFi to station mode
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  WiFi.setTxPower(WIFI_POWER_19_5dBm);


  Wire.begin(); // I2C bus

  display.init();
  DisplayStatus("Status: on");
   if(!hasWIFI){
    DisplayText("Connecting to WIFI: try "+String(noWifiCount+1));
  }

  init_MLX90614();
  init_TSL2561();
  init_AS3935();
  FreqCountESP.begin(SQMpin, 1000);
  pinMode(rainS_DO, INPUT);
  pinMode(particle_pin, INPUT);
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

  if (WiFi.status() == WL_CONNECTED)
  {
    post_data();
    hasWIFI=true;
    DisplayText("Connected to WIFI - send data for the: "+ String(sendCount)+" time");
    sendCount++;
    //set custom sleep time
    sleepTime=SLEEPTIME_s;
  }
  //else wait for connection
  else if (noWifiCount==0)
  {
    sleepTime=2;
    //set sleep time = 2 sec
    noWifiCount++;
  }
  else{
    sleepTime=20;
    hasWIFI=false;
    //set sleep time = 20 sec
    noWifiCount++;
     //not >= because after first reset it also has no Wifi
    if(noWifiCount>NO_WIFI_MAX_RETRIES)
    {
      DisplayText("Sleeping - Couldn't find WIFI");
      //sleep forever
      esp_deep_sleep(0);
    }
  }


  WiFi.mode(WIFI_MODE_NULL); // Switch WiFi off
  //esp_wifi_stop();
  
  //disable sensor supply voltage
  digitalWrite(EN_3V3, LOW);
  digitalWrite(EN_5V, LOW);

  DisplayStatus("Status: sleeping");
  //start deep sleep
  esp_deep_sleep(sleepTime*1000000);
}
