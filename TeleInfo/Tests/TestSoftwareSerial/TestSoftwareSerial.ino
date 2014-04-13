
#include <SoftwareSerial.h>


#define rxSerialPin 2  // RX PIN for Software Serial
#define txSerialPin 3  // TX PIN for Software Serial (unused)

#define txLed 7  // TX LED
#define koLed 13  // KO LED

// EDF Link
SoftwareSerial _edfLink(rxSerialPin, txSerialPin); // rx, tx pins

void setup() {
  Serial.begin(1200);
  Serial.println("--- SETUP ---");

  pinMode(txLed, OUTPUT); 
  pinMode(koLed, OUTPUT); 
  digitalWrite(txLed, LOW);
 digitalWrite(koLed, LOW);

  // Start Serial with EDF box
  _edfLink.begin(1200);

}

void loop() {


  int readV = _edfLink.read();

  if(readV<0)
  {
    flash(koLed);
  } 
  else
  {
  // Turn LED On
    flash(txLed);
  }


}


void flash(int pin)
{
  digitalWrite(pin, HIGH);
  delay(10);
  digitalWrite(pin, LOW);
  delay(20);
}


void test1()
{
  int readV = _edfLink.read();

  if(readV >= 0)
  {
    // Turn LED On
    digitalWrite(txLed, HIGH);

    Serial.print("<-");
    Serial.write(readV);
    Serial.print("<-0x");
    Serial.print(readV, HEX);
    Serial.print("<-");
    Serial.print(readV, BIN);
    Serial.println("<-");


    _edfLink.write(readV);


    delay(100);

    // Turn LED Off
    digitalWrite(txLed, LOW);
  }
}

