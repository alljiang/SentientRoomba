// This arduino will relay commands from the computer to the drone and back
// using the HC-12 Module

#include <SoftwareSerial.h>

#include "DYPlayerArduino.h"
#include "map.h"
#include "states.h"

SoftwareSerial hc12(HC12_TX_PIN, HC12_RX_PIN);
SoftwareSerial player_serial(PLAYER_TX_PIN, PLAYER_RX_PIN);

DY::Player player(&player_serial);

// HC-12 Buffer FIFO
uint8_t hc12_input_buffer[HC12_INPUT_BUFFER_SIZE];
int hc12_input_buffer_head   = 0;
int hc12_input_buffer_length = 0;

enum State state               = STATE_OEM;
int last_heartbeat_time        = 0;
int last_limit_switch_time     = 0;
int last_limit_switch_state    = 0;
int last_motor_command_time    = 0;
int motor_left_override_speed  = 0;
int motor_right_override_speed = 0;
bool play_audio_flag           = false;

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
	player.setVolume(30);  // 50%, range of 0 to 30
	player.setCycleMode(DY::PlayMode::Random);
	player.next();
	player.play();

	// Initialize motor pins
	pinMode(PWM_A1_PIN, OUTPUT);
	pinMode(PWM_A2_PIN, OUTPUT);
	pinMode(PWM_B1_PIN, OUTPUT);
	pinMode(PWM_B2_PIN, OUTPUT);
	pinMode(RELAY_TOGGLE_PIN, OUTPUT);
	digitalWrite(RELAY_TOGGLE_PIN, LOW);
	analogWrite(PWM_A1_PIN, 0);  // 0 to 255
	analogWrite(PWM_A2_PIN, 0);  
	analogWrite(PWM_B1_PIN, 0);  
	analogWrite(PWM_B2_PIN, 0);

	// Initialize limit switch pin
	pinMode(LIMIT_SWITCH_PIN, INPUT);

	delay(300);

}

void
hc12_handler() {
	// Read any and all incoming data from the HC-12 module
	while (hc12.available() &&
	       hc12_input_buffer_length < HC12_INPUT_BUFFER_SIZE) {
		char c = hc12.read();

		if (hc12_input_buffer_length < HC12_INPUT_BUFFER_SIZE) {
			int tail = (hc12_input_buffer_head + hc12_input_buffer_length) %
			           HC12_INPUT_BUFFER_SIZE;
			hc12_input_buffer[tail] = c;
		}
	}

	// If there is sufficient data in the buffer, process it
	if (hc12_input_buffer_length >= 6) {
		uint8_t header = hc12_input_buffer[(hc12_input_buffer_head + 0) %
		                                   sizeof(hc12_input_buffer)];

		// verify header
		if (header > 15) {
			// invalid header, skip
			hc12_input_buffer_head =
			    (hc12_input_buffer_head + 1) %
			    sizeof(hc12_input_buffer);  // increment start
			hc12_input_buffer_length--;
			goto exit;
		}

		uint8_t checksum = hc12_input_buffer[(hc12_input_buffer_head + 5) %
		                                     sizeof(hc12_input_buffer)];

		// calculate xor checksum
		uint8_t xorChecksum = header;
		for (int i = 1; i < 5; i++) {
			xorChecksum ^= hc12_input_buffer[(hc12_input_buffer_head + i) %
			                                 sizeof(hc12_input_buffer)];
		}

		if (xorChecksum != checksum) {
			// checksum failed, skip
			hc12_input_buffer_head =
			    (hc12_input_buffer_head + 1) %
			    sizeof(hc12_input_buffer);  // increment start
			hc12_input_buffer_length--;
			goto exit;
		}

		if (header == 0) {
			// heartbeat, verify data
			if (hc12_input_buffer[(hc12_input_buffer_head + 1) %
			                      sizeof(hc12_input_buffer)] == 0x12 &&
			    hc12_input_buffer[(hc12_input_buffer_head + 2) %
			                      sizeof(hc12_input_buffer)] == 0x34 &&
			    hc12_input_buffer[(hc12_input_buffer_head + 3) %
			                      sizeof(hc12_input_buffer)] == 0x56 &&
			    hc12_input_buffer[(hc12_input_buffer_head + 4) %
			                      sizeof(hc12_input_buffer)] == 0x78) {
				last_heartbeat_time = millis();
			}
		} else if (header == 1) {
			// motor speed
			int16_t left = hc12_input_buffer[(hc12_input_buffer_head + 1) %
			                                 sizeof(hc12_input_buffer)]
			               << 8;
			left |= hc12_input_buffer[(hc12_input_buffer_head + 2) %
			                          sizeof(hc12_input_buffer)];
			int16_t right = hc12_input_buffer[(hc12_input_buffer_head + 3) %
			                                  sizeof(hc12_input_buffer)]
			                << 8;
			right |= hc12_input_buffer[(hc12_input_buffer_head + 4) %
			                           sizeof(hc12_input_buffer)];

			motor_left_override_speed  = left;
			motor_right_override_speed = right;
			last_motor_command_time    = millis();
		}

		hc12_input_buffer_head = (hc12_input_buffer_head + 6) %
		                         sizeof(hc12_input_buffer);  // increment start
		hc12_input_buffer_length -= 6;
	}

exit:
	return;
}

