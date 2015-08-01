//
// TX Node with Temperature + Humidity
//
// Based on Sensor AM2302 (DHT22) http://snootlab.com/adafruit/252-capteur-de-temperature-et-humidite-am2302.html

#include <Manchester.h>
#include <DHT.h>
#include <avr/wdt.h>
#include <avr/power.h>
#include <avr/interrupt.h>
#include <avr/sleep.h>


#define DHTTYPE DHT22   // DHT 22  (AM2302)

#define sensID_Temp 10  // Sensor ID for Temp
#define sensID_Humid 11 // Sensor ID for Humidity

#if defined(__AVR_ATtiny85__) // ATtiny85 mode
#define ECO // Low Power Mode

#define txLed 3  // TX LED
#define txPin 1  // TX Out
#define sensPin 0  // Sensor Pin
#define powerPin 4 // Power line for Tx module and sensor

#else
#define DEBUG  // Is serial debug active ?

#define txLed 13  // TX LED
#define txPin 12  // TX Out
#define sensPin 2  // Sensor Pin
#define powerPin 8 // Power line for Tx module and sensor
#endif

#define WATCHDOG_TIMER WDTO_8S // 8 sec
#define SLEEP_LOOP  4 // Deep Sleep total time = WATCHDOG_TIMER * SLEEP_LOOP

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

// WatchDof Counter
volatile byte _watchdogCounter;

// WatchDog interruption
ISR(WDT_vect) {
  _watchdogCounter++;
}


// Toggle flag : if true => Aquire Temp, if false => Acquire Humidity
boolean _flag = true;

// Initialize DHT sensor for normal 16mhz Arduino
DHT dht(sensPin, DHTTYPE);

void setup() {
    // Debug
  #ifdef DEBUG
    Serial.begin(9600);
    Serial.println("--- SETUP ---");
  #endif

}

void loop()
{
  // Power On !
  powerUp();

  // Turn LED On
  digitalWrite(txLed, HIGH);

  // Read Temp
  if (_flag && acquireTemp())
  {
    // Send !
    fireNewValueEvent();
  }
 
  // Read Humidity
  if (!_flag && acquireHumidity())
  {
    // Send !
    fireNewValueEvent();
  }

  // sets the LED off
  digitalWrite(txLed, LOW);

  // Toggle flag
  toggleFlag();

  // Power Off
  powerDown();

  // Go to Sleep Mode Zzzzz
  deepSleep();
}

// 
// Power Up Tx Module and sensor
// Turn txLed ON
void powerUp()
{
  // Power On
  pinMode(powerPin, OUTPUT);
  digitalWrite(powerPin,HIGH);

  // Led Setup
  pinMode(txLed, OUTPUT);
  
  delay(500);
  
  // DHT Startup
  dht.begin();

  delay(500);
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
  pinMode(sensPin,INPUT);
}

/*
 * Read Temp
 */
boolean acquireTemp()
{
  // Read temperature as Celsius
  float t = dht.readTemperature();
  
  if (isnan(t)){
#ifdef DEBUG
    Serial.println("Failed to read Temp from DHT sensor!");
#endif
    return false;
  }
  
  // Write Sensor ID
  _message.data.id = sensID_Temp;
  
  // Save temperature
  _message.data.value = t;
  
  return true;
}
/*
 * Read Humidity
 */
boolean acquireHumidity()
{
  // Reading temperature or humidity takes about 250 milliseconds!
  float h = dht.readHumidity();
   
  if (isnan(h)){
#ifdef DEBUG
    Serial.println("Failed to read Humidity from DHT sensor!");
#endif
    return false;
  }
  
  // Write Sensor ID
  _message.data.id = sensID_Humid;
  
  // Save Humidity
  _message.data.value = h;
  
  return true; 
}

//
// Send New Sensor Value
//
void fireNewValueEvent()
{
  // Restart Watchdog
  //wdt_reset();
  
  // Compute Checksum : CRC8 on ID+Value (Checksum is computed on the first 5 bytes)
  _message.data.checksum = crc8(_message.raw, 5);

#ifdef DEBUG
    Serial.print("Sensor="); 
    Serial.print(_message.data.id); 
    Serial.print(", Value="); 
    Serial.print(_message.data.value);
    Serial.print(", CheckSum=0x"); 
    Serial.println(_message.data.checksum, HEX);
#endif

  // TX Setup
  man.setupTransmit(txPin);
  
  // Send All Data
  man.transmitArray(6, _message.raw);
}


//
// Toggle Boolean Flag
//
void toggleFlag()
{
  if (_flag)
  {
    _flag = false;
  }
  else
  {
    _flag = true;
  }
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
  delay(120000); // 2 min sleep
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
