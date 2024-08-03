This firmware is best edited and compiled using PlatformIO. However, with a bit of doing the Arduino IDE (versions1.8 or 2.3) can be used.    
This has been tested on both versions of the IDE and it worked. But as they say, “Your mileage may vary.”    
This guide assumes that you are familiar with the operation of the Arduino IDE.   

First make sure the ESP32 extensions are installed in the IDE and they are up to date. Then the following libraries have to be installed.    
Use the IDE's Library Manager to search for them one by one, select the appropriate version, and click install.     

Tzapu WiFiManager v2.0.17   
Matthias Hertel OneButton v2.5.0   
ThingPulse ESP8266 and ESP32 driver for SSD1306 displays v4.5.0   
Sparkfun TMP102 Breakout v1.1.2   
Daniel Garcia FastLED v3.7.0   
Michael Margolis Time v1.6.1  (you may also see it credited to Paul Stoffregen)    
Jack Christensen Timezone v1.2.4   
Mitch Markin GeoIP v1.2.6   

As of this writing, these are the current versions of the libraries. Higher versions will probably work,    
but if there is trouble compiling the code, use the specified versions.

Create a local directory called EdgeLit_NTP_Clock_v4.4.4_Arduino.    
Copy the main.cpp file from the src directory in this repository to your EdgeLit_NTP_Clock_v4.4.4_Arduino directory.    
Rename the file EdgeLit_NTP_Clock_v4.4.4_Arduino.ino.    
Of course, you can call the directory anything you want as long as the .ino file has the same name.   

Copy the definitions.h and version.h files from the includes directory in this repository to your EdgeLit_NTP_Clock_v4.44_Arduino directory.    
Leave the names as they are.    

Open the EdgeLit_NTP_Clock_v4.44_Arduino.ino file in the Arduino IDE.  

Add one of these two statements to the definitions.h file:   
#define LED_ON HIGH   
#define LED_ON LOW   
The statement tells the firmware whether the blue LED on the ESP board is lit by writing a HIGH or a LOW to its GPIO pin. Most boards are HIGH.
See "Time Sync Indication" in the Edge-Lit Manual for details.
The statement can go anywhere in the file but at the bottom of the other #defines is best.    

In the Tools menu set the following items:   
Set the Board to ESP 32 Dev Module.   
Set the Port to whichever USB port the board is plugged into.   
Set Erase All Flash Before Sketch Upload to Enabled the first time the code is uploaded.    
  This will make sure the SPIFFS file system is formatted properly so the firmware will work.    
  It can be set to Disabled for uploads after the first one.     
Make sure the Partition Scheme is set to Default 4MB With SPIFFS.   
The rest of the default settings should be OK.   

PlatformIO would have found and installed all the libraries and done all the setup by itself.   

Click the Check-mark icon and a few minutes later the code should be compiled. Some warnings might show up but they can be ignored
as long as everything compiled without errors.
If you are feeling lucky just click the Arrow icon and it should compile the code and upload it to the ESP32.   
