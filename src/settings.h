
#ifndef SETTINGS_H
#define SETTINGS_H

// ===========================================================
//                 WIFI SETTINGS
// ===========================================================

// WiFi credentials.
#define FALLBACK_WIFI_SSID "SQM"
#define FALLBACK_WIFI_PASS "Passw0rt"

#define FALLBACK_SERVER_IP "192.168.43.130"
#define serverPort "5000"

// Post sensor values - Domain name with URL path or IP address with path
// Get settings - Domain name with URL path or IP address with path

// File paths to save input values permanently
#define ssidPath  "/ssid.txt"
#define passPath  "/pass.txt"
#define ipPath  "/ip.txt"

// ===========================================================
//                 PINS
// ===========================================================

// do not use GPIO0 - ESP32 wont boot correctly
#define particle_pin 27  // particle sensor pin; reciving the sensor values with it
#define lightning_pin 13 // lightning Sensor Pin; reciving the sensor values with it
#define SQMpin 25        // TSL237 OUT to digital pin 25
#define rainS_DO 18      // the rain sensor digital port pin, raining=yes/no

#define EN_3V3 15              // power-control pin to enable the 3,3V for the sensors
#define EN_5V 2              // power-control pin to enable the 5V for the sensors
#define EN_SEEING GPIO_NUM_14 // power-control pin to enable Seeing

#define EN_Display GPIO_NUM_4 // power-control pin to enable the display

#define MYPORT_TX 17 //TX Port for UART communication
#define MYPORT_RX 16 //RX Port for UART communication
 
#define SDA_2 14 // SDA for lightning sensor
#define SCL_2 5 // SCL for lightning sensor

// ===========================================================
//                 ESP32 SETTINGS
// ===========================================================

/* You only need to format SPIFFS the first time you run a
   test or else use the SPIFFS plugin to create a partition
   https://github.com/me−no−dev/arduino−esp32fs−plugin */
#define FORMAT_SPIFFS_IF_FAILED false

// Display
#define ESP_MODE 1                     // 0-Offline (just display sensor values), 1-Online (normal), 2-Test (testing sensors, showing errors on display)
#define FALLBACK_DISPLAY_TIMEOUT_s 200 /* Time Display will be on after start */
#define FALLBACK_DISPLAY_ON true       /* display on after start */

#define FALLBACK_SLEEPTIME_s 20             /* Time ESP32 will go to sleep (in seconds) */
#define FALLBACK_NO_WIFI_MAX_RETRIES 3       /* Time ESP32 will go to sleep (in seconds) */
#define NOWIFI_SLEEPTIME_s 5                /* if no wifi connection, Time ESP32 will go to sleep (in seconds) */
#define FALLBACK_seeing_thr 3

#define FALLBACK_SQM_LIMIT 21.83 // mag limit for earth is 21.95
#define FALLBACK_SP1 22 // Setpoint 1 (setpoint for clear skies)
#define FALLBACK_MAXLUX 50 // Max. Lux (for Seeing check)
#define FALLBACK_SP2 2 // Setpoint 2 (setpoint for cloudy skies)

// ===========================================================
//                 LIGHTNING SENSOR SETTINGS
// ===========================================================

#define AS3935_ADDR 0x00
//was AS3935_ADDR 0x00
// I2C Address
//  0x03 is default, but the address can also be 0x02, or 0x01.
//  Adjust the address jumpers on the underside of the product.

#define LOCATION 0xE
// where the sensor is (sensitivity depends of that)
// INDOOR 0x12
// OUTDOOR 0xE

#define noiseFloor 2
// Noise floor setting from 1-7, one being the lowest. Default setting is
// two. If you need to check the setting, the corresponding function for
// reading the function follows.

#define watchDogVal 2
// Watchdog threshold setting can be from 1-10, one being the lowest. Default setting is
// two. If you need to check the setting, the corresponding function for
// reading the function follows.

#define spike 2
// Spike Rejection setting from 1-11, one being the lowest. Default setting is
// two. If you need to check the setting, the corresponding function for
// reading the function follows.
// The shape of the spike is analyzed during the chip's
// validation routine. You can round this spike at the cost of sensitivity to
// distant events.

#define lightningThresh 1
// This setting will change when the lightning detector issues an interrupt.
// For example you will only get an interrupt after five lightning strikes
// instead of one. Default is one, and it takes settings of 1, 5, 9 and 16.
// Followed by its corresponding read function. Default is zero.

// ===========================================================
//                 LIGHT SENSOR SETTINGS
// ===========================================================

#define gain 0
// Gain setting, 0 = X1, 1 = X16;
// If gain = false (0), device is set to low gain (1X)
// If gain = high (1), device is set to high gain (16X)

#define shuttertime 1
// Integration ("shutter") time in milliseconds
// If time = 0, integration will be 13.7ms
// If time = 1, integration will be 101ms
// If time = 2, integration will be 402ms
// If time = 3, use manual start / stop to perform your own integration

// ===========================================================
//                 DUST SENSOR SETTINGS
// ===========================================================

#define sampletime_ms 3000 // sample 3s

#endif
