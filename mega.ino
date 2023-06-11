#include <SoftwareSerial.h>

SoftwareSerial hc12(10, 9);  // HC-12 RX Pin, HC-12 TX Pin

const String DRIVING_PATTERN = "d";
const String ARRIVED_PATTERN = "a";

const String BEGIN_PATTERN = ":XXX";
const String BLANK_PATTERN = ":...";
const String DRINK1_PATTERN = ":X.X";
const String DRINK2_PATTERN = ":.X.";
const String DRINK3_PATTERN = ":X..";
const String DRINK4_PATTERN = ":..X";
const String DRINK5_PATTERN = ":.XX";
const String UNUSED_PATTERN = ":XX.";

const int DRIVING = 0b10;
const int ARRIVED = 0b01;

const char BEGIN = 0b111;
const char BLANK = 0b000;
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

// Raspberry Pi Pin Declarations
const int isMixingOrderReady = 7;
const int drinkPin1 = 2;
const int drinkPin2 = 3;
const int drinkPin3 = 4;
const int drinkPin4 = 5;
const int drinkPin5 = 6;

// Water pumps
const int pumpPin1 = 44;
const int pumpPin2 = 46;
const int pumpPin3 = 48;
const int pumpPin4 = 50;
const int pumpPin5 = 52;

const int mixingPin = 23;

int ZutatenCounter = 0;  // Zählervariablen
int GefuelltCounter = 0;

int pumpDelay = 5000;


// Variables to store pin states
bool drink1State = false;
bool drink2State = false;
bool drink3State = false;
bool drink4State = false;
bool drink5State = false;

enum Phase {
  Auswahl,
  Mixing
};

Phase currentPhase;
bool hasState;
bool hasTarget;
bool hasArrived;

String getZumoState() {
  String state = "";
  int i = 0;

  if (hc12.available()) {         // If HC-12 has data
    int data = int(hc12.read());  // Read the data from HC-12

    // add status
    if (((data & STATUS_MASK) >> SHIFT_STATUS) == ARRIVED) {
      state += ARRIVED_PATTERN;
    } else if (((data & STATUS_MASK) >> SHIFT_STATUS) == DRIVING) {
      state += DRIVING_PATTERN;
    } else {
      state += "!";
    }

    // add target pattern, then current pattern
    state += toPositionPattern((data & TARGET_MASK) >> SHIFT_TARGET);
    state += toPositionPattern((data & CURRENT_MASK) >> SHIFT_CURRENT);

    hasState = true;
  } else {
    hasState = false;
  }

  return state;
}


String toPositionPattern(int position) {
  if (position == BEGIN) { return BEGIN_PATTERN; }
  if (position == BLANK) { return BLANK_PATTERN; }
  if (position == DRINK1) { return DRINK1_PATTERN; }
  if (position == DRINK2) { return DRINK2_PATTERN; }
  if (position == DRINK3) { return DRINK3_PATTERN; }
  if (position == DRINK4) { return DRINK4_PATTERN; }
  if (position == DRINK5) { return DRINK5_PATTERN; }
  return UNUSED_PATTERN;
}


String getRaspberryState() {
  String state = "";

  if (digitalRead(isMixingOrderReady) == HIGH) { state += "M"; } else { state += "_"; }
  if (digitalRead(drinkPin1) == HIGH) { state += "1"; } else { state += "."; }
  if (digitalRead(drinkPin2) == HIGH) { state += "1"; } else { state += "."; }
  if (digitalRead(drinkPin3) == HIGH) { state += "1"; } else { state += "."; }
  if (digitalRead(drinkPin4) == HIGH) { state += "1"; } else { state += "."; }
  if (digitalRead(drinkPin5) == HIGH) { state += "1"; } else { state += "."; }

  state += ":" + String(currentPhase);

  return state;
}


String getTargetPattern() {
  if (drink1State) { return DRINK1_PATTERN; }
  if (drink2State) { return DRINK2_PATTERN; }
  if (drink3State) { return DRINK3_PATTERN; }
  if (drink4State) { return DRINK4_PATTERN; }
  if (drink5State) { return DRINK5_PATTERN; }

  return BEGIN_PATTERN;
}


int getTarget() {
  if (drink1State) { return DRINK1; }
  if (drink2State) { return DRINK2; }
  if (drink3State) { return DRINK3; }
  if (drink4State) { return DRINK4; }
  if (drink5State) { return DRINK5; }

  return BEGIN;
}


