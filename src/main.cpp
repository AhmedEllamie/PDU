/*
 * PDU Main Program
 * 
 * This is the main entry point for the PDU controller system.
 * It initializes all components and runs the main control loop
 * that manages the PDU functionality.
 * 
 * Author: Ahmed Ellamie
 * Email: ahmed.ellamiee@gmail.com
 * Created: 5/7/2025
 * Modified: 7/29/2025
 */

#include <Arduino.h>
#include <WiFi.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <Preferences.h>
#include "pdu_config.h"
#include "pdu_controller.h"
#include "pdu_web_server.h"
#include "SerialCommandHandler.h"

// Global objects
PDUController pdu;
PDUWebServer webServer(pdu);

void setup() {
    Serial.begin(115200);
    
    // Initialize PDU controller
    pdu.begin();
    
    // Start Access Point
    WiFi.softAP(AP_SSID, AP_PASSWORD);
    IPAddress IP = WiFi.softAPIP();
    Serial.print("AP IP address: ");
    Serial.println(IP);

    // Initialize web server
    webServer.begin();
    Serial.println("HTTP server started");
}

void loop() {
    // Handle web client requests
    webServer.handleClient();
    
    // Update PDU state (temperature, voltage, etc)
    pdu.update();

    // Handle serial commands
    if (Serial.available() > 0) {
        String command = Serial.readStringUntil('\n');
        command.trim();
        SerialCommandHandler::handleCommand(command, pdu);
    }
}
