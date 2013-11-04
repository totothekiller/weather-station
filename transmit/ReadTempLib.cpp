#include "ReadTempLib.h"


ReadTempLib::ReadTempLib(uint8_t pin)
{
  // Init One Wire Lib
  _wire = new OneWire(pin);
}

boolean ReadTempLib::acquireTemp()
{
  byte i;
  byte data[12];
  byte addr[8];
  
  // Reset Search
  _wire->reset_search();
  
  // Get first sensor
  if ( !_wire->search(addr)) {
    Serial.println("No more addresses.");
    
    return false;
  }

  if (OneWire::crc8(addr, 7) != addr[7]) {
    Serial.println("CRC is not valid!");
    return false;
  }

  // Check if chip is DS18B20
  if(addr[0]!=0x28)
  {
    Serial.println("Chip is not DS18B20");
    return false;
  }

  _wire->reset();
  _wire->select(addr);
  _wire->write(0x44,1);// start conversion

  delay(1000);     // maybe 750ms is enough, maybe not

  _wire->reset();
  _wire->select(addr);    
  _wire->write(0xBE);// Read Scratchpad

  for ( i = 0; i < 9; i++) {// we need 9 bytes
    data[i] = _wire->read();
  }

  // Convert the data to actual temperature
  float raw = (data[1] << 8) | data[0];
  
  // Save temperature
  _temp = raw / 16.0;

  return true;
}

float ReadTempLib::getTemp()
{
  return _temp;
}









