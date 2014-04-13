/*
* TX Node for Teleinformation EDF (compteur type A14C5)
 *
 * Info from http://blog.cquad.eu/2012/02/02/recuperer-la-teleinformation-avec-un-arduino/
 * Doc from EDF http://norm.edf.fr/pdf/HN44S812emeeditionMars2007.pdf
 */

#include <Manchester.h>
#include <SoftwareSerial.h>
#include <avr/wdt.h>
#include <avr/power.h>
#include <avr/interrupt.h>
#include <avr/sleep.h>

#if defined(__AVR_ATtiny85__) // ATtiny85 mode
#define ECO // Low Power Mode

#define rxSerialPin 0  // RX PIN for Software Serial
#define txSerialPin 1  // TX PIN for Software Serial (unused)

#define txPin 2  // TX Out
//#define powerPin 1 // Power line for Tx share the same TxSerialPin

#else
#define DEBUG  // Is serial debug active ?

#define rxSerialPin 2  // RX PIN for Software Serial
#define txSerialPin 3  // TX PIN for Software Serial (unused)

#define txPin 8  // TX Out

#define rxLed 13  // RX LED
#define flagLed 7  // FLAG LED
#define powerPin 6 // Power line for Tx module

#endif

#define WATCHDOG_TIMER WDTO_8S // 8 sec
#define SLEEP_LOOP  30 // Deep Sleep total time = WATCHDOG_TIMER * SLEEP_LOOP
#define LINK_TIMEOUT  5000 // 5sec timeout

#define sensID 5  // Sensor ID

// Sensor Message Structure
typedef struct sensorData_t{
  uint8_t id; // size 1
  float value; // size 4
  uint8_t checksum; // size 1
};

typedef union sensorData_union_t{
  sensorData_t data;
  uint8_t raw[6]; // total size of 6 bytes
};

// Sensor Message
sensorData_union_t _message; 

// WatchDog Counter
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

  // LED Pin
  pinMode(rxLed, OUTPUT); 
  pinMode(flagLed, OUTPUT); 
#endif

  // Write Sensor ID
  _message.data.id = sensID;
}

void loop()
{
  // Acquire And Send Apparent Power  
  getAndSendApparentPower();

  // Power Off
  powerDown();

  // Go to Sleep Mode Zzzzz
  deepSleep();
}

/*
* Get then Send Apparent Power
 */
void getAndSendApparentPower()
{
  // EDF Link
  SoftwareSerial edfLink(rxSerialPin, txSerialPin); // rx, tx pins
  boolean insideLine = false;
  char buffer[40];
  int lenght = 0; // Buffer pointer

  // Start Serial with EDF box
  edfLink.begin(1200);
  
  // Start if the acquisition loop
  unsigned long beginSerial = millis();
  
  // Loop
  while (millis() - beginSerial < LINK_TIMEOUT)
  {
    // Read Serial
    int current = edfLink.read();

    if(current < 0 )
    {
      // Read Nothing
#ifdef DEBUG
      digitalWrite(rxLed, LOW);
#endif
    }
    else
    {
#ifdef DEBUG
      digitalWrite(rxLed, HIGH);
#endif

      // Convert Read Char
      char value = current & 0x7F;

      if (value==0x0A)
      {
        // It's the begin of a information line

#ifdef DEBUG
        digitalWrite(flagLed, HIGH);
#endif

        // Turn Flag
        insideLine = true;

        // Clear buffer pointer
        lenght = 0;
      }
      else if (value==0x0D)
      {
        // It's the end of a information line

#ifdef DEBUG
        digitalWrite(flagLed, LOW);
#endif

        // Find Instant Power information
        // This message have at least 10 char
        // pattern : PAPP XXXXX {checksum}
        if(lenght >= 10)
        {
          // Find PAPP value in line
          if(buffer[0]=='P' && buffer[1]=='A' && buffer[2]=='P' && buffer[3]=='P')
          {
            // Stop link interruption
            edfLink.end();

            // Remove CheckSum
            buffer[10] = '\0';

            // Convert to Float
            float value = atof(buffer+5);

            // New Value Event
            fireNewValueEvent(value);

            // exit
            return;
          }
        }

        // Turn Off Flag
        insideLine = false;

        // Clear buffer
        lenght = 0;

        // Clear Link
        edfLink.flush();
      }
      else
      {
        // Data
        if(insideLine && lenght < 39)
        {
          // Add Char
          buffer[lenght++] = value;
        }
      }
    }
  }
}

