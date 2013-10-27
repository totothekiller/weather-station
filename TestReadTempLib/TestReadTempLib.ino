
#include <OneWire.h>
#include "ReadTempLib.h"


ReadTempLib temp(10); // Init on Port 10

void setup() {
  Serial.begin(9600);
}

void loop() {
  
  // Get Temp
  boolean valid = temp.acquireTemp();

  if(valid)
  {
    float t =  temp.getTemp();
    Serial.print("Temp = ");
    Serial.println(t);
  }

  // Wait 1sec
  delay(1000);
}



