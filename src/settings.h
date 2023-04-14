
#ifndef SETTINGS_H
#define SETTINGS_H

// ===========================================================
//                 WIFI SETTINGS
// ===========================================================

// WiFi credentials.
#define serverPort "5000" // Port of the server

// File paths to save input values permanently
#define ssidPath  "/ssid.txt"
#define passPath  "/pass.txt"
#define ipPath  "/ip.txt"

// ===========================================================
//                 PINS
// ===========================================================

// do not use GPIO0 - ESP32 wont boot correctly
#define particle_pin 32  // particle sensor pin; reciving the sensor values with it
#define lightning_pin 13 // lightning Sensor Pin; reciving the sensor values with it
#define SQMpin 33        // TSL237 OUT to digital pin 25
#define rainS_DO 27      // the rain sensor digital port pin, raining=yes/no

//#define EN_3V3 15              // power-control pin to enable the 3,3V for the sensors
//#define EN_5V 2              // power-control pin to enable the 5V for the sensors
#define EN_SEEING GPIO_NUM_5 // power-control pin to enable Seeing
#define EN_Display GPIO_NUM_4 // power-control pin to enable the display

#define MYPORT_TX 17 //TX Port for UART communication
#define MYPORT_RX 16 //RX Port for UART communication
 
#define SDA_1 21 // SDA for other sensors
#define SCL_1 22 // SCL for other sensors
#define SDA_2 14 // SDA for lightning sensor
#define SCL_2 12 // SCL for lightning sensor


// ===========================================================
//                 ESP32 SETTINGS
// ===========================================================

/* You only need to format SPIFFS the first time you run a
   test or else use the SPIFFS plugin to create a partition
   https://github.com/me−no−dev/arduino−esp32fs−plugin */
#define FORMAT_SPIFFS_IF_FAILED false

// ===========================================================
//                 LIGHTNING SENSOR SETTINGS
// ===========================================================

#define AS3935_ADDR 0x00
// I2C Address
//  0x03 is default, but the address can also be 0x02, or 0x01.
//  Adjust the address jumpers on the underside of the product.

#define LOCATION 0xE
// location of the sensor (sensitivity depends of that)
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
