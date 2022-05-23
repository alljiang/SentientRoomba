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

enum State state                = STATE_OEM;
int64_t last_heartbeat_time     = 0;
int64_t last_limit_switch_time  = 0;
bool last_limit_switch_state    = false;
int64_t last_motor_command_time = 0;
int motor_left_override_speed   = 0;
int motor_right_override_speed  = 0;
bool play_audio_flag            = false;
int64_t last_wiggle_switch_time = 0;
bool wiggle_left                = false;
int random_index                = TOTAL_AUDIO_COUNT;
int random_sequence[TOTAL_AUDIO_COUNT];

void
setup() {
	Serial.begin(SERIAL_BAUD);

	Serial.println("Program Started");

	// Initialize the HC-12 module
	pinMode(HC12_SET_PIN, OUTPUT);
	digitalWrite(HC12_SET_PIN, HIGH);
	hc12.begin(HC12_BAUD);

	// Initialize the Player module
	pinMode(PLAYER_BUSY_PIN, INPUT);
	player.begin();
	player.setVolume(30);  // 50%, range of 0 to 30
	player.setCycleMode(DY::PlayMode::OneOff);
	player.stop();

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
	pinMode(LIMIT_SWITCH_L_PIN, INPUT);
	pinMode(LIMIT_SWITCH_R_PIN, INPUT);
	randomSeed(analogRead(3));

	// Initialize random sequence
	for (int i = 0; i < TOTAL_AUDIO_COUNT; i++) {
		random_sequence[i] = i;
	}

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
	float limit_switch_l_state = analogRead(LIMIT_SWITCH_L_PIN);
	float limit_switch_r_state = analogRead(LIMIT_SWITCH_R_PIN);

	bool limit_switch_activated =
	    limit_switch_l_state > 400 || limit_switch_r_state > 400;

	if (limit_switch_activated && !last_limit_switch_state &&
	    millis() - last_limit_switch_time > LIMIT_SWITCH_TIMEOUT_MS) {
		// Hit a wall, limit switch triggered
		play_audio_flag = true;
	}

	if (last_limit_switch_state != limit_switch_activated) {
		last_limit_switch_time = millis();
	}

	last_limit_switch_state = limit_switch_activated;
}

void
state_machine() {
	// Get current state
	// last_heartbeat_time = millis();
	// if (millis() - last_heartbeat_time > HEARTBEAT_TIMEOUT_MS) {
	// 	state = STATE_OEM;
	// 	Serial.println("OEM1");
	// } else
	if (state == STATE_SCREAMING) {
		// Serial.println("SCREAM");
	} else if (play_audio_flag) {
		state = STATE_START_SCREAMING;
		// Serial.println("PLAY_AUDIO");
	} else if (millis() - last_motor_command_time < MOTOR_COMMAND_TIMEOUT_MS) {
		// Serial.println("OVERRIDE");
		state = STATE_OVERRIDE;
	} else {
		state = STATE_OEM;
		// Serial.println("OEM");
	}

	if (state == STATE_OEM) {
		// set relay to low
		digitalWrite(RELAY_TOGGLE_PIN, LOW);

		sensor_handler();
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

		// make sure not playing
		if (analogRead(PLAYER_BUSY_PIN) > 400) {
			// kill motors
			digitalWrite(RELAY_TOGGLE_PIN, HIGH);

			analogWrite(PWM_A1_PIN, 0);
			analogWrite(PWM_A2_PIN, 0);
			analogWrite(PWM_B1_PIN, 0);
			analogWrite(PWM_B2_PIN, 0);

			// Choose a random song to play
			if (random_index >= TOTAL_AUDIO_COUNT) {
				for (size_t i = 0; i < TOTAL_AUDIO_COUNT; i++) {
					size_t j = random(i, TOTAL_AUDIO_COUNT);

					// swap x[i] and x[j]
					auto t             = random_sequence[i];
					random_sequence[i] = random_sequence[j];
					random_sequence[j] = t;
				}
				random_index = 0;
                Serial.println("shuffling");
                for(size_t i = 0; i < TOTAL_AUDIO_COUNT; i++) {
                    Serial.print(random_sequence[i]);
                    Serial.print(" ");
                }
                Serial.println("");
			}
			int song_index = random_sequence[random_index++];

			char header = SFW_HQ_HEADER;

			if (song_index >= SFW_HQ_COUNT) {
				song_index -= SFW_HQ_COUNT;
				header = SFW_LQ_HEADER;

				if (song_index >= SFW_LQ_COUNT) {
					song_index -= SFW_LQ_COUNT;
					header = NSFW_HQ_HEADER;

					if (song_index >= NSFW_HQ_COUNT) {
						song_index -= NSFW_HQ_COUNT;
						header = NSFW_LQ_HEADER;
					}
				}
			}

			char song_name[11];
			sprintf(song_name, "/%d%04d.mp3", header, song_index);

			Serial.println(song_name);

			wiggle_left             = false;
			last_wiggle_switch_time = millis();

			player.playSpecifiedDevicePath(DY::Device::Flash, song_name);
			delay(500);
		}

	} else if (state == STATE_SCREAMING) {
		digitalWrite(RELAY_TOGGLE_PIN, HIGH);

		if (wiggle_left) {
			analogWrite(PWM_A1_PIN, WIGGLE_SPEED);
			analogWrite(PWM_A2_PIN, 0);
			analogWrite(PWM_B1_PIN, WIGGLE_SPEED);
			analogWrite(PWM_B2_PIN, 0);
		} else {
			analogWrite(PWM_A1_PIN, 0);
			analogWrite(PWM_A2_PIN, WIGGLE_SPEED);
			analogWrite(PWM_B1_PIN, 0);
			analogWrite(PWM_B2_PIN, WIGGLE_SPEED);
		}

		if (millis() - last_wiggle_switch_time > WIGGLE_TIMEOUT_MS) {
			wiggle_left             = !wiggle_left;
			last_wiggle_switch_time = millis();
		}

		// Wait for audio to be done playing after it has already started
		if (analogRead(PLAYER_BUSY_PIN) > 400) {
			// Done playing
			state = STATE_OEM;

			analogWrite(PWM_A1_PIN, 0);
			analogWrite(PWM_A2_PIN, 0);
			analogWrite(PWM_B1_PIN, 0);
			analogWrite(PWM_B2_PIN, 0);
		}
	}
}

void
loop() {
	// hc12_handler();
	state_machine();
}