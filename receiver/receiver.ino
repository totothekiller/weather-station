//
// Test RX
//

#include <VirtualWire.h>

int rxPin = 11;
int rxLed = 13;

void setup()
{
  Serial.begin(9600);	// Debugging only
  Serial.println("setup");

  // Initialise the IO and ISR
  pinMode(rxLed, OUTPUT);  

  vw_setup(2000);	 // Bits per sec
  vw_set_rx_pin(rxPin); 
  vw_rx_start();       // Start the receiver PLL running


}

void loop()
{
  uint8_t buf[VW_MAX_MESSAGE_LEN];
  uint8_t buflen = VW_MAX_MESSAGE_LEN;

  if (vw_get_message(buf, &buflen)) // Non-blocking
  {
    int i;

    digitalWrite(rxLed, HIGH); // Flash a light to show received good message
    // Message with a good checksum received, dump it.
    Serial.print("Got: ");

    for (i = 0; i < buflen; i++)
    {
      Serial.print(buf[i], HEX);
      Serial.print(" ");
    }
    Serial.println("");
    digitalWrite(rxLed, LOW);
  }
}

