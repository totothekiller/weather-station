
#include <VirtualWire.h>

// Sensor Message Structure
typedef struct sensorData_t{
  byte id; // size 1
  float value; // size 4
};

typedef union sensorData_union_t{
  sensorData_t data;
  uint8_t raw[5]; // total size of 5 bytes
};

#define txLed 1  // TX LED
#define txPin 0 // TX Out

#define sensID 5  // Sensor ID

// Sensor Message
sensorData_union_t _message; 


void setup() {
  // Led Setup
  pinMode(txLed, OUTPUT);
  
  // randomSeed(analogRead(0));
  
    // TX Setup
  vw_set_tx_pin(txPin);
  vw_setup(2000);
  
    // Write Sensor ID
  _message.data.id = sensID;

  // Save temperature
  _message.data.value = 42.00;
  
  }

void loop() {
  // put your main code here, to run repeatedly: 
    digitalWrite(txLed, HIGH);
    
 
  
_message.data.value = _message.data.value +1;
  
  //random(1000000)/100.00 ;

    // Send
    vw_send(_message.raw, 5);
    vw_wait_tx(); // Wait until the whole message is gone
    
     digitalWrite(txLed, LOW);
         delay(10000);
    
}
