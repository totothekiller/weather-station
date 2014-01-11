/*
* TX Node for Teleinformation EDF (compteur type A14C5)
 *
 * Info from http://blog.cquad.eu/2012/02/02/recuperer-la-teleinformation-avec-un-arduino/
 * Doc from EDF http://norm.edf.fr/pdf/HN44S812emeeditionMars2007.pdf
 */

#include <SoftwareSerial.h>


#define rxSerialPin 2  // RX PIN for Software Serial
#define txSerialPin 3  // TX PIN for Software Serial

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

SoftwareSerial _edfLine(rxSerialPin, txSerialPin); // rx, tx pins

void setup() {
  Serial.begin(9600);

  _edfLine.begin(1200);

  Serial.println("---- Hello ----");
}

void loop() {
  // Read Apparent Power
  acquireApparentPower();
}


/*
*
 * Read Temp
 *
 */
boolean acquireApparentPower()
{
  boolean exit = false;
  boolean insideLine = false;
  char current;
  char buffer[50];

  while(!exit)
  {
    // Read
    current = _edfLine.read();

    // Check Value
    if(current>0)
    {
      // Clean Value
      current &=0x7F;  
      
      // Display Raw
      Serial.print("Raw :<");
      Serial.write(current);
      Serial.println('>');

      // Split String between LF and CR
      if(current==0x0A)
      {
        // Begin new Information Line
    
        // Turn Flag
        insideLine = true;
        
        // Clear buffer
        buffer[0] = '\0';
      }
      else if(current==0x0D)
      {
        // End of Information Line
        
        // Turn Off Flag
        insideLine = false;
        
        // TODO Analyse Line
        Serial.print("Data :");Serial.println(buffer);
      }
      else
      {
        // Data
        if(insideLine)
        {
           // Add Char
          strncpy (buffer, &current, 1);
        }
      }



    }



  }







}


