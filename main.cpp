
#include "definitions.h"

/********************************************************************************************************************************************
* Code to run once at beginning
********************************************************************************************************************************************/

void setup() 
{
  Serial.begin(115200);
  delay(500);  

  if (!LittleFS.begin(true))
  {
    Serial.println("An error has occurred while mounting LittleFS");
    return;
  }

  EEPROM.begin((picMax + 2) * sizeof(uint16_t));     // 2 extra EEPROM locations for Menu Item 0 and the check value at the end
  delay(100);

  menuIncrementButton.attachClick(menuIncrementClick);
  menuIncrementButton.attachDoubleClick(menuIncrementDoubleclick);
  menuIncrementButton.attachLongPressStop(menuIncrementLongPressStop);
  menuIncrementButton.attachLongPressStart(menuIncrementLongPressStart);
  menuIncrementButton.setDebounceMs(50);
  menuIncrementButton.setClickMs(400);                   // single click time
  menuIncrementButton.setPressMs(800);                   // double click time

  menuDecrementButton.attachClick(menuDecrementClick);
  menuDecrementButton.attachDoubleClick(menuDecrementDoubleclick);
  menuDecrementButton.attachLongPressStop(menuDecrementLongPressStop);
  menuDecrementButton.attachLongPressStart(menuDecrementLongPressStart);
  menuDecrementButton.setDebounceMs(50);
  menuDecrementButton.setClickMs(400);                  
  menuDecrementButton.setPressMs(800);

  dateTempButton.attachClick(dateTempClick);
  dateTempButton.attachLongPressStart(dateTempLongPressStart);
  dateTempButton.attachDoubleClick(dateTempDoubleclick);
  dateTempButton.setDebounceMs(50);
  dateTempButton.setClickMs(400);
  dateTempButton.setPressMs(800); 

  // Pin settings - data for display modules is on Pin 4
  // pinModes for Menu buttons and Temperature/Date button were set when their objects were created

  pinMode(BUILT_IN_LED, OUTPUT);
  digitalWrite(BUILT_IN_LED, !LED_ON);   // start with the sync LED off  
  pinMode(DISP_TYPE_ONES, INPUT_PULLUP);
  pinMode(DISP_TYPE_TWOS, INPUT_PULLUP);
  pinMode(OLED_FLIP, INPUT_PULLUP);

  //Wire.begin(SDA_PIN, SCL_PIN);  we are using the default I2C pins so no need to call the method             
  //delay(100);

  display.init();
  delay(100);
  display.setBrightness(255);
  if (digitalRead(OLED_FLIP) == HIGH) display.flipScreenVertically(); 

  setDigitModuleType();       

  tempSensorFlag = tempSensor.begin(); 
  Serial.print("Temp sensor init: ");  Serial.println(tempSensorFlag);

  if (tempSensorFlag)
  {
    // Initialize TMP102 settings  
    // Alert is not used here, the alert settings are specified anyway  
    tempSensor.setFault(0);           // trigger alarm immediately  
    tempSensor.setAlertPolarity(0);   // active LOW 
    tempSensor.setAlertMode(0);       // comparator mode  
    tempSensor.setConversionRate(2);  // 4Hz  
    tempSensor.setExtendedMode(0);    // 12-bit Temperature (-55C to +128C)
    tempSensor.setHighTempC(22.5);    // high limit  - Alert is HIGH if temp is below low limit and LOW if temp is above high limit                                    
    tempSensor.setLowTempC(20.5);     // low limit
  }

  // Leading zeros are supressed so VERSION_MAJOR shouldn't be zero
  sprintf(cVersion, "%i%i%i", VERSION_MAJOR, VERSION_MINOR, VERSION_BUILD);
  iVersion = strtol(cVersion, NULL, 10);                                          // integer
  sprintf(cVersion, "%i.%i.%i", VERSION_MAJOR, VERSION_MINOR, VERSION_BUILD);     // char array with dots 

  menuSettings();                   // set default values into menu struct 
  
  // Initialize EEPROM if the final EEPROM location doesn't contain the expected check value
  // OR restore default settings to EEPROM if Menu Increment switch is held down at start of booting
  // Release switch during or after version is displayed and before boot sequence finishes  
  uint16_t check;
  uint8_t address = (picMax + 1) * sizeof(uint16_t);
  EEPROM.get(address, check);
  if (digitalRead(MENU_INCREMENT_BUTTON) == LOW || check != EEPROM_CHECK) initializeEEPROM();
  //initializeEEPROM();

  uint16_t storedVersion;
  EEPROM.get(0, storedVersion);
  if (storedVersion != iVersion)     // update EEPROM in case the value read doesn't match current version number
  {  
    EEPROM.put(0, iVersion);                                                  
    EEPROM.commit();
  }

  // Display version on serial monitor    
  Serial.printf("\n\nNTP EdgeLit Clock v%s\n", cVersion);  
  
  readEEPROM();                    // now read stored settings into menu struct 

  setDSTRules();

  // Load values saved in SPIFFS
  
  bool res = readFile(LittleFS, NTPtimeServer, sizeof(NTPtimeServer), timeServerPath);
  Serial.printf("NTP server name read from file: %s\r\n", NTPtimeServer);

  if (!res)         // file not found so create it with default value
  {
    strncpy(NTPtimeServer, "pool.ntp.org", 13); 
    writeFile(LittleFS, timeServerPath, NTPtimeServer);
  }  
  //Serial.printf("Time server string: %s\r\n", NTPtimeServer);  

  FastLED.addLeds<WS2812B, DATA_PIN, GRB>(ledArray, totalLEDs);
  delay(100);
  
  uint8_t LEDaddress;
  for (LEDaddress = 0; LEDaddress < numModules * ledsPerModule; LEDaddress++) ledArray[LEDaddress] = CRGB(0, 0, 0); 
  FastLED.show();

  char buffer[7];                             // buffer for digit module output
  
  // Display version on OLED 
  display.clear();
  header();         
  display.display();

  // Display version on digit modules  
  snprintf(buffer, 7, "%i", iVersion);
  digitModuleWrite(buffer, CRGB::SlateGrey);   

  delay(2000);

  // Connect to WiFi network

  // Display orange 1 
  snprintf(buffer, 7, "%.1i-----", 1);                          
  digitModuleWrite(buffer, ORANGE, false);

  Serial.print("MAC address: ");  Serial.println(WiFi.macAddress()); 
  Serial.println("Connecting to WiFi ...");

  display.clear();
  header();  
  display.drawString(10, 29, "Connecting to");
  display.drawString(10, 38, "WiFi ..."); 
  display.display();      

  delay(1500);
  connectWiFi();

  // If we get here, control has been returned from WiFiManager 
  // Connect to time server and set UTC time

  // Display magenta 2                                            
  snprintf(buffer, 7, "-%.1i----", 2);                             
  digitModuleWrite(buffer, MAGENTA, false);
  
  display.clear();
  header();  
  display.drawString(10, 29, "Connected to");
  display.drawString(10, 38, WiFi.SSID());
  display.drawString(10, 45, "Setting time ..."); 
  display.display(); 

  delay(1500);  

  Serial.println("Syncing with NTP server ...");

  // Get UTC time from NTP server               
  Udp.begin(localPort);
  //setSyncProvider(getNtpTime);        // the usual function used
  setSyncProvider(setNtpTime);        // this one works better for GPS time server, also works for Internet servers
  setSyncInterval(SYNC_INTERVAL);

  if (timeStatus() != timeNotSet) 
  {
    // Display cyan 3 when time is set 
    snprintf(buffer, 7, "--%.1i---", 3);              
    digitModuleWrite(buffer, CYAN, false);    
    
    display.clear();
    header();
    display.drawString(10, 29, "UTC time set.");
    display.drawString(10, 38, "Setting");
    display.drawString(10, 47, "time zone ...");
    display.display();
  }  
  else
  {
    // Display red 3 when time is not set 
    snprintf(buffer, 7, "--%.1i---", 3);              
    digitModuleWrite(buffer, RED, false);
    digitalWrite(BUILT_IN_LED, !LED_ON);
    Serial.println("UTC time not set.");

    display.clear();
    header();
    display.drawString(10, 29, "UTC time not set.");
    display.drawString(10, 38, "Setting");
    display.drawString(10, 47, "time zone ...");
    display.display();
  }

  Serial.println("Setting time zone ...");  

  delay(1500);

  // Set time zone. These functions also show the 4 and 5 on the edge-lit displays
  // Auto TZ must be done after WiFi is connected

  if (menuItem[autoTZ].value == 0) setManualTimeZone();  // TZ set from menu items 
  else setTimeZone();                                    // Auto TZ with manual backup if needed

  display.clear();
  header();
  display.drawString(10, 29, "Time Zone set.");
  display.display();  

  showInfo();  
  delay(1500);

  setOTA();
  Serial.println("OTA ready");  
  display.clear();
  display.setTextAlignment(TEXT_ALIGN_CENTER);  
  display.drawStringMaxWidth(64, 10, 128, "OTA ready");  
  display.display(); 
  delay(1000); 

  FastLED.clear();

  if (tempSensorFlag)
  {
    tempSensor.wakeup();
    if (menuItem[tempUnits].value == 1) temperature = tempSensor.readTempC();
    else temperature = tempSensor.readTempF();
    tempSensor.sleep();
  }

  breatheStepTime = 900 / ((menuItem[colonMaximumBrightness].value - menuItem[colonMinimumBrightness].value) / BREATHE_LEVEL_CHANGE * 2);

  if (menuItem[autoDST].value == 1 )                     // Auto DST enabled
  {
    int16_t stdOffsetMinutes = loc.offsetSeconds / 60;      
    dstRule.offset = stdOffsetMinutes + 60;
    stdRule.offset = stdOffsetMinutes;  
    tz.setRules(dstRule, stdRule);
    local = tz.toLocal(now(), &tcr);
  } 
  else local = now() + loc.offsetSeconds;

  Serial.println("Setup done!"); 
  serialMonitor();
}

/*******************************************************************************************************************************************
* Main loop
********************************************************************************************************************************************/