void fireNewValueEvent(float value)
{
  // Save Power
  _message.data.value = value;

  // Compute Checksum : CRC8 on ID+Value (Checksum is computed on the first 5 bytes)
  _message.data.checksum = crc8(_message.raw, 5);

  // TX Setup
  man.setupTransmit(txPin);
  
  // Send All Data
  man.transmitArray(6, _message.raw);
}



//
// Shut down power line
void powerDown()
{
  // Power Down
  //digitalWrite(powerPin,LOW);

  // Set all pin in INPUT mode
  //pinMode(powerPin, INPUT);
  pinMode(txPin, INPUT);
  pinMode(rxSerialPin, INPUT);
  //pinMode(txSerialPin, INPUT);
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
  
  //PCMSK = 0x00;   // Disable all PCInt Interrupts 
  //GIMSK = 0x00; // Disable 
  bitClear(GIMSK, PCIE); // Disable all PCInt Interrupts 


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
  
  // Restore PCINT
  bitSet(GIMSK, PCIE);
  
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

// This table comes from Dallas sample code where it is freely reusable,
// though Copyright (C) 2000 Dallas Semiconductor Corporation
static const uint8_t PROGMEM dscrc_table[] = {
  0, 94,188,226, 97, 63,221,131,194,156,126, 32,163,253, 31, 65,
  157,195, 33,127,252,162, 64, 30, 95,  1,227,189, 62, 96,130,220,
  35,125,159,193, 66, 28,254,160,225,191, 93,  3,128,222, 60, 98,
  190,224,  2, 92,223,129, 99, 61,124, 34,192,158, 29, 67,161,255,
  70, 24,250,164, 39,121,155,197,132,218, 56,102,229,187, 89,  7,
  219,133,103, 57,186,228,  6, 88, 25, 71,165,251,120, 38,196,154,
  101, 59,217,135,  4, 90,184,230,167,249, 27, 69,198,152,122, 36,
  248,166, 68, 26,153,199, 37,123, 58,100,134,216, 91,  5,231,185,
  140,210, 48,110,237,179, 81, 15, 78, 16,242,172, 47,113,147,205,
  17, 79,173,243,112, 46,204,146,211,141,111, 49,178,236, 14, 80,
  175,241, 19, 77,206,144,114, 44,109, 51,209,143, 12, 82,176,238,
  50,108,142,208, 83, 13,239,177,240,174, 76, 18,145,207, 45,115,
  202,148,118, 40,171,245, 23, 73,  8, 86,180,234,105, 55,213,139,
  87,  9,235,181, 54,104,138,212,149,203, 41,119,244,170, 72, 22,
  233,183, 85, 11,136,214, 52,106, 43,117,151,201, 74, 20,246,168,
  116, 42,200,150, 21, 75,169,247,182,232, 10, 84,215,137,107, 53};

//
// Compute a Dallas Semiconductor 8 bit CRC. These show up in the ROM
// and the registers.  (note: this might better be done without to
// table, it would probably be smaller and certainly fast enough
// compared to all those delayMicrosecond() calls.  But I got
// confused, so I use this table from the examples.)
//
uint8_t crc8(const uint8_t *addr, uint8_t len)
{
  uint8_t crc = 0;

  while (len--) {
    crc = pgm_read_byte(dscrc_table + (crc ^ *addr++));
  }
  return crc;
}






