#define FASTLED_ALLOW_INTERRUPTS 0
#define FASTLED_INTERNAL

#include <Arduino.h>
#include <EEPROM.h>
#include <LittleFS.h> 
#include <OneButton.h> 
#include <SSD1306Wire.h>
#include <SparkFunTMP102.h>
#include <FastLED.h>
#include <TimeLib.h>
#include <Timezone.h>
#include <WiFiUdp.h>
#include <WiFiManager.h>
#include <ArduinoOTA.h> 
#include <Ticker.h>
#include <GeoIP.h>
#include "version.h"

#define BUILT_IN_LED (GPIO_NUM_2)            // Sync indicator - blue built-in LED 
#define SPARE_PIN (GPIO_NUM_4)        // Blue led activity (high or low to light)
#define DATA_PIN (GPIO_NUM_13)               // LED data 
#define DATE_TEMP_BUTTON (GPIO_NUM_18)       // Active low, toggles between auto and manual temperature and date display
#define MENU_INCREMENT_BUTTON (GPIO_NUM_19)  // Active low, Menu +
#define SDA_PIN (GPIO_NUM_21)                // Default I2C SDA pin
#define SCL_PIN (GPIO_NUM_22)                // Default I2C SCL pin
#define MENU_DECREMENT_BUTTON (GPIO_NUM_23)  // Actve low, Menu -
#define DISP_TYPE_ONES (GPIO_NUM_25)         // Display type, one's bit
#define DISP_TYPE_TWOS (GPIO_NUM_26)         // Display type, two's bit
#define OLED_FLIP (GPIO_NUM_27)              // Connect to ground to flip the text if OLED is mounted with pins at bottom
#define ADC_PIN (GPIO_NUM_34)                // Manual Color potentiometer
#define EEPROM_CHECK 34987                   // A number written to final location in EEPROM, if it's there it shows that the EEPROM is configured properly 
#define SYNC_INTERVAL 300                    // Time server sync interval in seconds
#define BREATHE_LEVEL_CHANGE 5               // Level change for each step in the breathing colons sequence, 5 to 10 seems to work
#define DATE_START 36                        // Seconds when auto date display starts
#define DATE_END 39                          // Seconds when auto date display ends, must be > start time
#define TEMP_START 6                         // Seconds when temperature display starts
#define TEMP_END 9                           // Seconds when temperature display ends, must be > start time
#define SPREAD 24                            // Hue difference for rainbow pairs mode

// HSV hues
#define ORANGE 15
#define YELLOW 60                   
#define GREEN 90  
#define CYAN 130
#define BLUE 160                      
#define MAGENTA 184
#define RED 254
#define DEFAULT_HUE 16              // Nixie orange

// Global variables - Yes, I know global variables should be avoided but several functions 
// need to access or modify these values and this is the most straight-forward way to do it.

const char     *timeServerPath = "/timeServer.txt";     // path to SPIFFS file that stores the URL or IP address for the time server
char           NTPtimeServer[30]; 
char           displayName[21]; 
char           cVersion[12]; 
const uint8_t  picMax = 34;                      // the number of highest menu item                    
uint8_t        pic = picMax + 1;                     
uint8_t        displayType;                      // 0 - Lixie II, 1 - EleksTube, 2 - Lixie, 3 - NixieCron  
uint8_t        totalLEDs;                   
uint8_t        numModules;
uint8_t        ledsPerModule;
uint8_t        panelOrder[10];
uint8_t        panelOrder2[10];                  // second LED for digits on Lixie II displays
uint8_t        hue = DEFAULT_HUE;
uint8_t        colonHue;
uint8_t        count;
uint16_t       iVersion;
uint16_t       breatheStepTime;
uint32_t       lastMillis = 0;
uint32_t       ledOnMillis;
uint32_t       maxTime = 15000;                  // number of milliseconds before display automatically reverts from menu to time/temperature/date
bool           showDate = false;                 // true to show the date continuously, false for auto 
bool           showTemp = false;                 // true to show the temperature continuously, false for auto
bool           tempDisplay;                      // flag to indicate temperature is being displayed
bool           dataChanged = false;              // flag to indicate menu setting was changed
bool           tempSensorFlag;                   // flag to indicate temp sensor is present and was initialized
bool           WiFiChanged = false;              // flag to indicate WiFi credentials were changed
bool           lightEleven = false;              // light eleventh panel on Lixie II displays
CRGB           ledArray[134];
CRGB           nixieIon = CRGB(212, 43, 2);     
CRGB           nixieAura = CRGB(70, 36, 140);
CRGB           scaledNixieAura;
time_t         previous = 0;
time_t         local;
time_t         lastFix;
location_t     loc;                              // struct to hold information returned from GeoIP 
float          temperature;
IPAddress      ntpServerIP;
uint16_t       localPort = 1023;                 // local port to listen for UDP packets