void loop()
{
  static uint32_t colonOnTime;
  uint32_t currentMillis = millis();
  static uint32_t previousMillis;

  // If WiFi is down, try reconnecting every 30 seconds
  if ((WiFi.status() != WL_CONNECTED) && (currentMillis - previousMillis >= 30000)) 
  {
    if (menuItem[serialMonitorDetail].value == 1) Serial.println("Reconnecting to WiFi...");
    WiFi.disconnect();
    WiFi.reconnect();
    previousMillis = currentMillis;
  } 

  // Turn off sync LED if it's 10 seconds before next sync
  if (currentMillis - ledOnMillis > (SYNC_INTERVAL - 10) * 1000) digitalWrite(BUILT_IN_LED, !LED_ON); 
       
  // Check the switches
  menuIncrementButton.tick();
  menuDecrementButton.tick();
  dateTempButton.tick();

  scaledNixieAura.r = nixieAura.r * (menuItem[NixieAura].value / 255.0);       
  scaledNixieAura.g = nixieAura.g * (menuItem[NixieAura].value / 255.0); 
  scaledNixieAura.b = nixieAura.b * (menuItem[NixieAura].value / 255.0);
  
  // reset pic if no clicks for 15 seconds 
  if (currentMillis - lastMillis >= maxTime) pic = picMax + 1;     

  if (pic > picMax)         // normal time/date/temperature display, if pic <= picMax menu items are displayed by menu button functions
  {
    if (dataChanged)        // see if any settings have changed if we got here by quitting the menu (double click or timeout)
    {
      updateEEPROM();       
      serialMonitor(); 
      dataChanged = false;
    } 

    display.clear();
    display.display();

    if (nightMode(menuItem[nightBeginHour].value, menuItem[nightEndHour].value)) FastLED.setBrightness(menuItem[nightBrigtness].value);   // night brightness
    else FastLED.setBrightness(menuItem[dayBrightness].value);     

    if (menuItem[colonMode].value == 0)   // colons off
    {
      breathe.detach();            // make sure breathing sequence isn't still running 
      clearDots();                 
    } 

    if (menuItem[colonMode].value == 1)   // colons steady
    {
      breathe.detach();
      displayDots();               // looks after 6/4 and temperature display too
    }

    //if (timeStatus() != timeNotSet)  // comment out so clock starts even if time is not set
    //{
      if (now() != previous)       // update display only if the time has changed  
      {
        previous = now();  
        displayDigitalData();  

        if (menuItem[colonMode].value == 2) breathe.attach_ms(breatheStepTime, sequenceBreathing);       // start the sequence if colon mode is breathing      
        if (menuItem[colonMode].value == 3)    // colons flashing
        {
          breathe.detach();
          colonOnTime = currentMillis;
          displayDots();        
        }                            

        // Set the hue for next second if auto color is enabled and Nixie mode is disabled
        // hue is an unsigned 8-bit variable, when it overflows at 256 it just goes back to 0
        if (menuItem[autoColor].value == 0) hue = analogRead(ADC_PIN) / 16;      // get hue in 0 to 255 range, analog input comes in 0 to 4095
        if (menuItem[autoColor].value == 1 && menuItem[NixieMode].value == 0) hue++;

        if (menuItem[serialMonitorDetail].value == 1 && (second() % 5) == 0) serialMonitor();
        //serialMonitor();
      }

      // If colons mode is flashing see if it's time to turn them off
      else if (menuItem[colonMode].value == 3 && (currentMillis - colonOnTime > 400)) clearDots(); 
    //}   // comment out so clock starts even if time is not set
  }
  else FastLED.setBrightness(255);                     // a menu item is displayed so set brightness to maximum

  ArduinoOTA.handle();   
  delay(1);       
}

/*******************************************************************************************************************************************
* Functions to send data to digit module
********************************************************************************************************************************************/

/* Multicolor version
*  Parameters: 
*  First  - Six-element ASCII character array with the numbers to be displayed in order from left to right.
*           If a module is to be blank, enter any non-numeric character in the appropriate position in the array
*  Second - Unsigned integer from 0 to 255 for the HSV hue
*  Third  - Bool, false for all the same color across the display, true for rainbow pairs
*/

void digitModuleWrite(char *ASCIIbuffer, uint8_t hsvHue, bool rainbow)
{ 
  uint8_t address; 

  // Clear the array so panels previously turned on won't show  
  for (address = 0; address < numModules * ledsPerModule; address++) ledArray[address] = CRGB(0, 0, 0);

  // Go through digit modules and character array, one element at a time, 
  // until end of array is reached or all digit modules have been addressed, whichever comes first

  uint8_t i = 0;  
  while (ASCIIbuffer[i] != '\0' && i < numModules)
  {
    if (ASCIIbuffer[i] > 47 && ASCIIbuffer[i] < 58)  // skip this digit module if ASCII character is not numeric                                   
    {
      uint8_t number = ASCIIbuffer[i] - 48;          // subtract 48 to get numeric value from ASCII character
            
      // Addresses in panelOrder array are for 0 to 9 on first digit module

      switch (displayType)
      {
      case 3:                                          // NixieCron           
        address = i * ledsPerModule + panelOrder[number];           
        ledArray[address] = CHSV(hsvHue, 255, 255);
        break;
      case 2:                                          // Lixie
        address = (numModules - i - 1) * ledsPerModule + panelOrder[number];      // the modules are addressed right to left      
        ledArray[address] = CHSV(hsvHue, 255, 255);
        ledArray[address + 10] = CHSV(hsvHue, 255, 255);          
        break;
      case 1:                                          // EleksTube   
        address = i * ledsPerModule + panelOrder[number];           
        ledArray[address] = CHSV(hsvHue, 255, 255);
        ledArray[address + 10] = CHSV(hsvHue, 255, 255); 
        break;
      case 0:                                          // Lixie II
        address = (numModules - i - 1) * ledsPerModule + panelOrder[number];
        ledArray[address] = CHSV(hsvHue, 255, 255);
        uint8_t address2 = (numModules - i - 1) * ledsPerModule + panelOrder2[number];       
        ledArray[address2] = CHSV(hsvHue, 255, 255);

        uint8_t addressEleven = (numModules - i - 1) * ledsPerModule + 4; 
        uint8_t addressEleven2 = addressEleven + 13;

        if (lightEleven)
        {
          ledArray[addressEleven] = CHSV(hsvHue, 255, 255);
          ledArray[addressEleven2] = CHSV(hsvHue, 255, 255);
        }        
        break;
      }   
      if (i == numModules / 2) colonHue  = hsvHue;        // middle value in case mode is rainbow pairs          
      if (rainbow && (i % 2 == 1)) hsvHue += SPREAD;      // change color every second digit module             
    }
    i++;
  } 
                                  
  FastLED.show();                                         // send the array to the hardware 
}

/* Nixie version
*  Parameter: 
*  Six-element ASCII character array with the numbers to be displayed in order from left to right.
*  If a module is to be blank, enter any non-numeric character in the appropriate position in the array
*/

void digitModuleWrite(char *ASCIIbuffer)
{    
  uint8_t address;  
  
  // Clear the array so panels turned on previously won't show and set Nixie aura on all LEDs
  for (address = 0; address < numModules * ledsPerModule; address++) ledArray[address] = scaledNixieAura; 

  uint8_t i = 0;
  while (ASCIIbuffer[i] != '\0' && i < numModules)
  {
    if (ASCIIbuffer[i] > 47 && ASCIIbuffer[i] < 58)
    {
      uint8_t number = ASCIIbuffer[i] - 48;
      
      switch (displayType)
      {
      case 3:                                          // NixieCron                
        address = i * ledsPerModule + panelOrder[number];          
        ledArray[address] = nixieIon;
        break;
      case 2:                                          // Lixie   
        address = (numModules - i - 1) * ledsPerModule + panelOrder[number];          
        ledArray[address] = nixieIon;
        ledArray[address + 10] = nixieIon; 
        break;
      case 1:                                         // EleksTube  
        address = i * ledsPerModule + panelOrder[number]; 
        ledArray[address] = nixieIon;         
        ledArray[address + 10] = nixieIon;
        break;
      case 0:                                         // Lixie II
        address = (numModules - i - 1) * ledsPerModule + panelOrder[number];
        ledArray[address] = nixieIon;
        uint8_t address2 = (numModules - i - 1) * ledsPerModule + panelOrder2[number];       
        ledArray[address2] = nixieIon;

        uint8_t addressEleven = (numModules - i - 1) * ledsPerModule + 4; 
        uint8_t addressEleven2 = addressEleven + 13;
        
        if (lightEleven)
        {
          ledArray[addressEleven] = nixieIon;
          ledArray[addressEleven2] = nixieIon;
        }
        break;        
      } 
    }
    i++;
  }                                
  FastLED.show();
}    

/* RGB version
*  Parameters: 
*  First  - Six-element ASCII character array with the numbers to be displayed in order from left to right.
*           If a module is to be blank, enter any non-numeric character in the appropriate position in the array
*  Second - CRGB value to specify the color
*/

void digitModuleWrite(char *ASCIIbuffer, CRGB rgbValue)
{    
  uint8_t address;

  for (address = 0; address < numModules * ledsPerModule; address++) ledArray[address] = CRGB(0, 0, 0); 

  uint8_t i = 0;
  while (ASCIIbuffer[i] != '\0' && i < numModules)
  {
    if (ASCIIbuffer[i] > 47 && ASCIIbuffer[i] < 58)
    {
      uint8_t number = ASCIIbuffer[i] - 48;                                              
      uint8_t panel = panelOrder[number];

     switch (displayType)
     {
      case 3:                                          // NixieCron                
        address = i * ledsPerModule + panel;          
        ledArray[address] = rgbValue;
        break; 
      case 2:                                          // Lixie  
        address = (numModules - i - 1) * ledsPerModule + panel;
        ledArray[address] = rgbValue;
        ledArray[address + 10] = rgbValue;   
        break;
      case 1:                                          // EleksTube             
        address = i * ledsPerModule + panel;          
        ledArray[address] = rgbValue;
        ledArray[address + 10] = rgbValue;
        break; 
      case 0:                                          // Lixie II         
        address = (numModules - i - 1) * ledsPerModule + panelOrder[number];
        ledArray[address] = rgbValue;
        uint8_t address2 = (numModules - i - 1) * ledsPerModule + panelOrder2[number];       
        ledArray[address2] = rgbValue;

        uint8_t addressEleven = (numModules - i - 1) * ledsPerModule + 4; 
        uint8_t addressEleven2 = addressEleven + 13;

        if (lightEleven)
        {
          ledArray[addressEleven] = rgbValue;
          ledArray[addressEleven2] = rgbValue;
        }
        break;        
      } 
    }
    i++;
  }                                  
  FastLED.show();
} 

/*******************************************************************************************************************************************
* Configure OTA
********************************************************************************************************************************************/

void setOTA()
{
  ArduinoOTA.setPassword("74177");
  ArduinoOTA.onStart([]() {
    Serial.println("OTA Start");
  });
  ArduinoOTA.onEnd([]() {
    Serial.println("\nOTA End");
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progress: %u%%\r\n", (progress / (total / 100)));
  });
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("OTA Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
    else if (error == OTA_BEGIN_ERROR)   Serial.println("Begin Failed");
    else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
    else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
    else if (error == OTA_END_ERROR)     Serial.println("End Failed");
  });
  ArduinoOTA.begin();
}

/*******************************************************************************************************************************************
* Connect using WiFiManager
********************************************************************************************************************************************/

