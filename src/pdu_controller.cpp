/*
 * PDU Controller Implementation
 * 
 * This file implements the core PDU controller functionality, including
 * power sequencing, channel control, temperature monitoring, and fault handling.
 * It serves as the main control logic for the PDU system.
 * 
 * Author: Ahmed Ellamie
 * Email: ahmed.ellamiee@gmail.com
 * Created: 5/7/2025
 * Modified: 7/29/2025
 */

#include "pdu_controller.h"

PDUController::PDUController() 
    : oneWire(TEMP_PIN)
    , sensors(&oneWire)
    , relayState(false)
    , relayFlag(true)
    , currentTemp(0.0f)
    , tempThreshold(DEFAULT_TEMP_THRESHOLD)
    , timeThreshold(DEFAULT_TIME_THRESHOLD)
    , vpResetAttempts(0)
    , batteryVoltage(0.0f)
    , ignLowStartTime(0)
    , lastTempCheckTime(0)
    , lastStableTime(0)
    , lastStableState(LOW)
    , lastVpPinState(-1)
    , lastCh1PinState(-1)
    , lastNormalOpTime(0)
    , faultHandlingInProgress(false)
{
}

void PDUController::begin() {
    initPins();
    sensors.begin();
    loadSettings();
}

void PDUController::initPins() {
    pinMode(F1_PIN, INPUT);
    pinMode(F2_PIN, INPUT);
    pinMode(F3_PIN, INPUT);
    pinMode(F4_PIN, INPUT);
    pinMode(IGN_PIN, INPUT);
    pinMode(VP_PIN, INPUT);
    pinMode(CH1_PIN, OUTPUT);
    pinMode(CH2_PIN, OUTPUT);
    pinMode(CH3_PIN, OUTPUT);
    pinMode(CH4_PIN, OUTPUT);
    pinMode(RELAY_PIN, OUTPUT);

    // Initialize all channels to OFF
    digitalWrite(CH1_PIN, HIGH);
    digitalWrite(CH2_PIN, HIGH);
    digitalWrite(CH3_PIN, HIGH);
    digitalWrite(CH4_PIN, HIGH);
    digitalWrite(RELAY_PIN, LOW);
}

void PDUController::turnOnSequence() {
    // Start with relay on
    digitalWrite(RELAY_PIN, HIGH);
    delay(RELAY_DELAY);

    // CH1 edge control sequence
    digitalWrite(CH1_PIN, LOW);   // Turn on CH1
    delay(CH1_PULSE);            // Wait 100ms
    digitalWrite(CH1_PIN, HIGH);  // Turn off CH1
    delay(CH1_OFF_TIME);         // Wait 50ms
    digitalWrite(CH1_PIN, LOW);   // Turn on CH1 again
    
    delay(CH_ACTIVATE_DELAY);    // Wait before other channels

    // Turn on other channels
    digitalWrite(CH2_PIN, LOW);
    digitalWrite(CH3_PIN, LOW);
    digitalWrite(CH4_PIN, LOW);

    relayState = true;
    Serial.println("Full sequence completed: CH1 edge control + other channels");
}

void PDUController::turnOffSequence() {
    digitalWrite(CH1_PIN, HIGH);
    digitalWrite(CH2_PIN, HIGH);
    digitalWrite(CH3_PIN, HIGH);
    digitalWrite(CH4_PIN, HIGH);
    digitalWrite(RELAY_PIN, LOW);
    relayState = false;
}

void PDUController::setChannel(uint8_t channel, bool state) {
    if (channel < 1 || channel > 4) return;
    
    int pin;
    switch(channel) {
        case 1: pin = CH1_PIN; break;
        case 2: pin = CH2_PIN; break;
        case 3: pin = CH3_PIN; break;
        case 4: pin = CH4_PIN; break;
        default: return;
    }
    
    digitalWrite(pin, state ? LOW : HIGH);
}

bool PDUController::getChannelState(uint8_t channel) const {
    if (channel < 1 || channel > 4) return false;
    
    int pin;
    switch(channel) {
        case 1: pin = CH1_PIN; break;
        case 2: pin = CH2_PIN; break;
        case 3: pin = CH3_PIN; break;
        case 4: pin = CH4_PIN; break;
        default: return false;
    }
    
    return digitalRead(pin) == LOW;
}

