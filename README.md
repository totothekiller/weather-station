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


Folder Structure
-------------------------

| Folder            | Description                     |
| ----------------- | ------------------------------- |
| BlinkAttiny85     | First tests with ATtiny85 core  |
| DeepSleep         | Put ATtiny85 into deep sleep    |
| I2C_scanner       | Scan I2C bus                    |
| LCD_display       | LCD Tests                       |
| **MainStation**   | Main Station                    |
| ReadTemp          | Test reading DS18B20            |
| TestReadTempLib   | Lib for reading DS18B20         |
| TouchSensor       | Test for capacitive touch       |
| **Tx_Node**       | Tx Node                         |
| hardware          | Cores for ATtiny                |
| libraries         | Libraries for Arduino IDE       |
| receiver          | RX receiver test                |
| schematic         | All schematics                  |
| testEthernet      | Test of Ethernet                |
| transmit          | TX transmit test                |

