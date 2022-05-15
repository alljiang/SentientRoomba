// This arduino will relay commands from the computer to the drone and back
// using the HC-12 Module

#include <SoftwareSerial.h>

#include "DYPlayerArduino.h"
#include "map.h"

SoftwareSerial hc12(HC12_TX_PIN, HC12_RX_PIN);
SoftwareSerial player_serial(PLAYER_TX_PIN, PLAYER_RX_PIN);

DY::Player player(&player_serial);

uint8_t hc12_input_buffer[HC12_INPUT_BUFFER_SIZE];

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

	delay(500);

	player.playSpecified(1);
}

uint8_t c = 0;
int count = 0;

void
loop() {
	if (hc12.available()) {
		byte toSend[1];
		hc12.readBytes(toSend, 1);
		Serial.write(toSend[0]);
	}
}