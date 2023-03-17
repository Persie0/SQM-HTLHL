#include <Arduino.h>
#include "spiffs_fkt.h"
#include <WiFi.h>
#include <HTTPClient.h>
#include <ESPAsyncWebServer.h>
#include <AsyncTCP.h>
#include <SPIFFS.h>
#include <sstream>
#include "settings.h"
#include <ArduinoJson.h>

// Function to activate access point and provide website for changing WIFI settings
void activate_access_point()
{
  // Initialize storage to later read/write from/to it
  initSPIFFS();

  // Create AsyncWebServer object on port 80
  AsyncWebServer server(80);

  // Switch WiFi off and switch to AP mode
  WiFi.mode(WIFI_MODE_NULL);
  WiFi.mode(WIFI_AP);
  delay(10);

  // AP IP settings
  IPAddress localIP(192, 168, 1, 2);
  IPAddress gateway(192, 168, 1, 0);
  IPAddress subnet(255, 255, 255, 0);
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

    char WIFI_SSID[100];
    char WIFI_PASS[100];
    char SERVER_IP[100];
    
    // Loop through the parameters
    for (int i = 0; i < params; i++) {
      AsyncWebParameter *p = request->getParam(i);
      if (p->isPost()) {
        // Handle the HTTP POST for the SSID parameter
        if (p->name() == "ssid") {
          // WIFI_SSID = on  website entered SSID
          (p->value()).toCharArray(WIFI_SSID, 100);
          // Write the SSID to a file for persistence
          if (!writeLineOfFile(SPIFFS, ssidPath, WIFI_SSID)) {
            Serial.println("Couldn't write");
          }
        }
        // Handle the HTTP POST for the password parameter
        if (p->name() == "pass") {
          (p->value()).toCharArray(WIFI_PASS, 100);
          // Write the password to a file for persistence
          writeLineOfFile(SPIFFS, passPath, WIFI_PASS);
        }
        // Handle the HTTP POST for the IP address parameter
        if (p->name() == "ip") {
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
  esp_deep_sleep(-77777777);
}

bool getSavedWifiSettings(char *WIFI_SSID, char *WIFI_PASS, char *SERVER_IP, char *SEND_VALUES_SERVER, char *FETCH_SETTINGS_SERVER)
{
  if (!initSPIFFS())
  {
    return false;
  }

  // Load values saved in SPIFFS
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
  ("http://" + String(SERVER_IP) + ":" + String(serverPort) + "/SQM").toCharArray(SEND_VALUES_SERVER, 100);
  ("http://" + String(SERVER_IP) + ":" + String(serverPort) + "/getsettings").toCharArray(FETCH_SETTINGS_SERVER, 100);

  SPIFFS.end();
  return true;
}

// Function to fetch settings from a server
bool fetch_settings(char *FETCH_SETTINGS_SERVER, int &seeing_thr, double &SP1, double &SP2, double &MAX_LUX, int &SLEEPTIME_s, int &DISPLAY_TIMEOUT_s, int &DISPLAY_ON, double &SQM_LIMIT)
{
  // Create client object for communication
  WiFiClient client;

  // Create HTTP client object
  HTTPClient http;

  // Send request to the server
  http.useHTTP10(true);
  http.begin(client, FETCH_SETTINGS_SERVER);
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
      DISPLAY_ON = doc["DISPLAY_ON"].as<int>();
    }
    if (doc.containsKey("set_sqm_limit"))
    {
      SQM_LIMIT = doc["set_sqm_limit"].as<double>();
    }
  }
  // Disconnect
  http.end();
  return httpResponseCode == 200;
}

// send the sensor values via http post request to the server
bool post_data(char *SEND_VALUES_SERVER, bool raining, float luminosity, String seeing, double nelm, int concentration, float object, float ambient, double lux, int lightning_distanceToStorm, std::vector<String> sensorErrors, bool SEEING_ENABLED)
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
  String sensor_data = "{\"raining\":\"" + String(raining) + "\",\"luminosity\":\"" + luminosity + "\",\"seeing\":\"" + seeing + "\",\"nelm\":\"" + nelm +
                       "\",\"concentration\":\"" + concentration + "\",\"object\":\"" + object + "\",\"ambient\":\"" + ambient + "\",\"lux\":\"" + lux +
                       "\",\"lightning_distanceToStorm\":\"" + lightning_distanceToStorm + "\",\"errors\":\"" + errors + "\",\"isSeeing\":\"" + SEEING_ENABLED + "\"}";
  // Start the HTTP connection
  http.begin(client, SEND_VALUES_SERVER);
  http.addHeader("Content-Type", "application/json");

  // Send the JSON string as the HTTP body
  int httpResponseCode = http.POST(sensor_data);
  http.end(); // End the connection

  // Return true if the post request is successful
  return httpResponseCode == 200;
}
