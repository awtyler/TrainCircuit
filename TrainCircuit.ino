#include "Arduino.h"

#define RUN     2
#define SLOW    3
#define REVERSE 4

#define REED1 8
#define REED2 14
#define REED3 18
#define REED4 22
#define REED5 28
#define REED6 26
#define REED8 50
#define REED7 A3

#define CALL1 9
#define CALL2 15
#define CALL3 19
#define CALL4 23
#define CALL5 29
#define CALL6 27
#define CALL8 52
#define CALL7 A7

#define STATION1 35
#define STATION2 37
#define STATION3 39
#define STATION4 41
#define STATION5 43
#define STATION6 45
#define STATION7 49
#define STATION8 47

#define STATION1A 10
#define STATION2A 16
#define STATION3A 20
#define STATION4A 24
#define STATION5A 30
#define STATION6A 32
#define STATION7A A11
#define STATION8A 51


#define TARGET1 34
#define TARGET2 36
#define TARGET3 38
#define TARGET4 40
#define TARGET5 42
#define TARGET6 44
#define TARGET7 48
#define TARGET8 46

#define TARGET1A 12
#define TARGET2A 17
#define TARGET3A 21
#define TARGET4A 25
#define TARGET5A 31
#define TARGET6A 33
#define TARGET7A A15
#define TARGET8A 53

#define JOYSTICK_REV A4   // GREEN
#define JOYSTICK_FST A8   // BLUE
#define JOYSTICK_FWD A12  // BROWN

#define LED 13

#define PRESSED LOW
#define UNPRESSED HIGH

#define LOCATION_LIGHTS_ENABLED 1
// #define TRAIN_RELAY_ON HIGH
// #define TRAIN_RELAY_OFF LOW
#define TRAIN_RELAY_ON LOW
#define TRAIN_RELAY_OFF HIGH

const int stationCount = 7;

const int reedDebounce = 1000;
const int callDebounce = 250;

double currentLocation = -0.5;
int lastLocation = -1;
int targetLocation = -1;

const int stations[] = {STATION1, STATION2, STATION3, STATION4, STATION5, STATION6, STATION7, STATION8};
const int targets[] = {TARGET1, TARGET2, TARGET3, TARGET4, TARGET5, TARGET6, TARGET7, TARGET8};
const int stationsa[] = {STATION1A, STATION2A, STATION3A, STATION4A, STATION5A, STATION6A, STATION7A, STATION8A};
const int targetsa[] = {TARGET1A, TARGET2A, TARGET3A, TARGET4A, TARGET5A, TARGET6A, TARGET7A, TARGET8A};

const int reeds[] = {REED1, REED2, REED3, REED4, REED5, REED6, REED7, REED8};
const int calls[] = {CALL1, CALL2, CALL3, CALL4, CALL5, CALL6, CALL7, CALL8};

// Debounce and state tracking arrays
unsigned long lastCallDebounceTime[stationCount] = {0};
unsigned long lastReedDebounceTime[stationCount] = {0};
const unsigned long callDebounceDelay = 50;
const unsigned long reedDebounceDelay = 200;
int lastCallButtonState[stationCount];
int callButtonState[stationCount];
int lastReedState[stationCount];
int reedState[stationCount];

