
#include "Arduino.h"

// ======== Pin Config ========

#define HC12_SET_PIN A0
#define HC12_TX_PIN  A1
#define HC12_RX_PIN  A2

#define PLAYER_TX_PIN   8
#define PLAYER_RX_PIN   7
#define PLAYER_BUSY_PIN A5

#define PWM_A1_PIN       5
#define PWM_A2_PIN       6
#define PWM_B1_PIN       9
#define PWM_B2_PIN       10
#define RELAY_TOGGLE_PIN 4

#define LIMIT_SWITCH_L_PIN A6
#define LIMIT_SWITCH_R_PIN A7

// =============================

#define SERIAL_BAUD 115200
#define HC12_BAUD   9600
#define PLAYER_BAUD 9600

#define HC12_INPUT_BUFFER_SIZE 32

#define HEARTBEAT_TIMEOUT_MS     1000
#define MOTOR_COMMAND_TIMEOUT_MS 300
#define LIMIT_SWITCH_TIMEOUT_MS  100
#define WIGGLE_TIMEOUT_MS        50

#define WIGGLE_SPEED 100