void connectWiFi()
{
  WiFi.mode(WIFI_STA); // explicitly set mode, esp defaults to STA+AP 

  // The extra parameters to be configured (id/name, placeholder/prompt, default, length)
  WiFiManagerParameter NTPserver("ntp_server", "NTP Server", "pool.ntp.org", 29);
     
  WiFiManager wm;

  if (digitalRead(MENU_DECREMENT_BUTTON) == LOW) 
  {
    wm.resetSettings();  
    WiFiChanged = true;
  } 
  wm.setDebugOutput(false); 
  wm.addParameter(&NTPserver);

  wm.setConfigPortalTimeout(180);
  wm.setAPCallback(configModeCallback); 
  wm.setTimeout(180); 
  
  // Automatically connect using saved credentials.
  // If connection fails an access point named "EdgeLit" starts where WiFi network credentials can de entered.
  // It then goes into a blocking loop awaiting user input and will return on successful connection.

  bool res;
  // If name is not specified it will be auto generated from chipid
  // If password is not specified the AP will be open and no password will be required to log in.
  // res = wm.autoConnect("name", "password");       
  res = wm.autoConnect("EdgeLit");                                                              

  if (!res) 
  {
    Serial.println("Failed to connect and hit timeout");
    delay(3000);
    ESP.restart();
  } 
  else 
  {
    // If you get here you have connected to the WiFi
       
    Serial.print("Connected to ");                    Serial.println(WiFi.SSID());
    Serial.print("IP address assigned by DHCP: ");    Serial.println(WiFi.localIP());
    Serial.print("Netmask: ");                        Serial.println(WiFi.subnetMask());
    Serial.print("Gateway: ");                        Serial.println(WiFi.gatewayIP());       
  }

  if (WiFiChanged)
  {
    memset(NTPtimeServer, '\0', 30); 
    strcpy(NTPtimeServer, NTPserver.getValue());
    writeFile(LittleFS, timeServerPath, NTPtimeServer);          
    WiFiChanged = false;
  }  
}

/*******************************************************************************************************************************************
* Function called if WiFiManager is in config mode (AP open)
********************************************************************************************************************************************/

void configModeCallback(WiFiManager *myWiFiManager) 
{ 
  Serial.println("Access Point mode. Digit modules set to red.");   
  Serial.println(myWiFiManager->getConfigPortalSSID());           // display the AP name

  char zero[7];     
  snprintf(zero, 7, "000000");                                    // display red zeros while waiting for user input  
  digitModuleWrite(zero, CRGB::Red);  
}

/*******************************************************************************************************************************************
* Function called when the time changes each second
* Changes the color if desired and displays the time, date, and temperature
********************************************************************************************************************************************/

void displayDigitalData()
{
  char buffer[7];                        // buffer for digit module output  
  uint8_t hueThisTime;
  
  if (tempSensorFlag && second() % 5 == 0 )
  {
    tempSensor.wakeup();
    if (menuItem[tempUnits].value == 1) temperature = tempSensor.readTempC();
    else temperature = tempSensor.readTempF();
    tempSensor.sleep();
  }

  if (menuItem[autoDST].value == 1 )                     // Auto DST enabled
  {
    int16_t stdOffsetMinutes = loc.offsetSeconds / 60;      
    dstRule.offset = stdOffsetMinutes + 60;
    stdRule.offset = stdOffsetMinutes;  
    tz.setRules(dstRule, stdRule);
    local = tz.toLocal(now(), &tcr);
  } 
  else local = now() + loc.offsetSeconds;
  
  //Serial.printf("Local: %.2i:%.2i:%.2i    %li\r\n", hour(local), minute(local), second(), loc.offsetSeconds);
  //Serial.printf("Date : %.2i/%.2i/%.2i    %i\r\n", month(local), day(local), year(local), stdOffsetMinutes);

  if ((minute(local) % 10) == 2 && second() == 15 && menuItem[randomMode].value == 1)     
  {
    breathe.detach();              // make sure colon breathing sequence isn't still running
    randomDigits();                             
  } 
  
  // Display the temperature during specified seconds if auto temperature/date is enabled, or if temperature/date switch was long pushed
  // also stops auto temperature display if showing date, or if TMP sensor is not present 

  if (tempSensorFlag && (!showDate && (((second() >= TEMP_START && second() <= TEMP_END) && menuItem[autoDate].value == 1) || showTemp)))        
  {
    tempDisplay = true;

    // Multiply temperature by 10 then do integer division to separate degrees and tenths    
    int tmp = temperature * 10;   
    uint8_t degree = abs(tmp / 10);     
    uint8_t tenths = abs(tmp % 10);
    if (tmp < 0) 
    {
      // Show a 0 on leftmost display if temperature is below 0
      // TMP102 lower limit is -55 degrees so we don't need three digits      
      snprintf(buffer, 7, "0-%.2u%.1u-", degree, tenths);                             
    }
    else
    {
      // TMP102 upper limit is 128 degrees C
      if (degree > 99) snprintf(buffer, 7, "-%.3u%.1u-", degree, tenths);                           
      else             snprintf(buffer, 7, "--%.2u%.1u-", degree, tenths);     
    }                  
    hueThisTime = hue + 85;                                              // change the color for temperature display
  }

  // Display the date during specified seconds and if auto temperature/date is enabled, or if temperature/date switch was short pushed 

  else if (((second() >= DATE_START && second() <= DATE_END) && menuItem[autoDate].value == 1) || showDate)         
  {
    tempDisplay = false;

    if (menuItem[dateFormat].value == 0)       
    {
      if (menuItem[digits].value == 1)    // six digit mode
      {
        if (menuItem[blanking].value == 1 && month(local) < 10) snprintf(buffer, 7, "-%.1i%.2i%.2i", month(local), day(local), year(local) % 100); 
        else snprintf(buffer, 7, "%.2i%.2i%.2i", month(local), day(local), year(local) % 100);
      }
      else                            // four digit mode
      {
        if (menuItem[blanking].value == 1 && month(local) < 10) snprintf(buffer, 7, "-%.1i%.2i--", month(local), day(local)); 
        else snprintf(buffer, 7, "%.2i%.2i--", month(local), day(local));
      }      
    }       
    else if (menuItem[dateFormat].value == 1)     
    {
      if (menuItem[digits].value == 1)
      {
        if (menuItem[blanking].value == 1 && day(local) < 10) snprintf(buffer, 7, "-%.1i%.2i%.2i", day(local), month(local), year(local) % 100);
        else snprintf(buffer, 7, "%.2i%.2i%.2i", day(local), month(local), year(local) % 100);
      }
      else
      {
        if (menuItem[blanking].value == 1 && day(local) < 10) snprintf(buffer, 7, "-%.i%1.2i--", day(local), month(local));
        else snprintf(buffer, 7, "%.2i%.2i--", day(local), month(local));
      }         
    }  
    else 
    {
      if (menuItem[digits].value == 1) snprintf(buffer, 7, "%.2i%.2i%.2i", year(local) % 100, month(local), day(local)); 
      else 
      {
        if (menuItem[blanking].value == 1 && month(local) < 10) snprintf(buffer, 7, "-%.1i%.2i--", month(local), day(local));
        else  snprintf(buffer, 7, "%.2i%.2i--", month(local), day(local));
      }  
    }            
    hueThisTime = hue + 170;             // change the color of date display                      
  }

  // Display the time

  else                                                                          
  {
    tempDisplay = false;
    uint8_t hr = hour(local);
    if (menuItem[hourFormat].value == 0)  hr = hourFormat12(local);                              // 12-hour mode

    //uint8_t onesH = hr % 10 + 48;     in case you don't want to use snprintf to set the buffer
    //uint8_t tensH = hr / 10 + 48;
    //uint8_t onesM = minute(local) % 10 + 48;
    //uint8_t tensM = minute(local) / 10 + 48;
    //uint8_t onesS = second(local) % 10 + 48;
    //uint8_t tensS = second(local) / 10 + 48;    

    if (menuItem[digits].value == 1)         // six digit mode
    {
      if (menuItem[blanking].value == 1 && hr < 10) snprintf(buffer, 7, "-%.1i%.2i%.2i", hr, minute(local), second());      // blank leading zero
      else snprintf(buffer, 7, "%.2i%.2i%.2i", hr, minute(local), second());    

      //if (menuItem[blanking].value == 1 && hr < 10) buffer[0] = 45;
      //else buffer[0] = tensH;        
      //buffer[1] = onesH; buffer[2] = tensM; buffer[3] = onesM; buffer[4] = tensS; buffer[5] = onesS; buffer[6] = '\0';        
    }
    else                                 // four digit mode
    {
      if (menuItem[blanking].value == 1 && hr < 10) snprintf(buffer, 7, "-%.1i%.2i--", hr, minute(local));      // blank leading zero
      else snprintf(buffer, 7, "%.2i%.2i--", hr, minute(local)); 

      //buffer[0] = 45; buffer[1] = 45;
      //if (menuItem[blanking].value == 1 && hr < 10) buffer[2] = 45;
      //else buffer[2] = tensH;        
      //buffer[3] = onesH; buffer[4] = tensM; buffer[5] = onesM; buffer[6] = '\0'; 
    }
    hueThisTime = hue;                   // don't change the color for time display                    
  }
  
  colonHue = hueThisTime;  
  if (menuItem[NixieMode].value == 1) digitModuleWrite(buffer);                      // call without hue and rainbow for Nixie simulation
  else digitModuleWrite(buffer, hueThisTime, menuItem[colorVariation].value);        // last parameter false for all digits the same color, true for rainbow  
  
  delay(20);
}

/*******************************************************************************************************************************************
 * Serial Monitor Display
********************************************************************************************************************************************/

