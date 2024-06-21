/* 
   On the OLED and serial monitor the version shows as VERSION_MAJOR.VERSION_MINOR.VERSION_BUILD
   eg: 4.12.11
   The version gets stored as an integer in EEPROM
   eg: 4.12.11 is represented as 41211
   Integer variables don't have leading zeros so VERSON_MAJOR won't be stored if it is 0 
   Version shows on the digit modules as VERSION_MAJOR:VERSION_MINOR:VERSIOM_BUILD 
   Two modules are allocated for each designator, therefore values > 99 cannot be displayed on the digit modules
   eg: 121 would show as 21 
*/

#define VERSION_MAJOR 4   
#define VERSION_MINOR 4
#define VERSION_BUILD 4