void PDUController::handleVPFault() {
    // Monitor VP_PIN and CH1_PIN states
    int currentVpPin = digitalRead(VP_PIN);
    int currentCh1Pin = digitalRead(CH1_PIN);
    
    // Report state changes
    if (currentVpPin != lastVpPinState || currentCh1Pin != lastCh1PinState) {
        Serial.println("Status Update:");
        Serial.println("VP_PIN State: " + String(currentVpPin ? "HIGH (Fault)" : "LOW (Normal)"));
        Serial.println("CH1_PIN State: " + String(currentCh1Pin ? "HIGH (OFF)" : "LOW (ON)"));
        Serial.println("Current Reset Attempts: " + String(vpResetAttempts));
        lastVpPinState = currentVpPin;
        lastCh1PinState = currentCh1Pin;
    }
    
    // VP_PIN fault handling for CH1 only
    if (!currentCh1Pin) {  // If CH1 is ON (LOW)
        if (currentVpPin == HIGH) {  // Fault detected
            if (!faultHandlingInProgress && vpResetAttempts < MAX_VP_RESETS) {
                faultHandlingInProgress = true;
                Serial.println("FAULT DETECTED: VP_PIN is HIGH while CH1 is ON");
                
                // Immediately turn CH1 OFF
                setChannel(1, false);
                Serial.println("CH1 turned OFF, waiting 30 seconds before reset...");
                
                // Wait 30 seconds
                delay(30000);
                
                // Turn CH1 back ON
                setChannel(1, true);
                Serial.println("CH1 turned back ON");
                
                // Increment reset counter
                vpResetAttempts++;
                Serial.println("Reset attempt " + String(vpResetAttempts) + " of " + String(MAX_VP_RESETS));
                
                faultHandlingInProgress = false;
            }
            
            // If we've reached max attempts, permanently disable CH1
            if (vpResetAttempts >= MAX_VP_RESETS) {
                Serial.println("CRITICAL: Maximum reset attempts reached!");
                // Permanent shutdown of CH1
                setChannel(1, false);
                Serial.println("VP fault: Maximum reset attempts reached. CH1 locked and saved to flash memory.");
                Serial.println("Manual intervention required to reset CH1");
                saveSettings();
            }
        } else {
            // If CH1 is ON but VP_PIN is LOW (normal operation)
            if (millis() - lastNormalOpTime >= 50000) {  // Print every 50 seconds during normal operation
                Serial.println("Normal Operation: CH1 is ON, VP_PIN is LOW (no fault)");
                lastNormalOpTime = millis();
            }
        }
    }
}

void PDUController::update() {
    updateSensors();
    handleVPFault();
    
    int ignState = debounceIgn();
    
    if (relayFlag) {
        bool tempCondition = currentTemp >= tempThreshold;
        bool timeCondition = (ignState == LOW) && 
                           (millis() - lastStableTime >= timeThreshold);

        if (tempCondition || timeCondition) {
            if (relayState) {
                turnOffSequence();
            }
        } else if (ignState == HIGH && !relayState) {
            turnOnSequence();
        }
    } else if (relayState) {
        turnOffSequence();
    }
}

void PDUController::updateSensors() {
    if (millis() - lastTempCheckTime >= TEMP_CHECK_INTERVAL) {
        sensors.requestTemperatures();
        currentTemp = sensors.getTempCByIndex(0);

        int batValue = analogRead(BAT_PIN);
        batteryVoltage = (batValue * 3.3 / 1024.0) * 3.3;

        lastTempCheckTime = millis();
    }
}

int PDUController::debounceIgn() {
    int reading = digitalRead(IGN_PIN);
    if (reading != lastStableState) {
        delay(DEBOUNCE_DELAY);
        reading = digitalRead(IGN_PIN);
        if (reading != lastStableState) {
            lastStableState = reading;
            lastStableTime = millis();
        }
    }
    return lastStableState;
}

void PDUController::loadSettings() {
    preferences.begin("pdu-settings", true);
    if (preferences.isKey("tempThresh")) {
        tempThreshold = preferences.getFloat("tempThresh", DEFAULT_TEMP_THRESHOLD);
        timeThreshold = preferences.getULong("timeThresh", DEFAULT_TIME_THRESHOLD);
        relayFlag = preferences.getBool("relayFlag", true);
    }
    preferences.end();
}

void PDUController::saveSettings() {
    preferences.begin("pdu-settings", false);
    preferences.putFloat("tempThresh", tempThreshold);
    preferences.putULong("timeThresh", timeThreshold);
    preferences.putBool("relayFlag", relayFlag);
    preferences.end();
}

void PDUController::setTempThreshold(float temp) {
    tempThreshold = temp;
    saveSettings();
}

void PDUController::setTimeThreshold(unsigned long time) {
    timeThreshold = time;
    saveSettings();
}

void PDUController::setRelayFlag(bool state) {
    relayFlag = state;
    saveSettings();
}

void PDUController::printStatus() const {
    Serial.println("System Status:");
    Serial.println("Temperature: " + String(currentTemp, 2) + "°C");
    Serial.println("Battery: " + String(batteryVoltage, 2) + "V");
    Serial.println("Relay: " + String(relayState ? "ON" : "OFF"));
    Serial.println("Channels:");
    for (int i = 1; i <= 4; i++) {
        Serial.println("CH" + String(i) + ": " + 
                      String(getChannelState(i) ? "ON" : "OFF"));
    }
}
