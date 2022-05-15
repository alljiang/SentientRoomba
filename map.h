
#include "Arduino.h"

// ======== Pin Config ========

#define HC12_SET_PIN A0
#define HC12_TX_PIN  A1
#define HC12_RX_PIN  A2

#define PLAYER_TX_PIN    8
#define PLAYER_TX_PIN    7
#define PLAYER_NBUSY_PIN 2

#define PWM_A_PIN        5
#define PWM_B_PIN        6
#define RELAY_TOGGLE_PIN 4

#define LIMIT_SWITCH_PIN A3

// =============================

#define SERIAL_BAUD 115200
#define HC12_BAUD   9600
#define PLAYER_BAUD 9600

#define HC12_INPUT_BUFFER_SIZE 32