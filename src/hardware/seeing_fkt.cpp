#include <Arduino.h>
#include <settings.h>
#include <vector>
#include <hardware\display_and_pins.h>

#define SKYCLEAR 1
#define SKYPCLOUDY 2
#define SKYCLOUDY 3
#define SKYUNKNOWN 0

HardwareSerial SerialPort(2); // use UART2

  // object temp is IR temp of sky which at night time will be a lot less than ambient temp
  // so TempDiff is basically ambient + abs(object)
  // setpoint 1 is set for clear skies
  // setpoint 2 is set for cloudy skies
  // setpoint2 should be lower than setpoint1
  // For clear, Object will be very low, so TempDiff is largest
  // For cloudy, Object is closer to ambient, so TempDiff will be lowest

  // Readings are only valid at night when dark and sensor is pointed to sky
  // During the day readings are meaningless

  
// calculate if cloudy/clear sky
int get_cloud_state(float ambient, float object, double SP1, double SP2)
{
  float TempDiff = ambient - object;
  int CLOUD_STATE = SKYUNKNOWN;
  //determine if clear or cloudy
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
  return CLOUD_STATE;
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

  SerialPort.end();

  // Check if the response is "ok"
  return response == "ok";
}

// get Seeing value over UART
bool UART_get_Seeing(String &seeing)
{
  // initialize the serial port with a baud rate of 9600
  SerialPort.begin(9600, 134217756U, MYPORT_RX, MYPORT_TX, false);
  // Send the "get" command
  SerialPort.println("get");
  String teststr = "";
  teststr=SerialPort.readString(); // read until timeout
  // trim the string
  teststr.trim();
  SerialPort.end();
  if(teststr.length() > 2)
  {
    // if string is not empty, set seeing
    seeing = teststr;
    return true;
  }
  // check if string is empty
  return false;
}

// check if sky quality was good for long enough
void check_seeing_threshhold(int seeing_thr, int &GOOD_SKY_STATE_COUNT, int &BAD_SKY_STATE_COUNT, std::vector<bool> &lastSeeingChecks, int CLOUD_STATE, float lux, double MAX_LUX, bool &SEEING_ENABLED, int SLEEPTIME_s)
{
  // check if sensor values are good and if seeing should be enabled
  // good sky state
  if ((CLOUD_STATE == SKYCLEAR && lux < MAX_LUX))
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
