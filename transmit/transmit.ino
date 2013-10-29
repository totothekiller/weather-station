// TX Sensor


// Info : 

// Snootlab : http://forum.snootlab.com/viewtopic.php?f=38&t=399
// Virtual Wire library : http://www.airspayce.com/mikem/arduino/VirtualWire/VirtualWire_8h.html
// http://www.pjrc.com/teensy/td_libs_VirtualWire.html


#include <VirtualWire.h>

int led1 = 3; // LED1
int led2 = 4; // LED2
int txPin = 0; // TX Out

void setup()
{
  pinMode(led1, OUTPUT);
  pinMode(led2, OUTPUT);
  
  digitalWrite(led2, HIGH);   // sets the LED on
  
  vw_setup(2000);// Bits par seconde (vous pouvez le modifier mais cela modifiera la portée). Voir la documentation de la librairie VirtualWire.
  vw_set_tx_pin(txPin);// Attiny pin 1 (with PWM)
  
  digitalWrite(led2, LOW);   // sets the LED on
}

void loop()
{
  digitalWrite(led1, HIGH);   // sets the LED on
  
  const char *msg = "TTK is here";

 
  vw_send((uint8_t *)msg, strlen(msg));
  vw_wait_tx(); // Wait until the whole message is gone



  digitalWrite(led1, LOW);    // sets the LED off
  
  // Sleep
  delay(1000);
}