struct menu
{
  char name[21];
  char settingName[13][18];
  uint16_t value;
  uint16_t lowLimit;
  uint16_t highLimit;
  uint16_t increment;   
} menuItem[picMax + 1];

enum menuItemNumber 
{
  TZinfo,         // must be the first item, put others in the order that they will be displayed
  hourFormat,
  blanking,
  digits,
  colonMode,
  colonMinimumBrightness,
  colonMaximumBrightness, 
  autoTZ,
  manualOffsetHour,
  manualOffsetMinute,
  manualOffsetPolarity,  
  dateFormat,
  autoDate,
  tempUnits,
  NixieMode,
  NixieAura,
  randomMode,  
  autoColor,
  colorVariation,  
  dayBrightness,
  nightBrigtness,
  nightBeginHour,
  nightEndHour,  
  autoDST,
  DSTstartWeek,
  DSTstartDay,
  DSTstartMonth,
  DSTstartHour,
  DSTendWeek,
  DSTendDay,
  DSTendMonth,
  DSTendHour,
  serialMonitorDetail, 
  syncLED,
  networkInfo                // must be the last item
};

TimeChangeRule *tcr;
TimeChangeRule dstRule; // = {"DST", Second, Sun, Mar, 2, 60};  DST dates get set from the menu and UTC offset gets 
TimeChangeRule stdRule; // = {"STD", First, Sun, Nov, 2, 0};     set from ipapi.co or manual offset from the menu


// Objects

OneButton menuIncrementButton(MENU_INCREMENT_BUTTON, true, true);      // active low with pullup
OneButton menuDecrementButton(MENU_DECREMENT_BUTTON, true, true);      
OneButton dateTempButton(DATE_TEMP_BUTTON, true, true);
SSD1306Wire display(0x3c, SDA, SCL); 
TMP102 tempSensor; 
GeoIP geoip; 
Timezone tz(dstRule, stdRule);
Ticker breathe;
WiFiUDP Udp;

// Function declarations

void connectWiFi();
void setOTA();
void configModeCallback(WiFiManager *myWiFiManager); 
void menuIncrementClick();
void menuIncrementDoubleclick(); 
void menuIncrementLongPressStart();
void menuIncrementLongPressStop();
void menuDecrementClick();
void menuDecrementDoubleclick();
void menuDecrementLongPressStart();
void menuDecrementLongPressStop();
void dateTempClick();
void dateTempLongPressStart();
void dateTempDoubleclick();
void incrementSetting();
void decrementSetting();
void setManualOffset();
void readEEPROM();
void initializeEEPROM();
void updateEEPROM();
time_t setNtpTime();
time_t getNtpTime();
void sendNTPpacket(IPAddress &address);
void setTimeZone();
void digitModuleWrite(char *ASCIIbuffer, uint8_t hsvHue, bool rainbow);
void digitModuleWrite(char *ASCIIbuffer);
void digitModuleWrite(char *ASCIIbuffer, CRGB rgbValue);
void displayDigitalData();
void serialMonitor();
void randomDigits();
bool nightMode(uint16_t start, uint16_t end);
void header();
void footer();
void showInfo();
void showNetInfo();
void menuHeader();
void menuFooter();
void showMenuItem();
void displayMenuItem(); 
void IRAM_ATTR sequenceBreathing();
void displayDots();
void clearDots();
void setDigitModuleType();
void menuSettings();
void setColonLevels();
void testDisplay(uint8_t hsvHue);
bool writeFile(fs::FS &fs, const char *path, const char *message);
bool readFile(fs::FS &fs, char *fileContent, size_t fileSize, const char *path);
void setDSTRules();
void setManualTimeZone();
void displayVersion();