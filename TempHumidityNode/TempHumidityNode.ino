//
// TX Node with Temperature + Humidity
//
// Based on Sensor AM2302 (DHT22) http://snootlab.com/adafruit/252-capteur-de-temperature-et-humidite-am2302.html

#include <Manchester.h>
#include "DHT.h"
#include <OneWire.h> // For CRC
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
#define SLEEP_LOOP  2 // Deep Sleep total time = WATCHDOG_TIMER * SLEEP_LOOP

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

  // Read Temp
  if (acquireTemp())
  {  
    // Turn LED On
    digitalWrite(txLed, HIGH);
        
    // Compute Checksum : CRC8 on ID+Value
    _message.data.checksum = OneWire::crc8(_message.raw, 5);
    
#ifdef DEBUG
    Serial.print("Sensor="); 
    Serial.print(_message.data.id); 
    Serial.print(", Temp="); 
    Serial.print(_message.data.value);
    Serial.print(", CheckSum="); 
    Serial.println(_message.data.checksum);
#endif
  
    // Send All Data
    man.transmitArray(6, _message.raw);
  }
  
  // Read Humidity
  if (acquireHumidity())
  {  
    // Turn LED On
    digitalWrite(txLed, HIGH);
        
    // Compute Checksum : CRC8 on ID+Value
    _message.data.checksum = OneWire::crc8(_message.raw, 5);
    
#ifdef DEBUG
    Serial.print("Sensor="); 
    Serial.print(_message.data.id); 
    Serial.print(", Humidity="); 
    Serial.print(_message.data.value);
    Serial.print(", CheckSum="); 
    Serial.println(_message.data.checksum);
#endif
  
    // Send All Data
    man.transmitArray(6, _message.raw);
  }

  // sets the LED off
  digitalWrite(txLed, LOW);   

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
  
  delay(10);

  // Led Setup
  pinMode(txLed, OUTPUT);

  // TX Setup
  man.setupTransmit(txPin);
  
  // DHT Startup
  dht.begin();
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