void
sensor_handler() {
	// Read the limit switch
	int limit_switch_state = digitalRead(LIMIT_SWITCH_PIN);

	if (limit_switch_state == HIGH && last_limit_switch_state == LOW &&
	    millis() - last_limit_switch_time > LIMIT_SWITCH_TIMEOUT_MS) {
		// Hit a wall, limit switch triggered
		play_audio_flag = true;
	}

	if (last_limit_switch_state != limit_switch_state) {
		last_limit_switch_time = millis();
	}

	last_limit_switch_state = limit_switch_state;
}

void
state_machine() {
	// Get current state
	if (millis() - last_heartbeat_time > HEARTBEAT_TIMEOUT_MS) {
		state = STATE_OEM;
	} else if (state == STATE_SCREAMING) {
	} else if (play_audio_flag) {
		state = STATE_START_SCREAMING;
	} else if (millis() - last_motor_command_time < MOTOR_COMMAND_TIMEOUT_MS) {
		state = STATE_OVERRIDE;
	} else {
		state = STATE_OEM;
	}

	if (state == STATE_OEM) {
		// set relay to low
		digitalWrite(RELAY_TOGGLE_PIN, LOW);
	} else if (state == STATE_OVERRIDE) {
		// set relay to high
		digitalWrite(RELAY_TOGGLE_PIN, HIGH);

		int left_speed  = map(motor_left_override_speed, -1000, 1000, 0, 255);
		int right_speed = -map(motor_right_override_speed, -1000, 1000, 0, 255);

        if (left_speed > 0) {
			analogWrite(PWM_A1_PIN, left_speed);
            analogWrite(PWM_A2_PIN, 0);
		} else {
            analogWrite(PWM_A1_PIN, 0);
            analogWrite(PWM_A2_PIN, left_speed);
        }

        if (right_speed > 0) {
            analogWrite(PWM_B1_PIN, right_speed);
            analogWrite(PWM_B2_PIN, 0);
        } else {
            analogWrite(PWM_B1_PIN, 0);
            analogWrite(PWM_B2_PIN, right_speed);
        }

	} else if (state == STATE_START_SCREAMING) {
		state           = STATE_SCREAMING;
		play_audio_flag = false;

		// Make sure player is not busy
		if (digitalRead(PLAYER_NBUSY_PIN) == HIGH) {
			// Choose a random song to play
			player.next();
			player.play();
		}
	} else if (state == STATE_SCREAMING) {
		// kill motors
		digitalWrite(RELAY_TOGGLE_PIN, HIGH);

		analogWrite(PWM_A1_PIN, 0);
		analogWrite(PWM_A2_PIN, 0);
		analogWrite(PWM_B1_PIN, 0);
		analogWrite(PWM_B2_PIN, 0);

		// Wait for audio to be done playing
		if (digitalRead(PLAYER_NBUSY_PIN) == LOW) {
			state = STATE_OEM;
		}
	}
}

void
loop() {
	hc12_handler();
	sensor_handler();
	state_machine();
}