void setup() {
    Serial.begin(115200);

    pinMode(LED, OUTPUT);
    ledOn();

    pinMode(RUN, OUTPUT);    
    digitalWrite(RUN, TRAIN_RELAY_OFF);
    
    pinMode(SLOW, OUTPUT);
    pinMode(REVERSE, OUTPUT);

    pinMode(JOYSTICK_FWD, INPUT_PULLUP);
    pinMode(JOYSTICK_FST, INPUT_PULLUP);
    pinMode(JOYSTICK_REV, INPUT_PULLUP);
    
    for(int i = 0; i < stationCount; i++) {
        pinMode(reeds[i], INPUT_PULLUP);
        pinMode(calls[i], INPUT_PULLUP);
        lastCallButtonState[i] = digitalRead(calls[i]);
        callButtonState[i] = lastCallButtonState[i];
        lastReedState[i] = digitalRead(reeds[i]);
        reedState[i] = lastReedState[i];

        if(LOCATION_LIGHTS_ENABLED == 1) {
            pinMode(stations[i], OUTPUT);
            pinMode(targets[i], OUTPUT);
            pinMode(stationsa[i], OUTPUT);
            pinMode(targetsa[i], OUTPUT);
        }
    }

    startupLights();
    
    Serial.println("Initialized.");
    ledOff();
}

void loop() {
    bool manualControl = false;
    if(digitalRead(JOYSTICK_FWD) == PRESSED || digitalRead(JOYSTICK_REV) == PRESSED) {
        bool slow = digitalRead(JOYSTICK_FST) != PRESSED;
        bool reverse = digitalRead(JOYSTICK_REV) == PRESSED;
        setTrain(true, reverse, slow);
        manualControl = true;
    }

    // Reed switch handling
    for (int i = 0; i < stationCount; i++) {
        if (debounceInput(reeds[i], lastReedState[i], lastReedDebounceTime[i], reedDebounceDelay, reedState[i])) {
            if (reedState[i] == PRESSED && i != lastLocation) {
                arrived(i);
            }
        }
    }

    if(manualControl) {
        targetLocation = -1;
        return;
    }

    // Call button handling
    for (int i = 0; i < stationCount; i++) {
        if (debounceInput(calls[i], lastCallButtonState[i], lastCallDebounceTime[i], callDebounceDelay, callButtonState[i])) {
            if (callButtonState[i] == PRESSED) {
                if (i == targetLocation) {
                    allStop();
                    Serial.print("All Stop: Target Location: ");
                    Serial.print(targetLocation);
                    Serial.print(" | Current Location: ");
                    Serial.print(currentLocation);
                    Serial.print(" | Last Location: ");
                    Serial.println(lastLocation);
                } else {
                    Serial.print("New Target Location: ");
                    Serial.print(i);
                    Serial.print(" | Current Location: ");
                    Serial.print(currentLocation);
                    Serial.print(" | Last Location: ");
                    Serial.println(lastLocation);
                    goTo(i);
                }
            }
        }
    }

    if (targetLocation >= 0 && targetLocation != currentLocation) {
        goTo(targetLocation);
    } else {
        allStop();
    }

    updateStatusLights();
}

bool debounceInput(int pin, int &lastStableState, unsigned long &lastDebounceTime, unsigned long debounceDelay, int &currentState) {
    int reading = digitalRead(pin);
    unsigned long currentTime = millis();

    if (reading != lastStableState) {
        if (currentTime - lastDebounceTime > debounceDelay) {
            lastStableState = reading;
            lastDebounceTime = currentTime;
            currentState = reading;

            // Log only when a stable change is detected
            Serial.print("Pin ");
            Serial.print(pin);
            Serial.print(" changed to ");
            Serial.println(currentState);

            return true;
        }
    }

    return false;
}

void startupLights() {
    if(LOCATION_LIGHTS_ENABLED == 1) {
        int delayAmount = 125;
        for(int i = 0; i < 3; i++) {        
            for(int i = 0; i < stationCount; i++) {
                digitalWrite(stations[i], HIGH);
                digitalWrite(targets[i], HIGH);
                digitalWrite(stationsa[i], HIGH);
                digitalWrite(targetsa[i], HIGH);
            }
            delay(delayAmount);
            for(int i = 0; i < stationCount; i++) {
                digitalWrite(stations[i], LOW);
                digitalWrite(targets[i], LOW);
                digitalWrite(stationsa[i], LOW);
                digitalWrite(targetsa[i], LOW);
            }
            delay(delayAmount);
        }
    }
}

