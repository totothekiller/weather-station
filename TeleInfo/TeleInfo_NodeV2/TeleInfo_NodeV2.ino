/*
  TX Node for Teleinformation EDF (compteur type A14C5)

   Info from http://blog.cquad.eu/2012/02/02/recuperer-la-teleinformation-avec-un-arduino/
   Doc from EDF http://norm.edf.fr/pdf/HN44S812emeeditionMars2007.pdf

   New Implementation based on Mini Ultra 8 MHz
   http://www.rocketscream.com/shop/mini-ultra-8-mhz-arduino-compatible

  Select boad : "Arduino Pro or Pro Mini"
  Select processor : "ATmega328 3.3V 8 Mhz"


  Low Power Library by RocketScream
  http://www.rocketscream.com/blog/2011/07/04/lightweight-low-power-arduino-library/

*/

#include <Manchester.h>
#include <LowPower.h>

void setup()
{

}

void loop()
{

    // Enter power down state for 8 s with ADC and BOD module disabled
  LowPower.powerDown(SLEEP_8S, ADC_OFF, BOD_OFF);
  
  // initialize digital pin 13 as an output.
  pinMode(13, OUTPUT);

  digitalWrite(13, HIGH);   // turn the LED on (HIGH is the voltage level)
  delay(1000);              // wait for a second
  digitalWrite(13, LOW);    // turn the LED off by making the voltage LOW
  delay(1000);              // wait for a second
  digitalWrite(13, HIGH);   // turn the LED on (HIGH is the voltage level)
  delay(1000);              // wait for a second
  digitalWrite(13, LOW);    // turn the LED off by making the voltage LOW
  delay(1000);              // wait for a second
  digitalWrite(13, HIGH);   // turn the LED on (HIGH is the voltage level)
  delay(500);              // wait for a second
  digitalWrite(13, LOW);    // turn the LED off by making the voltage LOW
  delay(500); 
  digitalWrite(13, HIGH);   // turn the LED on (HIGH is the voltage level)
  delay(500);              // wait for a second
  digitalWrite(13, LOW);
  delay(200);              // wait for a second
  digitalWrite(13, HIGH);   // turn the LED on (HIGH is the voltage level)
  delay(200);              // wait for a second
  digitalWrite(13, LOW);    // turn the LED off by making the voltage LOW
  delay(200); 
  digitalWrite(13, HIGH);   // turn the LED on (HIGH is the voltage level)
  delay(200);              // wait for a second
  digitalWrite(13, LOW);

    // initialize digital pin 13 as an output.
  pinMode(13, INPUT);
}