void setup() {
  pinMode(isMixingOrderReady, INPUT);
  pinMode(drinkPin1, INPUT);
  pinMode(drinkPin2, INPUT);
  pinMode(drinkPin3, INPUT);
  pinMode(drinkPin4, INPUT);
  pinMode(drinkPin5, INPUT);

  pinMode(pumpPin1, OUTPUT);
  pinMode(pumpPin2, OUTPUT);
  pinMode(pumpPin3, OUTPUT);
  pinMode(pumpPin4, OUTPUT);
  pinMode(pumpPin5, OUTPUT);

  pinMode(mixingPin, OUTPUT);

  Serial.begin(9600);
  hc12.begin(9600);

  currentPhase = Auswahl;
  hasState = false;
  hasTarget = false;
  hasArrived = false;
}


void loop() {
  if (currentPhase == Auswahl && digitalRead(isMixingOrderReady) == HIGH) {
    drink1State = digitalRead(drinkPin1) == HIGH;
    drink2State = digitalRead(drinkPin2) == HIGH;
    drink3State = digitalRead(drinkPin3) == HIGH;
    drink4State = digitalRead(drinkPin4) == HIGH;
    drink5State = digitalRead(drinkPin5) == HIGH;

    ZutatenCounter = 0;
    GefuelltCounter = 0;

    if (drink1State) { ZutatenCounter++; }
    if (drink2State) { ZutatenCounter++; }
    if (drink3State) { ZutatenCounter++; }
    if (drink4State) { ZutatenCounter++; }
    if (drink5State) { ZutatenCounter++; }

    currentPhase = Mixing;
    Serial.println("MixingPhase " + getRaspberryState());
  }

  int target = getTarget();

  String raspberryState = getRaspberryState();
  String targetPattern = getTargetPattern();
  String zumoState = getZumoState();

  if (hasState) {
    hasTarget = (startsWith(zumoState, ARRIVED_PATTERN + targetPattern) || startsWith(zumoState, DRIVING_PATTERN + targetPattern));
    hasArrived = startsWith(zumoState, ARRIVED_PATTERN + targetPattern + targetPattern);
    Serial.println("state zumo " + zumoState + " raspberry " + raspberryState + " phase " + String(currentPhase) + " target " + String(hasTarget) + " arrived " + String(hasArrived));
  }

  if (hasState && currentPhase == Mixing) {
    if (!hasTarget) {
      Serial.println("send new target " + String(target) + " pattern " + targetPattern);
      moveToPosition(target);
    }

    if (hasArrived) {
      if (GefuelltCounter < ZutatenCounter) {
        int pumpPin = getPumpPin(zumoState);

        if (pumpPin > 0) {
          Serial.println("filling drink pump pin " + String(pumpPin));

          fillDrink(pumpPin);
          GefuelltCounter++;

          if (pumpPin == pumpPin1) { drink1State = false; }
          if (pumpPin == pumpPin2) { drink2State = false; }
          if (pumpPin == pumpPin3) { drink3State = false; }
          if (pumpPin == pumpPin4) { drink4State = false; }
          if (pumpPin == pumpPin5) { drink5State = false; }
        }
      } else {
        moveToPosition(BEGIN);
        currentPhase = Auswahl;
      }
    }
  }

  delay(50);
}


void moveToPosition(int position) {
  hc12.write((char)position);
  Serial.println("zumo sent to " + String(position));
}


void fillDrink(int pumpPin) {
  Serial.println("start filling for " + pumpPin);

  digitalWrite(pumpPin, HIGH);        // Turn on the pump
  delay(pumpDelay / ZutatenCounter);  // Wait for 5 seconds (adjust as needed)
  digitalWrite(pumpPin, LOW);         // Turn off the pump

  Serial.println("filling finishend for " + pumpPin);
}


int getPumpPin(String zumoState) {
  int pin = -1;
  if (zumoState.startsWith(ARRIVED_PATTERN)) {
    if (zumoState.startsWith(ARRIVED_PATTERN + DRINK1_PATTERN)) { pin = pumpPin1; }
    if (zumoState.startsWith(ARRIVED_PATTERN + DRINK2_PATTERN)) { pin = pumpPin2; }
    if (zumoState.startsWith(ARRIVED_PATTERN + DRINK3_PATTERN)) { pin = pumpPin3; }
    if (zumoState.startsWith(ARRIVED_PATTERN + DRINK4_PATTERN)) { pin = pumpPin4; }
    if (zumoState.startsWith(ARRIVED_PATTERN + DRINK5_PATTERN)) { pin = pumpPin5; }
  }
  return pin;  // Return -1 if no valid pump pin is found
}

bool startsWith(const String& str, const String& prefix) {
  for (int i = 0; i < prefix.length(); i++) {
    if (str.charAt(i) != prefix.charAt(i)) {
      return false;
    }
  }

  return true;
}