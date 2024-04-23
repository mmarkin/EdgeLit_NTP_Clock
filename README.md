This firmware uses an ESP32 development board (devkit) to operate a digital clock using LED numeric displays that were inspired by Nixie tubes. 
ESP32 is a series of low-cost and low-power System-on-a-Chip (SoC) microcontrollers developed by Espressif Systems. They include integrated WiFi capability, dual-core processors, and much more. 
Numerous manufacturers make ESP32 development boards that also include 4MB of flash memory, USB connectivity, a 3.3-volt power regulator, a couple of LEDs, and buttons for reset and programming. This is all packaged on a convenient little circuit board about a third the size of a business card. 


This firmware syncs with a Network Time Protocol (NTP) server over WiFi to automatically set the clock's time and keep it accurate. The firmware can also automatically find the local time zone and it adjusts for Daylight Saving Time on its own, too.

Besides the time and date, the clock can also show the temperature. 

The firmware is menu-driven and has numerous settings to control the clock's operation and the display colors, including a Nixie tube simulation. The menu items and their settings are shown numerically on the edge-lit displays. They can also be shown with text on a small OLED display.

All setup is done from the menu except entering the credentials for the WiFi network that the clock will use for time syncs. The firmware gets the ESP32 to serve its own web pages for this that can be accessed over WiFi from a phone or computer.
Like Nixie tubes, each display used for the clock has the digits 0 to 9 stacked one in front of another and the appropriate digit is lit to display it. However the digits are engraved on clear acrylic panels instead of being electrodes shaped like numerals inside a glass tube filled with neon. The panels are edge-lit from the bottom with WS2812B multi-color LEDs to illuminate the engravings. So instead of being high voltage devices that can only show amber digits like Nixie tubes, these displays run on 5 volts and the LEDs can show pretty much any color. 

Several types of displays can be used:
NixieCron made by Led-Genial (any type, S, M, or L)
EleksTube made by EleksMaker
Lixie made by Connor Nishijima (Lixie Labs)

As of this writing, NixieCron displays are available from Led-Genial in Germany. 
The original EleksTube displays that this firmware works with are no longer available. They have been replaced by a version that uses little IPS LCD screens to display numbers or any other graphics that you care to use. They look a lot like Nixie tubes and if they are showing images of actual Nixie digits you'd have to look closely to know they aren't the real thing. However, the firmware needed to drive EleksTube IPS displays is very different from this. 
The original Lixie displays are no longer available, either. They were replaced by the Lixie II version. They still use acrylic panels and WS2812 LEDs but the panel order and the LED layout are different from the originals. They are also built more like NixieCron displays since the original design had some problems. This firmware can work with Lixie II displays but they are very hard to get. Lixie Labs still has a site on Tindie to sell them, however they have been out of stock since 2020. 
