#include <Arduino.h>
#include <avr/interrupt.h>
#include "codes.h"

/*
 * Runs on ATTiny84
 * Pin PB0 is the output, active high.
 * Pins PA0-PA3 are inputs from the RF receiver. While valid codes are received on
 * the inputs, the output will be held high. When valid codes are not seen for an
 * interval of time, the output will switch off.
*/

const int OUTPUT_PIN = 0;   // PB0

// Output state machine.
enum class MotorState : unsigned char { OFF, MANUAL_RUN, AUTO_RUN };
volatile MotorState currentOutputState = MotorState::OFF;
volatile MotorState priorOutputState = MotorState::OFF;

// Transition from MANUAL_RUN to OFF after this interval;
const unsigned long SHUTOFF_INTERVAL = 180000;  // 3 minutes for testing // 1200000; 20 minutes, in milliseconds

// Max interval allowed between codes in a multi-code sequnce
const unsigned long CODE_SEQ_INTERVAL = 1000;

// Interval after hearing no activity from tool transmitters
const unsigned long QUIET_INTERVAL = 5000;

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

  // Set pin modes: PB0 output
  DDRA = 0b00000000;
  DDRB = 0b00000001;

  // Turn off output
  PORTB = 0b00000000;

  // Configure pin change interrupt on PORTA bits 0-3
  PCMSK0 =  _BV(PCINT0) | _BV(PCINT1) | _BV(PCINT2) | _BV(PCINT3);
  GIMSK |= _BV(PCIE0);        // Enable Pin Change Interrupts
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

    // Heard any good codes lately?
    if (now - anyCodeReceivedTime > CODE_SEQ_INTERVAL) {
      newInput(Code::CODE_SEQ_TIMEOUT);
    }

    // See if we've been running too long
    if (now - motorStartTime > SHUTOFF_INTERVAL) {
      newInput(Code::SHUTOFF_TIMEOUT);
    }

    // Have transmitters all gone silent?
    if (now - runningCodeReceivedTime > QUIET_INTERVAL) {
      newInput(Code::TOOL_QUIET_TIMEOUT);
    }
  }
}

// Invoked by ISR on each change to the inputs, and by loop on timeouts.
// Advances the receiver state machine.
void newInput(Code currentCode) {

  switch (currentCode) {
    case Code::START:
    case Code::TOOL_STARTING:
      newMotorState(MotorState::MANUAL_RUN);
      runningCodeReceivedTime = millis();
      break;

    case Code::STOP:
    case Code::TOOL_QUIET_TIMEOUT:
    case Code::SHUTOFF_TIMEOUT:
      newMotorState(MotorState::OFF);
      break;

    case Code::TOOL_RUNNING:
      runningCodeReceivedTime = millis();
      break;

    default:
      break;
  }

}

void newMotorState(MotorState s) {

  if (s == currentOutputState)
    return;

  currentOutputState = s;

  switch (s) {
    case MotorState::OFF:
      turnOff();
      break;

    case MotorState::AUTO_RUN:
    case MotorState::MANUAL_RUN:
      if (currentOutputState == (MotorState::OFF)) {
        motorStartTime = millis();
        turnOn();
      }
      break;
  }


  priorOutputState = currentOutputState;
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
  Code c = (Code) (PINA & 0b00001111);

  // If code is invalid, just eat it.
  if (isValidCode(c)) {
    newInput(c);
  }
}