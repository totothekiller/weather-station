
//
// Reveiver Side


int rxLed = 13; // RX LED
int rxPin = 11; // RX In


#include <VirtualWire.h>


typedef struct sensorData_t{
  byte id; // size 1
  float value; // size 4
};

typedef union sensorData_union_t{
  sensorData_t data;
  uint8_t raw[5]; // total size of 5 bytes
};

// RX message
uint8_t message[VW_MAX_MESSAGE_LEN];

void setup()
{
  // Led Setup
  pinMode(rxLed, OUTPUT);

  // Debug
  Serial.begin(9600);
  Serial.println("setup");

  // Set Up RX
  vw_set_rx_pin(rxPin);
  vw_setup(2000);
  vw_rx_start();       // Start the receiver
}

void loop()
{
  // Wait Message
  vw_wait_rx();
  
  // message size
  uint8_t messageLen = VW_MAX_MESSAGE_LEN;

  if (vw_get_message(message, &messageLen)) // Read if OK
  {
    digitalWrite(rxLed, HIGH);
    
    // DEBUG
    if(Serial)
    {
      Serial.print("Got: ");

      // HEX DUMP
      for (int i = 0; i < messageLen; i++)
      {
        Serial.print(message[i], HEX);
      }
      Serial.println("");
    }

    // Check size
    if(messageLen>4)
    {
      // Convert Byte to Sensor Data
      sensorData_union_t sensor;

      for (int k=0; k < 5; k++)
      { 
        sensor.raw[k] = message[k];
      }

      Serial.print("Sensor=");
      Serial.print(sensor.data.id);
      Serial.print(", Temp=");
      Serial.println(sensor.data.value);
    } 
    else
    {
      Serial.println("Not enought data in transmition");
      
      // Try to restart RX module
      vw_rx_stop(); 
      delay(1000);
      vw_rx_start();      
    }
    digitalWrite(rxLed, LOW);
  }
}






