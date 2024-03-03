#include <Arduino.h>
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
// to NO_CODE.
const byte NO_CODE = 0;
const byte VALID_CODE = 1;
const byte INVALID_CODE = 2;
byte current_input_state = NO_CODE;
byte prior_input_state = NO_CODE;

// Output state machine.
const byte OFF = 0;
const byte STARTING = 1;
const byte RUNNING = 2;
byte current_output_state = OFF;
byte prior_output_state = OFF;

const unsigned long MAX_ON_INTERVAL = 900000;  // 15 minutes, in milliseconds
const unsigned long MAX_QUIET_INTERVAL = 5000;
void setup(void);
void loop(void);

void setup() {
  // Turn off voltage reference
  ACSR &= ~(1<<ACBG);

  // Set pin modes: PB0 output, PB1-PB4 input
  DDRB = 0b00000001;

  // Turn off output
  PORTB = 0b00000000;
}

// Set last code time to long ago so we start up off.
unsigned long last_code_time = millis() + 1000000;
void loop() {

    byte code = (PINB & CODE_MASK) >> 1;
    if (code == STARTUP_CODE || code == RUNNING_CODE) {
        current_input_state = VALID_CODE;
    }
    else if (code == NO)
}