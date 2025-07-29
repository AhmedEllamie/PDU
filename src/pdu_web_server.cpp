/*
 * PDU Web Server Implementation
 * 
 * This file implements the web server functionality for the PDU controller,
 * providing both a web interface and REST API endpoints for remote control
 * and monitoring of the PDU system.
 * 
 * Author: Ahmed Ellamie
 * Email: ahmed.ellamiee@gmail.com
 * Created: 5/7/2025
 * Modified: 7/29/2025
 */

#include "pdu_web_server.h"

PDUWebServer::PDUWebServer(PDUController& pduController)
    : server(80)
    , pdu(pduController)
{
}

void PDUWebServer::begin() {
    setupRoutes();
    WiFi.softAP(AP_SSID, AP_PASSWORD);
    server.begin();
    Serial.print("AP IP address: ");
    Serial.println(WiFi.softAPIP());
}

void PDUWebServer::handleClient() {
    server.handleClient();
}

void PDUWebServer::setupRoutes() {
    server.on("/", [this]() { handleRoot(); });
    server.on("/api/status", HTTP_GET, [this]() { handleApiStatus(); });
    server.on("/api/control", HTTP_POST, [this]() { handleApiControl(); });
    server.on("/api/setTemp", HTTP_POST, [this]() { handleApiSetTemp(); });
    server.on("/api/setTime", HTTP_POST, [this]() { handleApiSetTime(); });
}

bool PDUWebServer::authenticate() {
    if (!server.authenticate(HTTP_USERNAME, HTTP_PASSWORD)) {
        server.requestAuthentication();
        return false;
    }
    return true;
}

void PDUWebServer::handleRoot() {
    if (!authenticate()) return;
    server.send(200, "text/html", INDEX_HTML);
}

String PDUWebServer::createJsonResponse() {
    String jsonResponse = "{";
    jsonResponse += "\"f1\":" + String(digitalRead(F1_PIN)) + ",";
    jsonResponse += "\"f2\":" + String(digitalRead(F2_PIN)) + ",";
    jsonResponse += "\"f3\":" + String(digitalRead(F3_PIN)) + ",";
    jsonResponse += "\"f4\":" + String(digitalRead(F4_PIN)) + ",";
    jsonResponse += "\"ign\":" + String(digitalRead(IGN_PIN)) + ",";
    jsonResponse += "\"temp\":" + String(pdu.getCurrentTemp()) + ",";
    jsonResponse += "\"tempThreshold\":" + String(pdu.getTempThreshold()) + ",";
    jsonResponse += "\"battery\":" + String(pdu.getBatteryVoltage()) + ",";
    jsonResponse += "\"relay\":" + String(pdu.getRelayState()) + ",";
    jsonResponse += "\"timeThreshold\":" + String(pdu.getTimeThreshold()) + ",";
    jsonResponse += "\"ch1\":" + String(pdu.getChannelState(1)) + ",";
    jsonResponse += "\"ch2\":" + String(pdu.getChannelState(2)) + ",";
    jsonResponse += "\"ch3\":" + String(pdu.getChannelState(3)) + ",";
    jsonResponse += "\"ch4\":" + String(pdu.getChannelState(4));
    jsonResponse += "}";
    return jsonResponse;
}

void PDUWebServer::handleApiStatus() {
    if (!authenticate()) return;
    server.send(200, "application/json", createJsonResponse());
}

void PDUWebServer::handleApiControl() {
    if (!authenticate()) return;

    if (server.hasArg("device") && server.hasArg("state")) {
        String device = server.arg("device");
        int state = server.arg("state").toInt();
        bool success = false;
        String message = "";

        if (state == 0 || state == 1) {
            if (device.startsWith("CH") && device.length() == 3) {
                int channel = device.substring(2).toInt();
                if (channel >= 1 && channel <= 4) {
                    pdu.setChannel(channel, state == 0);
                    success = true;
                    message = device + " set to " + String(state == 0 ? "ON" : "OFF");
                }
            } else if (device == "RELAY") {
                pdu.setRelayFlag(state == 1);
                success = true;
                message = "RelayFlag set to " + String(state == 1 ? "ON" : "OFF");
            }
        }

        server.send(200, "application/json", 
            "{\"success\":" + String(success ? "true" : "false") + 
            ",\"message\":\"" + message + "\"}");
    } else {
        server.send(400, "application/json", 
            "{\"success\":false,\"message\":\"Missing parameters\"}");
    }
}

void PDUWebServer::handleApiSetTemp() {
    if (!authenticate()) return;

    if (server.hasArg("value")) {
        float temp = server.arg("value").toFloat();
        pdu.setTempThreshold(temp);
        server.send(200, "application/json", 
            "{\"success\":true,\"message\":\"Temperature threshold updated\"}");
    } else {
        server.send(400, "application/json", 
            "{\"success\":false,\"message\":\"Missing value parameter\"}");
    }
}

void PDUWebServer::handleApiSetTime() {
    if (!authenticate()) return;

    if (server.hasArg("value")) {
        unsigned long time = server.arg("value").toFloat() * 60000;
        pdu.setTimeThreshold(time);
        server.send(200, "application/json", 
            "{\"success\":true,\"message\":\"Time threshold updated\"}");
    } else {
        server.send(400, "application/json", 
            "{\"success\":false,\"message\":\"Missing value parameter\"}");
    }
}
