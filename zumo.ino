#include <ZumoShield.h>
#include <SoftwareSerial.h>

SoftwareSerial HC12(3, 6);  // HC-12 RX Pin, HC-12 TX Pin

ZumoReflectanceSensorArray reflectanceSensorArray;
ZumoMotors motors;

const String DRIVING_PATTERN = "d";
const String ARRIVED_PATTERN = "a";

const String BEGIN_PATTERN  = ":XXX";
const String BLANK_PATTERN  = ":...";
const String DRINK1_PATTERN = ":X.X";
const String DRINK2_PATTERN = ":.X.";
const String DRINK3_PATTERN = ":X..";
const String DRINK4_PATTERN = ":..X";
const String DRINK5_PATTERN = ":.XX";

// const String END_PATTERN = DRINK5_PATTERN;

const int DRIVING = 0b10;
const int ARRIVED = 0b01;

const char BEGIN  = 0b111;
const char BLANK  = 0b000;
const char DRINK1 = 0b101;
const char DRINK2 = 0b010;
const char DRINK3 = 0b100;
const char DRINK4 = 0b001;
const char DRINK5 = 0b011;

const int SHIFT_STATUS = 6;
const int SHIFT_TARGET = 3;
const int SHIFT_CURRENT = 0;

const int STATUS_MASK = 0b11 << SHIFT_STATUS;
const int TARGET_MASK = 0b111 << SHIFT_TARGET;
const int CURRENT_MASK = 0b111 << SHIFT_CURRENT;

const int THRESHOLD = 500;

const int SPEED_LEFT = 90;
const int SPEED_RIGHT = 70;
const int LOOP_WAIT = 70;

String sensor_values;
String zumo_state_pattern;
String driving_pattern;
String target_pattern;
String current_pattern;

int driving_state;
int target_position;
int current_position;


int scanPattern() {
  unsigned int sensors[6];
  reflectanceSensorArray.read(sensors);

  int sensor1 = (sensors[0] + sensors[1]) / 2;
  int sensor2 = (sensors[2] + sensors[3]) / 2;
  int sensor3 = (sensors[4] + sensors[5]) / 2;

  sensor_values = "(" + String(sensor1) + ", " + String(sensor2) + ", " + String(sensor3) + ")";
  current_pattern = ":";
  current_position = 0;

  if (sensor1 > THRESHOLD) { current_pattern += 'X'; current_position += 4; } else { current_pattern += '.'; }
  if (sensor2 > THRESHOLD) { current_pattern += 'X'; current_position += 2; } else { current_pattern += '.'; }
  if (sensor3 > THRESHOLD) { current_pattern += 'X'; current_position += 1; } else { current_pattern += '.'; }

  return current_position;
}


int getZumoState() {
  int state = (driving_state << SHIFT_STATUS) + (target_position << SHIFT_TARGET) + current_position;
  String state_pattern = "";

  if (driving_state == DRIVING) { state_pattern += DRIVING_PATTERN; } else  { state_pattern += ARRIVED_PATTERN; }
  state_pattern += target_pattern;
  state_pattern += current_pattern;

  zumo_state_pattern = "pattern " + state_pattern + " state " + String(state) + ": driving " + String(driving_state) + " target " + target_position + " current " + current_position + " sensors " + sensor_values;

  return state;
}


void updateStopPattern() {
  if (HC12.available()) {                   // If HC-12 has data
    int new_target = int(HC12.read());      // Read the data from HC-12
    Serial.println("new target: " + String(new_target) + " old target " + target_position);
    target_position = new_target;

    target_pattern = ":";
    if (new_target & 4) { target_pattern += "X"; } else  { target_pattern += "."; } 
    if (new_target & 2) { target_pattern += "X"; } else  { target_pattern += "."; } 
    if (new_target & 1) { target_pattern += "X"; } else  { target_pattern += "."; } 

    if (target_position == BEGIN) {
      motors.setSpeeds(-SPEED_LEFT, -SPEED_RIGHT);
      driving_state = DRIVING;
      driving_pattern = DRIVING_PATTERN;
    } else {
      motors.setSpeeds(SPEED_LEFT, SPEED_RIGHT);
      driving_state = DRIVING;
      driving_pattern = DRIVING_PATTERN;
    }
  }
}


void setup() {
  Serial.begin(9600);
  reflectanceSensorArray.init();

  HC12.begin(9600);  // Serial port to HC12
  motors.setSpeeds(0, 0);

  driving_state = ARRIVED;
  driving_pattern = ARRIVED_PATTERN;

  target_position = BEGIN;
  target_pattern = BEGIN_PATTERN;
 
  current_position = scanPattern();
}


void loop() {
  updateStopPattern();
  scanPattern();

  if (current_position == target_position) {
    Serial.print("STOP! ");

    // set state to arrived
    driving_state = ARRIVED;
    driving_pattern = ARRIVED_PATTERN;

    // stop motors
    motors.setSpeeds(0, 0);
  } else {
    Serial.print(">>>>> ");
  }

  int message_to_zumo = getZumoState();
  Serial.println(zumo_state_pattern + " message " + message_to_zumo + " mega " + String(HC12.available()));

  HC12.write(message_to_zumo);

  delay(LOOP_WAIT);
}
