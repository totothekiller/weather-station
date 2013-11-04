// TX Sensor


// Info : 

// Snootlab : http://forum.snootlab.com/viewtopic.php?f=38&t=399
// Virtual Wire library : http://www.airspayce.com/mikem/arduino/VirtualWire/VirtualWire_8h.html
// http://www.pjrc.com/teensy/td_libs_VirtualWire.html


#include <VirtualWire.h>
#include <OneWire.h>
#include "ReadTempLib.h"

int txLed = 13; // TX LED
int txPin = 12; // TX Out
int sensPin = 7; // Sensor Pin
uint8_t sensID = 42; // Sensor ID

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
sensorData_union_t message; 

// Sensor Library
ReadTempLib sensor(sensPin);

void setup()
{
  // Debug
  Serial.begin(9600);
  Serial.println("setup");
  
  // Led Setup
  pinMode(txLed, OUTPUT);

  // TX Setup
  vw_set_tx_pin(txPin);
  vw_setup(2000);
  
  // Write Sensor ID
  message.data.id = sensID;
}

void loop()
{
  digitalWrite(txLed, HIGH);

  // Read Temp
  if ( sensor.acquireTemp())
  {  
    // set Temperature into message
    message.data.value=sensor.getTemp();
    
    if(Serial)
    {
      Serial.print("Sensor=");
      Serial.print(message.data.id);
      Serial.print(", Temp=");
      Serial.println(message.data.value);
    }
    
    // Send
    vw_send(message.raw, 5);
    vw_wait_tx(); // Wait until the whole message is gone
  }


  digitalWrite(txLed, LOW);    // sets the LED off

  // Sleep
  delay(5000);
}







