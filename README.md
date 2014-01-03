weather-station
===============

An Arduino based Weather Station


Remote Sensors
-------------------------

Parts :
* ATtiny85
* DS18B20 (temperature)
* RF transmitter 434 MHz

[Arduino Sketch](Tx_Node/Tx_Node.ino)

[Schematics](schematic/TX-Node.png)


Main Station
-------------------------


Parts :
* Arduino Uno Rev 3
* BMP085 (pression + temperature)
* RF receiver 434 MHz
* ENC28J60 Ethernet LAN
* LCD 16x02 display

[Arduino Sketch](MainStation/MainStation.ino)