void blinkSlow(int count) {
  blink(LED, count, 400, 500);
}

void blinkFast(int count) {
  blink(LED, count, 200, 200);
}

void blink(int pin, int count, int speed, int initialDelay) {
  delay(initialDelay);
  for(int i = 0; i < count; i++) {
    digitalWrite(pin, HIGH);
    delay(speed);
    digitalWrite(pin, LOW);
    delay(speed);
  }
  digitalWrite(pin, LOW);
}

void ledOn() {
    digitalWrite(LED, HIGH);
}

void ledOff() {
    digitalWrite(LED, LOW);
}

bool isClose() {
    return abs(lastLocation - targetLocation) == 1;
}

void allStop() {
    targetLocation = -1;
    digitalWrite(REVERSE, LOW);
    digitalWrite(SLOW, LOW);
    digitalWrite(RUN, TRAIN_RELAY_OFF);
}

void updateStatusLights() {
    if(LOCATION_LIGHTS_ENABLED != 1) return;
    
    for(int i = 0; i < stationCount; i++) {
        if(i == lastLocation) {
            digitalWrite(stations[i], HIGH);
            digitalWrite(stationsa[i], HIGH);
        } else {
            digitalWrite(stations[i], LOW);
            digitalWrite(stationsa[i], LOW);
        }
        if(i == targetLocation) {
            digitalWrite(targets[i], HIGH);
            digitalWrite(targetsa[i], HIGH);
        } else {
            digitalWrite(targets[i], LOW);
            digitalWrite(targetsa[i], LOW);
        }
    }
}

void arrived(int location) {
    bool reversed = isReversed();    // Need to check before setting lastLocation

    lastLocation = location;

    if(targetLocation == lastLocation
        || (reversed  && lastLocation < targetLocation)
        || (!reversed && lastLocation > targetLocation)
    ) {
        currentLocation = lastLocation;
        allStop();
    } else {
        // if(reversed) {
        //     currentLocation = double(lastLocation) - 0.5;
        // } else {
        //     currentLocation = double(lastLocation) + 0.5;
        // }

        if(isClose()) {
            digitalWrite(SLOW, HIGH);
        } else {
            digitalWrite(SLOW, LOW);
        }
    }
    Serial.print("Arrived: ");
    Serial.print(location);
    Serial.print(" | Reversed: ");
    Serial.print(isReversed());
    Serial.print(" | Target Location: ");
    Serial.print(targetLocation);
    Serial.print(" | Current Location: ");
    Serial.print(currentLocation);
    Serial.print(" | Last Location: ");
    Serial.println(lastLocation);

}

void goTo(int location) {
    targetLocation = location;
    if(targetLocation < 0) return;
    currentLocation = lastLocation; //TODO: Remove this to make lastLocation work
    bool go = false;
    bool reverse = false;
    bool slow = false;
    if(location < currentLocation) {
        reverse = true;
    }
    if(isClose()) {
        slow = true;
    }
    if(location != currentLocation) {
        go = true;
    }

    setTrain(go, reverse, slow);

    if(go) {
        Serial.print("Setting currentLocation: ");
        Serial.print(lastLocation);
        Serial.print(" | ");
        Serial.println(reverse);
        // if(reverse) {
        //     currentLocation = lastLocation; // - 0.5;
        // } else {
        //     currentLocation = lastLocation; // + 0.5;
        // }
    }
}

void setTrain(bool go, bool reverse, bool slow) {
    if(!go) {
        allStop();
        return;
    }

    digitalWrite(REVERSE, reverse ? HIGH : LOW);
    digitalWrite(SLOW, slow ? HIGH : LOW);
    // digitalWrite(SLOW, LOW);
    digitalWrite(RUN, go ? TRAIN_RELAY_ON : TRAIN_RELAY_OFF);
}

bool isReversed() {
    return targetLocation < lastLocation;
}