void serialMonitor()
{   
  char buff[128];

  // Clear the screen (only works with a real terminal emulator like PuTTY)
  //Serial.write(27);                  // ESC command
  //Serial.print("[2J");               // clear screen command 
  //Serial.write(27);
  //Serial.print("[H");                // cursor to home command

  Serial.println("\n");

  snprintf(buff, 16, "UTC: %.2i:%.2i:%.2i  ", hour(), minute(), second());
  Serial.print(buff);

  Serial.print(dayStr(weekday()));   Serial.print(", ");
  Serial.print(monthStr(month()));   Serial.print(" ");
  Serial.print(day());               Serial.print(", ");
  Serial.println(year());   

  Serial.print("Timezone: "); 
  Serial.print(loc.timezone);       Serial.print("  ");
  Serial.print(loc.city);           Serial.print(", "); 
  Serial.print(loc.region);         Serial.print(" "); 
  Serial.print(loc.country);
  Serial.print("  Latitude: ");     Serial.print(loc.latitude); 
  Serial.print("  Longitude: ");    Serial.println(loc.longitude); 

  snprintf(buff, 15, "Local: %.2i:%.2i  ", hour(local), minute(local));
  Serial.print(buff); Serial.print(tcr -> abbrev);   Serial.print("  ");
  Serial.print(dayStr(weekday(local)));              Serial.print(", ");
  Serial.print(monthStr(month(local)));              Serial.print(" ");
  Serial.print(day(local));                          Serial.print(", ");
  Serial.println(year(local));

  Serial.print("Connected to ");             Serial.print(WiFi.SSID());
  Serial.print("  IP address: ");            Serial.println(WiFi.localIP());
  Serial.print("NTP server string: ");       Serial.print(NTPtimeServer);
  Serial.print("  Resolved IP address: ");   Serial.println(ntpServerIP);

  snprintf(buff, 27, "Most recent sync: %.2i:%.2i:%.2i", hour(lastFix), minute(lastFix), second(lastFix));
  Serial.println(buff);

  if (tempSensorFlag)
  {
    Serial.print("Temperature: "); Serial.print(temperature, 1);         
    Serial.print("\xC2\xB0");      // degree symbol, same as Serial.print(char(194)); Serial.print(char(176));  
    if (menuItem[tempUnits].value == 1)    Serial.print("C  ");
    else Serial.print("F  ");
  }
  else Serial.print("Temp sensor not present.  ");
    
  if (showTemp) Serial.println("Temperature only displayed.");  
  else if (showDate) Serial.println("Date only displayed.");
  else if (menuItem[autoDate].value == 1 && !tempSensorFlag)     Serial.println("Date displayed each minute.");
  else if (menuItem[autoDate].value == 1 && tempSensorFlag)      Serial.println("Temperature and date displayed each minute.");
  else Serial.println("Temperature and date not displayed.");  

  if (nightMode(menuItem[nightBeginHour].value, menuItem[nightEndHour].value)) Serial.print("Currently in Night Mode.   ");
  else Serial.print("Currently in Day Mode.   "); 

  Serial.print("Hue: ");    Serial.println(hue); 

  for (uint8_t i = 0; i <= picMax; i++)
  {    
    if (menuItem[i].settingName[0][0] == '\0') snprintf(buff, 37, "%.2u. %s: %u", i, menuItem[i].name, menuItem[i].value); 
    else snprintf(buff, 37, "%.2u. %s: %s", i, menuItem[i].name, menuItem[i].settingName[menuItem[i].value]); 

    if (i % 2 != 0) 
    {
      Serial.print(buff);      
      for (uint8_t j = strlen(buff); j < 48; j++) Serial.print(" ");      // odd item, pad with blanks so even column lines up
    }   
    else Serial.println(buff);                                            // even item 
  } 
  Serial.println(" ");  
}

/*******************************************************************************************************************************************
* Simulate Nixie anti-cathode poisoning routine 
* Not random, but it looks like there is no pattern and every display  
* gets all digits lit which might not happen with random numbers               
********************************************************************************************************************************************/

void randomDigits()
{
  char buffer[numModules + 1];
  uint8_t start[10] = { 3, 7, 2, 5, 1, 8, 0, 4, 6, 9 };  
  uint8_t del = 10;
  buffer[numModules] = '\0';

  /*
  This sequence gets sent ten times with a pause after each line
  starting with 10ms and increasing to 210ms.  
  3 6 9 2 5 8
  7 0 3 6 9 2  
  2 5 8 1 4 7
  5 8 1 4 7 0
  1 4 7 0 3 6
  8 1 4 7 0 3
  0 3 6 9 2 5
  4 7 0 3 6 9 
  6 9 2 5 8 1
  9 2 5 8 1 4.
  */ 

  for (uint8_t i = 0; i < 10; i++)      // do the whole routine ten times
  {     
    for (uint8_t j = 0; j < 10; j++)    
    {
      uint8_t n = start[j];        
        
      for (uint8_t k = 0; k < numModules; k++)   
      {        
        buffer[k] = n + 48;    // store the number's ASCII value
        n = ((n + 3) % 10);    // next number will be 3 higher, wrapping around if > 9
      } 

      if (menuItem[NixieMode].value == 1) digitModuleWrite(buffer);              // call without hue and rainbow for Nixie simulation
      else digitModuleWrite(buffer, hue, menuItem[colorVariation].value);   

      uint32_t time_now = millis();   
      while(millis() - time_now < del) yield();        // do nothing, but allow background processes to continue      
      del += 2;       
    }    
  }       
} 

/*******************************************************************************************************************************************
* EEPROM functions                 
********************************************************************************************************************************************/

void readEEPROM()
{
  uint8_t address = 0;
  uint16_t EEPROMcheck;
  char buff[25];

  Serial.println("Read EEPROM:");
  Serial.println("Menu Item  Address  Value    Menu Item  Address  Value");

  for (uint8_t i = 0; i <= picMax ; i++)
  {
    address = i * sizeof(uint16_t);
    EEPROM.get(address, menuItem[i].value);
    snprintf(buff, 25, "%.02u:        %.02u       %u", i, address, menuItem[i].value); 
    
    if (i % 2 == 0) 
    {
      Serial.print(buff);      
      for (uint8_t j = strlen(buff); j < 29; j++) Serial.print(" ");      // even item, pad with blanks so odd column lines up
    }   
    else Serial.println(buff);      
  }

  address = (picMax + 1) * sizeof(uint16_t);
  EEPROM.get(address, EEPROMcheck);
  Serial.printf("Check:     %.02u       %u\r\n", address, EEPROMcheck);    
}

void updateEEPROM()
{
  uint8_t address;
  uint16_t oldValue;

  for (uint8_t i = 1; i <= picMax; i++)
  {
    address = i * sizeof(uint16_t);
    EEPROM.get(address, oldValue);
    if (oldValue != menuItem[i].value)
    {
      EEPROM.put(address, menuItem[i].value); 
      // These functions were called by the menu incrementSettings() and decrementSettings() functions if the 
      // appropriate data changed. We might not have to call them here too, but it doesn't hurt to do it again. 
      if (i > 14 && i < 18) setManualOffset();        // update loc and UTC offset values 
      if (pic > 18 && pic < 21) setColonLevels();     // check and update colon levels
      if (i > 23 && i < 32) setDSTRules();            // update DST rules  
    }    
  }    

  EEPROM.commit();
  Serial.println("EEPROM updated.");     
}

void initializeEEPROM()
{
  uint8_t address = 0;

  for (uint8_t i = 0; i <= picMax; i++)
  {
    address = i * sizeof(uint16_t);
    EEPROM.put(address, menuItem[i].value);    
  }

  uint16_t check = EEPROM_CHECK;
  address = (picMax + 1) * sizeof(uint16_t);
  EEPROM.put(address, check);  

  EEPROM.commit();
  Serial.println("EEPROM initialized.");
}

/*******************************************************************************************************************************************
* Check for night mode                       
********************************************************************************************************************************************/

bool nightMode(uint16_t start, uint16_t end) 
{
  bool state = false;      // true if hour is between start and end, bridges midnight if necessary

  if (start == end) state = false;

  if (start < end)
  {
    if (hour(local) >= start && hour(local) < end) state = true; 
    else if (hour(local) >= end) state = false;
    else state = false;
  }

  if (start > end)
  {  
    if (hour(local) >= start && hour(local) <= 23) state = true;  
    else if (hour(local) < end) state = true;
    else if (hour(local) >= end && hour(local) < start) state = false;
  }
  return state;
} 

/*******************************************************************************************************************************************
* Button click functions                      
********************************************************************************************************************************************/

// Menu increment click event - select next menu item and refresh displays to show it

void menuIncrementClick() 
{
  lastMillis = millis();    
  pic++;
  if (pic > picMax) pic = 0;         // one extra step to show the geographic information and version
  if (menuItem[serialMonitorDetail].value == 1) 
  {
    Serial.print("Increment short press "); Serial.println(pic);
    serialMonitor();
  }
  
  displayMenuItem();                 // digit modules
  showMenuItem();                    // OLED   
}

// Menu increment double click event - quit menu display

void menuIncrementDoubleclick() 
{
  pic = picMax + 1;
  if (menuItem[serialMonitorDetail].value == 1) {Serial.print("Increment double press "); Serial.println(pic); serialMonitor();}        
}

// Menu increment long press event - change setting for selected item, refresh displays and serial monitor to show change

void menuIncrementLongPressStart()
{
  lastMillis = millis();
  incrementSetting();

  if (menuItem[serialMonitorDetail].value == 1)   
  {
    Serial.print("Increment long press "); Serial.println(pic);
    serialMonitor();
  }
    
  displayMenuItem();      // digit modules
  showMenuItem();         // OLED  
}

// Not used

void menuIncrementLongPressStop()
{
  ;
}

// Menu decrement click event - select next menu item and refresh displays to show it

void menuDecrementClick() 
{
  lastMillis = millis();    
  pic--;
  if (pic > picMax) pic = picMax;          // pic is an unsigned 8-bit integer, it underflows from 0 to 255    

  if (menuItem[serialMonitorDetail].value == 1) 
  {
    Serial.print("Decrement short press ");  Serial.println(pic);
    serialMonitor();
  }
  displayMenuItem();                       // digit modules
  showMenuItem();                          // OLED   
} 

// Menu decrement double click event - quit menu display

void menuDecrementDoubleclick() 
{
  pic = picMax + 1;
  if (menuItem[serialMonitorDetail].value == 1) 
  {
    Serial.print("Decrement double press "); Serial.println(pic); 
    serialMonitor();   
  }     
}

// Menu decrement long press event - change setting for selected item, refresh displays and serial monitor to show change

void menuDecrementLongPressStart()
{
  lastMillis = millis();
  decrementSetting();

  if (menuItem[serialMonitorDetail].value == 1) 
  {
    Serial.print("Decrement long press "); Serial.println(pic);
    serialMonitor(); 
  }
    
  displayMenuItem();      // digit modules
  showMenuItem();         // OLED   
}

// Not used

void menuDecrementLongPressStop()
{
  ;
}

// Date/Temperature click event - change continuous date display state

void dateTempClick()
{
  showTemp = false;
  showDate = !showDate;
  if (menuItem[serialMonitorDetail].value == 1) serialMonitor();   
}

// Date/Temperature long press event - change continuous temperature display state

void dateTempLongPressStart()
{
  showDate = false;
  if (!tempSensorFlag) showTemp = false;
  else showTemp = !showTemp;
  if (menuItem[serialMonitorDetail].value == 1) serialMonitor(); 
}

// Date/Temperature double click event - toggle eleventh panel illumination for Lixie II

void dateTempDoubleclick()
{
  lightEleven = !lightEleven;  
  if (menuItem[serialMonitorDetail].value == 1) serialMonitor(); 
}

// Update the settings

void incrementSetting()
{
  menuItem[pic].value += menuItem[pic].increment;
  if (menuItem[pic].value > menuItem[pic].highLimit) 
    menuItem[pic].value = menuItem[pic].lowLimit;

  // Changing these menu items requires more than just assigning a new number to the item's value          
  if (pic > 14 && pic < 18) setManualOffset(); 
  if (pic > 18 && pic < 21) setColonLevels();  
  if (pic > 23 && pic < 32) setDSTRules();      

  dataChanged = true;    // flag the main loop to call updateEEPROM() when the menu exits
}

