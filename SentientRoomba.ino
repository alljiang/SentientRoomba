// This arduino will relay commands from the computer to the drone and back
// using the HC-12 Module

#include <SoftwareSerial.h>

#include "DYPlayerArduino.h"
#include "map.h"

SoftwareSerial hc12(HC12_TX_PIN, HC12_RX_PIN);
SoftwareSerial player_serial(PLAYER_TX_PIN, PLAYER_RX_PIN);

DY::Player player(&player_serial);

// HC-12 Buffer FIFO
uint8_t hc12_input_buffer[HC12_INPUT_BUFFER_SIZE];
int hc12_input_buffer_head   = 0;
int hc12_input_buffer_length = 0;

void
setup() {
	Serial.begin(SERIAL_BAUD);

	Serial.println("Program Started");

	// Initialize the HC-12 module
	pinMode(HC12_SET_PIN, OUTPUT);
	digitalWrite(HC12_SET_PIN, HIGH);
	hc12.begin(HC12_BAUD);

	// Initialize the Player module
	pinMode(PLAYER_NBUSY_PIN, INPUT);
	player.begin();
	player.setVolume(15);  // 50%, range of 0 to 30

	// Initialize motor pins
	pinMode(PWM_A_PIN, OUTPUT);
	pinMode(PWM_B_PIN, OUTPUT);
	pinMode(RELAY_TOGGLE_PIN, OUTPUT);
	digitalWrite(RELAY_TOGGLE_PIN, LOW);
	analogWrite(PWM_A_PIN, 0);  // 0 to 255
	analogWrite(PWM_B_PIN, 0);

	// Initialize limit switch pin
	pinMode(LIMIT_SWITCH_PIN, INPUT);

	delay(300);

	player.playSpecified(1);
}

void
loop() {
	while (hc12.available() &&
	       hc12_input_buffer_length < HC12_INPUT_BUFFER_SIZE) {
		char c = hc12.read();

		if (hc12_input_buffer_length < HC12_INPUT_BUFFER_SIZE) {
			int tail = (hc12_input_buffer_head + hc12_input_buffer_length) %
			           HC12_INPUT_BUFFER_SIZE;
			hc12_input_buffer[tail] = c;
		}
	}
}