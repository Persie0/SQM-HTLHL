#include <Arduino.h>
#include "SSD1306Wire.h"
#include "settings.h"
#include <vector>

// for 128x64 displays:
static SSD1306Wire display(0x3c, SDA, SCL); // ADDRESS, SDA, SCL

// Display the current status message on the display
void DisplayStatusMessage(bool hasWIFI, bool hasServerError, bool settingsLoaded, int sendCount, int noWifiCount, bool sleepForever, bool DISPLAY_ON)
{
  //when display is off, do nothing
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
  if (sleepForever)
  {
    display.drawStringMaxWidth(0, 0, 128, "WIFI Dead -> AP");
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
