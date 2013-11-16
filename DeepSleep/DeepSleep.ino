
//
// Test Deep Sleep
//

// Info from :
// http://www.insidegadgets.com/2011/02/05/reduce-attiny-power-consumption-by-sleeping-with-the-watchdog-timer/
// http://www.nongnu.org/avr-libc/
// https://kalshagar.wikispaces.com/Watchdog+as+interrupt
// http://oregonembedded.com/batterycalc.htm

#include <avr/wdt.h>
#include <avr/power.h>
#include <avr/interrupt.h>
#include <avr/sleep.h>

#define ECO // Power Saving On/Off ?

#define WATCHDOG_TIMER WDTO_8S // 8 sec
#define SLEEP_LOOP  2 // Deep Sleep total time = WATCHDOG_TIMER * SLEEP_LOOP

int pinLed = 3;

#ifdef ECO

// WatchDof Counter
volatile byte _watchdogCounter;

// WatchDog interruption
ISR(WDT_vect) {
  _watchdogCounter++;
}

#endif

void setup() {
  // 
  // Setup
  pinMode(pinLed,OUTPUT);
  digitalWrite(pinLed,HIGH);

  delay(1000);

  digitalWrite(pinLed,LOW);
}

void loop() {
  // Turn LED ON then OFF
  digitalWrite(pinLed,HIGH);
  delay(5000);
  digitalWrite(pinLed,LOW);

  deepSleep();
}


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
  PRR = 0x00; //Restaure power reduction

#endif
}


#ifdef ECO

//
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




