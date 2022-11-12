#include <Arduino.h>
#include <settings.h>

// Permanently stored values
RTC_DATA_ATTR String WIFI_SSID = FALLBACK_WIFI_SSID;
RTC_DATA_ATTR String WIFI_PASS = FALLBACK_WIFI_PASS;
RTC_DATA_ATTR String SERVER_IP = FALLBACK_WIFI_PASS;
RTC_DATA_ATTR String SEND_SERVER = fallback_sendserverName;
RTC_DATA_ATTR String FETCH_SERVER = fallback_fetchserverName;

// RTC_DATA_ATTR values dont get reset after deepsleep
RTC_DATA_ATTR int noWifiCount = 0;
RTC_DATA_ATTR int sendCount = 0;
RTC_DATA_ATTR int serverErrorCount = 0;
RTC_DATA_ATTR bool settingsLoadCount = 0;

RTC_DATA_ATTR bool hasInitialized = false;
RTC_DATA_ATTR bool settingsLoaded = false;
RTC_DATA_ATTR bool hasWIFI = false;
RTC_DATA_ATTR bool hasServerError = false;

// settings that might get fetched from server
RTC_DATA_ATTR int SLEEPTIME_s = FALLBACK_SLEEPTIME_s;
RTC_DATA_ATTR int NO_WIFI_MAX_RETRIES = FALLBACK_NO_WIFI_MAX_RETRIES;
RTC_DATA_ATTR int DISPLAY_TIMEOUT_s = FALLBACK_DISPLAY_TIMEOUT_s;
RTC_DATA_ATTR bool DISPLAY_ON = FALLBACK_DISPLAY_ON;
RTC_DATA_ATTR bool ALWAYS_FETCH_SETTINGS = FALLBACK_ALWAYS_FETCH_SETTINGS;
RTC_DATA_ATTR double SQM_LIMIT = FALLBACK_SQM_LIMIT;
RTC_DATA_ATTR bool SEEING_ENABLED = false;

RTC_DATA_ATTR double SP1 = FALLBACK_SP1;
RTC_DATA_ATTR double SP2 = FALLBACK_SP2;
RTC_DATA_ATTR double MAX_LUX = FALLBACK_MAXLUX;

// Sky state indicators
RTC_DATA_ATTR int CLOUD_STATE = -1;
RTC_DATA_ATTR int BAD_SKY_STATE_COUNT = 0;
RTC_DATA_ATTR int GOOD_SKY_STATE_COUNT = 0;

#define SKYCLEAR 1
#define SKYPCLOUDY 2
#define SKYCLOUDY 3
#define SKYUNKNOWN 0

// sensor values
bool raining = false;
float ambient, object = -1; //TQ, HT
double lux = -1; // Resulting lux value
int lightning_distanceToStorm = -1;
float luminosity = -1; // the SQM value, sky magnitude
String seeing = "-1"; 
double nelm = -1; //NE
int concentration = -1;

// Search for parameter in HTTP POST request
const char *PARAM_INPUT_1 = "ssid";
const char *PARAM_INPUT_2 = "pass";
const char *PARAM_INPUT_3 = "ip";


