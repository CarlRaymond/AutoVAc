#include <Arduino.h>
#include <avr/sleep.h>
#include <avr/interrupt.h>
#include <avr/power.h>


/*
 * Pin PB0 is an input, and is triggered (active low) when the current
 * sensor detects acurrent draw above a threshold value.
 * Pins PB1 to PB4 are outputs connected to the 4-bit transmitter module.
 * While the trigger is asserted, we send a code repeatedly, with a random
 * interval between repeats. Initially, we send STARTUP_CODE a small number
 * of times, and then send RUNNING_CODE as long as the trigger remains
 * asserted.
 * 
 * The transmitter takes approximately 18ms to send a code word. The interval
 * between codes is randomly chosen between 350ms and 650ms. The random time
 * interval between transmissions is a simple protocol intended to reduce the
 * probability of repeated collissions (which are undetectable) when multiple
 * transmitters are active at the same time.
 * 
 * The receiver can distinguish between STARTUP_CODE and RUNNING_CODE, but
 * cannot identify which transmitter is sending codes, or even how many different
 * transmitters are active.
 * 
 * When the trigger is no longer asserted, we go to sleep.
 */

const int TRIGGER_PIN = 0;  // PB0
const int A_PIN = 1;        // PB1
const int B_PIN = 2;        // PB2
const int C_PIN = 3;        // PB3
const int D_PIN = 4;        // PB4

const byte STARTUP_CODE = 0b1011;
const int STARTUP_CODE_COUNT = 3;
const byte RUNNING_CODE = 0b1001;
const byte CODE_MASK = 0b1111;

// Transmit interval is 500ms +- 150ms
const int INTERVAL_MIN = 750; // milliseconds
const int INTERVAL_MAX = 2000; // milliseconds
const int BIT_ON_TIME = 45; // milliseconds
const int INTERBIT_INTERVAL = 265;
volatile int startupCodeCounter = 0;
volatile bool triggered = false;
bool justAwoke = false;

void waitInterval(uint16_t minMillis, uint16_t maxMillis);
void codeOn(byte code);
void codeOff(void);
void setup(void);
void loop(void);
void sleep(void);
void triggerOn(void);
void triggerOff(void);
void readTrigger(void);

// Wait a random length of time between min and max milliseconds.
void waitInterval(uint16_t min, uint16_t max)
{
  uint16_t width = max - min;
  int time = rand() % width + min;
  delay(time);
}

void codeOn(byte code) {
  // Assume all bits are currently zero.

  // Turn on code bits. Active low.
  byte bits = (code & CODE_MASK) << 1;
  byte invBits = ~bits;
  PORTB = invBits;
}

void codeOff() {
  // Possibly wait here
  // ...

  // Turn off (HIGH) all code bits.
  PORTB = (CODE_MASK << 1);
}

void setup() {

  // Turn off voltage reference
  ACSR &= ~(1<<ACBG);

  // Set pin modes: PB0 input, PB1-PB4 output
  DDRB = 0b00011110;
  codeOff();

  // Configure pin change interrupt
  PCMSK = _BV(PCINT0);       // Only PB0 raises interrupt
  GIMSK |= _BV(PCIE);        // Enable Pin Change Interrupts
  sei();

  // Take a nap until something happens.
  //sleep();

  // Startup test
  for (int i=0; i<5;  i++) {
    codeOn(0b0001);
    delay(BIT_ON_TIME);
    codeOff();
    delay(INTERBIT_INTERVAL);
    codeOn(0b0010);
    delay(BIT_ON_TIME);
    codeOff();
    delay(INTERBIT_INTERVAL);
    codeOn(0b0100);
    delay(BIT_ON_TIME);
    codeOff();
    delay(INTERBIT_INTERVAL);
    codeOn(0b1000);
    delay(BIT_ON_TIME);
    codeOff();
    delay(INTERBIT_INTERVAL);
    codeOff();
  }

  readTrigger();
}

void loop() {
  while (triggered) {
    if (startupCodeCounter > 0) {
      codeOn(STARTUP_CODE);
      delay(BIT_ON_TIME);
      codeOff();
      startupCodeCounter--;
    }
    else {
      codeOn(RUNNING_CODE);
      delay(BIT_ON_TIME);
      codeOff();
    }

    waitInterval(INTERVAL_MIN, INTERVAL_MAX);
  }

  //sleep();
}

void sleep()
{
  // Prepare for Power Down sleep
  
  cli();                                  // Disable interrupts
  set_sleep_mode(SLEEP_MODE_PWR_DOWN);
  sleep_enable();                         // Set SE bit
  sei();                                  // Enable interrupts
  

  // Sleep until wake event (any interrupt) happens.
  justAwoke = true;  // Get ready for waking up later
  sleep_cpu();                            // ZZZzzzz.
  
  // Now we are awake again!
  cli();                                  // Disable interrupts
  sleep_disable();                        // Clear SE bit
  sei();                                  // Enable interrupts
}

// Called when first triggered to prepare for triggerOn.
// Invoked by interrupt routine; any global variables changed should be declared volatile.
void triggerOn() {
  startupCodeCounter = STARTUP_CODE_COUNT;
}

// Called when trigger released.
// Invoked by interrupt routine; any global variables changed should be declared volatile.
void triggerOff() {

}

void readTrigger() {
  int pin = digitalRead(TRIGGER_PIN);
  triggered = (pin == LOW);

  if (triggered) {
    triggerOn();
  }
  else {
    triggerOff();
  }

}

// Pin change interrupt
ISR(PCINT0_vect) {
  readTrigger();
}