void decrementSetting()
{
  menuItem[pic].value -= menuItem[pic].increment;
  // The setting values are unsigned 16-bit integers. If they are 0 and decremented by 1 they underflow to 65,535
  if (menuItem[pic].value > menuItem[pic].highLimit || menuItem[pic].value < menuItem[pic].lowLimit) 
    menuItem[pic].value = menuItem[pic].highLimit;

  if (pic > 14 && pic < 18) setManualOffset(); 
  if (pic > 18 && pic < 21) setColonLevels(); 
  if (pic > 23 && pic < 32) setDSTRules();           

  dataChanged = true;    
}

void setManualOffset()
{
  if (menuItem[manualOffsetPolarity].value == 0)
  {
    snprintf(loc.timezone, 24, "UTC - %.2u:%.2u", menuItem[manualOffsetHour].value, menuItem[manualOffsetMinute].value);
    loc.offsetSeconds = -1 * (menuItem[manualOffsetHour].value * 3600 + menuItem[manualOffsetMinute].value * 60);
  } 
  else
  {
    snprintf(loc.timezone, 24, "UTC + %.2u:%.2u", menuItem[manualOffsetHour].value, menuItem[manualOffsetMinute].value);
    loc.offsetSeconds = menuItem[manualOffsetHour].value * 3600 + menuItem[manualOffsetMinute].value * 60; 
  }                         
  strncpy(loc.city, "Set from menu", 14); 
  strncpy(loc.region, "Restart the clock", 18);
  strncpy(loc.country,  "to use Auto TZ", 15);
  loc.latitude = 0.0;
  loc.longitude = 0.0;   
}

// Make sure colon max level is greater than min level, also update breathing step time
void setColonLevels()
{
  if (menuItem[colonMinimumBrightness].value >= menuItem[colonMaximumBrightness].value)   
    menuItem[colonMinimumBrightness].value = (menuItem[colonMaximumBrightness].value - menuItem[colonMinimumBrightness].increment); 
  breatheStepTime = 900 / ((menuItem[colonMaximumBrightness].value - menuItem[colonMinimumBrightness].value) / BREATHE_LEVEL_CHANGE * 2);   
}

void setDSTRules()
{
  strncpy(dstRule.abbrev, "DST", 4);
  dstRule.week = menuItem[DSTstartWeek].value;
  dstRule.dow = menuItem[DSTstartDay].value;
  dstRule.month = menuItem[DSTstartMonth].value;
  dstRule.hour = menuItem[DSTstartHour].value;

  strncpy(stdRule.abbrev, "STD", 4);
  stdRule.week = menuItem[DSTendWeek].value;
  stdRule.dow = menuItem[DSTendDay].value;
  stdRule.month = menuItem[DSTendMonth].value;
  stdRule.hour = menuItem[DSTendHour].value;

  tz.setRules(dstRule, stdRule);
}

/*******************************************************************************************************************************************
* OLED functions                      
********************************************************************************************************************************************/

void header()
{
  char buffer[36];
  snprintf(buffer, 36, "%s Clock v%s", displayName, cVersion);

  display.setFont(ArialMT_Plain_10);
  display.setTextAlignment(TEXT_ALIGN_LEFT);
  display.drawString(5, 0, buffer);
  display.drawLine(0, 13, 128, 13);  
}

void footer()
{
  char buffer[36];
  snprintf(buffer, 36, "%s Clock v%i.%i%i", displayName, VERSION_MAJOR, VERSION_MINOR, VERSION_BUILD);

  display.drawString(5, 52, buffer);
  display.drawLine(0 ,51, 128, 51);
}

void menuHeader()
{
  display.setFont(ArialMT_Plain_10);
  display.setTextAlignment(TEXT_ALIGN_LEFT);
  display.drawString(10, 0, "Settings Menu");  
  display.drawLine(0, 13, 128, 13);                         
}

void menuFooter()
{
  display.drawString(5, 45, "Long press to change");
  display.drawLine(0 ,44, 128, 44);  
}

void showMenuItem()
{
  char buffer[20];
  display.clear();
  menuHeader();

  snprintf(buffer, 20, "%.2u.", pic);
  display.drawString(10, 18, buffer); 
  display.drawString(26, 18, menuItem[pic].name);

  if (pic == 0) showInfo();                  // show geographic information and version at start of menu sequence
  else if (pic == picMax) showNetInfo();     // show network information and version at end of menu sequence
  else                                       // show  current menu item and its setting
  {
    if (menuItem[pic].settingName[0][0] == '\0') 
    {
      snprintf(buffer, 20, "%u", menuItem[pic].value);
      display.drawString(26, 28, buffer);
    }
    else
    {
      display.drawString(26, 28, menuItem[pic].settingName[menuItem[pic].value]);      
    } 
    menuFooter();
  }     
  display.display();
  delay(5);  
}

void showInfo()
{
  display.clear();  
  header();
  char buff[24];

  if (menuItem[autoDST].value == 1 )          // Auto DST enabled
  {
    int16_t stdOffsetMinutes = loc.offsetSeconds / 60;      
    dstRule.offset = stdOffsetMinutes + 60;
    stdRule.offset = stdOffsetMinutes;
    tz.setRules(dstRule, stdRule);

    if (tz.locIsDST(local)) display.drawString(10, 18, "Time Zone (DST):");
    else display.drawString(10, 18, "Time Zone (ST):");
  } 
  else display.drawString(10, 18, "Time Zone (No DST):");

  display.drawString(10, 27, loc.timezone);
  display.drawString(10, 36, loc.city);
  display.drawString(10, 45, loc.region);
  display.drawString(10, 54, loc.country);    
  
  display.display();
  delay(5);
}  

void showNetInfo()
{
  display.clear();  
  header();
  char buff[24];
  
  snprintf(buff, 24, "%s", WiFi.SSID());
  display.drawString(10, 18, buff);

  snprintf(buff, 24, "Signal: %idB", WiFi.RSSI());
  display.drawString(10, 27, buff);

  IPAddress ip = WiFi.localIP();
  snprintf(buff, 16, "%d.%d.%d.%d", ip[0], ip[1], ip[2], ip[3]);
  display.drawString(10, 36, buff);
  display.drawString(10, 45, NTPtimeServer);

  snprintf(buff, 18,"Fix: %.2i:%.2i:%.2i  %.2u", hour(lastFix), minute(lastFix), second(lastFix), count);
  display.drawString(10, 54, buff);      
  
  display.display();
  delay(5);
}

/*******************************************************************************************************************************************
* Display menu settings on digit modules                    
********************************************************************************************************************************************/

void displayMenuItem()
{
  char buffer[7];

  // Clear the colons  
  breathe.detach();            // make sure breathing sequence isn't still running
  for (uint8_t i = totalLEDs - 2; i < totalLEDs; i++) ledArray[i] = CRGB(0, 0, 0);

  if (pic == picMax) snprintf(buffer, 7, "%.2i%.2i%.2i", hour(lastFix), minute(lastFix), second(lastFix));
  else if (pic == 0) 
  {
    uint16_t iOff = abs(loc.offsetSeconds);  
    uint8_t hr = iOff / 3600;
    uint8_t mn = (iOff - (hr * 3600)) / 60;
    if (loc.offsetSeconds < 0) snprintf(buffer, 7, "0-%.2u%.2u", hr, mn);
    else snprintf(buffer, 7, "--%.2u%.2u", hr, mn);
  }
  else
  {    
    buffer[0] = pic / 10 + 48;
    buffer[1] = pic % 10 + 48;
    buffer[2] = 45;               // ASCII "-" (to blank this position)  

    uint16_t val = menuItem[pic].value;
    uint8_t hundreds = val / 100;
    val = val % 100;
    uint8_t tens = val / 10;
    uint8_t ones = val % 10;                 

    if (hundreds > 0)
    {      
      buffer[3] = hundreds + 48;
      buffer[4] = tens + 48;      
    }  
    else if (tens > 0) 
    {
      buffer[3] = 45;
      buffer[4] = tens + 48;     
    }    
    else 
    {
      buffer[3] = 45;
      buffer[4] = 45;      
    } 
    buffer[5] = ones + 48;    
    buffer[6] = '\0';
  }    
  
  digitModuleWrite(buffer, CRGB::DarkOrange);    
  delay(10);                  // let digit modules settle down   
}

/*******************************************************************************************************************************************
* Breathing colon sequencer - put in RAM so it runs faster                      
********************************************************************************************************************************************/

void IRAM_ATTR sequenceBreathing()
{  
  static bool decrease = false;                        // initial values when first called
  static uint16_t level = menuItem[colonMinimumBrightness].value;          

  //Serial.println(level);

  if (menuItem[NixieMode].value == 1)                          // Nixie mode
  { 
    CRGB ion = nixieIon;
    ion.nscale8(level);
    if (tempDisplay)
    {
      ledArray[totalLEDs - 2] = ion;                   
      ledArray[totalLEDs - 1] = scaledNixieAura;
    }
    else if (menuItem[digits].value == 0)        // four digit mode
    {
      // In four digit mode the first LED connected to DOUT of the last digit module is off
      ledArray[totalLEDs - 1] = ion;                   
      ledArray[totalLEDs - 2] = scaledNixieAura;
    }
    else
    {
      ledArray[totalLEDs - 2] = ion;
      ledArray[totalLEDs - 1] = ion;
    }      
  }  
  else        
  {
    if (tempDisplay)
    {
      ledArray[totalLEDs - 2] = CHSV(colonHue, 255, level);
      ledArray[totalLEDs - 1] = CRGB::Black; 
    }
    else if (menuItem[digits].value == 0)        // four digit mode
    {
      ledArray[totalLEDs - 1] = CHSV(colonHue, 255, level);
      ledArray[totalLEDs - 2] = CRGB::Black; 
    }
    else
    {
      ledArray[totalLEDs - 2] = CHSV(colonHue, 255, level);
      ledArray[totalLEDs - 1] = CHSV(colonHue, 255, level);
    }    
  }            
  FastLED.show();    

  if ((level < menuItem[colonMaximumBrightness].value) && !decrease) level += BREATHE_LEVEL_CHANGE;      // increase the brightness
  else decrease = true;                                                              // increasing finished, time to decrease
  
  if (decrease) level -= BREATHE_LEVEL_CHANGE;                                       // decrease the brightness  
  if (decrease && (level < menuItem[colonMinimumBrightness].value))                  // sequence done, turn off timer and reset for next second
  {                                               
    breathe.detach();             
    decrease = false;                 
    level = menuItem[colonMinimumBrightness].value;                             // colon min level
  }
  delay(1);                 // yield to background functions  
}

/*******************************************************************************************************************************************
* Set timezone                   
********************************************************************************************************************************************/

