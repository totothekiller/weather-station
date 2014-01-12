/*
* TX Node for Teleinformation EDF (compteur type A14C5)
 *
 * Info from http://blog.cquad.eu/2012/02/02/recuperer-la-teleinformation-avec-un-arduino/
 * Doc from EDF http://norm.edf.fr/pdf/HN44S812emeeditionMars2007.pdf
 */

#include <SoftwareSerial.h>
#include <VirtualWire.h>
#include <avr/wdt.h>
#include <avr/power.h>
#include <avr/interrupt.h>
#include <avr/sleep.h>


#undef  ATtiny85 // ATtiny85 mode

#ifdef ATtiny85
#define ECO // Low Power Mode

#define rxSerialPin 2  // RX PIN for Software Serial
#define txSerialPin 3  // TX PIN for Software Serial

#define txLed 3  // TX LED
#define txPin 1  // TX Out
#define powerPin 4 // Power line for Tx module and sensor

#else
#define DEBUG  // Is serial debug active ?

#define rxSerialPin 2  // RX PIN for Software Serial
#define txSerialPin 3  // TX PIN for Software Serial

#define txLed 13  // TX LED
#define txPin 12  // TX Out
#define powerPin 14 // Power line for Tx module and sensor

#endif

#define WATCHDOG_TIMER WDTO_8S // 8 sec
#define SLEEP_LOOP  2 // Deep Sleep total time = WATCHDOG_TIMER * SLEEP_LOOP

#define sensID 5  // Sensor ID


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

// EDF Link
SoftwareSerial _edfLine(rxSerialPin, txSerialPin); // rx, tx pins

// WatchDof Counter
volatile byte _watchdogCounter;

// WatchDog interruption
ISR(WDT_vect) {
  _watchdogCounter++;
}

void setup() {
  
    // Debug
#ifdef DEBUG
  Serial.begin(57600);
  Serial.println("--- SETUP ---");
#endif

  // Start Serial with EDF box
  _edfLine.begin(1200);
  
  // Write Sensor ID
  _message.data.id = sensID;
}

void loop()
{
  // Power On !
  powerUp();

  // Acquire Apparent Power
  if (acquireApparentPower())
  {  
    // Turn LED On
    digitalWrite(txLed, HIGH);

#ifdef DEBUG
    Serial.print("Sensor="); 
    Serial.print(_message.data.id); 
    Serial.print(", Value="); 
    Serial.println(_message.data.value);
#endif

    // Send
    vw_send(_message.raw, 5); // Send Raw message (size = 5 bytes)
    vw_wait_tx(); // Wait until the whole message is gone
  }

  // sets the LED off
  digitalWrite(txLed, LOW);   

  // Power Off
  powerDown();

  // Go to Sleep Mode Zzzzz
  deepSleep();
}


/*
*
 * Acquire Apparent Power
 *
 */
boolean acquireApparentPower()
{
  boolean exit = false;
  boolean insideLine = false;
  char current;
  char buffer[40];

  // Clear buffer
  buffer[0] = '\0';

  while(!exit)
  {
    // Wait a Char
    if(_edfLine.available())
    {
      // Read
      current = _edfLine.read();

      // Clean Value
      current &=0x7F;  

      // Display Raw
      //Serial.print("Raw :<"); Serial.write(current); Serial.println('>'); Serial.println(current,HEX);

      // Split String between LF and CR
      if (current==0x0A)
      {
        // Begin new Information Line

        // Turn Flag
        insideLine = true;

        // Clear buffer
        buffer[0] = '\0';
      }
      else if (current==0x0D)
      {
        // End of Information Line

        // Turn Off Flag
        insideLine = false;

#ifdef DEBUG
        //Serial.print("Data : <"); Serial.print(buffer); Serial.println(">");
#endif

        char * papp;
        papp = strstr(buffer,"PAPP");

        if (papp!=NULL)
        {
          // Remove CheckSum
          papp[10] = '\0';
          
          // Convert to Float
          float value = atof(papp+5);

#ifdef DEBUG
          Serial.print("PAPP = <"); Serial.print(papp); Serial.println(">");
          Serial.print("PAPP = <"); Serial.print(value); Serial.println(">");
#endif

            // Save Power
            _message.data.value = value;
          
            // Exit
            return true;
        }

      }
      else
      {
        // Data
        if(insideLine)
        {
          // Add Char
          strncat(buffer, &current, 1);
        }
      }
    }
  }

  return false;
}

// 
// Power Up Tx Module and sensor
// Turn txLed ON
void powerUp()
{
  // Power On
  pinMode(powerPin, OUTPUT);
  digitalWrite(powerPin,HIGH);
  
  delay(10);

  // Led Setup
  pinMode(txLed, OUTPUT);

  // TX Setup
  vw_set_tx_pin(txPin);
  vw_setup(2000); // Transmition speed
}

//
// Shut down power line
void powerDown()
{
  // Power Down
  digitalWrite(powerPin,LOW);

  // Set all pin in INPUT mode
  pinMode(powerPin, INPUT);
  pinMode(txLed, INPUT);
  pinMode(txPin, INPUT);
}


//
// Put ATtiny85 in deep sleep mode 
//
// Power is reduce to :
//    - ~7µA BOD disabled
//    - ~30µA BOD enabled
//
void deepSleep()
{
#ifndef ECO
  delay(8000*SLEEP_LOOP);
#else
  
  //
  // Prepare for shutdown
  
  bitClear(ADCSRA,ADEN); //Disable ADC
  //bitSet(PRR,PRADC); not working...
  //power_adc_disable(); not working...

  ACSR = (1<<ACD); //Disable the analog comparator
  DIDR0 = 0x3F; //Disable digital input buffers on all ADC0-ADC5 pins

  PRR = 0x0F; //Reduce all power right before sleep
  
  
  //
  // Start Watchdog
  setup_watchdog(WATCHDOG_TIMER);
  
  //
  // Prepare for Sleep
  
  while(_watchdogCounter<SLEEP_LOOP){
  
  set_sleep_mode(SLEEP_MODE_PWR_DOWN); // sleep mode
  
  noInterrupts();
  sleep_enable();
  // Disable BOD : my ATtiny85 doesn't have this fonctionality ...
  interrupts();
  
  
  sleep_cpu();                   // System sleeps here
  sleep_disable();               // System continues execution here when watchdog timed out 
  
  } 
  
  //
  // Stop WatchDog
  setup_watchdog(-1);
  
  // Restore Power
  PRR = 0x00; //Restore power reduction
  DIDR0 = 0x00; //Restore digital input buffers on all ADC0-ADC5 pins
#endif
}


#ifdef ECO

//
// Watchdog setup
// From https://github.com/jcw/jeelib/blob/master/Ports.cpp
// 
void setup_watchdog(char mode) {

  // correct for the fact that WDP3 is *not* in bit position 3!
  if (mode & bit(3))
  { 
    mode ^= bit(3) | bit(WDP3);
  }
  // pre-calculate the WDTCSR value, can't do it inside the timed sequence
  // we only generate interrupts, no reset
  byte wdtcsr = mode >= 0 ? bit(WDIE) | mode : 0;
  
  MCUSR &= ~(1<<WDRF);

  noInterrupts();

  WDTCR |= (1<<WDCE) | (1<<WDE); // timed sequence
  WDTCR = wdtcsr;
  
  // Reset counter
  _watchdogCounter=0;
  
  interrupts();
}

#endif




