#include <Arduino.h>
#include <avr/interrupt.h>
#include "codes.h"

/*
 * Pin PB0 is the output, active high.
 * Pins PB1-PB4 are inputs from the RF receiver. While valid codes are received on
 * the inputs, the output will be held high. When valid codes are not seen for an
 * interval of time, the output will switch off.
*/

const int OUTPUT_PIN = 0;   // PB0

// Output state machine.
enum class MotorState : unsigned char { OFF, STARTING, RUNNING };
volatile MotorState currentOutputState = MotorState::OFF;
volatile MotorState priorOutputState = MotorState::OFF;

// Transition from STARTING to RUNNING after this interval
const unsigned long MAX_STARTING_INTERVAL = 5000;

// Transition from RUNNING to OFF after this interval;
const unsigned long MAX_RUNNING_INTERVAL = 60000; // 900000; 15 minutes, in milliseconds

// Transition from RUNNING to OFF after this interval when no STARTING or RUNNING codes
// are received
const unsigned long MAX_QUIET_INTERVAL = 5000;

volatile unsigned long motorStartTime = 0;
volatile unsigned long runningCodeReceivedTime = millis();
volatile unsigned long anyCodeReceivedTime = millis();

// Function declarations
void setup(void);
void loop(void);
void newInput(Code);
void newMotorState(MotorState);
void turnOff(void);
void turnOn(void);



void setup() {
  // Turn off voltage reference
  ACSR &= ~(1<<ACBG);

  // Set pin modes: PB0 output, PB1-PB4 input
  DDRB = 0b00000001;

  // Turn off output
  PORTB = 0b00000000;

  // Configure pin change interrupt on PORTB bits 1-4
  PCMSK = _BV(PCINT1) | _BV(PCINT2) | _BV(PCINT3) | _BV(PCINT4);
  GIMSK |= _BV(PCIE);        // Enable Pin Change Interrupts
  sei();

  // Toggle LED
  turnOn();
  delay(250);
  turnOff();
  delay(250);
  turnOn();
  delay(250);
  turnOff();
  delay(250);
  turnOn();
  delay(250);
  turnOff();
  delay(250);
  turnOn();
  delay(250);
  turnOff();
}


// Loop's job is to process timeouts. When a timeout occurrs, insert a
// pseudocode into the codestream.
void loop() {

  unsigned long now = millis();
  if (currentOutputState != MotorState::OFF) {
    // Time to transition from STARTING to RUNNING?
    if (now - motorStartTime > MAX_STARTING_INTERVAL) {
      newInput(Code::STARTING_TIMEOUT);
    }
    // See if we've been running too long
    if (now - motorStartTime > MAX_RUNNING_INTERVAL) {
      newInput(Code::RUNNING_TIMEOUT);
    }
    // Have transmitters all gone silent?
    if (now - runningCodeReceivedTime > MAX_QUIET_INTERVAL) {
      newInput(Code::STOP);
    }
  }
}

// Invoked by ISR on each change to the inputs, and by loop on timeouts.
// Advances the receiver state machine.
void newInput(Code currentCode) {

  switch (currentCode) {
    case Code::START:
      newMotorState(MotorState::STARTING);
      break;

    case Code::STARTING_TIMEOUT:
      newMotorState(MotorState::RUNNING);
      break;

    case Code::STOP:
    case Code::RUNNING_TIMEOUT:
    case Code::QUIET_TIMEOUT:
      newMotorState(MotorState::OFF);
      break;

    case Code::RUNNING:
    case Code::STARTING:
      runningCodeReceivedTime = millis();
      break;

    default:
      break;
  }

}

void newMotorState(MotorState s) {

  if (s == priorOutputState)
    return;

  priorOutputState = currentOutputState;
  currentOutputState = s;
  switch (s) {
    case MotorState::OFF:
      turnOff();
      break;

    case MotorState::STARTING:
    case MotorState::RUNNING:
      turnOn();
      motorStartTime = millis();
      break;

    default:
      break;
  }
}

void inline turnOn() {
  PORTB |= _BV(PINB0);
}

void inline turnOff() {
  PORTB &= ~_BV(PINB0);
}

// Pin change interrupt. Invoked on change to any input bit.
ISR(PCINT0_vect) {

  // Delay to allow all bits to settle if the receiver doesn't set them all at once.
  delay(1);
  Code c = (Code) ((PINB & 0b00011110) >> 1);

  // If code is invalid, just eat it.
  if (isValidCode(c)) {
    newInput(c);
  }
}