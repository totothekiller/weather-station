//
// TX Sensor
//

// Info : 
// Snootlab : http://forum.snootlab.com/viewtopic.php?f=38&t=399
// Virtual Wire library : http://www.airspayce.com/mikem/arduino/VirtualWire/VirtualWire_8h.html
// http://www.pjrc.com/teensy/td_libs_VirtualWire.html


#include <VirtualWire.h>
#include <OneWire.h>

#define  ATtiny85 // ATtiny85 mode

#ifdef ATtiny85
  #define txLed 3  // TX LED
  #define txPin 1  // TX Out
  #define sensPin 0  // Sensor Pin
#else
  #define DEBUG  // Is serial debug active ?
  
  #define txLed 13  // TX LED
  #define txPin 12  // TX Out
  #define sensPin 7  // Sensor Pin
#endif

#define sensID 42  // Sensor ID

#define SENSOR_UPDATE  5000 // Delay

// Sensor Message Structure
typedef struct sensorData_t{
  byte id; // size 1
  float value; // size 4
};

typedef union sensorData_union_t{
  sensorData_t data;
  uint8_t raw[5]; // total size of 5 bytes
};

// Sensor Message
sensorData_union_t _message; 

// OneWire protocol
OneWire _wire(sensPin);

void setup()
{
  // Debug
  #ifdef DEBUG
  Serial.begin(9600);
  Serial.println("--- SETUP ---");
  #endif

  // Led Setup
  pinMode(txLed, OUTPUT);

  // TX Setup
  vw_set_tx_pin(txPin);
  vw_setup(2000);

  // Write Sensor ID
  _message.data.id = sensID;
}

void loop()
{
  digitalWrite(txLed, HIGH);

  // Read Temp
  if (acquireTemp())
  {  
    #ifdef DEBUG
    Serial.print("Sensor="); Serial.print(_message.data.id); Serial.print(", Temp="); Serial.println(_message.data.value);
    #endif

    // Send
    vw_send(_message.raw, 5);
    vw_wait_tx(); // Wait until the whole message is gone
  }


  digitalWrite(txLed, LOW);    // sets the LED off

  // Sleep
  delay(SENSOR_UPDATE);
}


/*
*
* Read Temp
*
*/
boolean acquireTemp()
{
  byte i;
  byte data[12];
  byte addr[8];

  // Reset Search
  _wire.reset_search();

  // Get first sensor
  if ( !_wire.search(addr)) {
    #ifdef DEBUG
    Serial.println("No more addresses.");
    #endif

    return false;
  }

  if (OneWire::crc8(addr, 7) != addr[7]) {
    #ifdef DEBUG
    Serial.println("CRC is not valid!");
    #endif

    return false;
  }

  // Check if chip is DS18B20
  if(addr[0]!=0x28)
  {
    #ifdef DEBUG
    Serial.println("Chip is not DS18B20");
    #endif

    return false;
  }

  _wire.reset();
  _wire.select(addr);
  _wire.write(0x44,1);// start conversion

  delay(1000);     // maybe 750ms is enough, maybe not

  _wire.reset();
  _wire.select(addr);    
  _wire.write(0xBE);// Read Scratchpad

  for ( i = 0; i < 9; i++) {// we need 9 bytes
    data[i] = _wire.read();
  }

  // Convert the data to actual temperature
  float raw = (data[1] << 8) | data[0];

  // Save temperature
  _message.data.value = raw / 16.0;

  return true;
}




