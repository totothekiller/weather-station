//
// Tx Node
//
// Low Power Temperature Sensor
//

// Info : 
// Snootlab : http://forum.snootlab.com/viewtopic.php?f=38&t=399
// Virtual Wire library : 
// http://www.airspayce.com/mikem/arduino/VirtualWire/VirtualWire_8h.html
// http://www.pjrc.com/teensy/td_libs_VirtualWire.html
//
//
// Low Power Sleep :
//
// http://www.insidegadgets.com/2011/02/05/reduce-attiny-power-consumption-by-sleeping-with-the-watchdog-timer/
// http://www.nongnu.org/avr-libc/
// https://kalshagar.wikispaces.com/Watchdog+as+interrupt
// http://oregonembedded.com/batterycalc.htm


#include <VirtualWire.h>
#include <OneWire.h>
#include <avr/wdt.h>
#include <avr/power.h>
#include <avr/interrupt.h>
#include <avr/sleep.h>

#define  ATtiny85 // ATtiny85 mode

#ifdef ATtiny85
#define ECO // Low Power Mode

#define txLed 3  // TX LED
#define txPin 1  // TX Out
#define sensPin 0  // Sensor Pin
#define powerPin 4 // Power line for Tx module and sensor

#else
#define DEBUG  // Is serial debug active ?

#define txLed 13  // TX LED
#define txPin 12  // TX Out
#define sensPin 7  // Sensor Pin
#define powerPin 14 // Power line for Tx module and sensor
#endif

#define WATCHDOG_TIMER WDTO_8S // 8 sec
#define SLEEP_LOOP  2 // Deep Sleep total time = WATCHDOG_TIMER * SLEEP_LOOP

#define sensID 42  // Sensor ID

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

// WatchDof Counter
volatile byte _watchdogCounter;

// WatchDog interruption
ISR(WDT_vect) {
  _watchdogCounter++;
}


void setup()
{
  // Debug
#ifdef DEBUG
  Serial.begin(9600);
  Serial.println("--- SETUP ---");
#endif

  // Write Sensor ID
  _message.data.id = sensID;
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

#ifdef DEBUG
    Serial.print("Sensor="); 
    Serial.print(_message.data.id); 
    Serial.print(", Temp="); 
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
  pinMode(sensPin,INPUT);
}


/*
*
 * Read Temp
 *
 */
boolean acquireTemp()
{
  byte i;
  byte data[12];
  byte addr[8];
  
  // OneWire protocol
  OneWire wire(sensPin);

  // Reset Search
  wire.reset_search();

  // Get first sensor
  if ( !wire.search(addr)) {
#ifdef DEBUG
    Serial.println("No more addresses.");
#endif

    return false;
  }

  if (OneWire::crc8(addr, 7) != addr[7]) {
#ifdef DEBUG
    Serial.println("CRC is not valid!");
#endif

    return false;
  }

  // Check if chip is DS18B20
  if(addr[0]!=0x28)
  {
#ifdef DEBUG
    Serial.println("Chip is not DS18B20");
#endif

    return false;
  }

  wire.reset();
  wire.select(addr);
  wire.write(0x44,1);// start conversion

  delay(1000);     // maybe 750ms is enough, maybe not

  wire.reset();
  wire.select(addr);    
  wire.write(0xBE);// Read Scratchpad

  for ( i = 0; i < 9; i++) {// we need 9 bytes
    data[i] = wire.read();
  }

  // Convert the data to actual temperature
  float raw = (data[1] << 8) | data[0];

  // Save temperature
  _message.data.value = raw / 16.0;

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




