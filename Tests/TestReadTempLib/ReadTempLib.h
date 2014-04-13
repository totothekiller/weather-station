#ifndef ReadTempLib_h
#define ReadTempLib_h

#include <OneWire.h>


//
// Simple Read Temperature
class ReadTempLib
{

private:
  OneWire * _wire;
  float _temp;

public:
  // OneWire Pin setting
  ReadTempLib(uint8_t pin);

  // Read temperature from sensor
  // return True if OK
  boolean acquireTemp();

  // Return temperature
  float getTemp();

};


#endif

