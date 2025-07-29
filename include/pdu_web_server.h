/*
 * PDU Web Server Header
 * 
 * This header defines the PDUWebServer class which handles all web interface
 * and API functionality for the PDU controller system.
 * 
 * Author: Ahmed Ellamie
 * Email: ahmed.ellamiee@gmail.com
 * Created: 5/7/2025
 * Modified: 7/29/2025
 */

#ifndef PDU_WEB_SERVER_H
#define PDU_WEB_SERVER_H

#include <WebServer.h>
#include "pdu_controller.h"
#include "html_content.h"
#include "pdu_config.h"

class PDUWebServer {
public:
    PDUWebServer(PDUController& pduController);
    void begin();
    void handleClient();

private:
    WebServer server;
    PDUController& pdu;

    void setupRoutes();
    void handleRoot();
    void handleApiStatus();
    void handleApiControl();
    void handleApiSetTemp();
    void handleApiSetTime();

    bool authenticate();
    String createJsonResponse();
};

#endif // PDU_WEB_SERVER_H
