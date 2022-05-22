
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

#define SFW_HQ
#define SFW_LQ
#define NSFW_HQ
#define NSFW_LQ

#ifdef SFW_HQ
#define SFW_HQ_COUNT  1
#else
#define SFW_HQ_COUNT  0
#endif

#ifdef SFW_LQ
#define SFW_LQ_COUNT  1
#else
#define SFW_LQ_COUNT  0
#endif

#ifdef NSFW_HQ
#define NSFW_HQ_COUNT 5
#else
#define NSFW_HQ_COUNT 0
#endif

#ifdef NSFW_LQ
#define NSFW_LQ_COUNT 1
#else
#define NSFW_LQ_COUNT 0
#endif

#define SFW_HQ_HEADER  1
#define SFW_LQ_HEADER  2
#define NSFW_HQ_HEADER 3
#define NSFW_LQ_HEADER 4

#define TOTAL_AUDIO_COUNT (SFW_HQ_COUNT + SFW_LQ_COUNT + NSFW_HQ_COUNT + NSFW_LQ_COUNT)

#define WIGGLE_SPEED 70