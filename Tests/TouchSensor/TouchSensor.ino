#include <CapacitiveSensor.h>

/*
 * Test Touch Sensor
 *
 *  from library : https://github.com/arduino-libraries/CapacitiveSensor
 * tuto : http://playground.arduino.cc//Main/CapacitiveSensor
 *
 * test with 2M resistance
 */


CapacitiveSensor   cs_2_4 = CapacitiveSensor(2,4);        // 10M resistor between pins 4 & 2, pin 4 is sensor pin, add a wire and or foil if desired

void setup()                    
{
  //cs_4_2.set_CS_AutocaL_Millis(0xFFFFFFFF);     // turn off autocalibrate on channel 1 - just as an example
  Serial.begin(9600);
}

void loop()                    
{
  long start = millis();
  long total1 =  cs_2_4.capacitiveSensor(30);

  Serial.print(millis() - start);        // check on performance in milliseconds
  Serial.print("\t");                    // tab character for debug windown spacing

  Serial.println(total1);                  // print sensor output 1

    if(total1>60) // Seuil
  {
    Serial.println("TOUCH !");    
  }


  delay(500);                             // arbitrary delay to limit data to serial port 
}

