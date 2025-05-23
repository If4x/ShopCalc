# ShopCalc
This is a simple web-based shop calculator running on ESP32. It's intended to serve as a basic calculator for school events to optimize the process of selling products timewise but also to minimize calculation errors occurring quite frequently when performed by head by younger students.
It's designed in a way that just takes a stock ESP32-Dev and that's it. No need for soldering, additional components and so on!

It is up to you to use this system as is or to adapt it to yout needs. Please respect the license though!

# Functions
- Utilizes onboard LED for status
- Utilizes oboard WIFI-Module for WIFI connection to client
- Saves Product config to EEPROM so it will be available after powerloss
- When all products are being deleted, program will reset back to default product list that serve as examples. Those examples can be deleted r modified if not wanted
- Shop page is running on port 80 which is default
- Configuation Page is running on port 8080. Has to be manually typed into adress in order to access setup page
- Exremely low powerconsumtion (0,65W on average) for maximum runtime
- Easy to use with any Powerbank or powersupply
- Runs a local network to be independent of network availability at site
- No matter what system you're running (iOS, Andriod, Linux, ...) this tool will work for you! No need to install apps or so, you just need a browser
- Keeps track of all sold items for statistical usage
- Export total sold stock to CSV for statistical usage
- Option to reset EEPROM save of total sold stock to reset before/after an event so statistics are accurate

## Additional 3D-printed case
There are STL files for an additional ESP case that has buttons for EN abd BOOT to be able to restart the ESP32. It also has "air vents" for the Processor due to heat generation caused by using the WIFI module actively.

# Setup
The system is intended to ron on ESP32-Dev. It uses onboard components only to minimize the requiered technical sklills to almost zero.

## First Powerup
1) connect ESP32-Dev to computer
2) Flash main.cpp to ESP32-Dev (recommended using PIO for quick compilation, Arduino IDE works too but slower)
3) Connect your smartphone to wifi (SSID: Kasse | Password: BitteGeld) Can be modified in the main.cpp code at the beginning of the file
4) Go to your browser and ytpe 192.168.4.1:80 into the search bar to access shop page
5) Go to your browser and type 192.168.4.1:80/sales to go to the overview page of sold products where you can export this for statistical usage
6) Go to your browser and type 192.168.4.1:8080 into the search bar to access config page
And you're done! As simple as this!

## After first Powerup
1) Connect ESP32-Dev to Power
2) Connect to wifi (if not modified, default connection see First Powerup step 3)
3) Go to your browser and ytpe 192.168.4.1 into the search bar to access shop page
4) Go to your browser and type 192.168.4.1:8080 into the search bar to access config page
5) Go to your browser and type 192.168.4.1/sales to go to the overview page of sold products where you can export this for statistical usage
And you're done! As simple as this!

# Limitations
- Default max. number of products in the shop (not cart) is 50. This is due to EEPROM optimization but can be modified to include more than 50 products in the code by increasing the MAX_PRODUCTS and EEPROM_SIZE
- Default max. character length of product name is 30 due to EEPROM optimization. Can be increased by increasing "char name[30]" in the Product struct and the EEPROM_SIZE
- Due to using the onboard components, the WIFI range is limited to about 10m line of sight and about to 3m with walls inbetween. To increase this, expand the system with more powerfull components. 

# Comming soon
- Quickreference to config page on shop page with password so no unauthorized person can edit product page
