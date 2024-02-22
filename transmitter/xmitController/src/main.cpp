#include <Arduino.h>
#include <avr/sleep.h>
#include <avr/interrupt.h>
#include <avr/power.h>


/*
// When triggered, send the startup code STARTUP_CODE_COUNT times,
// then send the running code as long as the trigger is asserted.
// To minimize the chance of repeated interference with another
// transmitter, The time between codes is randomly chosen from an
// interval.
// When the trigger is no longer asserted, go to sleep.
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
const int INTERVAL_MIN = 350; // milliseconds
const int INTERVAL_MAX = 650; // milliseconds

int startupCodeCounter = 0;
bool triggered = false;
bool justAwoke = false;

void waitInterval(uint16_t minMillis, uint16_t maxMillis);
void codeOn(byte code);
void codeOff();
void setup();
void loop();
void sleep();
void startup();
void shutdown();

// Wait a random length of time between min and max milliseconds.
void waitInterval(uint16_t min, uint16_t max)
{
  uint16_t width = max - min;
  int time = rand() % width + min;
  delay(time);
}

void codeOn(byte code) {
  // Assume all bits are currently zero.

  // Turn on code bits
  PORTB |= (code << 1);
}

void codeOff() {
  // Possibly wait here
  // ...

  // Turn off all code bits
  PORTB &= ~(CODE_MASK << 1);
}

void setup() {

  // Turn off voltage reference
  ACSR &= ~(1<<ACBG);

  // Set pin modes
  pinMode(TRIGGER_PIN, INPUT);
  pinMode(A_PIN, OUTPUT);
  pinMode(B_PIN, OUTPUT);
  pinMode(C_PIN, OUTPUT);
  pinMode(D_PIN, OUTPUT);
  codeOff();

  // Configure pin change interrupt
  PCMSK = _BV(PCINT0);       // Only PB0 raises interrupt
  GIMSK |= _BV(PCIE);        // Enable Pin Change Interrupts

  // Take a nap until something happens.
  sleep();
}

void loop() {
  while (triggered) {
    if (startupCodeCounter > 0) {
      codeOn(STARTUP_CODE);
      codeOff();
      startupCodeCounter--;
    }
    else {
      codeOn(RUNNING_CODE);
      codeOff();
    }

    waitInterval(INTERVAL_MIN, INTERVAL_MAX);
  }

  sleep();
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

  // Turn on voltage reference
  ACSR |= (1<<ACBG);

  sei();                                  // Enable interrupts
}

// Called when first triggered to prepare for startup.
void startup() {
  startupCodeCounter = STARTUP_CODE_COUNT;
}

// Called when trigger released to prepart for shutdown.
void shutdown() {

}

ISR(PCINT0_vect) {
  int pin = digitalRead(TRIGGER_PIN);
  triggered = (pin == LOW);

  if (triggered) {
    startup();
  }
  else {
    shutdown();
  }
}