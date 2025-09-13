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
#define REED7 50
#define REED8 A3

#define CALL1 9
#define CALL2 15
#define CALL3 19
#define CALL4 23
#define CALL5 29
#define CALL6 27
#define CALL7 52
#define CALL8 A7

#define STATION1 35
#define STATION2 37
#define STATION3 39
#define STATION4 41
#define STATION5 43
#define STATION6 45
#define STATION7 47
#define STATION8 49

#define STATION1A 10
#define STATION2A 16
#define STATION3A 20
#define STATION4A 24
#define STATION5A 30
#define STATION6A 32
#define STATION7A 51
#define STATION8A A11


#define TARGET1 34
#define TARGET2 36
#define TARGET3 38
#define TARGET4 40
#define TARGET5 42
#define TARGET6 44
#define TARGET7 46
#define TARGET8 48

#define TARGET1A 11
#define TARGET2A 17
#define TARGET3A 21
#define TARGET4A 25
#define TARGET5A 31
#define TARGET6A 33
#define TARGET7A 53
#define TARGET8A A15


#define LED 13

#define PRESSED LOW
#define UNPRESSED HIGH

#define LOCATION_LIGHTS_ENABLED 1
#define TRAIN_RELAY_ON HIGH
#define TRAIN_RELAY_OFF LOW
//#define TRAIN_RELAY_ON LOW
//#define TRAIN_RELAY_OFF HIGH

const int stationCount = 8;

const int reedDebounce = 1000;
const int callDebounce = 250;

int currentLocation = 0;
int targetLocation = -1;

const int stations[] = {STATION1, STATION2, STATION3, STATION4, STATION5, STATION6, STATION7, STATION8};
const int targets[] = {TARGET1, TARGET2, TARGET3, TARGET4, TARGET5, TARGET6, TARGET7, TARGET8};
const int stationsa[] = {STATION1A, STATION2A, STATION3A, STATION4A, STATION5A, STATION6A, STATION7A, STATION8A};
const int targetsa[] = {TARGET1A, TARGET2A, TARGET3A, TARGET4A, TARGET5A, TARGET6A, TARGET7A, TARGET8A};

const int reeds[] = {REED1, REED2, REED3, REED4, REED5, REED6, REED7, REED8};
const int calls[] = {CALL1, CALL2, CALL3, CALL4, CALL5, CALL6, CALL7, CALL8};

void setup() {
    Serial.begin(115200);

    pinMode(LED, OUTPUT);
    ledOn();

    pinMode(RUN, OUTPUT);    
    digitalWrite(RUN, TRAIN_RELAY_OFF);
    
    pinMode(SLOW, OUTPUT);
    pinMode(REVERSE, OUTPUT);

    for(int i = 0; i < stationCount; i++) {
        pinMode(reeds[i], INPUT_PULLUP);
        pinMode(calls[i], INPUT_PULLUP);

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
    int delayTime = 0;
    ledOff();
    // Handle reed arrivals
    for(int i = 0; i < stationCount; i++) {
        if(digitalRead(reeds[i]) == PRESSED) {
            arrived(i);
            ledOn();
            if(currentLocation != targetLocation) {
              delayTime = reedDebounce;
            }
        }
    }

    // Handle call buttons
    for(int i = 0; i < stationCount; i++) {
      if(digitalRead(calls[i]) == PRESSED) {
          if(i == targetLocation) {
              allStop();
          } else {
              goTo(i);
          }
          ledOn();
          delayTime = callDebounce;
      }
    }
    
    if(targetLocation >= 0 && targetLocation != currentLocation) {
        goTo(targetLocation);
    } else {
        allStop();
    }

    // Uncomment if you want to blink when not moving
    // if(targetLocation < 0) {
    //   blinkFast(currentLocation);
    // }
    updateStatusLights();
    delay(delayTime);
}

void startupLights() {
    if(LOCATION_LIGHTS_ENABLED == 1) {
        int delayAmount = 125;
        for(int i = 0; i < stationCount; i++) {
            blink(stations[i], 1, delayAmount, 0);
        }
        for(int i = 0; i < stationCount; i++) {
            blink(targets[i], 1, delayAmount, 0);
        }
        
        for(int i = 0; i < stationCount; i++) {
            blink(stationsa[i], 1, delayAmount, 0);
        }
        for(int i = 0; i < stationCount; i++) {
            blink(targetsa[i], 1, delayAmount, 0);
        }
        
        for(int i = 0; i < stationCount; i++) {
            digitalWrite(stations[i], HIGH);
            digitalWrite(targets[i], HIGH);
            digitalWrite(stationsa[i], HIGH);
            digitalWrite(targetsa[i], HIGH);
        }
        delay(200);
        for(int i = 0; i < stationCount; i++) {
            digitalWrite(stations[i], LOW);
            digitalWrite(targets[i], LOW);
            digitalWrite(stationsa[i], LOW);
            digitalWrite(targetsa[i], LOW);
        }
        delay(200);
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
    return abs(currentLocation - targetLocation) == 1;
}

void allStop() {
    targetLocation = -1;
    setTrain(false, false, false);
}

void updateStatusLights() {
    if(LOCATION_LIGHTS_ENABLED != 1) return;
    
    for(int i = 0; i < stationCount; i++) {
        if(i == currentLocation) {
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
    bool reversed = isReversed();    // Need to check before setting currentLocation
    Serial.print("reversed: ");
    Serial.println(isReversed());

    Serial.print("Arrived: ");
    Serial.println(location);
    currentLocation = location;

    if(targetLocation == currentLocation
        || (reversed  && currentLocation < targetLocation)
        || (!reversed && currentLocation > targetLocation)
    ) {
        allStop();
    }

    if(isClose()) {
        digitalWrite(SLOW, HIGH);
    } else {
        digitalWrite(SLOW, LOW);
    }
}

void goTo(int location) {
    targetLocation = location;
    if(targetLocation < 0) return;

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
}

void setTrain(bool go, bool reverse, bool slow) {
    digitalWrite(REVERSE, reverse ? HIGH : LOW);
    digitalWrite(SLOW, slow ? HIGH : LOW);
    digitalWrite(RUN, go ? TRAIN_RELAY_ON : TRAIN_RELAY_OFF);
}

bool isReversed() {
    return targetLocation < currentLocation;
}