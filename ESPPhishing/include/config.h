// Filesystem SPIFFS or LittleFS. SPIFFS is deprecated.
#define FILESYSTEM LittleFS

// If True, enables serial output.
#define DEBUG false
#define DEBUG_SERIAL \
  if (DEBUG) Serial

// Sleep time of deepSleep
#define SLEEPTIMEuS 1800e6
#define SLEEPTIME SLEEPTIMEuS/1000000
// Average time drift when coming out of sleep mode
#define TIMEDRIFT 60

#define TIMER_INTERVAL_MS 1000

// 28/11/1996 - Noon
// The author's birthday ;)
// UTC
#define RTC_START_UNIXTIME 849182400

// DeepSleep between 22h and 8h
#define SLEEP_MAX_HOUR 20
#define SLEEP_MIN_HOUR 8

#define CUTOFF_VOLTAGE 3200

#define LOCALE "pt-BR"

/* PINS */

#define ADC_IN A0
#define TEMP_SENSOR D5
#define VD_PANEL D1
#define VD_BATTERY D7

/* Inverted logic */
#define LED_ON LOW
#define LED_OFF HIGH
