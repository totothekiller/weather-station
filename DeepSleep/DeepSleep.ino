
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

#undef ECO // Power Saving On/Off ?

int setupLed = 3;
int loopLed = 4;

volatile boolean isOn = true;


// WatchDog interruption
EMPTY_INTERRUPT(WDT_vect);

void setup() {
  // 
  // Setup
  pinMode(setupLed,OUTPUT);
  pinMode(loopLed,OUTPUT);
  digitalWrite(setupLed,HIGH);
  digitalWrite(loopLed,HIGH);

  delay(1000);


#ifdef ECO
  // Turn Off ADC
  power_adc_disable();

  // Turn Off USI
  power_usi_disable();


  // Setup WatchDog
  setupWatchdog();
#endif


  digitalWrite(setupLed,LOW);
}

void loop() {

  if(isOn)
  {
    digitalWrite(loopLed,LOW);
  }
  else{
    digitalWrite(loopLed,HIGH);
  }

  // Toggle
  isOn=!isOn;

  deepSleep();
}



void deepSleep()
{
#ifndef ECO

  delay(8000);

#else

  // Power Down
  set_sleep_mode(SLEEP_MODE_PWR_DOWN);

  // Stop Interruption
  cli();
  sleep_enable();
  //sleep_bod_disable(); // Turn Off BOD

  // Start interrupt then go to sleep !
  sei();
  sleep_cpu(); // Bye Bye

  sleep_disable(); // Wake Up

#endif
}


#ifdef ECO
//setup the watchdog to timeout every 8 second and make an interrupt (not a reset!)
void setupWatchdog(){
  //README : must set the fuse WDTON to 0 to enable the watchdog

    //disable interrupts
  cli();

  //make sure watchdod will be followed by a reset (must set this one to 0 because it resets the WDE bit)
  MCUSR &= ~(1 << WDRF);
  //set up WDT interrupt (from that point one have 4 cycle to modify WDTCSR)
  WDTCR = (1<<WDCE)|(1<<WDE);
  //Start watchdog timer with 1s prescaller and interrupt only
  WDTCR = (1<<WDIE)|(0<<WDE)|(1<<WDP3)|(1<<WDP0);

  //Enable global interrupts
  sei();
}
#endif


