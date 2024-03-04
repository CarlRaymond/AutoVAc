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
const int A_PIN = 1;        // PB1
const int B_PIN = 2;        // PB2
const int C_PIN = 3;        // PB3
const int D_PIN = 4;        // PB4

// Recevied code state machine. A code is considered received when the state goes from VALID_CODE back
// to READY.
enum class ReceiverState : unsigned char { READY, VALID_CODE, INVALID_CODE };
volatile ReceiverState currentReceiverState = ReceiverState::READY;
volatile ReceiverState priorReceiverState = ReceiverState::READY;

// Output state machine.
enum class OutputState : unsigned char { OFF, STARTING, RUNNING };
volatile OutputState currentOutputState = OutputState::OFF;
volatile OutputState priorOutputState = OutputState::OFF;

const unsigned long MAX_RUN_INTERVAL = 10000;  // 15 minutes, in milliseconds
const unsigned long MAX_QUIET_INTERVAL = 5000;
const unsigned long STARTUP_INTERVAL = 5000;

volatile unsigned long outputStartTime = 0;

void setup(void);
void loop(void);
void newInput(Code);
void newOutputState(OutputState);
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

// Set last code time to long ago so we start up off.
unsigned long last_code_time = millis() + 1000000;

// Loop's job is to process timeouts.
void loop() {

  // See if we've been running too long
  if (currentOutputState != OutputState::OFF) {
    if (millis() - outputStartTime > MAX_RUN_INTERVAL) {
      newInput(Code::TIMEOUT);
    }
  }
}

// Invoked by ISR on each change to the inputs. Advances the receiver state machine.
void newInput(Code c) {
  static Code priorCode = Code::NONE;

  switch (priorCode) {
    case Code::NONE:
      switch (c) {
        case Code::START:
          newOutputState(OutputState::RUNNING);
          outputStartTime = millis();
          break;

        case Code::STOP:
        case Code::TIMEOUT:
          newOutputState(OutputState::OFF);
          break;

        default:
          break;
      }
      break;

    default:
      break;
  }

  priorCode = c;
}

void newOutputState(OutputState s) {

  if (s == priorOutputState)
    return;

  priorOutputState = currentOutputState;
  currentOutputState = s;
  switch (s) {
    case OutputState::OFF:
      turnOff();
      break;

    case OutputState::RUNNING:
      turnOn();
      break;

    default:
      break;
  }
}

void turnOn() {
  PORTB |= _BV(PINB0);
}

void turnOff() {
  PORTB &= ~_BV(PINB0);
}

// Pin change interrupt. Invoked on change to any input bit.
ISR(PCINT0_vect) {
  
  // Delay to allow all bits to settle if the receiver doesn't set them all at once.
  delay(10);
  Code c = (Code) ((PINB & 0b00011110) >> 1);

  // If code is invalid, just eat it.
  if (isValidCode(c)) {
    newInput(c);
  }
}