void setTimeZone()
{
  char buffer[7];
  uint8_t count = 0;
  int16_t stdOffsetMinutes;

  loc.status = false;
  while (!loc.status && count < 3)     // try three times to connect to geoip server
  {
    count++;
    Serial.print("Auto attempt no. ");  Serial.print(count);  Serial.println(" of 3");
    snprintf(buffer, 7, "---%.1i%.1u-", 4, count);          // display blue 4 and attempt number
    digitModuleWrite(buffer, BLUE, false);
    loc = geoip.getGeoFromWiFi(true);                       // true to show results on serial monitor  
  } 
 
  if (!loc.status)   // time zone information is not valid
  {   
    Serial.print("Auto time zone failed. Attempts: "); Serial.println(count);  
    Serial.println("Using manual settings.");   

    setManualOffset();   

    if (menuItem[autoDST].value == 1 )          // Auto DST enabled
    {
      stdOffsetMinutes = loc.offsetSeconds / 60;      
      dstRule.offset = stdOffsetMinutes + 60;
      stdRule.offset = stdOffsetMinutes;
      tz.setRules(dstRule, stdRule);
      local = tz.toLocal(now(), &tcr);
    }
    else local = now() + loc.offsetSeconds;

   snprintf(buffer, 7, "-----%.1i", 5);          // display yellow 5 if time zone was not received from GeoIP
   digitModuleWrite(buffer, YELLOW, false);     
  }                                  
  else
  {
    Serial.print("Auto time zone successful, attempts: ");  Serial.println(count);         

    // ipapi.co supplies local UTC offset already adjusted for Daylight Saving Time.
    // However displayDigitalData() function expects Standard Time and it does the adjustment for DST if necessary.
    // Therefore we fix the offset from ipapi if it comes in during DST.
    // This avoids having to call ipapi every day to see if DST has changed.    

    if (menuItem[autoDST].value == 1 )          // Auto DST enabled
    {
      stdOffsetMinutes = loc.offsetSeconds / 60;      
      dstRule.offset = stdOffsetMinutes + 60;
      stdRule.offset = stdOffsetMinutes;
      tz.setRules(dstRule, stdRule);

      local = tz.toLocal(now(), &tcr);       
      if (tz.locIsDST(local)) loc.offsetSeconds -= 3600;   // remove one hour to convert to Standard time 
    }
    else local = now() + loc.offsetSeconds; 

    snprintf(buffer, 7, "-----%.1i", 5);           // display green 5 if time zone was received from GeoIP
    digitModuleWrite(buffer, GREEN, false);      
  }

  Serial.print("DST: ");               
  if (menuItem[autoDST].value == 1) Serial.println(tz.locIsDST(local));
  else                         Serial.println("Disabled");
  Serial.print("Timezone set to ");            Serial.println(loc.timezone);  
  Serial.print("Offset seconds (STD time): "); Serial.println(loc.offsetSeconds);      
}

void setManualTimeZone()
{
  char buffer[7];

  setManualOffset();

  if (menuItem[autoDST].value == 1 )          
  {
    int16_t stdOffsetMinutes = loc.offsetSeconds / 60;      
    dstRule.offset = stdOffsetMinutes + 60;
    stdRule.offset = stdOffsetMinutes;
    tz.setRules(dstRule, stdRule);
    local = tz.toLocal(now(), &tcr);
  }
  else local = now() + loc.offsetSeconds;
  
  snprintf(buffer, 7, "---%.1i%.1u-", 4, 0);          // display blue 4 and 0 AutoTZ attempts
  digitModuleWrite(buffer, BLUE, false);
  delay(1500);
  snprintf(buffer, 7, "-----%.1i", 5);                // display yellow 5
  digitModuleWrite(buffer, YELLOW, false);     

  Serial.print("DST: ");               
  if (menuItem[autoDST].value == 1) Serial.println(tz.locIsDST(local));
  else                              Serial.println("Disabled");
  Serial.print("Timezone set to ");            Serial.println(loc.timezone);  
  Serial.print("Offset seconds (STD time): "); Serial.println(loc.offsetSeconds);
}

/*******************************************************************************************************************************************
* Colons display                       
********************************************************************************************************************************************/

void displayDots()
{
  if (menuItem[NixieMode].value == 1)                      // Nixie mode
  { 
    CRGB dotColor = nixieIon;
    dotColor.nscale8(menuItem[colonMaximumBrightness].value);
    if (tempDisplay)
    {
      ledArray[totalLEDs - 2] = dotColor;          
      ledArray[totalLEDs - 1] = scaledNixieAura;
    }
    else if (menuItem[digits].value == 0)    // four digit mode
    {
      // In four digit mode the first LED connected to DOUT on the last digit module is off
      ledArray[totalLEDs - 1] = dotColor;          
      ledArray[totalLEDs - 2] = scaledNixieAura;
    }
    else
    {
      ledArray[totalLEDs - 2] = dotColor;
      ledArray[totalLEDs - 1] = dotColor;
    }      
  }  
  else        
  {
    if (tempDisplay)
    {
      ledArray[totalLEDs - 2] = CHSV(colonHue, 255, menuItem[colonMaximumBrightness].value);          
      ledArray[totalLEDs - 1] = CRGB::Black;
    }
    else if (menuItem[digits].value == 0)    // four digit mode
    {
      ledArray[totalLEDs - 1] = CHSV(colonHue, 255, menuItem[colonMaximumBrightness].value);
      ledArray[totalLEDs - 2] = CRGB::Black; 
    }
    else
    {
      ledArray[totalLEDs - 2] = CHSV(colonHue, 255, menuItem[colonMaximumBrightness].value);
      ledArray[totalLEDs - 1] = CHSV(colonHue, 255, menuItem[colonMaximumBrightness].value);
    }    
  }            
  FastLED.show();      
}

void clearDots()
{
  ledArray[totalLEDs - 2] = CRGB::Black;
  ledArray[totalLEDs - 1] = CRGB::Black;  
  FastLED.show();
}

/*******************************************************************************************************************************************
* Set parameters for the digit modules                    
********************************************************************************************************************************************/

void setDigitModuleType()
{
  displayType = digitalRead(DISP_TYPE_TWOS);    // read the two's bit
  displayType <<= 1;                            // shift it left one bit
  displayType |= digitalRead(DISP_TYPE_ONES);   // read the one's bit then OR it with the twos

  switch (displayType)
  {
  case 0b11:
    strncpy(displayName, "NixieCron", 10);
    ledsPerModule = 10;

    panelOrder[0] = 5;              // LED address to light the 0 panel on the first NixieCron
    panelOrder[1] = 0;              // LED address to light the 1 panel on the first NixieCron
    panelOrder[2] = 6;              
    panelOrder[3] = 1;              // NixieCron S and M displays have two LEDs per panel, L displays have three 
    panelOrder[4] = 7;              // the second and third LEDs are looked after by the hardware
    panelOrder[5] = 2; 
    panelOrder[6] = 8; 
    panelOrder[7] = 3; 
    panelOrder[8] = 9; 
    panelOrder[9] = 4;              // LED address to light the 9 panel on the first NixieCron  
    break;
  case 0b10:
    strncpy(displayName, "Lixie", 6);
    ledsPerModule = 20;  

    panelOrder[0] = 3;              // LED address to light the first LED for 0 panel on the first EleksTube.
    panelOrder[1] = 4; 
    panelOrder[2] = 2; 
    panelOrder[3] = 0;              // Lixie displays have two LEDs per panel,  
    panelOrder[4] = 8;              // the second LED is 10 addresses higher than the first 
    panelOrder[5] = 6; 
    panelOrder[6] = 5; 
    panelOrder[7] = 7; 
    panelOrder[8] = 9;  
    panelOrder[9] = 1;
    break;
  case 0b01:
    strncpy(displayName, "EleksTube", 10);
    ledsPerModule = 20; 

    panelOrder[0] = 5;              // LED address to light the first LED for 0 panel on the first EleksTube. 
    panelOrder[1] = 0; 
    panelOrder[2] = 6; 
    panelOrder[3] = 1;              // EleksTube displays have two LEDs per panel, 
    panelOrder[4] = 7;              // the second LED is 10 addresses higher than the first
    panelOrder[5] = 2; 
    panelOrder[6] = 8; 
    panelOrder[7] = 3; 
    panelOrder[8] = 9; 
    panelOrder[9] = 4;
    break;        
  case 0b00:
    strncpy(displayName, "Lixie II", 9);
    ledsPerModule = 22;

    panelOrder[0] = 7;              // LED address to light the first LED for 0 panel on the first Lixie. 
    panelOrder[1] = 0; 
    panelOrder[2] = 8; 
    panelOrder[3] = 6;               
    panelOrder[4] = 2;              
    panelOrder[5] = 10; 
    panelOrder[6] = 3; 
    panelOrder[7] = 5; 
    panelOrder[8] = 9; 
    panelOrder[9] = 1;

    panelOrder2[0] = 14;            // LED address to light the second LED for 0 panel on the first Lixie. 
    panelOrder2[1] = 21; 
    panelOrder2[2] = 13; 
    panelOrder2[3] = 15;              
    panelOrder2[4] = 19;             
    panelOrder2[5] = 11; 
    panelOrder2[6] = 18; 
    panelOrder2[7] = 16; 
    panelOrder2[8] = 12; 
    panelOrder2[9] = 20;             
    break;  
  }
  
  // Common parameters

  Serial.printf("Display type: %s\r\n", displayName);
  WiFi.hostname(displayName);    
  ArduinoOTA.setHostname(displayName);  
  numModules = 6;
  totalLEDs = numModules * ledsPerModule + 2;     // + 2 for the colons
}

/********************************************************************************************************************************************
* Called when TimeLib wants to sync with time server, tries ten times if necessary                    
********************************************************************************************************************************************/

time_t setNtpTime()
{
  count = 0;
  time_t t = 0;
  Udp.begin(localPort);

  while (t == 0 && count < 10) 
  {
    count++;
    t = getNtpTime();
    delay(5);  
  }

  Udp.stop();
  if (t > 0) 
  {
    lastFix = t;
    Serial.printf("Time set, UTC: %.2i:%.2i:%.2i   Attempts: %u\r\n", 
      hour(lastFix), minute(lastFix), second(lastFix), count);  
  }
  else Serial.printf("Time was not able to be set. Attempts: %u\r\n", count);

  return t;   
}

/********************************************************************************************************************************************
*  NTP Code that everybody uses                     
********************************************************************************************************************************************/

const int NTP_PACKET_SIZE = 48;           // NTP time is in the first 48 bytes of message
byte packetBuffer[NTP_PACKET_SIZE];       // buffer to hold incoming & outgoing packets

