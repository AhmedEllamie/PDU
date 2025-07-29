/*
 * PDU Controller Header
 * 
 * This header defines the PDUController class which manages the core functionality
 * of the Power Distribution Unit, including power sequencing, monitoring, and
 * safety features.
 * 
 * Author: Ahmed Ellamie
 * Email: ahmed.ellamiee@gmail.com
 * Created: 5/7/2025
 * Modified: 7/29/2025
 */

#ifndef PDU_CONTROLLER_H
#define PDU_CONTROLLER_H

#include <Arduino.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <Preferences.h>
#include "pdu_config.h"

class PDUController {
public:
    PDUController();
    void begin();
    void update();
    
    // Channel Control
    void turnOnSequence();
    void turnOffSequence();
    void setChannel(uint8_t channel, bool state);
    bool getChannelState(uint8_t channel) const;
    void handleVPFault();
    
    // Settings
    void setTempThreshold(float temp);
    void setTimeThreshold(unsigned long time);
    void setRelayFlag(bool state);
    float getTempThreshold() const { return tempThreshold; }
    unsigned long getTimeThreshold() const { return timeThreshold; }
    bool getRelayFlag() const { return relayFlag; }
    
    // Status
    float getCurrentTemp() const { return currentTemp; }
    float getBatteryVoltage() const { return batteryVoltage; }
    bool getRelayState() const { return relayState; }
    void printStatus() const;

private:
    // Hardware interfaces
    OneWire oneWire;
    DallasTemperature sensors;
    Preferences preferences;

    // State variables
    bool relayState;
    bool relayFlag;
    float currentTemp;
    float tempThreshold;
    unsigned long timeThreshold;
    int vpResetAttempts;
    float batteryVoltage;
    unsigned long ignLowStartTime;
    unsigned long lastTempCheckTime;
    unsigned long lastStableTime;
    int lastStableState;
    
    // VP monitoring state
    int lastVpPinState;
    int lastCh1PinState;
    unsigned long lastNormalOpTime;
    bool faultHandlingInProgress;

    // Private methods
    void initPins();
    void loadSettings();
    void saveSettings();
    int debounceIgn();
    void updateSensors();
};

#endif // PDU_CONTROLLER_H
