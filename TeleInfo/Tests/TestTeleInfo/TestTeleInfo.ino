/*
* Test Teleinformation EDF compteur type A14C5
*
* Info from http://blog.cquad.eu/2012/02/02/recuperer-la-teleinformation-avec-un-arduino/
* Doc from EDF http://norm.edf.fr/pdf/HN44S812emeeditionMars2007.pdf
*/

#include <SoftwareSerial.h>

SoftwareSerial _edfSerial(2, 3); // rx, tx pins

void setup() {
        Serial.begin(1200);
        
        _edfSerial.begin(1200);
        
        Serial.println("---- Hello ----");
}

void loop() {
  if (_edfSerial.available())
    Serial.write(_edfSerial.read() & 0x7F);
}