time_t getNtpTime()
{
  while (Udp.parsePacket() > 0) delay(0);          // discard any previously received packets  
  
  // Resolve NTP server's IP address
  int res = WiFi.hostByName(NTPtimeServer, ntpServerIP);      
  Serial.println("HostByName results:");    
  Serial.print("  NTP server string:   ");  Serial.println(NTPtimeServer);  
  Serial.print("  Resolved IP address: ");  Serial.println(ntpServerIP);
  Serial.print("  Status returned:     ");  Serial.println(res); 

  if (res != 1) 
  {
    Serial.println("IP address not resolved :-(");
    return 0;
  }  

  Serial.println("Transmit NTP Request");   
  sendNTPpacket(ntpServerIP);  
  uint32_t beginWait = millis();
  while (millis() - beginWait < 1500) 
  {
    delay(0);
    int size = Udp.parsePacket();
    if (size >= NTP_PACKET_SIZE) 
    {
      if (menuItem[serialMonitorDetail].value == 1 ) Serial.println("Received NTP response :-)");
      ledOnMillis = millis();
      digitalWrite(BUILT_IN_LED, LED_ON);           // turn on the LED
      lastFix = local;       

      Udp.read(packetBuffer, NTP_PACKET_SIZE);     // read packet into the buffer

      // Convert four bytes starting at location 40 to a long integer
      unsigned long secsSince1900;      
      secsSince1900 =  (unsigned long)packetBuffer[40] << 24;
      secsSince1900 |= (unsigned long)packetBuffer[41] << 16;
      secsSince1900 |= (unsigned long)packetBuffer[42] << 8;
      secsSince1900 |= (unsigned long)packetBuffer[43];

      return secsSince1900 - 2208988800UL;
    }
  }

  Serial.println("No NTP response :-(");
  return 0;             // return 0 if unable to get the time
}

// Send an NTP request to the time server at the given address

void sendNTPpacket(IPAddress &address)
{
  // set all bytes in the buffer to 0
  memset(packetBuffer, 0, NTP_PACKET_SIZE);
  
  // Initialize values needed to form NTP request
  // (see URL above for details on the packets)
  
  packetBuffer[0] = 0b11100011;   // LI, Version, Mode
  packetBuffer[1] = 0;            // Stratum, or type of clock
  packetBuffer[2] = 6;            // Polling Interval
  packetBuffer[3] = 0xEC;         // Peer Clock Precision
  
  // Bytes 4 to 11 left set to zero for Root Delay & Root Dispersion
  
  packetBuffer[12] = 49;
  packetBuffer[13] = 0x4E;
  packetBuffer[14] = 49;
  packetBuffer[15] = 52;
  
  // All NTP fields have been given values, now send a packet requesting a timestamp
  
  Udp.beginPacket(address, 123);            // NTP requests are to port 123
  Udp.write(packetBuffer, NTP_PACKET_SIZE);
  Udp.endPacket();
}

/*******************************************************************************************************************************************
* Set menu item names, limits, and default values
* If the settings for an item are numeric or don't have names don't specify anything in the item's settingName array                        
********************************************************************************************************************************************/

