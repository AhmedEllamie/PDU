/*
 * Serial Command Handler Header
 * 
 * This header defines the SerialCommandHandler class which processes
 * serial commands for controlling and monitoring the PDU system.
 * 
 * Author: Ahmed Ellamie
 * Email: ahmed.ellamiee@gmail.com
 * Created: 5/7/2025
 * Modified: 7/29/2025
 */

#ifndef SERIAL_COMMAND_HANDLER_H
#define SERIAL_COMMAND_HANDLER_H

#include <Arduino.h>
#include "pdu_controller.h"

class SerialCommandHandler {
public:
    static void handleCommand(const String& command, PDUController& pdu);

private:
    static void handleSetCommand(const String& command, PDUController& pdu);
    static void handleGetCommand(const String& command, PDUController& pdu);
    static void printStatus(PDUController& pdu);
};

#endif // SERIAL_COMMAND_HANDLER_H
