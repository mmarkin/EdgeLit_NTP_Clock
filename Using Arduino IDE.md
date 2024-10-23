
This firmware is best edited and compiled using PlatformIO. However, with a bit of doing the Arduino IDE (versions1.8.19 or 2.3.3) can be used. This has been tested on both versions of the IDE and it worked. But as they say, “Your mileage may vary.”
This guide assumes that you are familiar with the operation of the Arduino IDE.

The latest version of the code is available from this repository. Just click the “Code” button to download the repository to a local directory (choose the “Download Zip” option then unzip the file you receive). 
The resulting EdgeLit_NTP_Clock-main directory is set up for PlatformIO, so you will have to do some re-arranging to use it with the Arduino IDE:
- Move the main.cpp file from the src sub-directory to the main directory and rename it EdgeLit_NTP_Clock-main.ino.
  Ignore the warning that comes up. Changing the file's name and its extension is necessary. 
- Delete the src subdirectory.
- Move the definitions.h and version.h files from the includes sub-directory to the main directory. Don't change the names.
- You can delete the platformio.ini file if you want. 

If the code was supplied on a flash drive or other media, all the files the Arduino IDE needs for this project are in the EdgeLit_NTP_Clock-main directory. It's ready to go so no changes need to be made. 

You won't have to change anything in any of these files unless you want to modify the firmware. 

To compile the code, first make sure the ESP32 extensions are installed in the IDE and they are up to date. 
Then the following libraries have to be installed. Use the IDE's Library Manager to search for them one by one, select the appropriate version, and click install:  

- Tzapu WiFiManager v2.0.17
- Matthias Hertel OneButton v2.5.0   
- ThingPulse ESP8266 and ESP32 driver for SSD1306 displays v4.5.0
- Sparkfun TMP102 Breakout v1.1.2
- Daniel Garcia FastLED v3.7.0
- Michael Margolis Time v1.6.1  (you may also see it credited to Paul Stoffregen) 
- Jack Christensen Timezone v1.2.4
- Mitch Markin GeoIP v1.3.2

As of this writing, these are the current versions of the libraries. Higher versions will probably work, but if there is trouble compiling the code use the versions that were specified.

Open the EdgeLit_NTP_Clock-main.ino file in the Arduino IDE.

In theTools menu set the following items:
- Set the Board to ESP 32 Dev Module.
- Set the Port to whichever USB port the board is plugged into.
- Set Erase All Flash Before Sketch Upload to Enabled the first time the code is uploaded.
  This will make sure the SPIFFS file system is formatted properly so the firmware will work.
  It can be set to Disabled for uploads after the first one.  
- Make sure the Partition Scheme is set to Default 4MB With SPIFFS

The rest of the default settings should be OK.

PlatformIO would have found and installed all the libraries and done all the setup by itself!

Click the Check-mark icon and hopefully the code will be compiled. This takes while so just be patient! 
Some warnings might show up but they can be ignored as long as everything compiled without errors. 
If you are feeling lucky just click the Arrow icon and it should upload to the ESP32. 

When the upload finishes the firmware should start running and you can follow the instructions in the Quick Start First Time Setup Guide in the manual. The IDE's serial monitor can be opened to check that everything is working properly (see the Serial Monitor menu item in the manual).