void menuSettings()
{
  // Zero the items 
  for (uint8_t i = 0; i <= picMax; i++)
  {
    menuItem[i].name[0] = '\0'; 
    for (uint8_t j = 0; j < 13; j++) menuItem[i].settingName[j][0] = '\0';
    menuItem[i].lowLimit = 0;
    menuItem[i].highLimit = 0;
    menuItem[i].increment = 0;
    menuItem[i].value = 0;
  }

  // This must be the first menu item. Shows version along with time zone and geographic info on OLED                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                             
  strncpy(menuItem[TZinfo].name, "Version", 8);     
  menuItem[TZinfo].lowLimit = iVersion;
  menuItem[TZinfo].highLimit = iVersion;
  menuItem[TZinfo].increment = 0;
  menuItem[TZinfo].value = iVersion;
  
  strncpy(menuItem[hourFormat].name, "Hour format", 12);
  strncpy(menuItem[hourFormat].settingName[0], "12", 3);
  strncpy(menuItem[hourFormat].settingName[1], "24", 3);
  menuItem[hourFormat].lowLimit = 0;
  menuItem[hourFormat].highLimit = 1;
  menuItem[hourFormat].increment = 1;
  menuItem[hourFormat].value = 1;  

  strncpy(menuItem[blanking].name, "Lead zero blanking", 19);
  strncpy(menuItem[blanking].settingName[0], "Disabled", 9);
  strncpy(menuItem[blanking].settingName[1], "Enabled", 8);              
  menuItem[blanking].lowLimit = 0;
  menuItem[blanking].highLimit = 1;
  menuItem[blanking].increment = 1;
  menuItem[blanking].value = 1;                         

  strncpy(menuItem[dateFormat].name, "Date format", 12);
  strncpy(menuItem[dateFormat].settingName[0], "MM DD YY", 9);   
  strncpy(menuItem[dateFormat].settingName[1], "DD MM YY", 9);   
  strncpy(menuItem[dateFormat].settingName[2], "YY MM DD", 9);   
  menuItem[dateFormat].lowLimit = 0;
  menuItem[dateFormat].highLimit = 2;
  menuItem[dateFormat].increment = 1;
  menuItem[dateFormat].value = 0;                          

  strncpy(menuItem[autoDate].name, "Date/Temperature", 17);
  strncpy(menuItem[autoDate].settingName[0], "Time only", 10);   
  strncpy(menuItem[autoDate].settingName[1], "Auto", 5);   
  menuItem[autoDate].lowLimit = 0;
  menuItem[autoDate].highLimit = 1;
  menuItem[autoDate].increment = 1;;
  menuItem[autoDate].value = 1;                          

  strncpy(menuItem[tempUnits].name, "Temperature units", 18);
  strncpy(menuItem[tempUnits].settingName[0], "Fahrenheit", 11);   
  strncpy(menuItem[tempUnits].settingName[1], "Celsius", 8);   
  menuItem[tempUnits].lowLimit = 0;
  menuItem[tempUnits].highLimit = 1;
  menuItem[tempUnits].increment = 1;
  menuItem[tempUnits].value = 1;                          

  strncpy(menuItem[NixieMode].name, "Nixie mode", 11);
  strncpy(menuItem[NixieMode].settingName[0], "Disabled", 9);   
  strncpy(menuItem[NixieMode].settingName[1], "Enabled", 8);   
  menuItem[NixieMode].lowLimit = 0;
  menuItem[NixieMode].highLimit = 1;
  menuItem[NixieMode].increment = 1;
  menuItem[NixieMode].value = 1;                        

  strncpy(menuItem[autoColor].name, "Auto color", 11);
  strncpy(menuItem[autoColor].settingName[0], "Static, manual", 15);   
  strncpy(menuItem[autoColor].settingName[1], "Changing, auto", 15);   
  menuItem[autoColor].lowLimit = 0;
  menuItem[autoColor].highLimit = 1;
  menuItem[autoColor].increment = 1;
  menuItem[autoColor].value = 1;                          

  strncpy(menuItem[colorVariation].name, "Color variation", 16);
  strncpy(menuItem[colorVariation].settingName[0], "All the same", 13);   
  strncpy(menuItem[colorVariation].settingName[1], "Rainbow pairs", 14);   
  menuItem[colorVariation].lowLimit = 0;
  menuItem[colorVariation].highLimit = 1;
  menuItem[colorVariation].increment = 1;
  menuItem[colorVariation].value = 0;                         

  strncpy(menuItem[randomMode].name, "Random", 7);
  strncpy(menuItem[randomMode].settingName[0], "Disabled", 9);   
  strncpy(menuItem[randomMode].settingName[1], "Enabled", 8);   
  menuItem[randomMode].lowLimit = 0;
  menuItem[randomMode].highLimit = 1;
  menuItem[randomMode].increment = 1;
  menuItem[randomMode].value = 1;                         

  strncpy(menuItem[dayBrightness].name, "Day brightness", 15);
  menuItem[dayBrightness].lowLimit = 0;
  menuItem[dayBrightness].highLimit = 255;
  menuItem[dayBrightness].increment = 15;
  menuItem[dayBrightness].value = 255;                         

  strncpy(menuItem[nightBrigtness].name, "Night brightness", 17);
  menuItem[nightBrigtness].lowLimit = 0;
  menuItem[nightBrigtness].highLimit = 255;
  menuItem[nightBrigtness].increment = 15;
  menuItem[nightBrigtness].value = 30; 

  strncpy(menuItem[nightBeginHour].name, "Night begin hour", 17);
  menuItem[nightBeginHour].lowLimit = 0;
  menuItem[nightBeginHour].highLimit = 23;
  menuItem[nightBeginHour].increment = 1;
  menuItem[nightBeginHour].value = 23;                          
  
  strncpy(menuItem[nightEndHour].name, "Night end hour", 15);
  menuItem[nightEndHour].lowLimit = 0;
  menuItem[nightEndHour].highLimit = 23;
  menuItem[nightEndHour].increment = 1;
  menuItem[nightEndHour].value = 8;                          

  strncpy(menuItem[digits].name, "Digits", 7);
  strncpy(menuItem[digits].settingName[0], "Four", 5);   
  strncpy(menuItem[digits].settingName[1], "Six", 4);   
  menuItem[digits].lowLimit = 0;
  menuItem[digits].highLimit = 1;
  menuItem[digits].increment = 1;
  menuItem[digits].value = 1;                        

  strncpy(menuItem[manualOffsetHour].name, "Manual offset hour", 19);
  menuItem[manualOffsetHour].lowLimit = 0;
  menuItem[manualOffsetHour].highLimit = 14;
  menuItem[manualOffsetHour].increment = 1;
  menuItem[manualOffsetHour].value = 8;

  strncpy(menuItem[manualOffsetMinute].name, "Manual offset minute", 21);
  menuItem[manualOffsetMinute].lowLimit = 0;
  menuItem[manualOffsetMinute].highLimit = 45;
  menuItem[manualOffsetMinute].increment = 15;
  menuItem[manualOffsetMinute].value = 0; 

  strncpy(menuItem[manualOffsetPolarity].name, "Man. offset polarity", 21);
  strncpy(menuItem[manualOffsetPolarity].settingName[0], "Behind UTC (-)", 15);   
  strncpy(menuItem[manualOffsetPolarity].settingName[1], "Ahead UTC (+)", 14);   
  menuItem[manualOffsetPolarity].lowLimit = 0;
  menuItem[manualOffsetPolarity].highLimit = 1;
  menuItem[manualOffsetPolarity].increment = 1;
  menuItem[manualOffsetPolarity].value = 0;                        

  strncpy(menuItem[colonMode].name, "Colons", 7);
  strncpy(menuItem[colonMode].settingName[0], "Off", 4);   
  strncpy(menuItem[colonMode].settingName[1], "Steady", 7);   
  strncpy(menuItem[colonMode].settingName[2], "Breathing", 10);   
  strncpy(menuItem[colonMode].settingName[3], "Flashing", 9);   
  menuItem[colonMode].lowLimit = 0;
  menuItem[colonMode].highLimit = 3;
  menuItem[colonMode].increment = 1;
  menuItem[colonMode].value = 2;                        

  strncpy(menuItem[colonMinimumBrightness].name, "Colon min brightness", 21);
  menuItem[colonMinimumBrightness].lowLimit = 0;
  menuItem[colonMinimumBrightness].highLimit = 255;
  menuItem[colonMinimumBrightness].increment = 15;
  menuItem[colonMinimumBrightness].value = 30; 

  strncpy(menuItem[colonMaximumBrightness].name, "Colon max brightness", 21);
  menuItem[colonMaximumBrightness].lowLimit = 0;
  menuItem[colonMaximumBrightness].highLimit = 255;
  menuItem[colonMaximumBrightness].increment = 15;
  menuItem[colonMaximumBrightness].value = 120;

  strncpy(menuItem[NixieAura].name, "Nixie aura intensity", 21);
  menuItem[NixieAura].lowLimit = 0;
  menuItem[NixieAura].highLimit = 15;
  menuItem[NixieAura].increment = 1;
  menuItem[NixieAura].value = 2;

  strncpy(menuItem[serialMonitorDetail].name, "Serial monitor", 15);
  strncpy(menuItem[serialMonitorDetail].settingName[0], "Minimal", 8);   
  strncpy(menuItem[serialMonitorDetail].settingName[1], "Detailed", 9);   
  menuItem[serialMonitorDetail].lowLimit = 0;
  menuItem[serialMonitorDetail].highLimit = 1;
  menuItem[serialMonitorDetail].increment = 1;
  menuItem[serialMonitorDetail].value = 0;

  strncpy(menuItem[autoDST].name, "Auto DST", 9);
  strncpy(menuItem[autoDST].settingName[0], "Disabled", 9);   
  strncpy(menuItem[autoDST].settingName[1], "Enabled", 8);   
  menuItem[autoDST].lowLimit = 0;
  menuItem[autoDST].highLimit = 1;
  menuItem[autoDST].increment = 1;
  menuItem[autoDST].value = 1;  

  strncpy(menuItem[DSTstartWeek].name, "DST start week", 15);
  strncpy(menuItem[DSTstartWeek].settingName[0], "Last", 5);   
  strncpy(menuItem[DSTstartWeek].settingName[1], "First", 6);   
  strncpy(menuItem[DSTstartWeek].settingName[2], "Second", 7); 
  strncpy(menuItem[DSTstartWeek].settingName[3], "Third", 6);   
  strncpy(menuItem[DSTstartWeek].settingName[4], "Fourth", 7);   
  menuItem[DSTstartWeek].lowLimit = 0;
  menuItem[DSTstartWeek].highLimit = 4;
  menuItem[DSTstartWeek].increment = 1;
  menuItem[DSTstartWeek].value = 2; 

  strncpy(menuItem[DSTstartDay].name, "DST start day", 14);
  strncpy(menuItem[DSTstartDay].settingName[0], "DOW", 4);       // placeholder, does not display
  strncpy(menuItem[DSTstartDay].settingName[1], "Sunday", 7);   
  strncpy(menuItem[DSTstartDay].settingName[2], "Monday", 7); 
  strncpy(menuItem[DSTstartDay].settingName[3], "Tuesday", 8);   
  strncpy(menuItem[DSTstartDay].settingName[4], "Wednesday", 10);
  strncpy(menuItem[DSTstartDay].settingName[5], "Thursday", 9);
  strncpy(menuItem[DSTstartDay].settingName[6], "Friday", 7);
  strncpy(menuItem[DSTstartDay].settingName[7], "Saturday", 9);   
  menuItem[DSTstartDay].lowLimit = 1;
  menuItem[DSTstartDay].highLimit = 7;
  menuItem[DSTstartDay].increment = 1;
  menuItem[DSTstartDay].value = 1; 

  strncpy(menuItem[DSTstartMonth].name, "DST start month", 16);
  strncpy(menuItem[DSTstartMonth].settingName[0], "Month", 6);       // placeholder, does not display
  strncpy(menuItem[DSTstartMonth].settingName[1], "January", 8);   
  strncpy(menuItem[DSTstartMonth].settingName[2], "February", 9); 
  strncpy(menuItem[DSTstartMonth].settingName[3], "March", 6);   
  strncpy(menuItem[DSTstartMonth].settingName[4], "April", 6);
  strncpy(menuItem[DSTstartMonth].settingName[5], "May", 4);
  strncpy(menuItem[DSTstartMonth].settingName[6], "June", 5);
  strncpy(menuItem[DSTstartMonth].settingName[7], "July", 5); 
  strncpy(menuItem[DSTstartMonth].settingName[8], "August", 7); 
  strncpy(menuItem[DSTstartMonth].settingName[9], "September", 10); 
  strncpy(menuItem[DSTstartMonth].settingName[10], "October", 8); 
  strncpy(menuItem[DSTstartMonth].settingName[11], "November", 9); 
  strncpy(menuItem[DSTstartMonth].settingName[12], "December", 9);   
  menuItem[DSTstartMonth].lowLimit = 1;
  menuItem[DSTstartMonth].highLimit = 12;
  menuItem[DSTstartMonth].increment = 1;
  menuItem[DSTstartMonth].value = 3;

  strncpy(menuItem[DSTstartHour].name, "DST start hour", 15);
  menuItem[DSTstartHour].lowLimit = 0;
  menuItem[DSTstartHour].highLimit = 23;
  menuItem[DSTstartHour].increment = 1;
  menuItem[DSTstartHour].value = 2;

  strncpy(menuItem[DSTendWeek].name, "DST end week", 13);
  strncpy(menuItem[DSTendWeek].settingName[0], "Last", 5);   
  strncpy(menuItem[DSTendWeek].settingName[1], "First", 6);   
  strncpy(menuItem[DSTendWeek].settingName[2], "Second", 7); 
  strncpy(menuItem[DSTendWeek].settingName[3], "Third", 6);   
  strncpy(menuItem[DSTendWeek].settingName[4], "Fourth", 7);   
  menuItem[DSTendWeek].lowLimit = 0;
  menuItem[DSTendWeek].highLimit = 4;
  menuItem[DSTendWeek].increment = 1;
  menuItem[DSTendWeek].value = 1; 

  strncpy(menuItem[DSTendDay].name, "DST end day", 12);
  strncpy(menuItem[DSTendDay].settingName[0], "DOW", 4);       // placeholder, does not display  
  strncpy(menuItem[DSTendDay].settingName[1], "Sunday", 7);   
  strncpy(menuItem[DSTendDay].settingName[2], "Monday", 7); 
  strncpy(menuItem[DSTendDay].settingName[3], "Tuesday", 8);   
  strncpy(menuItem[DSTendDay].settingName[4], "Wednesday", 10);
  strncpy(menuItem[DSTendDay].settingName[5], "Thursday", 9);
  strncpy(menuItem[DSTendDay].settingName[6], "Friday", 7);
  strncpy(menuItem[DSTendDay].settingName[7], "Saturday", 9);   
  menuItem[DSTendDay].lowLimit = 1;
  menuItem[DSTendDay].highLimit = 7;
  menuItem[DSTendDay].increment = 1;
  menuItem[DSTendDay].value = 1; 

  strncpy(menuItem[DSTendMonth].name, "DST end month", 14);
  strncpy(menuItem[DSTendMonth].settingName[0], "Month", 6);       // placeholder, does not display
  strncpy(menuItem[DSTendMonth].settingName[1], "January", 8);   
  strncpy(menuItem[DSTendMonth].settingName[2], "February", 9); 
  strncpy(menuItem[DSTendMonth].settingName[3], "March", 6);   
  strncpy(menuItem[DSTendMonth].settingName[4], "April", 6);
  strncpy(menuItem[DSTendMonth].settingName[5], "May", 4);
  strncpy(menuItem[DSTendMonth].settingName[6], "June", 5);
  strncpy(menuItem[DSTendMonth].settingName[7], "July", 5); 
  strncpy(menuItem[DSTendMonth].settingName[8], "August", 7); 
  strncpy(menuItem[DSTendMonth].settingName[9], "September", 10); 
  strncpy(menuItem[DSTendMonth].settingName[10], "October", 8); 
  strncpy(menuItem[DSTendMonth].settingName[11], "November", 9); 
  strncpy(menuItem[DSTendMonth].settingName[12], "December", 9);   
  menuItem[DSTendMonth].lowLimit = 1;
  menuItem[DSTendMonth].highLimit = 12;
  menuItem[DSTendMonth].increment = 1;
  menuItem[DSTendMonth].value = 11;

  strncpy(menuItem[DSTendHour].name, "DST end hour", 15);
  menuItem[DSTendHour].lowLimit = 0;
  menuItem[DSTendHour].highLimit = 23;
  menuItem[DSTendHour].increment = 1;
  menuItem[DSTendHour].value = 2;

  strncpy(menuItem[autoTZ].name, "Auto Time Zone", 15);
  strncpy(menuItem[autoTZ].settingName[0], "Disabled", 9);   
  strncpy(menuItem[autoTZ].settingName[1], "Enabled", 8);   
  menuItem[autoTZ].lowLimit = 0;
  menuItem[autoTZ].highLimit = 1;
  menuItem[autoTZ].increment = 1;
  menuItem[autoTZ].value = 1;  
  
  // This must be the final menu item. Shows display type along with network info on OLED
  strncpy(menuItem[networkInfo].name, displayName, 21);     
  menuItem[networkInfo].lowLimit = displayType;
  menuItem[networkInfo].highLimit = displayType;
  menuItem[networkInfo].increment = 0;
  menuItem[networkInfo].value = displayType;
}

/*******************************************************************************************************************************************
* Function that can be used for testing during development - it should display 0 to 5 across the digit modules in the specified hue         
********************************************************************************************************************************************/

void testDisplay(uint8_t hsvHue)
{  
  CHSV displayColor = CHSV(hsvHue, 255, 255);  
  uint8_t address;
     
  // Clear the array so panels previously turned on won't show  
  for (address = 0; address < numModules * ledsPerModule; address++) ledArray[address] = CRGB(0, 0, 0); 

  for (uint8_t i = 0; i < numModules; i++) 
  {
    address = i * ledsPerModule + panelOrder[i];                       
    ledArray[address] = displayColor;              
    if (displayType != 3) ledArray[address + 10] = displayColor; 
  }   

  FastLED.show(); 
  delay(10);   
} 

/*******************************************************************************************************************************************
* SPIFFS utilities (LittleFS)                      
********************************************************************************************************************************************/

bool readFile(fs::FS &fs, char *fileContent, size_t fileSize, const char *path)
{
  memset(fileContent, '\0', fileSize);   
  Serial.printf("Reading SPIFFS file: %s\r\n", path);
  File file = fs.open(path);

  if (!file || file.isDirectory())
  {
    Serial.println("- failed to open file for reading");
    return false;
  }   
  if (file.read((uint8_t *)fileContent, fileSize))
  { 
    Serial.println("- file read");
    file.close();
    return true;
  }  
   else 
  {
    Serial.println("- file read failed");
    file.close();
    return false;
  }  
}

bool writeFile(fs::FS &fs, const char *path, const char *message)
{
  Serial.printf("Writing SPIFFS file: %s\r\n", path);
  File file = fs.open(path, FILE_WRITE);

  if (!file || file.isDirectory())
  {
    Serial.println("- failed to open file for writing");
    return false;
  }
  if (file.print(message)) 
  {
    Serial.println("- file written");
    file.close();
    return true;
  }
  else 
  {
    Serial.println("- file write failed");
    file.close();
    return false;
  }  
}

/*******************************************************************************************************************************************
* End                        
********************************************************************************************************************************************/
