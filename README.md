This firmware uses an ESP32 development board (devkit) to operate a digital clock using LED numeric displays inspired by Nixie tubes. 

Like Nixie tubes, each display used for the clock has the digits 0 to 9 stacked one in front of another and the appropriate digit is lit to display it. However, the digits are engraved on clear acrylic panels instead of being electrodes shaped like numerals inside a glass tube filled with neon. The panels are edge-lit from the bottom with WS2812B multi-color LEDs to illuminate the engravings. So instead of being high voltage devices that can only show amber digits like Nixie tubes, these displays run on 5 volts and the LEDs can show pretty much any color. 

Several types of commercially made displays can be used – NixieCron, EleksTube, and Lixie. Unfortunately, all of these displays are very hard or impossible to get these days so this firmware is mainly intended as an upgrade for the stock firmware in existing clocks. It's also possible to make your own displays but you would need access to a CO2 laser cutter to cut and engrave the digit panels and a good way to solder lots of surface-mount parts to circuit boards. Here is a link to a github repository posted by the designer of Lixie displays, Connor Nishijima. https://github.com/connornishijima/Lixie_II_OSHW. It has lots of information on how to make them including the fabrication files for the digit panels and circuit boards.   

This firmware syncs with a Network Time Protocol (NTP) server over WiFi to automatically set the clock's time and keep it accurate. The firmware can automatically find the local time zone and it also adjusts for Daylight Saving Time on its own. 

Besides the time and date, the clock can also show the temperature.  

The firmware is menu-driven and has numerous settings to control the clock's operation and the display colors, including a Nixie tube simulation. The menu items and their settings are shown numerically on the edge-lit displays. They can also be shown with text on a small OLED display.

All setup is done from the menu except entering the credentials for the WiFi network that the clock will use for time syncs. The firmware gets the ESP32 to serve its own little website for this. The site can be accessed over WiFi from a phone or computer.

Detailed information about the firmware's functionality and the hardware needed for a clock is in the Edge-Lit Manual.

This repository is set up for the PlatformIO IDE. That's the best way to compile or edit the code. Just download the entire repository into a local directory (click "Code" then 
"Download Zip"). Unzip the file received and open the resulting EdgeLit_NTP_Clock-main directory in PlatformIO.
The src sub-directory has the main.cpp file. The include sub-directory has two other files needed, definitions.h and version.h. PlatformIO will automatically find all the libraries needed.    

The Arduino IDE can also be used, but it's not recommended. The Using Arduino IDE file in this repository will help you get things set up. 

Several types of displays can be used:     
NixieCron made by Led-Genial (any type, S, M, or L)        
EleksTube made by EleksMaker (R version only, not the IPS version)        
Lixie made by Lixie Labs (original and Lixie II)

NixieCron   

NixieCron displays are available from Led-Genial in Germany. https://www.led-genial.de/LED-Nixie. Howerver, they have limited international shipping
and they no longer ship to North America.
Here is a picture of a clock that uses NixieCron "M" displays running this firmware set for a Nixie tube simulation.

![NixieCron Clock_bb-menor](https://github.com/mmarkin/EdgeLit_NTP_Clock/blob/main/images/NixieCron%20M%20Clock.JPG)

EleksTube   

The displays that this firmware works with were available as the EleksTube R clock kit from EleksMaker in China. However, that kit has been discontinued and replaced with the R2 version. 
https://elekstube.com/products/elekstube-r2-6-bit-electronic-led-luminous-retro-glows-analog-nixie-tube-clock.
The microcontroller the original kit came with is not an ESP32. It has no WiFi capability, so it can't sync to NTP servers unless the clock is connected to a computer with Internet access. 
Therefore, the kit's stock microcontroller board would have to be replaced with an ESP32 module to use this firmware. The newer R2 version has WiFi to serve a setup menu and sync to NTP servers, 
so it doesn't appear to need this firmware upgrade.  

Here is a picture of an assembled EleksTube Clock kit.

![EleksTube Clock_bb-menor](https://github.com/mmarkin/EdgeLit_NTP_Clock/blob/main/images/EleksTube%20Clock.jpg)     

Lixie   

The original Lixie displays are no longer available. They were replaced by the Lixie II version. They still use acrylic panels and WS2812B LEDs but the panel order and the LED layout are different from the originals. They are also built more like NixieCron displays since the original design had some problems. This firmware can work with either type and Lixie Labs still has a site on Tindie to sell Lixie II displays.  
https://www.tindie.com/products/lixielabs/lixie-ii-the-newnixie-for-arduino-digit-kit. However, they are usually out of stock. If you are adventurous enough to make your own, though, as mentioned earlier, the designs are open source and the fabrication files to make the panels and circuit boards are free to download.    

Here is a picture of a clock using original Lixie displays.

![Lixie Clock_bb-menor](https://github.com/mmarkin/EdgeLit_NTP_Clock/blob/main/images/Lixie%20Clock.jpg)
