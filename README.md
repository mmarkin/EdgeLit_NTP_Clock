This firmware uses an ESP32 development board (devkit) to operate a digital clock using LED numeric displays inspired by Nixie tubes. 

This firmware syncs with a Network Time Protocol (NTP) server over WiFi to automatically set the clock's time and keep it accurate. The firmware can automatically find the local time zone and it also adjusts for Daylight Saving Time on its own. 

Besides the time and date, the clock can also show the temperature.  

The firmware is menu-driven and has numerous settings to control the clock's operation and the display colors, including a Nixie tube simulation. The menu items and their settings are shown numerically on the edge-lit displays. They can also be shown with text on a small OLED display.

All setup is done from the menu except entering the credentials for the WiFi network that the clock will use for time syncs. The firmware gets the ESP32 to serve its own little website for this. The site can be accessed over WiFi from a phone or computer.

Detailed information about the firmware's functionality and the hardware needed for a clock is in the Edge-Lit Manual.

This repository is set up for the PlatformIO IDE. That's the best way to compile or edit the code. Just download the entire repository into a local directory and open that directory in PlatformIO.
The src directory has the main.cpp file. The includes directory has two other files needed, definitions.h and version.h. PlatformIO will automatically find all the libraries needed.    

The Arduino IDE can also be used. The Using Arduino IDE file in this repository will help you get things set up. 

Like Nixie tubes, each display used for the clock has the digits 0 to 9 stacked one in front of another and the appropriate digit is lit to display it. However, the digits are engraved on clear acrylic panels instead of being electrodes shaped like numerals inside a glass tube filled with neon. The panels are edge-lit from the bottom with WS2812B multi-color LEDs to illuminate the engravings. So instead of being high voltage devices that can only show amber digits like Nixie tubes, these displays run on 5 volts and the LEDs can show pretty much any color. 

Several types of displays can be used:     
NixieCron made by Led-Genial (any type, S, M, or L)        
EleksTube made by EleksMaker (R version only, not the IPS version)        
Lixie made by Lixie Labs (original and Lixie II)   

NixieCron displays are available from Led-Genial in Germany, https://www.led-genial.de/LED-Nixie. 
Here is a picture of a clock that uses NixieCron "M" displays running this firmware set for a Nixie tube simulation.

![NixieCron Clock_bb-menor](https://github.com/mmarkin/EdgeLit_NTP_Clock/blob/main/images/NixieCron%20M%20Clock.JPG)     

The EleksTube displays that this firmware works with are available as a clock kit from EleksMaker in China. 
https://elekstube.com/products/elekstube-r-6-bit-kit-electronic-led-luminous-retro-glows-analog-nixie-tube-clock. 
The microcontroller the kit comes with is not an ESP32. It has no WiFi capability so it can't sync to NTP servers unless the clock is connected to a computer with Internet access. Therefore the kit's stock microcontroller board would have to be replaced with an ESP32 module to use this firmware. Here is a picture of an assembled EleksTube Clock kit.

![EleksTube Clock_bb-menor](https://github.com/mmarkin/EdgeLit_NTP_Clock/blob/main/images/EleksTube%20Clock.jpg)      

The original Lixie displays are no longer available. They were replaced by the Lixie II version. They still use acrylic panels and WS2812 LEDs but the panel order and the LED layout are different from the originals. They are also built more like NixieCron displays since the original design had some problems. This firmware can work with Lixie II displays and Lixie Labs still has a site on Tindie to sell them. However, they are very hard to get. If you are adventurous enough to make your own, though, the designs are open source and Lixie Labs freely offers the CAM files to make the panels and circuit boards.
https://www.tindie.com/products/lixielabs/lixie-ii-the-newnixie-for-arduino-digit-kit 

Here is a picture of a clock using original Lixie displays.

![Lixie Clock_bb-menor](https://github.com/mmarkin/EdgeLit_NTP_Clock/blob/main/images/Lixie%20Clock.jpg)





