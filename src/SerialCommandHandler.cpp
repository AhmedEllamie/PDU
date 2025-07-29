/*
 * Serial Command Handler Implementation
 * 
 * This file implements the serial command interface for the PDU system,
 * allowing control and monitoring through serial commands. It processes
 * both GET and SET commands for all PDU functionality.
 * 
 * Author: Ahmed Ellamie
 * Email: ahmed.ellamiee@gmail.com
 * Created: 5/7/2025
 * Modified: 7/29/2025
 */

#include "SerialCommandHandler.h"

void SerialCommandHandler::handleCommand(const String& command, PDUController& pdu) {
    if (command == "GET_STATUS") {
        printStatus(pdu);
        return;
    }

    // Handle GET commands
    if (command.startsWith("GET_")) {
        handleGetCommand(command, pdu);
    }
    // Handle SET commands
    else {
        handleSetCommand(command, pdu);
    }
}

void SerialCommandHandler::handleGetCommand(const String& command, PDUController& pdu) {
    if (command == "GET_F1") Serial.println("F1:" + String(digitalRead(F1_PIN)));
    else if (command == "GET_F2") Serial.println("F2:" + String(digitalRead(F2_PIN)));
    else if (command == "GET_F3") Serial.println("F3:" + String(digitalRead(F3_PIN)));
    else if (command == "GET_F4") Serial.println("F4:" + String(digitalRead(F4_PIN)));
    else if (command == "GET_IGN") Serial.println("IGN:" + String(digitalRead(IGN_PIN)));
    else if (command == "GET_TEMP") Serial.println("TEMP:" + String(pdu.getCurrentTemp(), 2));
    else if (command == "GET_BAT") Serial.println("BAT:" + String(pdu.getBatteryVoltage(), 2));
    else if (command == "GET_RELAY") Serial.println("RELAY:" + String(pdu.getRelayFlag() ? "1" : "0"));
    else if (command == "GET_CH1") Serial.println("CH1:" + String(pdu.getChannelState(1) ? "1" : "0"));
    else if (command == "GET_CH2") Serial.println("CH2:" + String(pdu.getChannelState(2) ? "1" : "0"));
    else if (command == "GET_CH3") Serial.println("CH3:" + String(pdu.getChannelState(3) ? "1" : "0"));
    else if (command == "GET_CH4") Serial.println("CH4:" + String(pdu.getChannelState(4) ? "1" : "0"));
}

void SerialCommandHandler::handleSetCommand(const String& command, PDUController& pdu) {
    int spaceIndex = command.indexOf(':');
    if (spaceIndex <= 0) {
        Serial.println("Invalid command format");
        return;
    }

    String cmd = command.substring(0, spaceIndex);
    String valueStr = command.substring(spaceIndex + 1);
    int value = valueStr.toInt();

    if (cmd.startsWith("SET_CH") && cmd.length() == 7) {
        int channel = cmd[6] - '0';
        if (channel >= 1 && channel <= 4) {
            pdu.setChannel(channel, value == 0);
            Serial.println(cmd.substring(4) + ":" + String(pdu.getChannelState(channel) ? "1" : "0"));
        }
    }
    else if (cmd == "SET_RELAY") {
        pdu.setRelayFlag(value == 1);
        Serial.println("RELAY:" + String(pdu.getRelayFlag() ? "1" : "0"));
    }
    else if (cmd == "SET_TSTemp") {
        pdu.setTempThreshold(value);
        Serial.println("TSTEMP:" + String(pdu.getTempThreshold()));
    }
    else if (cmd == "SET_TSTime") {
        pdu.setTimeThreshold(value * 60000);
        Serial.println("TSTOFF:" + String(value));
    }
    else {
        Serial.println("Unknown command");
    }
}

void SerialCommandHandler::printStatus(PDUController& pdu) {
    Serial.println("System Status:");
    Serial.println("F1:" + String(digitalRead(F1_PIN)));
    Serial.println("F2:" + String(digitalRead(F2_PIN)));
    Serial.println("F3:" + String(digitalRead(F3_PIN)));
    Serial.println("F4:" + String(digitalRead(F4_PIN)));
    Serial.println("IGN:" + String(digitalRead(IGN_PIN)));
    Serial.println("TEMP:" + String(pdu.getCurrentTemp(), 2));
    Serial.println("BAT:" + String(pdu.getBatteryVoltage(), 2));
    Serial.println("CH1:" + String(pdu.getChannelState(1) ? "1" : "0"));
    Serial.println("CH2:" + String(pdu.getChannelState(2) ? "1" : "0"));
    Serial.println("CH3:" + String(pdu.getChannelState(3) ? "1" : "0"));
    Serial.println("CH4:" + String(pdu.getChannelState(4) ? "1" : "0"));
    Serial.println("RELAY:" + String(pdu.getRelayFlag() ? "1" : "0"));
    Serial.println("TEMP_THRESH:" + String(pdu.getTempThreshold()));
    Serial.println("TIME_THRESH_MIN:" + String(pdu.getTimeThreshold() / 60000.0));
}
