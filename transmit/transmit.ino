// TX Sensor


// Info : 

// Snootlab : http://forum.snootlab.com/viewtopic.php?f=38&t=399
// Virtual Wire library : http://www.airspayce.com/mikem/arduino/VirtualWire/VirtualWire_8h.html


#include <VirtualWire.h>

int ledPin = 3; // LED

void setup()
{
  pinMode(ledPin, OUTPUT);
  
  vw_setup(2000);// Bits par seconde (vous pouvez le modifier mais cela modifiera la portée). Voir la documentation de la librairie VirtualWire.
  vw_set_tx_pin(1);// Attiny pin 1 (with PWM ?)
}

void loop()
{
  const char *msg = "TTK is here";

  digitalWrite(ledPin, HIGH);   // sets the LED on

  vw_send((uint8_t *)msg, strlen(msg));
  vw_wait_tx(); // Wait until the whole message is gone

  digitalWrite(ledPin, LOW);    // sets the LED off
  
  // Sleep
  delay(1000);
}

