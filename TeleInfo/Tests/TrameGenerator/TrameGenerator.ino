
#define rxSerialPin 2  // RX PIN for Software Serial
#define txSerialPin 3  // TX PIN for Software Serial (unused)

#define txLed 13  // TX LED

#include <SoftwareSerial.h>

// EDF Link
SoftwareSerial _edfLink(rxSerialPin, txSerialPin); // rx, tx pins

void setup() {
  Serial.begin(57600);
  Serial.println("--- SETUP ---");

  // Start Serial with EDF box
  _edfLink.begin(1200);

}

void loop() {

  int readV = _edfLink.read();

  if(readV >= 0)
  {

    // Turn LED On
    digitalWrite(txLed, HIGH);

    // Echo
    Serial.print("<-");
    Serial.write(readV);
    Serial.print("<-0x");
    Serial.print(readV, HEX);
    Serial.print("<-");
    Serial.print(readV, BIN);
    Serial.println("<-");

    // Turn LED Off
    digitalWrite(txLed, LOW);

  }


  // Send to Tiny
  if(Serial.available())
  {
    // Turn LED On
    digitalWrite(txLed, HIGH);

    int c = Serial.read();


    Serial.print("->");
    Serial.write(c);
    Serial.print("->");
    Serial.print(c, BIN);
    Serial.print("->0x");
    Serial.print(c, HEX);
    Serial.println("->");

    // Echo
    _edfLink.write(c);

    // Turn LED Off
    digitalWrite(txLed, LOW);
    
    delay(10);

  }





}

